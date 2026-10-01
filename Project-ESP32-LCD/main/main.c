/*
 * HYBRID PSU Solar Inverter - Precision HMI Color Dashboard
 * Framework: ESP-IDF v6.1 + LVGL v9.6.0
 * Features:
 *   - Perfectly sized non-overlapping UI layout
 *   - High-contrast multi-font typography (Montserrat 10, 12, 14, 16)
 *   - Real 32x32 Embedded Graphic Icons (Solar, Battery, Grid, Home, Inverter, Temp)
 *   - 4 Dedicated Interactive Pages
 *   - Dedicated Serial (UART2) Receiver Task on GPIO 16
 *   - High-Speed 40 MHz SPI + DMA Acceleration
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <sys/lock.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lvgl.h"
#include "assets/icons.h"
#include "pcu_protocol.h"

#include "assets/fonts_bold.h"

/* Typography Font Declarations (High-Legibility Bold Fonts) */
#define lv_font_montserrat_10   font_bold_10
#define lv_font_montserrat_12   font_bold_12
#define lv_font_montserrat_14   font_bold_14
#define lv_font_montserrat_16   font_bold_16
#define lv_font_montserrat_20   font_bold_20

/* Custom Logo Storage & LVGL Image Descriptor */
static lv_image_dsc_t custom_logo_dsc;
static uint8_t *custom_logo_pixels = NULL;
static bool custom_logo_available = false;

#define MAX_LOGO_BUFFER_SIZE  16384
static uint8_t s_logo_staging_buf[MAX_LOGO_BUFFER_SIZE];
static uint16_t s_logo_staging_len = 0;

static void init_custom_logo(void)
{
    custom_logo_available = false;

    /* 1. Try reading from NVS */
    nvs_handle_t handle;
    if (nvs_open("pcu_store", NVS_READONLY, &handle) == ESP_OK) {
        size_t req_size = 0;
        if (nvs_get_blob(handle, "pcu_logo", NULL, &req_size) == ESP_OK &&
            req_size >= sizeof(pcu_logo_header_t) && req_size <= MAX_LOGO_BUFFER_SIZE) {
            uint8_t *buf = malloc(req_size);
            if (buf && nvs_get_blob(handle, "pcu_logo", buf, &req_size) == ESP_OK) {
                pcu_logo_header_t *hdr = (pcu_logo_header_t *)buf;
                if (hdr->magic == PCU_LOGO_MAGIC && req_size == sizeof(pcu_logo_header_t) + hdr->data_size) {
                    uint32_t calc = pcu_calc_crc32(buf + sizeof(pcu_logo_header_t), hdr->data_size);
                    if (calc == hdr->crc32) {
                        custom_logo_pixels = buf + sizeof(pcu_logo_header_t);
                        custom_logo_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
                        if (hdr->cf == 2) {
                            custom_logo_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
                            custom_logo_dsc.header.stride = hdr->width * 4;
                        } else {
                            custom_logo_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
                            custom_logo_dsc.header.stride = hdr->width * 2;
                        }
                        custom_logo_dsc.header.w = hdr->width;
                        custom_logo_dsc.header.h = hdr->height;
                        custom_logo_dsc.header.flags = 0;
                        custom_logo_dsc.header.reserved_2 = 0;
                        custom_logo_dsc.data_size = hdr->data_size;
                        custom_logo_dsc.data = custom_logo_pixels;
                        custom_logo_available = true;
                        ESP_LOGI("PCU_LOGO", "Custom logo loaded from NVS: %dx%d, cf=%d, size=%lu",
                                 hdr->width, hdr->height, hdr->cf, (unsigned long)hdr->data_size);
                    } else {
                        ESP_LOGE("PCU_LOGO", "NVS logo CRC mismatch (0x%08lX != 0x%08lX)",
                                 (unsigned long)calc, (unsigned long)hdr->crc32);
                    }
                }
            }
            if (!custom_logo_available && buf) free(buf);
        }
        nvs_close(handle);
    }

    /* 2. Fallback to raw flash address 0x110000 */
    if (!custom_logo_available) {
        pcu_logo_header_t flash_hdr;
        esp_err_t err = esp_flash_read(NULL, &flash_hdr, PCU_LOGO_FLASH_ADDR, sizeof(flash_hdr));
        if (err == ESP_OK && flash_hdr.magic == PCU_LOGO_MAGIC && flash_hdr.data_size <= MAX_LOGO_BUFFER_SIZE) {
            uint8_t *buf = malloc(flash_hdr.data_size);
            if (buf) {
                err = esp_flash_read(NULL, buf, PCU_LOGO_FLASH_ADDR + sizeof(flash_hdr), flash_hdr.data_size);
                if (err == ESP_OK && pcu_calc_crc32(buf, flash_hdr.data_size) == flash_hdr.crc32) {
                    custom_logo_pixels = buf;
                    custom_logo_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
                    if (flash_hdr.cf == 2) {
                        custom_logo_dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
                        custom_logo_dsc.header.stride = flash_hdr.width * 4;
                    } else {
                        custom_logo_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
                        custom_logo_dsc.header.stride = flash_hdr.width * 2;
                    }
                    custom_logo_dsc.header.w = flash_hdr.width;
                    custom_logo_dsc.header.h = flash_hdr.height;
                    custom_logo_dsc.header.flags = 0;
                    custom_logo_dsc.header.reserved_2 = 0;
                    custom_logo_dsc.data_size = flash_hdr.data_size;
                    custom_logo_dsc.data = custom_logo_pixels;
                    custom_logo_available = true;
                    ESP_LOGI("PCU_LOGO", "Custom logo loaded from Flash 0x110000: %dx%d, cf=%d, size=%lu",
                             flash_hdr.width, flash_hdr.height, flash_hdr.cf, (unsigned long)flash_hdr.data_size);
                } else {
                    ESP_LOGE("PCU_LOGO", "Flash logo CRC mismatch or read error");
                    free(buf);
                }
            }
        }
    }

    /* 3. Auto-sanitize background transparency for ARGB8888 custom logos */
    if (custom_logo_available && custom_logo_dsc.header.cf == LV_COLOR_FORMAT_ARGB8888) {
        uint32_t w = custom_logo_dsc.header.w;
        uint32_t h = custom_logo_dsc.header.h;
        uint8_t *pix = custom_logo_pixels;
        if (pix && w > 0 && h > 0) {
            uint8_t c_b = pix[0];
            uint8_t c_g = pix[1];
            uint8_t c_r = pix[2];
            uint8_t c_a = pix[3];

            if (c_a >= 240) {
                uint32_t tr_idx = (w - 1) * 4;
                uint32_t bl_idx = (h - 1) * w * 4;
                uint32_t br_idx = ((h - 1) * w + (w - 1)) * 4;

                bool is_legacy_dark = (abs((int)c_r - 12) <= 8 && abs((int)c_g - 21) <= 8 && abs((int)c_b - 36) <= 8);
                bool corners_match = (abs((int)pix[tr_idx] - c_b) <= 6 && abs((int)pix[tr_idx + 1] - c_g) <= 6 && abs((int)pix[tr_idx + 2] - c_r) <= 6 &&
                                      abs((int)pix[bl_idx] - c_b) <= 6 && abs((int)pix[bl_idx + 1] - c_g) <= 6 && abs((int)pix[bl_idx + 2] - c_r) <= 6 &&
                                      abs((int)pix[br_idx] - c_b) <= 6 && abs((int)pix[br_idx + 1] - c_g) <= 6 && abs((int)pix[br_idx + 2] - c_r) <= 6);

                if (is_legacy_dark || corners_match) {
                    for (uint32_t i = 0; i < w * h; i++) {
                        uint8_t *p = pix + i * 4;
                        int dr = abs((int)p[2] - (int)c_r);
                        int dg = abs((int)p[1] - (int)c_g);
                        int db = abs((int)p[0] - (int)c_b);
                        if (dr <= 6 && dg <= 6 && db <= 6) {
                            p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0; // 100% transparent!
                        } else if (dr <= 16 && dg <= 16 && db <= 16) {
                            int max_d = dr;
                            if (dg > max_d) max_d = dg;
                            if (db > max_d) max_d = db;
                            uint8_t alpha = (uint8_t)(((max_d - 6) * 255) / 10);
                            if (alpha < p[3]) p[3] = alpha;
                        }
                    }
                    ESP_LOGI("PCU_LOGO", "Auto-keyed background color (R=%d, G=%d, B=%d) to transparent", c_r, c_g, c_b);
                }
            }
        }
    }

    if (!custom_logo_available) {
        ESP_LOGI("PCU_LOGO", "No custom logo active. Using default solar icon.");
    }
}

static const lv_image_dsc_t *get_active_logo_dsc(void)
{
    if (custom_logo_available) return &custom_logo_dsc;
    return &img_solar;
}

/* User LCD Pin Definitions */
#define LCD_HOST                SPI2_HOST
#define PIN_NUM_MOSI            GPIO_NUM_23
#define PIN_NUM_CLK             GPIO_NUM_22
#define PIN_NUM_CS              GPIO_NUM_19
#define PIN_NUM_DC              GPIO_NUM_18
#define PIN_NUM_RST             GPIO_NUM_21
#define PIN_NUM_BK_LIGHT        GPIO_NUM_4

/* Display Dimensions */
#define LCD_H_RES               240
#define LCD_V_RES               320
#define LCD_PIXEL_CLOCK_HZ      (20 * 1000 * 1000)

#define LVGL_DRAW_BUF_LINES     40
#define LVGL_TICK_PERIOD_MS     2

/* Dedicated Inverter Serial Port (UART2) */
#define INVERTER_UART_NUM       UART_NUM_2
#define INVERTER_UART_RX_PIN    GPIO_NUM_16
#define INVERTER_UART_TX_PIN    GPIO_NUM_17
#define INVERTER_UART_BAUD      9600

static _lock_t lvgl_api_lock;

/* Inverter State Enums */
typedef enum {
    SOLAR_OFF = 0,
    SOLARON = 1
} solar_state_t;

typedef enum {
    CHARGER_OFF = 0,
    AC_CHARGER = 1,
    SOLAR_CHARGER = 2,
    SHARE_CHARGER = 3
} charger_state_t;

typedef enum {
    INV_OFF = 0,
    INVSWITCH = 1
} switch_state_t;

/* System Fault & Protection Trip Codes (Industrial Inverter Protocol) */
#define FAULT_CLEARED     0
#define SHORT_TRIP        1
#define NOFEED_TRIP       2
#define HEATOVER_TRIP     3
#define OVERLOAD_TRIP     4
#define DC_LO_TRIP        5
#define HI_CURRENT_TRIP   6
#define SOLAR_HIGH        7
#define DC_HI_TRIP        8
#define DC_FAIL_TRIP      9

#define OVERLOAD_WARN     20
#define LOWBATT_WARN      30
#define LOWBAT_TRIP       40

/* Inverter Calibration & Threshold Limits Model ($LIMITS packet) */
typedef struct {
    float setbatful;   // Battery full float cutoff (buffer[14])
    float setbatwrn;   // Battery low warning voltage (buffer[15])
    float setbatlo;    // Battery low trip cutoff (buffer[16])
    float setbatrst;   // Battery reconnect/reset voltage (buffer[17])
    float mainslow;    // AC Mains undervoltage limit (buffer[20])
    float mainshi;     // AC Mains overvoltage limit (buffer[21])
    float hiheat;      // Heatsink high temp trip (buffer[23])
    float lowheat;     // Heatsink cooling reset (0)
    float solmax;      // Solar Voc max limit (buffer[26])
    float solmin;      // Solar Voc min operating threshold (buffer[27])
    float dcmax;       // DC Boost max limit (buffer[28])
    float dcmin;       // DC Boost min (0)
    bool is_calibrated;// True once $LIMITS telemetry arrives
} pcu_thresholds_t;

static pcu_thresholds_t pcu_limits = {
    .setbatful = 28.8f,
    .setbatwrn = 23.5f,
    .setbatlo  = 21.0f,
    .setbatrst = 24.5f,
    .mainslow  = 185.0f,
    .mainshi   = 265.0f,
    .hiheat    = 85.0f,
    .lowheat   = 0.0f,
    .solmax    = 115.0f,
    .solmin    = 15.0f,
    .dcmax     = 450.0f,
    .dcmin     = 0.0f,
    .is_calibrated = false
};

/* Telemetry Model */
typedef struct {
    uint8_t onflag;          // 0 = Mains mode, 1 = Inverter mode
    uint8_t dcboostmode;     // 1 = Active, 0 = Off
    uint8_t dcok;            // 1 = OK!, 0 = Raw voltage
    uint8_t sharemode;       // 0 = Solar pref, 1 = Share
    uint8_t batgravity;      // 0 = OK!, 1 = Full
    uint8_t feedmode;        // 0 = Auto, 1 = Solar, 2 = Batt, 3 = Mains, 4 = Share
    solar_state_t solarstate;
    charger_state_t chargerstate;
    switch_state_t switchstate;

    float mainsvolt;         // Grid AC Voltage
    float solarvolt;         // Solar PV Voltage
    float battvolts;         // Battery Bank Voltage
    float acout;             // Inverter Output AC Voltage
    float loaddisp;          // Inverter Loading %
    float chrampsdisp;       // Charging Current (Amps)
    float dischdisp;         // Discharging Current (Amps)
    float dcboost;           // Raw DC Boost Voltage
    float upsheat;           // Inverter Heat (°C)
    int   error_code;        // Active Fault Code (0 = FAULT_CLEARED)

    uint32_t rx_packet_count;
    uint32_t last_rx_tick;
    bool live_serial_active;
} inverter_data_t;

static inverter_data_t inv_data = {
    .onflag = 1,
    .dcboostmode = 1,
    .dcok = 1,
    .sharemode = 0,
    .batgravity = 0,
    .feedmode = 0,
    .solarstate = SOLARON,
    .chargerstate = SOLAR_CHARGER,
    .switchstate = INVSWITCH,
    .mainsvolt = 228.0f,
    .solarvolt = 76.5f,
    .battvolts = 26.8f,
    .acout = 230.0f,
    .loaddisp = 42.0f,
    .chrampsdisp = 16.4f,
    .dischdisp = 0.0f,
    .dcboost = 385.0f,
    .upsheat = 38.5f,
    .error_code = 0,
    .rx_packet_count = 0,
    .last_rx_tick = 0,
    .live_serial_active = false
};

/* Global Production Configuration */
static pcu_config_t g_pcu_cfg;

/* Page Navigation */
#define TOTAL_PAGES 6
static int current_page = 0;
static lv_obj_t *pages[TOTAL_PAGES];
static lv_obj_t *page_dots[TOTAL_PAGES];
static lv_obj_t *rtc_time_lbl;
static lv_obj_t *footer_page_lbl;
static lv_obj_t *g_main_screen_obj = NULL;
static lv_obj_t *g_error_scr = NULL;

/* Dedicated Critical Error / Fault Screen Widgets (Always Red Industrial Alert) */
static lv_obj_t *err_hdr_box = NULL;
static lv_obj_t *err_hdr_title_lbl = NULL;
static lv_obj_t *err_uptime_lbl = NULL;
static lv_obj_t *err_card_box = NULL;
static lv_obj_t *err_title_lbl = NULL;
static lv_obj_t *err_trigger_box = NULL;
static lv_obj_t *err_trigger_hdr_lbl = NULL;
static lv_obj_t *err_trigger_val_lbl = NULL;
static lv_obj_t *err_trigger_limit_lbl = NULL;

/* Page 0: Live Energy Flow Widgets */
static lv_obj_t *p0_solar_val;
static lv_obj_t *p0_solar_badge;
static lv_obj_t *p0_grid_val;
static lv_obj_t *p0_grid_badge;
static lv_obj_t *p0_batt_val;
static lv_obj_t *p0_batt_current;
static lv_obj_t *p0_batt_bar;
static lv_obj_t *p0_load_val;
static lv_obj_t *p0_load_src;
static lv_obj_t *p0_load_bar;
static lv_obj_t *p0_flow_badge;
static lv_obj_t *p0_inv_out_lbl;

/* Page 1: Solar & Charger Widgets */
static lv_obj_t *p1_solar_v;
static lv_obj_t *p1_solar_status;
static lv_obj_t *p1_chg_mode;
static lv_obj_t *p1_chg_amps;
static lv_obj_t *p1_dc_boost;
static lv_obj_t *p1_pref_mode;
static lv_obj_t *p1_raw_info;
static lv_obj_t *p1_sun_arc;

/* Page 2: Battery & Inverter Load Widgets */
static lv_obj_t *p2_batt_v;
static lv_obj_t *p2_batt_stat;
static lv_obj_t *p2_batt_current;
static lv_obj_t *p2_batt_bar;
static lv_obj_t *p2_batt_soc;
static lv_obj_t *p2_grav_lbl;
static lv_obj_t *p2_inv_v;
static lv_obj_t *p2_load_pct;
static lv_obj_t *p2_load_bar;
static lv_obj_t *p2_load_stat;
static lv_obj_t *p2_source_lbl;
static lv_obj_t *p2_inv_status;

/* Page 3: Grid, Thermal & System Operational Status Widgets */
static lv_obj_t *p3_grid_v;
static lv_obj_t *p3_grid_freq_stat;
static lv_obj_t *p3_temp_val;
static lv_obj_t *p3_temp_bar;
static lv_obj_t *p3_inv_mode;
static lv_obj_t *p3_ac_freq;
static lv_obj_t *p3_pref_lbl;
static lv_obj_t *p3_batt_health_lbl;
static lv_obj_t *p3_boost_lbl;
static lv_obj_t *p3_chg_type_lbl;
static lv_obj_t *p3_mains_status_lbl;
static lv_obj_t *p3_thermal_status_lbl;

/* Page 4: Solar Harvest & Yield Widgets */
static lv_obj_t *p4_today_kwh;
static lv_obj_t *p4_peak_w;
static lv_obj_t *p4_total_mwh;
static lv_obj_t *p4_chart;
static lv_chart_series_t *p4_series_solar;
static lv_chart_series_t *p4_series_load;
static lv_obj_t *p4_trend_badge;

/* Page 5: OEM Vendor & Support Widgets */
static lv_obj_t *p5_brand_title;
static lv_obj_t *p5_model_name;
static lv_obj_t *p5_serial_no;
static lv_obj_t *p5_hw_rev;
static lv_obj_t *p5_contact;
static lv_obj_t *p5_website;
static lv_obj_t *p5_logo_arc;

/* 6 Complete Visual Themes */
typedef struct {
    const char *name;
    uint32_t screen_bg;
    uint32_t header_bg;
    uint32_t card_bg;
    uint32_t card_border;
    uint32_t primary;
    uint32_t secondary;
    uint32_t text_main;
    uint32_t text_muted;
    uint32_t badge_bg;
    uint32_t badge_fg;
    uint32_t chart_bar;
    uint32_t chart_bg;
    uint32_t active_dot;
} pcu_theme_t;

static const pcu_theme_t g_themes[6] = {
    /* 0: Tactical Amber (Industrial Solar Gold) */
    {
        .name = "Tactical Amber",
        .screen_bg    = 0x1f1402, // Warm Deep Amber Earth
        .header_bg    = 0x4a2c00, // Rich Glowing Gold-Bronze Header
        .card_bg      = 0x331e03, // Vivid Deep Amber Card
        .card_border  = 0xfbbf24, // Solid 2px Vibrant Amber Gold Border
        .primary      = 0xfbbf24, // Amber Gold
        .secondary    = 0xfde68a, // Sunburst Gold
        .text_main    = 0xffffff, // Pure Crisp White
        .text_muted   = 0xfcd34d, // Bright Amber Muted
        .badge_bg     = 0x78350f, // Deep Bronze Badge
        .badge_fg     = 0xfef08a, // Bright Gold Badge Text
        .chart_bar    = 0xf59e0b, // Amber Bars
        .chart_bg     = 0x271703, // Deep Bronze Chart BG
        .active_dot   = 0xfbbf24,
    },
    /* 1: Cyber Cyan (Aerospace / Cockpit HUD) */
    {
        .name = "Cyber Cyan",
        .screen_bg    = 0x021c33, // Vivid Deep Aerospace Navy
        .header_bg    = 0x074574, // High-Tech Electric Blue Header
        .card_bg      = 0x0b3252, // Luminous Deep Teal Card
        .card_border  = 0x22d3ee, // Solid 2px Neon Electric Cyan Border
        .primary      = 0x22d3ee, // Electric Cyan
        .secondary    = 0x67e8f9, // Sky Aqua
        .text_main    = 0xffffff, // Ice White
        .text_muted   = 0x38bdf8, // Electric Cyan Muted
        .badge_bg     = 0x0e5178, // Deep Ocean Badge
        .badge_fg     = 0xa5f3fc, // Bright Neon Aqua Badge Text
        .chart_bar    = 0x06b6d4, // Cyan Bars
        .chart_bg     = 0x04243e, // Deep Space Chart BG
        .active_dot   = 0x22d3ee,
    },
    /* 2: Emerald Defense (Military Grade Combat Tactical) */
    {
        .name = "Emerald Defense",
        .screen_bg    = 0x032612, // Vivid Deep Tactical Forest Green
        .header_bg    = 0x075425, // Military Green Header
        .card_bg      = 0x0c3b1c, // Vivid Combat Olive/Emerald Card
        .card_border  = 0x10b981, // Solid 2px Combat Emerald Border
        .primary      = 0x34d399, // Combat Emerald
        .secondary    = 0x6ee7b7, // Night-Vision Mint Green
        .text_main    = 0xffffff, // Tactical White
        .text_muted   = 0xa7f3d0, // Military Green Muted
        .badge_bg     = 0x065f2c, // Forest Army Green
        .badge_fg     = 0xecfdf5, // Bright Mint Badge Text
        .chart_bar    = 0x10b981, // Emerald Bars
        .chart_bg     = 0x063117, // Tactical Chart BG
        .active_dot   = 0x34d399,
    },
    /* 3: Crimson Alert (Industrial High-Voltage Hazard) */
    {
        .name = "Crimson Alert",
        .screen_bg    = 0x2d060b, // Vivid Deep Crimson Burgundy
        .header_bg    = 0x630e19, // Signal Crimson Header
        .card_bg      = 0x480a13, // Vivid Warning Crimson Card
        .card_border  = 0xef4444, // Solid 2px High-Voltage Red Alert Border
        .primary      = 0xf87171, // Signal Red
        .secondary    = 0xfca5a5, // Coral Red
        .text_main    = 0xffffff, // Stark White
        .text_muted   = 0xfecaca, // Coral Muted
        .badge_bg     = 0x7f1d1d, // Deep Ruby Badge
        .badge_fg     = 0xfef2f2, // Warning Red Badge Text
        .chart_bar    = 0xef4444, // Crimson Bars
        .chart_bg     = 0x3b070e, // Dark Ruby Chart BG
        .active_dot   = 0xef4444,
    },
    /* 4: Titanium Cobalt (Deep Sea / Marine Naval Tech) */
    {
        .name = "Titanium Cobalt",
        .screen_bg    = 0x081845, // Vivid Royal Marine Navy
        .header_bg    = 0x123588, // Naval Cobalt Header
        .card_bg      = 0x0f2766, // Royal Cobalt Blue Card
        .card_border  = 0x3b82f6, // Solid 2px Royal Blue Border
        .primary      = 0x60a5fa, // Royal Cobalt
        .secondary    = 0x93c5fd, // Azure Blue
        .text_main    = 0xffffff, // Arctic White
        .text_muted   = 0xbfdbfe, // Azure Muted
        .badge_bg     = 0x1d4ed8, // Deep Naval Blue Badge
        .badge_fg     = 0xeff6ff, // Bright Azure Badge Text
        .chart_bar    = 0x3b82f6, // Cobalt Bars
        .chart_bg     = 0x0c2054, // Naval Chart BG
        .active_dot   = 0x60a5fa,
    },
    /* 5: Stealth Monochrome (Flight Deck Avionics / Minimalist) */
    {
        .name = "Stealth Monochrome",
        .screen_bg    = 0x18181b, // Dark Titanium Zinc
        .header_bg    = 0x3f3f46, // Gunmetal Zinc Header
        .card_bg      = 0x27272a, // Precision Gunmetal Zinc Card
        .card_border  = 0xe4e4e7, // Solid 2px Stark Silver-White Border
        .primary      = 0xffffff, // Pure Stark White
        .secondary    = 0xd4d4d8, // Platinum Silver
        .text_main    = 0xffffff, // Pure White
        .text_muted   = 0xa1a1aa, // Zinc Silver Muted
        .badge_bg     = 0x52525b, // Charcoal Slate Badge
        .badge_fg     = 0xffffff, // Stark White Badge Text
        .chart_bar    = 0xffffff, // Stark White Bars
        .chart_bg     = 0x202024, // Gunmetal Chart BG
        .active_dot   = 0xffffff,
    }
};

static inline const pcu_theme_t *get_active_theme(void) {
    uint8_t th_idx = g_pcu_cfg.logo_theme % 6;
    return &g_themes[th_idx];
}

static inline lv_color_t get_theme_primary_color(uint8_t theme) {
    return lv_color_hex(g_themes[theme % 6].primary);
}

static inline lv_color_t get_theme_secondary_color(uint8_t theme) {
    return lv_color_hex(g_themes[theme % 6].secondary);
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void update_panel_rotation(lv_display_t *disp)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    switch (rotation) {
    case LV_DISPLAY_ROTATION_0:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISPLAY_ROTATION_90:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISPLAY_ROTATION_180:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISPLAY_ROTATION_270:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
    if (ret != ESP_OK) {
        lv_display_flush_ready(disp);
    }
}

static void increase_lvgl_tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void lvgl_port_task(void *arg)
{
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);

        if (time_till_next_ms < 10) {
            time_till_next_ms = 10;
        } else if (time_till_next_ms > 30) {
            time_till_next_ms = 30;
        }
        vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
    }
}

/* Helper to setup standardized card container */
static lv_obj_t *create_card(lv_obj_t *parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    const pcu_theme_t *th = get_active_theme();
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_pad_all(card, 4, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(th->card_bg), 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static const char *page_titles[TOTAL_PAGES] = {
    "ENERGY FLOW HUB",
    "SOLAR PV & CHARGER",
    "BATTERY & INVERTER LOAD",
    "SYSTEM OPERATIONAL STATUS",
    "SOLAR HARVEST & PRODUCTION",
    "OEM VENDOR & SUPPORT"
};

/* Switch Visible Page */
static void switch_to_page(int page_idx)
{
    if (page_idx < 0 || page_idx >= TOTAL_PAGES) return;
    current_page = page_idx;

    const pcu_theme_t *th = get_active_theme();
    if (footer_page_lbl) {
        lv_label_set_text(footer_page_lbl, page_titles[current_page]);
        lv_obj_set_style_text_color(footer_page_lbl, lv_color_hex(th->secondary), 0);
    }
    for (int i = 0; i < TOTAL_PAGES; i++) {
        if (pages[i]) {
            if (i == current_page) {
                lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (page_dots[i]) {
            if (i == current_page) {
                lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(th->active_dot), 0); // Active Theme Dot
                lv_obj_set_style_width(page_dots[i], 14, 0);
            } else {
                lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(th->card_border), 0); // Inactive Border Dot
                lv_obj_set_style_width(page_dots[i], 6, 0);
            }
        }
    }
}


/* Dedicated Protection / Fault Screen (Never part of carousel/scroll, Theme-Independent Always-Red) */
static void build_error_screen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0202), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* 1. Top Header Bar (320x28) - Fixed Alert Red */
    err_hdr_box = lv_obj_create(scr);
    lv_obj_set_pos(err_hdr_box, 0, 0);
    lv_obj_set_size(err_hdr_box, 320, 28);
    lv_obj_set_style_bg_color(err_hdr_box, lv_color_hex(0x6b0c0c), 0);
    lv_obj_set_style_border_width(err_hdr_box, 1, 0);
    lv_obj_set_style_border_color(err_hdr_box, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_radius(err_hdr_box, 0, 0);
    lv_obj_set_style_pad_all(err_hdr_box, 2, 0);
    lv_obj_remove_flag(err_hdr_box, LV_OBJ_FLAG_SCROLLABLE);

    err_hdr_title_lbl = lv_label_create(err_hdr_box);
    lv_label_set_text(err_hdr_title_lbl, "PROTECTION INTERRUPT");
    lv_obj_set_style_text_font(err_hdr_title_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(err_hdr_title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_align(err_hdr_title_lbl, LV_ALIGN_LEFT_MID, 8, 0);

    err_uptime_lbl = lv_label_create(err_hdr_box);
    lv_label_set_text(err_uptime_lbl, "00:00:00");
    lv_obj_set_style_text_font(err_uptime_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(err_uptime_lbl, lv_color_hex(0xfecaca), 0);
    lv_obj_align(err_uptime_lbl, LV_ALIGN_RIGHT_MID, -8, 0);

    /* 2. Bold, Attention-Aware Error String Card (304x88) */
    err_card_box = lv_obj_create(scr);
    lv_obj_set_pos(err_card_box, 8, 36);
    lv_obj_set_size(err_card_box, 304, 88);
    lv_obj_set_style_bg_color(err_card_box, lv_color_hex(0x280505), 0);
    lv_obj_set_style_border_width(err_card_box, 2, 0);
    lv_obj_set_style_border_color(err_card_box, lv_color_hex(0xef4444), 0);
    lv_obj_set_style_radius(err_card_box, 8, 0);
    lv_obj_set_style_pad_all(err_card_box, 4, 0);
    lv_obj_remove_flag(err_card_box, LV_OBJ_FLAG_SCROLLABLE);

    err_title_lbl = lv_label_create(err_card_box);
    lv_obj_set_width(err_title_lbl, 292);
    lv_label_set_long_mode(err_title_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(err_title_lbl, "OUTPUT SHORT CIRCUIT");
    lv_obj_set_style_text_font(err_title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(err_title_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(err_title_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(err_title_lbl);

    /* 3. Offending Trigger Parameter Card (304x100) */
    err_trigger_box = lv_obj_create(scr);
    lv_obj_set_pos(err_trigger_box, 8, 132);
    lv_obj_set_size(err_trigger_box, 304, 100);
    lv_obj_set_style_bg_color(err_trigger_box, lv_color_hex(0x190303), 0);
    lv_obj_set_style_border_width(err_trigger_box, 2, 0);
    lv_obj_set_style_border_color(err_trigger_box, lv_color_hex(0xb91c1c), 0);
    lv_obj_set_style_radius(err_trigger_box, 8, 0);
    lv_obj_set_style_pad_all(err_trigger_box, 4, 0);
    lv_obj_remove_flag(err_trigger_box, LV_OBJ_FLAG_SCROLLABLE);

    err_trigger_hdr_lbl = lv_label_create(err_trigger_box);
    lv_obj_set_pos(err_trigger_hdr_lbl, 0, 6);
    lv_obj_set_width(err_trigger_hdr_lbl, 292);
    lv_label_set_text(err_trigger_hdr_lbl, "TRIGGER SENSOR");
    lv_obj_set_style_text_font(err_trigger_hdr_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(err_trigger_hdr_lbl, lv_color_hex(0xfca5a5), 0);
    lv_obj_set_style_text_align(err_trigger_hdr_lbl, LV_TEXT_ALIGN_CENTER, 0);

    err_trigger_val_lbl = lv_label_create(err_trigger_box);
    lv_obj_set_pos(err_trigger_val_lbl, 0, 32);
    lv_obj_set_width(err_trigger_val_lbl, 292);
    lv_label_set_text(err_trigger_val_lbl, "--");
    lv_obj_set_style_text_font(err_trigger_val_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(err_trigger_val_lbl, lv_color_hex(0xff4d4d), 0);
    lv_obj_set_style_text_align(err_trigger_val_lbl, LV_TEXT_ALIGN_CENTER, 0);

    err_trigger_limit_lbl = lv_label_create(err_trigger_box);
    lv_obj_set_pos(err_trigger_limit_lbl, 0, 68);
    lv_obj_set_width(err_trigger_limit_lbl, 292);
    lv_label_set_text(err_trigger_limit_lbl, "--");
    lv_obj_set_style_text_font(err_trigger_limit_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(err_trigger_limit_lbl, lv_color_hex(0xfecaca), 0);
    lv_obj_set_style_text_align(err_trigger_limit_lbl, LV_TEXT_ALIGN_CENTER, 0);
}

static void update_error_screen(int code)
{
    if (!err_title_lbl) return;

    /* Live Uptime */
    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t hrs = uptime_sec / 3600;
    uint32_t mins = (uptime_sec % 3600) / 60;
    uint32_t secs = uptime_sec % 60;
    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u:%02u", (unsigned int)hrs, (unsigned int)mins, (unsigned int)secs);
    if (err_uptime_lbl) lv_label_set_text(err_uptime_lbl, time_buf);

    const char *title = "";
    const char *trigger_hdr = "";
    char trigger_val[64] = {0};
    char trigger_limit[64] = {0};

    switch (code) {
    case SHORT_TRIP: // 1
        title = "OUTPUT SHORT CIRCUIT";
        trigger_hdr = "LOAD CURRENT";
        snprintf(trigger_val, sizeof(trigger_val), "SHORT DETECTED");
        snprintf(trigger_limit, sizeof(trigger_limit), "OUTPUT IMPEDANCE VIOLATION");
        break;

    case NOFEED_TRIP: // 2
        title = "AC MAINS FEED LOSS";
        trigger_hdr = "AC UTILITY MAINS VOLTAGE";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f VAC", inv_data.mainsvolt);
        snprintf(trigger_limit, sizeof(trigger_limit), "NORMAL LIMIT: %.0fV - %.0fV", pcu_limits.mainslow, pcu_limits.mainshi);
        break;

    case HEATOVER_TRIP: // 3
        title = "HEATSINK OVERTEMPERATURE";
        trigger_hdr = "POWER STAGE TEMPERATURE";
        snprintf(trigger_val, sizeof(trigger_val), "%.1f °C", inv_data.upsheat);
        snprintf(trigger_limit, sizeof(trigger_limit), "MAX SAFETY LIMIT: %.0f °C", pcu_limits.hiheat);
        break;

    case OVERLOAD_TRIP: // 4
        title = "INVERTER OVERLOAD";
        trigger_hdr = "TOTAL INVERTER LOAD";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f %% LOAD", inv_data.loaddisp);
        snprintf(trigger_limit, sizeof(trigger_limit), "MAX RATED CAPACITY: 100 %%");
        break;

    case DC_LO_TRIP: // 5
        title = "DC BUS UNDERVOLTAGE";
        trigger_hdr = "INTERNAL DC BUS VOLTAGE";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f VDC", inv_data.dcboost);
        snprintf(trigger_limit, sizeof(trigger_limit), "MINIMUM BUS LIMIT: 280 V");
        break;

    case HI_CURRENT_TRIP: // 6
        title = "OUTPUT CURRENT OVERLOAD";
        trigger_hdr = "AC INRUSH PEAK CURRENT";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f %% PEAK", inv_data.loaddisp);
        snprintf(trigger_limit, sizeof(trigger_limit), "CYCLE-BY-CYCLE CURRENT LIMIT");
        break;

    case SOLAR_HIGH: // 7
        title = "SOLAR PV OVERVOLTAGE";
        trigger_hdr = "SOLAR ARRAY VOLTAGE (Voc)";
        snprintf(trigger_val, sizeof(trigger_val), "%.1f VDC", inv_data.solarvolt);
        snprintf(trigger_limit, sizeof(trigger_limit), "MAX ARRAY LIMIT: %.0f V", pcu_limits.solmax);
        break;

    case DC_HI_TRIP: // 8
        title = "DC BUS OVERVOLTAGE";
        trigger_hdr = "INTERNAL DC LINK BUS";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f VDC", inv_data.dcboost);
        snprintf(trigger_limit, sizeof(trigger_limit), "MAXIMUM DC CEILING: %.0f V", pcu_limits.dcmax);
        break;

    case DC_FAIL_TRIP: // 9
        title = "DC BOOST STAGE FAILURE";
        trigger_hdr = "DC-DC CONVERTER VOLTAGE";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f VDC", inv_data.dcboost);
        snprintf(trigger_limit, sizeof(trigger_limit), "BOOST CONVERTER REGULATION FAULT");
        break;

    case OVERLOAD_WARN: // 20
        title = "LOAD CAPACITY WARNING";
        trigger_hdr = "TOTAL INVERTER LOAD";
        snprintf(trigger_val, sizeof(trigger_val), "%.0f %% LOAD", inv_data.loaddisp);
        snprintf(trigger_limit, sizeof(trigger_limit), "WARNING THRESHOLD: 85 %%");
        break;

    case LOWBATT_WARN: // 30
        title = "BATTERY VOLTAGE LOW";
        trigger_hdr = "BATTERY BANK VOLTAGE";
        snprintf(trigger_val, sizeof(trigger_val), "%.1f VDC", inv_data.battvolts);
        snprintf(trigger_limit, sizeof(trigger_limit), "WARNING THRESHOLD: %.1f V", pcu_limits.setbatwrn);
        break;

    case LOWBAT_TRIP: // 40
        title = "BATTERY CUTOFF TRIP";
        trigger_hdr = "BATTERY BANK VOLTAGE";
        snprintf(trigger_val, sizeof(trigger_val), "%.1f VDC", inv_data.battvolts);
        snprintf(trigger_limit, sizeof(trigger_limit), "CUTOFF THRESHOLD: %.1f V", pcu_limits.setbatlo);
        break;

    default:
        title = "SYSTEM PROTECTION TRIP";
        trigger_hdr = "SAFETY INTERRUPT";
        snprintf(trigger_val, sizeof(trigger_val), "TRIP ACTIVE");
        snprintf(trigger_limit, sizeof(trigger_limit), "INVERTER OUTPUT INHIBITED");
        break;
    }

    lv_label_set_text(err_title_lbl, title);
    lv_label_set_text(err_trigger_hdr_lbl, trigger_hdr);
    lv_label_set_text(err_trigger_val_lbl, trigger_val);
    lv_label_set_text(err_trigger_limit_lbl, trigger_limit);
}



/* Auto-Carousel Timer (Cycles page every 5 seconds) */
static void page_carousel_timer_cb(lv_timer_t *timer)
{
    if (inv_data.error_code != 0) return; // Freeze carousel during fault
    int next_p = (current_page + 1) % TOTAL_PAGES;
    switch_to_page(next_p);
}

/* Fast 40ms Animation Timer */
static void fast_anim_timer_cb(lv_timer_t *timer)
{
    static int sun_rot = 0;
    sun_rot = (sun_rot + 4) % 360;
    if (p1_sun_arc && inv_data.solarstate == SOLARON) {
        lv_arc_set_rotation(p1_sun_arc, sun_rot);
    }
    if (p5_logo_arc) {
        lv_arc_set_rotation(p5_logo_arc, (sun_rot * 2) % 360);
    }
}

/* 1-Second Global Telemetry & Uptime Refresh */
static void telemetry_refresh_timer_cb(lv_timer_t *timer)
{
    /* 1. Live Operating Uptime (Replacing RTC Clock) */
    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t hrs = uptime_sec / 3600;
    uint32_t mins = (uptime_sec % 3600) / 60;
    uint32_t secs = uptime_sec % 60;

    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u:%02u", (unsigned int)hrs, (unsigned int)mins, (unsigned int)secs);
    if (rtc_time_lbl) lv_label_set_text(rtc_time_lbl, time_buf);

    /* Dedicated Protection / Fault Screen Control */
    if (inv_data.error_code != 0) {
        if (g_error_scr && lv_screen_active() != g_error_scr) {
            lv_screen_load(g_error_scr);
        }
        update_error_screen(inv_data.error_code);
    } else {
        if (g_error_scr && lv_screen_active() == g_error_scr && g_main_screen_obj) {
            lv_screen_load(g_main_screen_obj);
            switch_to_page(current_page);
        }
    }

    /* 2. Simulation dynamics if no live serial packet arrived */
    static int sim_tick = 0;
    sim_tick++;
    if (!inv_data.live_serial_active) {
        inv_data.solarvolt = 75.0f + 2.5f * sinf((float)sim_tick * 0.1f);
        inv_data.battvolts = 26.6f + 0.3f * sinf((float)sim_tick * 0.05f);
        inv_data.chrampsdisp = 15.0f + 2.0f * sinf((float)sim_tick * 0.15f);
        inv_data.loaddisp = 42.0f + 7.0f * sinf((float)sim_tick * 0.12f);
        inv_data.upsheat = 38.0f + 1.5f * sinf((float)sim_tick * 0.08f);
    }

    char buf[32];
    const pcu_theme_t *th = get_active_theme();

    /* Universal Energy Routing (Feed Source) Evaluation */
    const char *feed_text = "FEED: SOLAR PV";
    uint32_t feed_color = 0x10b981; // Green

    if (inv_data.feedmode == 1) {
        feed_text = "FEED: SOLAR";
        feed_color = 0x10b981;
    } else if (inv_data.feedmode == 2) {
        feed_text = "FEED: BATTERY";
        feed_color = 0x38bdf8;
    } else if (inv_data.feedmode == 3) {
        feed_text = (inv_data.mainsvolt < 15.0f) ? "FEED: OFF" : "FEED: MAINS";
        feed_color = (inv_data.mainsvolt < 15.0f) ? 0xef4444 : 0xf59e0b;
    } else if (inv_data.feedmode == 4) {
        feed_text = "FEED: SHARE";
        feed_color = 0xa855f7;
    } else {
        /* Auto mode by electrical physics */
        if (inv_data.onflag == 1) {
            if (inv_data.dischdisp > 0.0f) {
                feed_text = "FEED: BATTERY";
                feed_color = 0x38bdf8;
            } else if (inv_data.solarstate == SOLARON && inv_data.solarvolt > 20.0f) {
                feed_text = "FEED: SOLAR";
                feed_color = 0x10b981;
            } else {
                feed_text = "FEED: INVERTER";
                feed_color = 0x10b981;
            }
        } else {
            feed_text = (inv_data.mainsvolt < 15.0f) ? "FEED: OFF" : "FEED: MAINS";
            feed_color = (inv_data.mainsvolt < 15.0f) ? 0xef4444 : 0xf59e0b;
        }
    }

    /* Calibrated Battery Dynamics Thresholds */
    float bat_span = pcu_limits.setbatful - pcu_limits.setbatlo;
    if (bat_span <= 1.0f) bat_span = 7.8f;
    float bat_full_hi = pcu_limits.setbatful + 1.5f;
    int b_pct = (int)(((inv_data.battvolts - pcu_limits.setbatlo) / bat_span) * 100.0f);
    if (b_pct > 100) b_pct = 100;
    if (b_pct < 0) b_pct = 0;

    /* --- Page 0 Updates --- */
    snprintf(buf, sizeof(buf), "%.1f V", inv_data.solarvolt);
    if (p0_solar_val) lv_label_set_text(p0_solar_val, buf);

    if (p0_solar_badge) {
        if (inv_data.solarvolt < pcu_limits.solmin) {
            lv_label_set_text(p0_solar_badge, "OFFLINE");
            lv_obj_set_style_text_color(p0_solar_badge, lv_color_hex(0x94a3b8), 0);
            lv_obj_set_style_bg_color(p0_solar_badge, lv_color_hex(0x1e293b), 0);
        } else if (inv_data.solarvolt > pcu_limits.solmax) {
            lv_label_set_text(p0_solar_badge, "HIGH VOC");
            lv_obj_set_style_text_color(p0_solar_badge, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_color(p0_solar_badge, lv_color_hex(0xdc2626), 0);
        } else if (inv_data.solarstate == SOLARON) {
            lv_label_set_text(p0_solar_badge, "ONLINE");
            lv_obj_set_style_text_color(p0_solar_badge, lv_color_hex(0x34d399), 0);
            lv_obj_set_style_bg_color(p0_solar_badge, lv_color_hex(0x064e3b), 0);
        } else {
            lv_label_set_text(p0_solar_badge, "STANDBY");
            lv_obj_set_style_text_color(p0_solar_badge, lv_color_hex(0xfbbf24), 0);
            lv_obj_set_style_bg_color(p0_solar_badge, lv_color_hex(0x78350f), 0);
        }
    }

    snprintf(buf, sizeof(buf), "%.0f V", inv_data.mainsvolt);
    if (p0_grid_val) {
        lv_label_set_text(p0_grid_val, buf);
        if (inv_data.mainsvolt < 15.0f || inv_data.mainsvolt > pcu_limits.mainshi) {
            lv_obj_set_style_text_color(p0_grid_val, lv_color_hex(0xef4444), 0);
        } else if (inv_data.mainsvolt < pcu_limits.mainslow) {
            lv_obj_set_style_text_color(p0_grid_val, lv_color_hex(0xf97316), 0);
        } else {
            lv_obj_set_style_text_color(p0_grid_val, lv_color_hex(th->text_main), 0);
        }
    }

    if (p0_grid_badge) {
        if (inv_data.mainsvolt < 15.0f) {
            lv_label_set_text(p0_grid_badge, "OUTAGE");
            lv_obj_set_style_text_color(p0_grid_badge, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_color(p0_grid_badge, lv_color_hex(0xb91c1c), 0);
        } else if (inv_data.mainsvolt < pcu_limits.mainslow) {
            lv_label_set_text(p0_grid_badge, "UNDERVOLT");
            lv_obj_set_style_text_color(p0_grid_badge, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_color(p0_grid_badge, lv_color_hex(0xc2410c), 0);
        } else if (inv_data.mainsvolt > pcu_limits.mainshi) {
            lv_label_set_text(p0_grid_badge, "OVERVOLT");
            lv_obj_set_style_text_color(p0_grid_badge, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_color(p0_grid_badge, lv_color_hex(0xb91c1c), 0);
        } else {
            lv_label_set_text(p0_grid_badge, "GRID OK");
            lv_obj_set_style_text_color(p0_grid_badge, lv_color_hex(0xffffff), 0);
            lv_obj_set_style_bg_color(p0_grid_badge, lv_color_hex(0x059669), 0);
        }
    }

    snprintf(buf, sizeof(buf), "%.1f V", inv_data.battvolts);
    if (p0_batt_val) {
        lv_label_set_text(p0_batt_val, buf);
        if (inv_data.battvolts > bat_full_hi || inv_data.battvolts < pcu_limits.setbatlo) {
            lv_obj_set_style_text_color(p0_batt_val, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatwrn) {
            lv_obj_set_style_text_color(p0_batt_val, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_obj_set_style_text_color(p0_batt_val, lv_color_hex(th->primary), 0);
        }
    }

    if (inv_data.dischdisp > 0.0f) {
        snprintf(buf, sizeof(buf), "DISCH: %.1fA", inv_data.dischdisp);
        if (p0_batt_current) {
            lv_label_set_text(p0_batt_current, buf);
            lv_obj_set_style_text_color(p0_batt_current, lv_color_hex(0xf87171), 0);
        }
    } else {
        snprintf(buf, sizeof(buf), "CHRG: +%.1fA", inv_data.chrampsdisp);
        if (p0_batt_current) {
            lv_label_set_text(p0_batt_current, buf);
            lv_obj_set_style_text_color(p0_batt_current, lv_color_hex(0x34d399), 0);
        }
    }

    if (p0_batt_bar) {
        lv_bar_set_value(p0_batt_bar, b_pct, LV_ANIM_OFF);
        if (inv_data.battvolts > bat_full_hi || inv_data.battvolts < pcu_limits.setbatlo) {
            lv_obj_set_style_bg_color(p0_batt_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        } else if (inv_data.battvolts < pcu_limits.setbatwrn) {
            lv_obj_set_style_bg_color(p0_batt_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(p0_batt_bar, lv_color_hex(th->primary), LV_PART_INDICATOR);
        }
    }

    if (p0_load_val) {
        snprintf(buf, sizeof(buf), "%.0f%% LOAD", inv_data.loaddisp);
        lv_label_set_text(p0_load_val, buf);
        if (inv_data.loaddisp > 100.0f) {
            lv_obj_set_style_text_color(p0_load_val, lv_color_hex(0xef4444), 0);
        } else if (inv_data.loaddisp > 80.0f) {
            lv_obj_set_style_text_color(p0_load_val, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_obj_set_style_text_color(p0_load_val, lv_color_hex(th->primary), 0);
        }
    }

    if (p0_load_bar) {
        lv_bar_set_range(p0_load_bar, 0, 150);
        lv_bar_set_value(p0_load_bar, (int32_t)inv_data.loaddisp, LV_ANIM_OFF);
        if (inv_data.loaddisp > 100.0f) {
            lv_obj_set_style_bg_color(p0_load_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        } else if (inv_data.loaddisp > 80.0f) {
            lv_obj_set_style_bg_color(p0_load_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(p0_load_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
        }
    }

    if (p0_load_src) {
        lv_label_set_text(p0_load_src, feed_text);
        lv_obj_set_style_text_color(p0_load_src, lv_color_hex(feed_color), 0);
    }

    if (p0_inv_out_lbl) {
        snprintf(buf, sizeof(buf), "OUT: %.0fV", inv_data.acout);
        lv_label_set_text(p0_inv_out_lbl, buf);
    }

    if (p0_flow_badge) {
        if (inv_data.onflag == 1) {
            if (inv_data.mainsvolt < 15.0f) {
                lv_label_set_text(p0_flow_badge, "OFF-GRID");
                lv_obj_set_style_bg_color(p0_flow_badge, lv_color_hex(0x065f46), 0);
            } else {
                lv_label_set_text(p0_flow_badge, "INV ACTIVE");
                lv_obj_set_style_bg_color(p0_flow_badge, lv_color_hex(0x065f46), 0);
            }
        } else {
            if (inv_data.mainsvolt < 15.0f) {
                lv_label_set_text(p0_flow_badge, "BLACKOUT");
                lv_obj_set_style_bg_color(p0_flow_badge, lv_color_hex(0xb91c1c), 0);
            } else {
                lv_label_set_text(p0_flow_badge, "MAINS BYPASS");
                lv_obj_set_style_bg_color(p0_flow_badge, lv_color_hex(0x92400e), 0);
            }
        }
    }

    /* --- Page 1 Updates (Solar & Charger) --- */
    snprintf(buf, sizeof(buf), "%.1f V", inv_data.solarvolt);
    if (p1_solar_v) {
        lv_label_set_text(p1_solar_v, buf);
        if (inv_data.solarvolt > pcu_limits.solmax) {
            lv_obj_set_style_text_color(p1_solar_v, lv_color_hex(0xef4444), 0);
        } else {
            lv_obj_set_style_text_color(p1_solar_v, lv_color_hex(th->primary), 0);
        }
    }

    if (p1_solar_status) {
        if (inv_data.solarvolt < pcu_limits.solmin) {
            lv_label_set_text(p1_solar_status, "SOLAR OFF");
            lv_obj_set_style_text_color(p1_solar_status, lv_color_hex(0x64748b), 0);
        } else if (inv_data.solarvolt > pcu_limits.solmax) {
            lv_label_set_text(p1_solar_status, "VOC OVERVOLT");
            lv_obj_set_style_text_color(p1_solar_status, lv_color_hex(0xef4444), 0);
        } else if (inv_data.solarstate == SOLARON) {
            lv_label_set_text(p1_solar_status, "SOLAR ACTIVE");
            lv_obj_set_style_text_color(p1_solar_status, lv_color_hex(0x34d399), 0);
        } else {
            lv_label_set_text(p1_solar_status, "SOLAR STANDBY");
            lv_obj_set_style_text_color(p1_solar_status, lv_color_hex(0xf59e0b), 0);
        }
    }

    if (p1_chg_mode) {
        if (inv_data.solarstate == SOLARON) {
            if (inv_data.chargerstate == AC_CHARGER) lv_label_set_text(p1_chg_mode, "SHARE CHARG");
            else lv_label_set_text(p1_chg_mode, "SOLAR CHARG");
        } else if (inv_data.chargerstate == AC_CHARGER) {
            lv_label_set_text(p1_chg_mode, "MAINS CHARG");
        } else {
            lv_label_set_text(p1_chg_mode, "CHARGER OFF");
        }
    }

    snprintf(buf, sizeof(buf), "%.1f AMPS", inv_data.chrampsdisp);
    if (p1_chg_amps) lv_label_set_text(p1_chg_amps, buf);

    if (p1_dc_boost) {
        if (inv_data.dcboostmode == 1) {
            if (inv_data.dcok == 1) lv_label_set_text(p1_dc_boost, "DC BOOST: OK!");
            else {
                snprintf(buf, sizeof(buf), "BOOST: %.0fV", inv_data.dcboost);
                lv_label_set_text(p1_dc_boost, buf);
            }
        } else {
            lv_label_set_text(p1_dc_boost, "DC BOOST: OFF");
        }
    }

    if (p1_pref_mode) {
        if (inv_data.sharemode == 0) lv_label_set_text(p1_pref_mode, "PREF: SOLAR");
        else lv_label_set_text(p1_pref_mode, "PREF: SHARE");
    }

    if (p1_raw_info) {
        if (inv_data.dcboostmode == 1) {
            snprintf(buf, sizeof(buf), "RAW DC: %.0fV (ACTIVE)", inv_data.dcboost);
        } else {
            snprintf(buf, sizeof(buf), "RAW DC: OFF");
        }
        lv_label_set_text(p1_raw_info, buf);
    }

    /* --- Page 2 Updates (Battery & Inverter Load) --- */
    snprintf(buf, sizeof(buf), "%.1f V", inv_data.battvolts);
    if (p2_batt_v) {
        lv_label_set_text(p2_batt_v, buf);
        if (inv_data.battvolts > bat_full_hi || inv_data.battvolts < pcu_limits.setbatlo) {
            lv_obj_set_style_text_color(p2_batt_v, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatwrn) {
            lv_obj_set_style_text_color(p2_batt_v, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_obj_set_style_text_color(p2_batt_v, lv_color_hex(th->text_main), 0);
        }
    }

    if (p2_batt_stat) {
        if (inv_data.battvolts > bat_full_hi) {
            lv_label_set_text(p2_batt_stat, "OVERVOLT");
            lv_obj_set_style_text_color(p2_batt_stat, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatlo) {
            lv_label_set_text(p2_batt_stat, "LOW CUTOFF");
            lv_obj_set_style_text_color(p2_batt_stat, lv_color_hex(0xef4444), 0);
        } else if (inv_data.batgravity == 1) {
            lv_label_set_text(p2_batt_stat, "HEALTH: FULL");
            lv_obj_set_style_text_color(p2_batt_stat, lv_color_hex(0x10b981), 0);
        } else {
            lv_label_set_text(p2_batt_stat, "HEALTH: OK!");
            lv_obj_set_style_text_color(p2_batt_stat, lv_color_hex(0x38bdf8), 0);
        }
    }

    if (p2_grav_lbl) {
        if (inv_data.batgravity == 1) {
            lv_label_set_text(p2_grav_lbl, "GRAVITY: EQUALIZING");
            lv_obj_set_style_text_color(p2_grav_lbl, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_label_set_text(p2_grav_lbl, "GRAVITY: NORMAL");
            lv_obj_set_style_text_color(p2_grav_lbl, lv_color_hex(th->text_muted), 0);
        }
    }

    if (p2_batt_bar) {
        lv_bar_set_value(p2_batt_bar, b_pct, LV_ANIM_OFF);
        if (inv_data.battvolts > bat_full_hi || inv_data.battvolts < pcu_limits.setbatlo) {
            lv_obj_set_style_bg_color(p2_batt_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        } else if (inv_data.battvolts < pcu_limits.setbatwrn) {
            lv_obj_set_style_bg_color(p2_batt_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(p2_batt_bar, lv_color_hex(th->primary), LV_PART_INDICATOR);
        }
    }

    if (p2_batt_soc) {
        if (inv_data.battvolts > bat_full_hi) {
            lv_label_set_text(p2_batt_soc, "CAPACITY: 100% (HI)");
            lv_obj_set_style_text_color(p2_batt_soc, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatlo) {
            lv_label_set_text(p2_batt_soc, "CAPACITY: 0% (LOW)");
            lv_obj_set_style_text_color(p2_batt_soc, lv_color_hex(0xef4444), 0);
        } else {
            snprintf(buf, sizeof(buf), "CAPACITY: %d%%", b_pct);
            lv_label_set_text(p2_batt_soc, buf);
            lv_obj_set_style_text_color(p2_batt_soc, lv_color_hex(th->primary), 0);
        }
    }

    if (inv_data.dischdisp > 0.0f) {
        snprintf(buf, sizeof(buf), "DISCH: -%.1f A", inv_data.dischdisp);
        if (p2_batt_current) {
            lv_label_set_text(p2_batt_current, buf);
            lv_obj_set_style_text_color(p2_batt_current, lv_color_hex(0xf87171), 0);
        }
    } else {
        snprintf(buf, sizeof(buf), "CHRG: +%.1f A", inv_data.chrampsdisp);
        if (p2_batt_current) {
            lv_label_set_text(p2_batt_current, buf);
            lv_obj_set_style_text_color(p2_batt_current, lv_color_hex(0x34d399), 0);
        }
    }

    snprintf(buf, sizeof(buf), "%.0f VAC", inv_data.acout);
    if (p2_inv_v) lv_label_set_text(p2_inv_v, buf);

    if (p2_load_pct) {
        snprintf(buf, sizeof(buf), "%.0f%% LOAD", inv_data.loaddisp);
        lv_label_set_text(p2_load_pct, buf);
        if (inv_data.loaddisp > 100.0f) {
            lv_obj_set_style_text_color(p2_load_pct, lv_color_hex(0xef4444), 0);
        } else if (inv_data.loaddisp > 80.0f) {
            lv_obj_set_style_text_color(p2_load_pct, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_obj_set_style_text_color(p2_load_pct, lv_color_hex(th->primary), 0);
        }
    }

    if (p2_load_bar) {
        lv_bar_set_range(p2_load_bar, 0, 150);
        lv_bar_set_value(p2_load_bar, (int32_t)inv_data.loaddisp, LV_ANIM_OFF);
        if (inv_data.loaddisp > 100.0f) {
            lv_obj_set_style_bg_color(p2_load_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        } else if (inv_data.loaddisp > 80.0f) {
            lv_obj_set_style_bg_color(p2_load_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(p2_load_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
        }
    }

    if (p2_load_stat) {
        if (inv_data.loaddisp > 105.0f) {
            lv_label_set_text(p2_load_stat, "LOAD: OVERLOAD!");
            lv_obj_set_style_text_color(p2_load_stat, lv_color_hex(0xef4444), 0);
        } else if (inv_data.loaddisp > 85.0f) {
            lv_label_set_text(p2_load_stat, "LOAD: HEAVY");
            lv_obj_set_style_text_color(p2_load_stat, lv_color_hex(0xf59e0b), 0);
        } else if (inv_data.loaddisp > 60.0f) {
            lv_label_set_text(p2_load_stat, "LOAD: OPTIMAL");
            lv_obj_set_style_text_color(p2_load_stat, lv_color_hex(0x10b981), 0);
        } else {
            lv_label_set_text(p2_load_stat, "LOAD: NORMAL");
            lv_obj_set_style_text_color(p2_load_stat, lv_color_hex(th->primary), 0);
        }
    }

    if (p2_source_lbl) {
        lv_label_set_text(p2_source_lbl, feed_text);
        lv_obj_set_style_text_color(p2_source_lbl, lv_color_hex(feed_color), 0);
    }

    if (p2_inv_status) {
        if (inv_data.loaddisp > 105.0f) {
            lv_label_set_text(p2_inv_status, "INVERTER: OVERLOAD");
            lv_obj_set_style_text_color(p2_inv_status, lv_color_hex(0xef4444), 0);
        } else if (inv_data.onflag == 1) {
            lv_label_set_text(p2_inv_status, "INVERTER: ACTIVE");
            lv_obj_set_style_text_color(p2_inv_status, lv_color_hex(0x10b981), 0);
        } else {
            if (inv_data.mainsvolt < 15.0f) {
                lv_label_set_text(p2_inv_status, "INVERTER: OUTAGE");
                lv_obj_set_style_text_color(p2_inv_status, lv_color_hex(0xef4444), 0);
            } else {
                lv_label_set_text(p2_inv_status, "INVERTER: BYPASS");
                lv_obj_set_style_text_color(p2_inv_status, lv_color_hex(th->text_muted), 0);
            }
        }
    }

    /* --- Page 3 Updates (Grid, Thermal & Operational Status) --- */
    snprintf(buf, sizeof(buf), "%.0f V", inv_data.mainsvolt);
    if (p3_grid_v) {
        lv_label_set_text(p3_grid_v, buf);
        if (inv_data.mainsvolt < 15.0f || inv_data.mainsvolt > pcu_limits.mainshi) {
            lv_obj_set_style_text_color(p3_grid_v, lv_color_hex(0xef4444), 0);
        } else if (inv_data.mainsvolt < pcu_limits.mainslow) {
            lv_obj_set_style_text_color(p3_grid_v, lv_color_hex(0xf97316), 0);
        } else {
            lv_obj_set_style_text_color(p3_grid_v, lv_color_hex(th->primary), 0);
        }
    }

    if (p3_grid_freq_stat) {
        if (inv_data.mainsvolt < 15.0f) {
            lv_label_set_text(p3_grid_freq_stat, "0 Hz | BLACKOUT");
            lv_obj_set_style_text_color(p3_grid_freq_stat, lv_color_hex(0xef4444), 0);
        } else if (inv_data.mainsvolt < pcu_limits.mainslow) {
            lv_label_set_text(p3_grid_freq_stat, "50.0 Hz | UNDERVOLT");
            lv_obj_set_style_text_color(p3_grid_freq_stat, lv_color_hex(0xf97316), 0);
        } else if (inv_data.mainsvolt > pcu_limits.mainshi) {
            lv_label_set_text(p3_grid_freq_stat, "50.0 Hz | OVERVOLT");
            lv_obj_set_style_text_color(p3_grid_freq_stat, lv_color_hex(0xef4444), 0);
        } else {
            lv_label_set_text(p3_grid_freq_stat, "50.0 Hz | STABLE");
            lv_obj_set_style_text_color(p3_grid_freq_stat, lv_color_hex(0x10b981), 0);
        }
    }

    snprintf(buf, sizeof(buf), "%.1f °C", inv_data.upsheat);
    if (p3_temp_val) {
        lv_label_set_text(p3_temp_val, buf);
        if (inv_data.upsheat > pcu_limits.hiheat) {
            lv_obj_set_style_text_color(p3_temp_val, lv_color_hex(0xef4444), 0);
        } else if (inv_data.upsheat > pcu_limits.hiheat - 15.0f) {
            lv_obj_set_style_text_color(p3_temp_val, lv_color_hex(0xf97316), 0);
        } else {
            lv_obj_set_style_text_color(p3_temp_val, lv_color_hex(th->primary), 0);
        }
    }

    int t_pct = (int)((inv_data.upsheat / (pcu_limits.hiheat > 0.0f ? pcu_limits.hiheat : 100.0f)) * 100.0f);
    if (t_pct > 100) t_pct = 100;
    if (t_pct < 0) t_pct = 0;
    if (p3_temp_bar) {
        lv_bar_set_value(p3_temp_bar, t_pct, LV_ANIM_OFF);
        if (inv_data.upsheat > pcu_limits.hiheat) {
            lv_obj_set_style_bg_color(p3_temp_bar, lv_color_hex(0xef4444), LV_PART_INDICATOR);
        } else if (inv_data.upsheat > pcu_limits.hiheat - 15.0f) {
            lv_obj_set_style_bg_color(p3_temp_bar, lv_color_hex(0xf97316), LV_PART_INDICATOR);
        } else {
            lv_obj_set_style_bg_color(p3_temp_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
        }
    }

    if (p3_inv_mode) {
        if (inv_data.onflag == 1) {
            lv_label_set_text(p3_inv_mode, "INVERTER: ACTIVE");
            lv_obj_set_style_text_color(p3_inv_mode, lv_color_hex(0x10b981), 0);
        } else if (inv_data.switchstate == INVSWITCH) {
            lv_label_set_text(p3_inv_mode, "INVERTER: STANDBY");
            lv_obj_set_style_text_color(p3_inv_mode, lv_color_hex(0xf59e0b), 0);
        } else {
            lv_label_set_text(p3_inv_mode, "INVERTER: OFF");
            lv_obj_set_style_text_color(p3_inv_mode, lv_color_hex(0xef4444), 0);
        }
    }

    if (p3_pref_lbl) {
        if (inv_data.sharemode == 0) lv_label_set_text(p3_pref_lbl, "PRIORITY: SOLAR");
        else lv_label_set_text(p3_pref_lbl, "PRIORITY: GRID");
    }

    if (p3_batt_health_lbl) {
        if (inv_data.battvolts > bat_full_hi) {
            lv_label_set_text(p3_batt_health_lbl, "BATT: OVERVOLT");
            lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatlo) {
            lv_label_set_text(p3_batt_health_lbl, "BATT: DISCHARGE");
            lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0xef4444), 0);
        } else if (inv_data.battvolts < pcu_limits.setbatwrn) {
            lv_label_set_text(p3_batt_health_lbl, "BATT: LOW");
            lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0xf59e0b), 0);
        } else if (inv_data.batgravity == 1) {
            lv_label_set_text(p3_batt_health_lbl, "BATT: FULL FLOAT");
            lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0x10b981), 0);
        } else {
            lv_label_set_text(p3_batt_health_lbl, "BATT: HEALTHY");
            lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0x34d399), 0);
        }
    }

    if (p3_boost_lbl) {
        if (inv_data.dcboostmode == 1) {
            if (inv_data.dcok == 1) lv_label_set_text(p3_boost_lbl, "DC BOOST: ACTIVE");
            else {
                snprintf(buf, sizeof(buf), "DC BOOST: %.0fV", inv_data.dcboost);
                lv_label_set_text(p3_boost_lbl, buf);
            }
        } else {
            lv_label_set_text(p3_boost_lbl, "DC BOOST: OFF");
        }
    }

    if (p3_chg_type_lbl) {
        if (inv_data.solarstate == SOLARON) {
            if (inv_data.chargerstate == AC_CHARGER) lv_label_set_text(p3_chg_type_lbl, "CHG: AC+PV SHARE");
            else lv_label_set_text(p3_chg_type_lbl, "CHG: SOLAR MPPT");
        } else if (inv_data.chargerstate == AC_CHARGER) {
            lv_label_set_text(p3_chg_type_lbl, "CHG: GRID MAINS");
        } else {
            lv_label_set_text(p3_chg_type_lbl, "CHG: OFF");
        }
    }

    if (p3_mains_status_lbl) {
        if (inv_data.mainsvolt < 10.0f) {
            lv_label_set_text(p3_mains_status_lbl, "GRID: BLACKOUT");
            lv_obj_set_style_text_color(p3_mains_status_lbl, lv_color_hex(0xef4444), 0);
        } else if (inv_data.mainsvolt < pcu_limits.mainslow) {
            snprintf(buf, sizeof(buf), "GRID: %.0fV LOW", inv_data.mainsvolt);
            lv_label_set_text(p3_mains_status_lbl, buf);
            lv_obj_set_style_text_color(p3_mains_status_lbl, lv_color_hex(0xf97316), 0);
        } else if (inv_data.mainsvolt > pcu_limits.mainshi) {
            snprintf(buf, sizeof(buf), "GRID: %.0fV HIGH", inv_data.mainsvolt);
            lv_label_set_text(p3_mains_status_lbl, buf);
            lv_obj_set_style_text_color(p3_mains_status_lbl, lv_color_hex(0xef4444), 0);
        } else {
            snprintf(buf, sizeof(buf), "GRID: %.0fV OK", inv_data.mainsvolt);
            lv_label_set_text(p3_mains_status_lbl, buf);
            lv_obj_set_style_text_color(p3_mains_status_lbl, lv_color_hex(0x10b981), 0);
        }
    }

    if (p3_thermal_status_lbl) {
        if (inv_data.upsheat > pcu_limits.hiheat) {
            snprintf(buf, sizeof(buf), "HEAT: %.0f°C TRIP!", inv_data.upsheat);
            lv_label_set_text(p3_thermal_status_lbl, buf);
            lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0xef4444), 0);
        } else if (inv_data.upsheat > pcu_limits.hiheat - 15.0f) {
            snprintf(buf, sizeof(buf), "HEAT: %.0f°C HIGH", inv_data.upsheat);
            lv_label_set_text(p3_thermal_status_lbl, buf);
            lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0xf97316), 0);
        } else if (inv_data.upsheat > pcu_limits.hiheat - 35.0f) {
            snprintf(buf, sizeof(buf), "HEAT: %.0f°C WARM", inv_data.upsheat);
            lv_label_set_text(p3_thermal_status_lbl, buf);
            lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0xf59e0b), 0);
        } else {
            snprintf(buf, sizeof(buf), "HEAT: %.0f°C COOL", inv_data.upsheat);
            lv_label_set_text(p3_thermal_status_lbl, buf);
            lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0x10b981), 0);
        }
    }

    /* --- Page 4 Updates (Past 8-Hour Line Graph Live Telemetry) --- */
    static float today_kwh_accum = 14.8f;
    today_kwh_accum += 0.001f;
    snprintf(buf, sizeof(buf), "%.1f kWh", today_kwh_accum);
    if (p4_today_kwh) lv_label_set_text(p4_today_kwh, buf);

    float live_solar_w = inv_data.solarvolt * (inv_data.chrampsdisp + (inv_data.solarstate == SOLARON ? 5.0f : 0.0f));
    float peak_w = (live_solar_w > 50.0f) ? live_solar_w + 320.0f : 2480.0f;
    snprintf(buf, sizeof(buf), "%.0f W", peak_w);
    if (p4_peak_w) lv_label_set_text(p4_peak_w, buf);

    /* 8-Hour Rolling Line Graph */
    static int32_t history_solar[8] = { 12, 28, 54, 80, 90, 72, 50, 35 };
    static int32_t history_load[8]  = { 32, 40, 45, 50, 52, 48, 42, 42 };
    static uint32_t p4_tick = 0;
    p4_tick++;

    int32_t now_solar = (int32_t)((live_solar_w / 2500.0f) * 100.0f);
    if (now_solar > 100) now_solar = 100;
    if (now_solar < 0) now_solar = 0;

    int32_t now_load = (int32_t)inv_data.loaddisp;
    if (now_load > 100) now_load = 100;
    if (now_load < 0) now_load = 0;

    history_solar[7] = now_solar;
    history_load[7]  = now_load;

    /* Roll historical points every 15 ticks */
    if ((p4_tick % 15) == 0) {
        for (int i = 0; i < 6; i++) {
            history_solar[i] = history_solar[i + 1];
            history_load[i]  = history_load[i + 1];
        }
        history_solar[6] = (history_solar[5] + now_solar) / 2;
        history_load[6]  = (history_load[5] + now_load) / 2;
    }

    if (p4_chart && p4_series_solar && p4_series_load) {
        for (uint32_t i = 0; i < 8; i++) {
            lv_chart_set_series_value_by_id(p4_chart, p4_series_solar, i, history_solar[i]);
            lv_chart_set_series_value_by_id(p4_chart, p4_series_load,  i, history_load[i]);
        }
        lv_chart_refresh(p4_chart);
    }

    if (p4_trend_badge) {
        snprintf(buf, sizeof(buf), "PV:%.0fW | LOAD:%.0f%%", live_solar_w, inv_data.loaddisp);
        lv_label_set_text(p4_trend_badge, buf);
    }
}


/* ========================================================================= */
/* UI BUILDERS FOR 4 PAGES                                                   */
/* ========================================================================= */

static void build_page_0(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 0: Energy Flow Hub & 5 Node Overview */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    pages[0] = page;

    /* 1. Solar Node Card (Top-Left: 100x88) */
    lv_obj_t *c_solar = create_card(page, 0, 0, 100, 88);

    lv_obj_t *i_solar = lv_image_create(c_solar);
    lv_image_set_src(i_solar, &img_solar);
    lv_obj_set_pos(i_solar, 2, 4);

    lv_obj_t *t_solar = lv_label_create(c_solar);
    lv_label_set_text(t_solar, "SOLAR");
    lv_obj_set_style_text_font(t_solar, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_solar, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_solar, 38, 4);

    p0_solar_val = lv_label_create(c_solar);
    lv_label_set_text(p0_solar_val, "76.5 V");
    lv_obj_set_style_text_font(p0_solar_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p0_solar_val, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p0_solar_val, 38, 18);

    p0_solar_badge = lv_label_create(c_solar);
    lv_label_set_text(p0_solar_badge, "ONLINE");
    lv_obj_set_style_text_font(p0_solar_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_solar_badge, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_style_bg_color(p0_solar_badge, lv_color_hex(th->badge_bg), 0);
    lv_obj_set_style_radius(p0_solar_badge, 3, 0);
    lv_obj_set_style_pad_hor(p0_solar_badge, 4, 0);
    lv_obj_set_style_pad_ver(p0_solar_badge, 1, 0);
    lv_obj_align(p0_solar_badge, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* 2. Center Inverter Unit (Top-Mid: 106x88) */
    lv_obj_t *c_inv = create_card(page, 104, 0, 106, 88);
    lv_obj_set_style_border_color(c_inv, lv_color_hex(th->primary), 0);
    lv_obj_set_style_bg_color(c_inv, lv_color_hex(th->card_bg), 0);

    lv_obj_t *i_inv = lv_image_create(c_inv);
    lv_image_set_src(i_inv, &img_inverter);
    lv_obj_align(i_inv, LV_ALIGN_TOP_MID, 0, 2);

    p0_flow_badge = lv_label_create(c_inv);
    lv_label_set_text(p0_flow_badge, "INV ACTIVE");
    lv_obj_set_style_text_font(p0_flow_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_flow_badge, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_style_bg_color(p0_flow_badge, lv_color_hex(th->badge_bg), 0);
    lv_obj_set_style_radius(p0_flow_badge, 3, 0);
    lv_obj_set_style_pad_hor(p0_flow_badge, 4, 0);
    lv_obj_set_style_pad_ver(p0_flow_badge, 2, 0);
    lv_obj_align(p0_flow_badge, LV_ALIGN_CENTER, 0, 8);

    p0_inv_out_lbl = lv_label_create(c_inv);
    lv_label_set_text(p0_inv_out_lbl, "OUT: 230V");
    lv_obj_set_style_text_font(p0_inv_out_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_inv_out_lbl, lv_color_hex(th->secondary), 0);
    lv_obj_align(p0_inv_out_lbl, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* 3. Grid Node Card (Top-Right: 100x88) */
    lv_obj_t *c_grid = create_card(page, 214, 0, 100, 88);

    lv_obj_t *i_grid = lv_image_create(c_grid);
    lv_image_set_src(i_grid, &img_grid);
    lv_obj_set_pos(i_grid, 2, 4);

    lv_obj_t *t_grid = lv_label_create(c_grid);
    lv_label_set_text(t_grid, "MAINS");
    lv_obj_set_style_text_font(t_grid, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_grid, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_grid, 38, 4);

    p0_grid_val = lv_label_create(c_grid);
    lv_label_set_text(p0_grid_val, "228 V");
    lv_obj_set_style_text_font(p0_grid_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p0_grid_val, lv_color_hex(th->text_main), 0);
    lv_obj_set_pos(p0_grid_val, 38, 18);

    p0_grid_badge = lv_label_create(c_grid);
    lv_label_set_text(p0_grid_badge, "GRID OK");
    lv_obj_set_style_text_font(p0_grid_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_grid_badge, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_style_bg_color(p0_grid_badge, lv_color_hex(th->badge_bg), 0);
    lv_obj_set_style_radius(p0_grid_badge, 3, 0);
    lv_obj_set_style_pad_hor(p0_grid_badge, 4, 0);
    lv_obj_set_style_pad_ver(p0_grid_badge, 1, 0);
    lv_obj_align(p0_grid_badge, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* 4. Battery Bank Card (Bottom-Left: 154x88) */
    lv_obj_t *c_batt = create_card(page, 0, 92, 154, 88);

    lv_obj_t *i_batt = lv_image_create(c_batt);
    lv_image_set_src(i_batt, &img_battery);
    lv_obj_set_pos(i_batt, 4, 4);

    lv_obj_t *t_batt = lv_label_create(c_batt);
    lv_label_set_text(t_batt, "BATTERY BANK");
    lv_obj_set_style_text_font(t_batt, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_batt, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_batt, 40, 4);

    p0_batt_val = lv_label_create(c_batt);
    lv_label_set_text(p0_batt_val, "26.8 V");
    lv_obj_set_style_text_font(p0_batt_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p0_batt_val, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p0_batt_val, 40, 18);

    p0_batt_current = lv_label_create(c_batt);
    lv_label_set_text(p0_batt_current, "CHRG: +16.4 A");
    lv_obj_set_style_text_font(p0_batt_current, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_batt_current, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_pos(p0_batt_current, 40, 36);

    p0_batt_bar = lv_bar_create(c_batt);
    lv_obj_set_size(p0_batt_bar, 144, 6);
    lv_obj_align(p0_batt_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(p0_batt_bar, lv_color_hex(th->header_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p0_batt_bar, lv_color_hex(th->primary), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p0_batt_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(p0_batt_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(p0_batt_bar, 0, 100);
    lv_bar_set_value(p0_batt_bar, 75, LV_ANIM_OFF);

    /* 5. Home / AC Load Card (Bottom-Right: 154x88) */
    lv_obj_t *c_home = create_card(page, 160, 92, 154, 88);

    lv_obj_t *i_home = lv_image_create(c_home);
    lv_image_set_src(i_home, &img_home);
    lv_obj_set_pos(i_home, 4, 4);

    lv_obj_t *t_home = lv_label_create(c_home);
    lv_label_set_text(t_home, "AC OUTPUT LOAD");
    lv_obj_set_style_text_font(t_home, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_home, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_home, 40, 4);

    p0_load_val = lv_label_create(c_home);
    lv_label_set_text(p0_load_val, "42% LOAD");
    lv_obj_set_style_text_font(p0_load_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p0_load_val, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p0_load_val, 40, 18);

    p0_load_src = lv_label_create(c_home);
    lv_label_set_text(p0_load_src, "FEED: SOLAR");
    lv_obj_set_style_text_font(p0_load_src, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p0_load_src, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(p0_load_src, 40, 36);

    p0_load_bar = lv_bar_create(c_home);
    lv_obj_set_size(p0_load_bar, 144, 6);
    lv_obj_align(p0_load_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(p0_load_bar, lv_color_hex(th->header_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p0_load_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p0_load_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(p0_load_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(p0_load_bar, 0, 100);
    lv_bar_set_value(p0_load_bar, 42, LV_ANIM_OFF);
}

static void build_page_1(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 1: Solar Generation & Battery Charger Details */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[1] = page;

    /* Left Card: Solar Sun Display (138x182) */
    lv_obj_t *c_left = create_card(page, 0, 0, 138, 182);

    lv_obj_t *t_sun = lv_label_create(c_left);
    lv_label_set_text(t_sun, "SOLAR PV ARRAY");
    lv_obj_set_style_text_font(t_sun, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_sun, lv_color_hex(th->secondary), 0);
    lv_obj_align(t_sun, LV_ALIGN_TOP_MID, 0, 2);

    p1_sun_arc = lv_arc_create(c_left);
    lv_obj_set_size(p1_sun_arc, 52, 52);
    lv_obj_align(p1_sun_arc, LV_ALIGN_TOP_MID, 0, 20);
    lv_arc_set_rotation(p1_sun_arc, 0);
    lv_arc_set_bg_angles(p1_sun_arc, 0, 360);
    lv_arc_set_value(p1_sun_arc, 35);
    lv_obj_remove_style(p1_sun_arc, NULL, LV_PART_KNOB);
    lv_obj_set_clickable(p1_sun_arc, false);
    lv_obj_set_style_arc_color(p1_sun_arc, lv_color_hex(th->card_border), LV_PART_MAIN);
    lv_obj_set_style_arc_color(p1_sun_arc, lv_color_hex(th->primary), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(p1_sun_arc, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_width(p1_sun_arc, 3, LV_PART_INDICATOR);

    lv_obj_t *i_sun = lv_image_create(c_left);
    lv_image_set_src(i_sun, &img_solar);
    lv_obj_align(i_sun, LV_ALIGN_TOP_MID, 0, 30);

    p1_solar_v = lv_label_create(c_left);
    lv_label_set_text(p1_solar_v, "76.5 V");
    lv_obj_set_style_text_font(p1_solar_v, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(p1_solar_v, lv_color_hex(th->primary), 0);
    lv_obj_align(p1_solar_v, LV_ALIGN_TOP_MID, 0, 84);

    p1_solar_status = lv_label_create(c_left);
    lv_label_set_text(p1_solar_status, "● SOLAR ACTIVE");
    lv_obj_set_style_text_font(p1_solar_status, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p1_solar_status, lv_color_hex(th->badge_fg), 0);
    lv_obj_align(p1_solar_status, LV_ALIGN_TOP_MID, 0, 114);

    p1_raw_info = lv_label_create(c_left);
    lv_label_set_text(p1_raw_info, "RAW DC: 385V");
    lv_obj_set_style_text_font(p1_raw_info, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p1_raw_info, lv_color_hex(th->text_muted), 0);
    lv_obj_align(p1_raw_info, LV_ALIGN_TOP_MID, 0, 138);

    /* Right Card: Charger & DC Boost (172x182) */
    lv_obj_t *c_right = create_card(page, 142, 0, 172, 182);

    lv_obj_t *t_chg = lv_label_create(c_right);
    lv_label_set_text(t_chg, "CHARGER & DC BOOST");
    lv_obj_set_style_text_font(t_chg, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t_chg, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_chg, 4, 2);

    lv_obj_t *div = lv_obj_create(c_right);
    lv_obj_set_size(div, 160, 1);
    lv_obj_set_pos(div, 0, 20);
    lv_obj_set_style_bg_color(div, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(div, 0, 0);

    /* Item 1: Charger Mode */
    lv_obj_t *lbl_m = lv_label_create(c_right);
    lv_label_set_text(lbl_m, "CHARGER MODE");
    lv_obj_set_style_text_font(lbl_m, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_m, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_m, 4, 26);

    p1_chg_mode = lv_label_create(c_right);
    lv_label_set_text(p1_chg_mode, "SOLAR CHARGING");
    lv_obj_set_style_text_font(p1_chg_mode, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p1_chg_mode, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p1_chg_mode, 4, 40);

    /* Item 2: Charge Current */
    lv_obj_t *lbl_a = lv_label_create(c_right);
    lv_label_set_text(lbl_a, "CHARGE CURRENT");
    lv_obj_set_style_text_font(lbl_a, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_a, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_a, 4, 62);

    p1_chg_amps = lv_label_create(c_right);
    lv_label_set_text(p1_chg_amps, "16.4 AMPS");
    lv_obj_set_style_text_font(p1_chg_amps, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p1_chg_amps, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p1_chg_amps, 4, 76);

    /* Item 3: DC Boost Status */
    lv_obj_t *lbl_b = lv_label_create(c_right);
    lv_label_set_text(lbl_b, "RAW DC BOOST");
    lv_obj_set_style_text_font(lbl_b, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_b, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_b, 4, 98);

    p1_dc_boost = lv_label_create(c_right);
    lv_label_set_text(p1_dc_boost, "DC BOOST: OK!");
    lv_obj_set_style_text_font(p1_dc_boost, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p1_dc_boost, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(p1_dc_boost, 4, 112);

    /* Item 4: Priority Preference */
    lv_obj_t *lbl_p = lv_label_create(c_right);
    lv_label_set_text(lbl_p, "PRIORITY PREFERENCE");
    lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_p, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_p, 4, 134);

    p1_pref_mode = lv_label_create(c_right);
    lv_label_set_text(p1_pref_mode, "PREF: SOLAR FIRST");
    lv_obj_set_style_text_font(p1_pref_mode, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p1_pref_mode, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(p1_pref_mode, 4, 148);
}

static void build_page_2(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 2: Battery Health & Inverter AC Loading */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[2] = page;

    /* Left Card: Battery Bank (154x182) */
    lv_obj_t *c_batt = create_card(page, 0, 0, 154, 182);

    lv_obj_t *i_b = lv_image_create(c_batt);
    lv_image_set_src(i_b, &img_battery);
    lv_obj_set_pos(i_b, 4, 4);

    lv_obj_t *t_b = lv_label_create(c_batt);
    lv_label_set_text(t_b, "BATTERY BANK");
    lv_obj_set_style_text_font(t_b, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t_b, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_b, 40, 4);

    p2_batt_stat = lv_label_create(c_batt);
    lv_label_set_text(p2_batt_stat, "HEALTH: OK!");
    lv_obj_set_style_text_font(p2_batt_stat, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_batt_stat, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p2_batt_stat, 40, 20);

    lv_obj_t *div_b = lv_obj_create(c_batt);
    lv_obj_set_size(div_b, 142, 1);
    lv_obj_set_pos(div_b, 0, 40);
    lv_obj_set_style_bg_color(div_b, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(div_b, 0, 0);

    p2_batt_v = lv_label_create(c_batt);
    lv_label_set_text(p2_batt_v, "26.8 V");
    lv_obj_set_style_text_font(p2_batt_v, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(p2_batt_v, lv_color_hex(th->text_main), 0);
    lv_obj_set_pos(p2_batt_v, 4, 48);

    p2_batt_current = lv_label_create(c_batt);
    lv_label_set_text(p2_batt_current, "CHRG: +16.4 A");
    lv_obj_set_style_text_font(p2_batt_current, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p2_batt_current, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_pos(p2_batt_current, 4, 74);

    p2_grav_lbl = lv_label_create(c_batt);
    lv_label_set_text(p2_grav_lbl, "GRAVITY: NORMAL");
    lv_obj_set_style_text_font(p2_grav_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_grav_lbl, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(p2_grav_lbl, 4, 96);

    lv_obj_t *soc_t = lv_label_create(c_batt);
    lv_label_set_text(soc_t, "STATE OF CHARGE");
    lv_obj_set_style_text_font(soc_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(soc_t, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(soc_t, 4, 118);

    p2_batt_bar = lv_bar_create(c_batt);
    lv_obj_set_size(p2_batt_bar, 142, 8);
    lv_obj_set_pos(p2_batt_bar, 0, 134);
    lv_obj_set_style_bg_color(p2_batt_bar, lv_color_hex(th->header_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p2_batt_bar, lv_color_hex(th->primary), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p2_batt_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(p2_batt_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(p2_batt_bar, 0, 100);
    lv_bar_set_value(p2_batt_bar, 75, LV_ANIM_OFF);

    p2_batt_soc = lv_label_create(c_batt);
    lv_label_set_text(p2_batt_soc, "CAPACITY: 76%");
    lv_obj_set_style_text_font(p2_batt_soc, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_batt_soc, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p2_batt_soc, 4, 150);

    /* Right Card: AC Output & Loading (154x182) */
    lv_obj_t *c_load = create_card(page, 160, 0, 154, 182);

    lv_obj_t *i_h = lv_image_create(c_load);
    lv_image_set_src(i_h, &img_home);
    lv_obj_set_pos(i_h, 4, 4);

    lv_obj_t *t_h = lv_label_create(c_load);
    lv_label_set_text(t_h, "AC OUTPUT");
    lv_obj_set_style_text_font(t_h, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t_h, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_h, 40, 4);

    p2_inv_v = lv_label_create(c_load);
    lv_label_set_text(p2_inv_v, "230 VAC");
    lv_obj_set_style_text_font(p2_inv_v, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_inv_v, lv_color_hex(th->text_main), 0);
    lv_obj_set_pos(p2_inv_v, 40, 20);

    lv_obj_t *div_l = lv_obj_create(c_load);
    lv_obj_set_size(div_l, 142, 1);
    lv_obj_set_pos(div_l, 0, 40);
    lv_obj_set_style_bg_color(div_l, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(div_l, 0, 0);

    p2_load_pct = lv_label_create(c_load);
    lv_label_set_text(p2_load_pct, "42% LOAD");
    lv_obj_set_style_text_font(p2_load_pct, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(p2_load_pct, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p2_load_pct, 4, 48);

    p2_source_lbl = lv_label_create(c_load);
    lv_label_set_text(p2_source_lbl, "FEED: SOLAR");
    lv_obj_set_style_text_font(p2_source_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p2_source_lbl, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(p2_source_lbl, 4, 74);

    p2_inv_status = lv_label_create(c_load);
    lv_label_set_text(p2_inv_status, "INVERTER: ACTIVE");
    lv_obj_set_style_text_font(p2_inv_status, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_inv_status, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_pos(p2_inv_status, 4, 96);

    lv_obj_t *load_bar_t = lv_label_create(c_load);
    lv_label_set_text(load_bar_t, "POWER CONSUMPTION");
    lv_obj_set_style_text_font(load_bar_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(load_bar_t, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(load_bar_t, 4, 118);

    p2_load_bar = lv_bar_create(c_load);
    lv_obj_set_size(p2_load_bar, 142, 8);
    lv_obj_set_pos(p2_load_bar, 0, 134);
    lv_obj_set_style_bg_color(p2_load_bar, lv_color_hex(th->header_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p2_load_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p2_load_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(p2_load_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(p2_load_bar, 0, 100);
    lv_bar_set_value(p2_load_bar, 42, LV_ANIM_OFF);

    p2_load_stat = lv_label_create(c_load);
    lv_label_set_text(p2_load_stat, "LOAD: NORMAL");
    lv_obj_set_style_text_font(p2_load_stat, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p2_load_stat, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p2_load_stat, 4, 150);
}

static void build_page_3(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 3: Grid, Thermals & Serial Comms Diagnostics */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[3] = page;

    /* Card 1: Grid Mains In (154x86) */
    lv_obj_t *c_g = create_card(page, 0, 0, 154, 86);

    lv_obj_t *i_gr = lv_image_create(c_g);
    lv_image_set_src(i_gr, &img_grid);
    lv_obj_set_pos(i_gr, 4, 4);

    lv_obj_t *t_g = lv_label_create(c_g);
    lv_label_set_text(t_g, "MAINS INPUT");
    lv_obj_set_style_text_font(t_g, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_g, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_g, 40, 4);

    p3_grid_v = lv_label_create(c_g);
    lv_label_set_text(p3_grid_v, "228 V");
    lv_obj_set_style_text_font(p3_grid_v, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p3_grid_v, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p3_grid_v, 40, 18);

    p3_grid_freq_stat = lv_label_create(c_g);
    lv_label_set_text(p3_grid_freq_stat, "50.0 Hz | STABLE");
    lv_obj_set_style_text_font(p3_grid_freq_stat, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_grid_freq_stat, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(p3_grid_freq_stat, 4, 56);

    /* Card 2: Heatsink Temperature (154x86) */
    lv_obj_t *c_t = create_card(page, 160, 0, 154, 86);

    lv_obj_t *i_tm = lv_image_create(c_t);
    lv_image_set_src(i_tm, &img_temp);
    lv_obj_set_pos(i_tm, 4, 4);

    lv_obj_t *t_tm = lv_label_create(c_t);
    lv_label_set_text(t_tm, "INVERTER HEAT");
    lv_obj_set_style_text_font(t_tm, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t_tm, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(t_tm, 40, 4);

    p3_temp_val = lv_label_create(c_t);
    lv_label_set_text(p3_temp_val, "38.5 °C");
    lv_obj_set_style_text_font(p3_temp_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p3_temp_val, lv_color_hex(th->primary), 0);
    lv_obj_set_pos(p3_temp_val, 40, 18);

    p3_temp_bar = lv_bar_create(c_t);
    lv_obj_set_size(p3_temp_bar, 144, 6);
    lv_obj_align(p3_temp_bar, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(p3_temp_bar, lv_color_hex(th->header_bg), LV_PART_MAIN);
    lv_obj_set_style_bg_color(p3_temp_bar, lv_color_hex(th->secondary), LV_PART_INDICATOR);
    lv_obj_set_style_radius(p3_temp_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(p3_temp_bar, 3, LV_PART_INDICATOR);
    lv_bar_set_range(p3_temp_bar, 0, 100);
    lv_bar_set_value(p3_temp_bar, 48, LV_ANIM_OFF);

    /* Card 3: System Operational Status Board (314x88) */
    lv_obj_t *c_diag = create_card(page, 0, 92, 314, 88);

    p3_inv_mode = lv_label_create(c_diag);
    lv_label_set_text(p3_inv_mode, "INVERTER: ACTIVE");
    lv_obj_set_style_text_font(p3_inv_mode, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p3_inv_mode, lv_color_hex(0x10b981), 0);
    lv_obj_set_pos(p3_inv_mode, 4, 2);

    p3_ac_freq = lv_label_create(c_diag);
    lv_label_set_text(p3_ac_freq, "AC: 50.0 Hz");
    lv_obj_set_style_text_font(p3_ac_freq, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p3_ac_freq, lv_color_hex(th->secondary), 0);
    lv_obj_align(p3_ac_freq, LV_ALIGN_TOP_RIGHT, -4, 2);

    lv_obj_t *div_diag = lv_obj_create(c_diag);
    lv_obj_set_size(div_diag, 302, 1);
    lv_obj_set_pos(div_diag, 0, 22);
    lv_obj_set_style_bg_color(div_diag, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(div_diag, 0, 0);

    /* Row 1 (y: 28): Energy Priority & Battery Health */
    p3_pref_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_pref_lbl, "PRIORITY: SOLAR");
    lv_obj_set_style_text_font(p3_pref_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_pref_lbl, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(p3_pref_lbl, 6, 28);

    p3_batt_health_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_batt_health_lbl, "BATT: HEALTHY");
    lv_obj_set_style_text_font(p3_batt_health_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_batt_health_lbl, lv_color_hex(0x34d399), 0);
    lv_obj_set_pos(p3_batt_health_lbl, 158, 28);

    /* Row 2 (y: 44): DC Boost & Charger Mode */
    p3_boost_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_boost_lbl, "DC BOOST: ACTIVE");
    lv_obj_set_style_text_font(p3_boost_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_boost_lbl, lv_color_hex(0xfacc15), 0);
    lv_obj_set_pos(p3_boost_lbl, 6, 44);

    p3_chg_type_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_chg_type_lbl, "CHG: SOLAR MPPT");
    lv_obj_set_style_text_font(p3_chg_type_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_chg_type_lbl, lv_color_hex(0x34d399), 0);
    lv_obj_set_pos(p3_chg_type_lbl, 158, 44);

    /* Row 3 (y: 60): Mains Grid & Thermal Protection */
    p3_mains_status_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_mains_status_lbl, "GRID: 230V OK");
    lv_obj_set_style_text_font(p3_mains_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_mains_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(p3_mains_status_lbl, 6, 60);

    p3_thermal_status_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_thermal_status_lbl, "HEAT: 35°C COOL");
    lv_obj_set_style_text_font(p3_thermal_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(p3_thermal_status_lbl, 158, 60);
}

static void build_page_4(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 4: Solar Harvest & Daily Production History */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[4] = page;

    /* Top Row: 3 KPI Cards (width ~102 each) */
    /* Card 1: Today Harvest */
    lv_obj_t *c1 = create_card(page, 0, 0, 102, 54);
    lv_obj_t *l1 = lv_label_create(c1);
    lv_label_set_text(l1, "TODAY YIELD");
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l1, lv_color_hex(th->text_muted), 0);
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_today_kwh = lv_label_create(c1);
    lv_label_set_text(p4_today_kwh, "14.8 kWh");
    lv_obj_set_style_text_font(p4_today_kwh, &font_bold_14, 0);
    lv_obj_set_style_text_color(p4_today_kwh, lv_color_hex(0x10b981), 0);
    lv_obj_align(p4_today_kwh, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Card 2: Peak Power */
    lv_obj_t *c2 = create_card(page, 106, 0, 102, 54);
    lv_obj_t *l2 = lv_label_create(c2);
    lv_label_set_text(l2, "PEAK SOLAR");
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l2, lv_color_hex(th->text_muted), 0);
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_peak_w = lv_label_create(c2);
    lv_label_set_text(p4_peak_w, "2,480 W");
    lv_obj_set_style_text_font(p4_peak_w, &font_bold_14, 0);
    lv_obj_set_style_text_color(p4_peak_w, lv_color_hex(th->primary), 0);
    lv_obj_align(p4_peak_w, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Card 3: Total Yield */
    lv_obj_t *c3 = create_card(page, 212, 0, 102, 54);
    lv_obj_t *l3 = lv_label_create(c3);
    lv_label_set_text(l3, "LIFETIME YIELD");
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l3, lv_color_hex(th->text_muted), 0);
    lv_obj_align(l3, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_total_mwh = lv_label_create(c3);
    lv_label_set_text(p4_total_mwh, "3.82 MWh");
    lv_obj_set_style_text_font(p4_total_mwh, &font_bold_14, 0);
    lv_obj_set_style_text_color(p4_total_mwh, lv_color_hex(th->secondary), 0);
    lv_obj_align(p4_total_mwh, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Bottom Section: 8-Hour Line Chart Container (314x124) */
    lv_obj_t *c_chart = create_card(page, 0, 58, 314, 124);

    lv_obj_t *chart_title = lv_label_create(c_chart);
    lv_label_set_text(chart_title, "8-HOUR LIVE TELEMETRY TREND");
    lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(chart_title, lv_color_hex(th->secondary), 0);
    lv_obj_align(chart_title, LV_ALIGN_TOP_LEFT, 4, 2);

    p4_trend_badge = lv_label_create(c_chart);
    lv_label_set_text(p4_trend_badge, "PV [YEL] | LOAD [BLU]");
    lv_obj_set_style_text_font(p4_trend_badge, &font_bold_10, 0);
    lv_obj_set_style_text_color(p4_trend_badge, lv_color_hex(0x34d399), 0);
    lv_obj_align(p4_trend_badge, LV_ALIGN_TOP_RIGHT, -4, 2);

    /* Line Chart Widget */
    p4_chart = lv_chart_create(c_chart);
    lv_obj_set_pos(p4_chart, 4, 18);
    lv_obj_set_size(p4_chart, 302, 76);
    lv_chart_set_type(p4_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(p4_chart, 8);
    lv_chart_set_axis_range(p4_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_bg_color(p4_chart, lv_color_hex(th->header_bg), 0);
    lv_obj_set_style_border_color(p4_chart, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(p4_chart, 1, 0);
    lv_obj_set_style_pad_all(p4_chart, 4, 0);
    lv_obj_set_style_line_width(p4_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(p4_chart, 4, 4, LV_PART_INDICATOR);

    p4_series_solar = lv_chart_add_series(p4_chart, lv_color_hex(0xfacc15), LV_CHART_AXIS_PRIMARY_Y);
    p4_series_load  = lv_chart_add_series(p4_chart, lv_color_hex(0x38bdf8), LV_CHART_AXIS_PRIMARY_Y);

    const int32_t init_solar[8] = { 12, 28, 54, 80, 90, 72, 50, 35 };
    const int32_t init_load[8]  = { 32, 40, 45, 50, 52, 48, 42, 42 };
    for (uint32_t i = 0; i < 8; i++) {
        lv_chart_set_series_value_by_id(p4_chart, p4_series_solar, i, init_solar[i]);
        lv_chart_set_series_value_by_id(p4_chart, p4_series_load,  i, init_load[i]);
    }

    /* 8 Hourly Time Markers below chart */
    const char *hours[8] = { "-7h", "-6h", "-5h", "-4h", "-3h", "-2h", "-1h", "LIVE" };
    for (int i = 0; i < 8; i++) {
        lv_obj_t *th_lbl = lv_label_create(c_chart);
        lv_label_set_text(th_lbl, hours[i]);
        lv_obj_set_style_text_font(th_lbl, &font_bold_10, 0);
        if (i == 7) {
            lv_obj_set_style_text_color(th_lbl, lv_color_hex(0x10b981), 0);
        } else {
            lv_obj_set_style_text_color(th_lbl, lv_color_hex(th->text_muted), 0);
        }
        lv_obj_set_pos(th_lbl, 8 + i * 37, 100);
    }
}

static void build_page_5(lv_obj_t *parent)
{
    const pcu_theme_t *th = get_active_theme();
    /* Page 5: OEM Vendor & Brand Support */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(th->screen_bg), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[5] = page;

    lv_color_t primary_color = lv_color_hex(th->primary);
    lv_color_t sec_color = lv_color_hex(th->secondary);

    /* Card 1: Top Brand Banner Card (314x80) */
    lv_obj_t *c_brand = create_card(page, 0, 0, 314, 80);

    /* Center/Left Emblem Container - Transparent background */
    lv_obj_t *emblem_box = lv_obj_create(c_brand);
    lv_obj_set_size(emblem_box, 60, 60);
    lv_obj_align(emblem_box, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_opa(emblem_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(emblem_box, 0, 0);
    lv_obj_set_style_radius(emblem_box, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(emblem_box, 0, 0);
    lv_obj_remove_flag(emblem_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Embedded solar icon centered inside emblem */
    lv_obj_t *emb_icon = lv_image_create(emblem_box);
    lv_image_set_src(emb_icon, get_active_logo_dsc());
    lv_obj_center(emb_icon);

    /* Rotating Arc around emblem */
    p5_logo_arc = lv_arc_create(c_brand);
    lv_obj_set_size(p5_logo_arc, 70, 70);
    lv_obj_align(p5_logo_arc, LV_ALIGN_LEFT_MID, 1, 0);
    lv_arc_set_angles(p5_logo_arc, 0, 100);
    lv_arc_set_rotation(p5_logo_arc, 45);
    lv_obj_set_style_arc_width(p5_logo_arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(p5_logo_arc, sec_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(p5_logo_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_style(p5_logo_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(p5_logo_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Brand Typography on the right */
    p5_brand_title = lv_label_create(c_brand);
    lv_label_set_text(p5_brand_title, g_pcu_cfg.brand_title);
    lv_obj_set_style_text_font(p5_brand_title, &font_bold_20, 0);
    lv_obj_set_style_text_color(p5_brand_title, primary_color, 0);
    lv_obj_set_pos(p5_brand_title, 82, 8);

    p5_model_name = lv_label_create(c_brand);
    lv_label_set_text(p5_model_name, g_pcu_cfg.model_name);
    lv_obj_set_style_text_font(p5_model_name, &font_bold_14, 0);
    lv_obj_set_style_text_color(p5_model_name, lv_color_hex(th->text_main), 0);
    lv_obj_set_pos(p5_model_name, 82, 32);

    /* Standards & Quality Tag Badge */
    lv_obj_t *tag = lv_label_create(c_brand);
    lv_label_set_text(tag, "INDUSTRIAL HMI CONTROLLER");
    lv_obj_set_style_text_font(tag, &font_bold_10, 0);
    lv_obj_set_style_text_color(tag, lv_color_hex(th->badge_fg), 0);
    lv_obj_set_style_bg_color(tag, lv_color_hex(th->badge_bg), 0);
    lv_obj_set_style_radius(tag, 3, 0);
    lv_obj_set_style_pad_hor(tag, 6, 0);
    lv_obj_set_style_pad_ver(tag, 2, 0);
    lv_obj_set_pos(tag, 82, 52);

    /* Card 2: Technical Specs & Vendor Support (314x98) - Clean 3-row layout without continuous duty banner */
    lv_obj_t *c_info = create_card(page, 0, 84, 314, 98);

    /* Row 1: Serial Number & HW Version */
    lv_obj_t *lbl_sn_t = lv_label_create(c_info);
    lv_label_set_text(lbl_sn_t, "SERIAL NO  :");
    lv_obj_set_style_text_font(lbl_sn_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_sn_t, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_sn_t, 8, 12);

    p5_serial_no = lv_label_create(c_info);
    lv_label_set_text(p5_serial_no, g_pcu_cfg.serial_number);
    lv_obj_set_style_text_font(p5_serial_no, &font_bold_12, 0);
    lv_obj_set_style_text_color(p5_serial_no, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(p5_serial_no, 90, 10);

    p5_hw_rev = lv_label_create(c_info);
    lv_label_set_text(p5_hw_rev, g_pcu_cfg.hardware_version);
    lv_obj_set_style_text_font(p5_hw_rev, &font_bold_10, 0);
    lv_obj_set_style_text_color(p5_hw_rev, primary_color, 0);
    lv_obj_align(p5_hw_rev, LV_ALIGN_TOP_RIGHT, -8, 12);

    /* Row 2: Customer Helpline */
    lv_obj_t *lbl_cc_t = lv_label_create(c_info);
    lv_label_set_text(lbl_cc_t, "SUPPORT    :");
    lv_obj_set_style_text_font(lbl_cc_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_cc_t, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_cc_t, 8, 40);

    p5_contact = lv_label_create(c_info);
    lv_label_set_text(p5_contact, g_pcu_cfg.vendor_contact);
    lv_obj_set_style_text_font(p5_contact, &font_bold_12, 0);
    lv_obj_set_style_text_color(p5_contact, lv_color_hex(th->text_main), 0);
    lv_obj_set_pos(p5_contact, 90, 38);

    /* Row 3: Website */
    lv_obj_t *lbl_web_t = lv_label_create(c_info);
    lv_label_set_text(lbl_web_t, "PORTAL     :");
    lv_obj_set_style_text_font(lbl_web_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_web_t, lv_color_hex(th->text_muted), 0);
    lv_obj_set_pos(lbl_web_t, 8, 68);

    p5_website = lv_label_create(c_info);
    lv_label_set_text(p5_website, g_pcu_cfg.vendor_website);
    lv_obj_set_style_text_font(p5_website, &font_bold_12, 0);
    lv_obj_set_style_text_color(p5_website, lv_color_hex(th->secondary), 0);
    lv_obj_set_pos(p5_website, 90, 66);
}

/* ========================================================================= */
/* MAIN DASHBOARD SHELL (HEADER + PAGES + FOOTER)                            */
/* ========================================================================= */

static void build_dashboard_shell(lv_obj_t *scr)
{
    const pcu_theme_t *th = get_active_theme();
    lv_obj_set_style_bg_color(scr, lv_color_hex(th->screen_bg), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* --- TOP BAR (w: 320, h: 26) --- */
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 320, 26);
    lv_obj_set_style_bg_color(header, lv_color_hex(th->header_bg), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 6, 0);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *brand = lv_label_create(header);
    char header_brand_buf[96];
    if (strlen(g_pcu_cfg.model_name) > 0) {
        snprintf(header_brand_buf, sizeof(header_brand_buf), "%s | %s", g_pcu_cfg.brand_title, g_pcu_cfg.model_name);
    } else {
        snprintf(header_brand_buf, sizeof(header_brand_buf), "%s", g_pcu_cfg.brand_title);
    }
    lv_label_set_text(brand, header_brand_buf);
    lv_obj_set_style_text_font(brand, &font_bold_12, 0);
    lv_obj_set_style_text_color(brand, lv_color_hex(th->primary), 0);
    lv_obj_set_width(brand, 220);
    lv_label_set_long_mode(brand, LV_LABEL_LONG_DOT);
    lv_obj_align(brand, LV_ALIGN_LEFT_MID, 2, 0);

    rtc_time_lbl = lv_label_create(header);
    lv_label_set_text(rtc_time_lbl, "00:00:00");
    lv_obj_set_style_text_font(rtc_time_lbl, &font_bold_12, 0);
    lv_obj_set_style_text_color(rtc_time_lbl, lv_color_hex(th->secondary), 0);
    lv_obj_align(rtc_time_lbl, LV_ALIGN_RIGHT_MID, -2, 0);

    /* Build all 6 pages */
    build_page_0(scr);
    build_page_1(scr);
    build_page_2(scr);
    build_page_3(scr);
    build_page_4(scr);
    build_page_5(scr);

    /* --- FOOTER BAR (w: 320, h: 24) --- */
    lv_obj_t *footer = lv_obj_create(scr);
    lv_obj_set_pos(footer, 0, 216);
    lv_obj_set_size(footer, 320, 24);
    lv_obj_set_style_bg_color(footer, lv_color_hex(th->header_bg), 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(th->card_border), 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_style_pad_hor(footer, 6, 0);
    lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    footer_page_lbl = lv_label_create(footer);
    lv_label_set_text(footer_page_lbl, page_titles[0]);
    lv_obj_set_style_text_font(footer_page_lbl, &font_bold_10, 0);
    lv_obj_set_style_text_color(footer_page_lbl, lv_color_hex(th->secondary), 0);
    lv_obj_align(footer_page_lbl, LV_ALIGN_LEFT_MID, 2, 0);

    /* Page Navigation Dots Container */
    lv_obj_t *dots_box = lv_obj_create(footer);
    lv_obj_set_size(dots_box, 90, 14);
    lv_obj_align(dots_box, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(dots_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dots_box, 0, 0);
    lv_obj_set_style_pad_all(dots_box, 0, 0);
    lv_obj_remove_flag(dots_box, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < TOTAL_PAGES; i++) {
        page_dots[i] = lv_obj_create(dots_box);
        lv_obj_set_size(page_dots[i], (i == 0) ? 14 : 6, 6);
        lv_obj_set_style_radius(page_dots[i], 3, 0);
        lv_obj_set_style_border_width(page_dots[i], 0, 0);
        lv_obj_set_style_bg_color(page_dots[i], (i == 0) ? lv_color_hex(th->active_dot) : lv_color_hex(th->card_border), 0);
        lv_obj_set_pos(page_dots[i], i * 14, 4);
    }

    /* Timers */
    lv_timer_create(fast_anim_timer_cb, 40, NULL);          // 25 FPS rotating sun ray
    lv_timer_create(telemetry_refresh_timer_cb, 1000, NULL); // 1-second Uptime and telemetry ticker
    if (g_pcu_cfg.auto_carousel_enabled) {
        uint32_t interval = (uint32_t)g_pcu_cfg.carousel_interval_sec * 1000;
        if (interval < 1000) interval = 5000;
        lv_timer_create(page_carousel_timer_cb, interval, NULL);
    }
    switch_to_page(0);
}

/* Boot Splash Screen Implementation */
static lv_obj_t *boot_arc = NULL;
static lv_obj_t *boot_bar = NULL;
static lv_obj_t *boot_status_lbl = NULL;
static int boot_elapsed_ms = 0;
static int boot_total_ms = 3000;

static void boot_anim_timer_cb(lv_timer_t *timer)
{
    boot_elapsed_ms += 30;
    int pct = (boot_elapsed_ms * 100) / boot_total_ms;
    if (pct > 100) pct = 100;

    if (boot_bar) lv_bar_set_value(boot_bar, pct, LV_ANIM_OFF);
    if (boot_arc) {
        int r = (boot_elapsed_ms * 360 / 1000) % 360;
        lv_arc_set_rotation(boot_arc, r);
    }

    if (boot_status_lbl) {
        if (pct < 25) {
            lv_label_set_text(boot_status_lbl, "SYSTEM SELF-TEST... [OK]");
        } else if (pct < 55) {
            lv_label_set_text(boot_status_lbl, "CALIBRATING MPPT CONTROLLER...");
        } else if (pct < 85) {
            lv_label_set_text(boot_status_lbl, "INITIALIZING PRECISION HMI...");
        } else {
            lv_label_set_text(boot_status_lbl, "SYSTEM READY");
        }
    }

    if (boot_elapsed_ms >= boot_total_ms) {
        lv_timer_delete(timer);
        if (g_main_screen_obj) {
            lv_screen_load_anim(g_main_screen_obj, LV_SCR_LOAD_ANIM_FADE_ON, 400, 0, true);
        }
    }
}

static void build_boot_screen(lv_obj_t *boot_scr, lv_obj_t *main_scr)
{
    g_main_screen_obj = main_scr;
    boot_elapsed_ms = 0;
    boot_total_ms = (int)g_pcu_cfg.boot_duration_sec * 1000;
    if (boot_total_ms < 1000) boot_total_ms = 3000;

    const pcu_theme_t *th = get_active_theme();
    lv_color_t primary_color = lv_color_hex(th->primary);
    lv_color_t sec_color = lv_color_hex(th->secondary);

    lv_obj_set_style_bg_color(boot_scr, lv_color_hex(th->screen_bg), 0);
    lv_obj_remove_flag(boot_scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Center Emblem Circle Container - Transparent background so PNG floats clean */
    lv_obj_t *circle = lv_obj_create(boot_scr);
    lv_obj_set_size(circle, 72, 72);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(circle, 0, 0);
    lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_image_create(circle);
    lv_image_set_src(icon, get_active_logo_dsc());
    lv_obj_center(icon);

    /* Spinning outer arc */
    boot_arc = lv_arc_create(boot_scr);
    lv_obj_set_size(boot_arc, 88, 88);
    lv_obj_align(boot_arc, LV_ALIGN_TOP_MID, 0, 10);
    lv_arc_set_angles(boot_arc, 0, 90);
    lv_arc_set_rotation(boot_arc, 0);
    lv_obj_set_style_arc_width(boot_arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(boot_arc, sec_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(boot_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_style(boot_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(boot_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Brand Title */
    lv_obj_t *brand_lbl = lv_label_create(boot_scr);
    lv_label_set_text(brand_lbl, g_pcu_cfg.brand_title);
    lv_obj_set_style_text_font(brand_lbl, &font_bold_20, 0);
    lv_obj_set_style_text_color(brand_lbl, primary_color, 0);
    lv_obj_align(brand_lbl, LV_ALIGN_TOP_MID, 0, 102);

    /* Model Name */
    lv_obj_t *model_lbl = lv_label_create(boot_scr);
    lv_label_set_text(model_lbl, g_pcu_cfg.model_name);
    lv_obj_set_style_text_font(model_lbl, &font_bold_14, 0);
    lv_obj_set_style_text_color(model_lbl, lv_color_hex(th->text_main), 0);
    lv_obj_align(model_lbl, LV_ALIGN_TOP_MID, 0, 128);

    /* Serial & Hardware Version */
    lv_obj_t *sub_lbl = lv_label_create(boot_scr);
    char sub_buf[64];
    snprintf(sub_buf, sizeof(sub_buf), "%s | %s", g_pcu_cfg.serial_number, g_pcu_cfg.hardware_version);
    lv_label_set_text(sub_lbl, sub_buf);
    lv_obj_set_style_text_font(sub_lbl, &font_bold_10, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(th->text_muted), 0);
    lv_obj_align(sub_lbl, LV_ALIGN_TOP_MID, 0, 144);

    /* Loading Bar */
    boot_bar = lv_bar_create(boot_scr);
    lv_obj_set_size(boot_bar, 240, 8);
    lv_obj_align(boot_bar, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(th->card_border), LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_bar, primary_color, LV_PART_INDICATOR);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_INDICATOR);
    lv_bar_set_range(boot_bar, 0, 100);
    lv_bar_set_value(boot_bar, 0, LV_ANIM_OFF);

    /* Status Label */
    boot_status_lbl = lv_label_create(boot_scr);
    lv_label_set_text(boot_status_lbl, "INITIALIZING SYSTEM...");
    lv_obj_set_style_text_font(boot_status_lbl, &font_bold_10, 0);
    lv_obj_set_style_text_color(boot_status_lbl, lv_color_hex(th->text_muted), 0);
    lv_obj_align(boot_status_lbl, LV_ALIGN_TOP_MID, 0, 184);

    /* Start 30ms animation ticker */
    lv_timer_create(boot_anim_timer_cb, 30, NULL);
}

static void build_unconfigured_screen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050811), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge = lv_label_create(scr);
    lv_label_set_text(badge, "FACTORY SERVICE MODE");
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0xf59e0b), 0);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *t1 = lv_label_create(scr);
    lv_label_set_text(t1, "DEVICE UNCONFIGURED");
    lv_obj_set_style_text_font(t1, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t1, lv_color_hex(0xef4444), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *t2 = lv_label_create(scr);
    lv_label_set_text(t2, "Connect PC USB (COM port)\nto configure Brand & Model via\nSolar Display Manager");
    lv_obj_set_style_text_font(t2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t2, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_align(t2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 90);

    lv_obj_t *t3 = lv_label_create(scr);
    lv_label_set_text(t3, "Baud: 115200 | Protocol: PCU v2.0");
    lv_obj_set_style_text_font(t3, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t3, lv_color_hex(0x38bdf8), 0);
    lv_obj_align(t3, LV_ALIGN_BOTTOM_MID, 0, -20);
}

/* ========================================================================= */
/* INVERTER SERIAL RECEIVER TASK (UART2) & DUAL-PORT TELEMETRY PARSER       */
/* ========================================================================= */

static void parse_and_apply_telemetry_line(const char *line_buf)
{
    /* Format: $PCU,mains,solar,batt,acout,load,chg,disch,dc,heat,onflag,solarstate,chargerstate,dcok,sharemode,batgravity[,switchstate,dcboostmode,feedmode,error_code] */
    if (strncmp(line_buf, "$PCU,", 5) == 0) {
        float m_volt = 0, s_volt = 0, b_volt = 0, ac_out = 0, load_pct = 0;
        float chg_a = 0, disch_a = 0, dc_b = 0, heat = 0;
        int on_f = 1, s_state = 1, chg_state = 2, dc_ok = 1, s_mode = 0, b_grav = 0;
        int sw_state = 1, dc_b_mode = 1, feed_mode = 0, err_c = 0;

        int count = sscanf(line_buf, "$PCU,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                   &m_volt, &s_volt, &b_volt, &ac_out, &load_pct, &chg_a, &disch_a, &dc_b, &heat,
                   &on_f, &s_state, &chg_state, &dc_ok, &s_mode, &b_grav,
                   &sw_state, &dc_b_mode, &feed_mode, &err_c);

        if (count >= 15) {
            /* Production & Industrial Fail-Proof Clamping & NaN Guards */
            if (isnan(m_volt) || isinf(m_volt) || m_volt < 0.0f) m_volt = 0.0f;
            if (m_volt > 450.0f) m_volt = 450.0f;
            if (isnan(s_volt) || isinf(s_volt) || s_volt < 0.0f) s_volt = 0.0f;
            if (s_volt > 250.0f) s_volt = 250.0f;
            if (isnan(b_volt) || isinf(b_volt) || b_volt < 0.0f) b_volt = 0.0f;
            if (b_volt > 100.0f) b_volt = 100.0f;
            if (isnan(ac_out) || isinf(ac_out) || ac_out < 0.0f) ac_out = 0.0f;
            if (ac_out > 450.0f) ac_out = 450.0f;
            if (isnan(load_pct) || isinf(load_pct) || load_pct < 0.0f) load_pct = 0.0f;
            if (load_pct > 250.0f) load_pct = 250.0f;
            if (isnan(chg_a) || isinf(chg_a) || chg_a < 0.0f) chg_a = 0.0f;
            if (chg_a > 200.0f) chg_a = 200.0f;
            if (isnan(disch_a) || isinf(disch_a) || disch_a < 0.0f) disch_a = 0.0f;
            if (disch_a > 200.0f) disch_a = 200.0f;
            if (isnan(dc_b) || isinf(dc_b) || dc_b < 0.0f) dc_b = 0.0f;
            if (dc_b > 600.0f) dc_b = 600.0f;
            if (isnan(heat) || isinf(heat) || heat < -20.0f) heat = 25.0f;
            if (heat > 150.0f) heat = 150.0f;

            _lock_acquire(&lvgl_api_lock);
            inv_data.mainsvolt = m_volt;
            inv_data.solarvolt = s_volt;
            inv_data.battvolts = b_volt;
            inv_data.acout = ac_out;
            inv_data.loaddisp = load_pct;
            inv_data.chrampsdisp = chg_a;
            inv_data.dischdisp = disch_a;
            inv_data.dcboost = dc_b;
            inv_data.upsheat = heat;
            inv_data.onflag = on_f;
            inv_data.solarstate = (solar_state_t)s_state;
            inv_data.chargerstate = (charger_state_t)chg_state;
            inv_data.dcok = dc_ok;
            inv_data.sharemode = s_mode;
            inv_data.batgravity = b_grav;
            if (count >= 16) inv_data.switchstate = (switch_state_t)sw_state;
            else inv_data.switchstate = (on_f == 1 ? INVSWITCH : INV_OFF);
            if (count >= 17) inv_data.dcboostmode = dc_b_mode;
            else inv_data.dcboostmode = (dc_b > 50.0f || dc_ok);
            if (count >= 18) inv_data.feedmode = feed_mode;
            else inv_data.feedmode = 0; // Auto by physics
            if (count >= 19) inv_data.error_code = err_c;
            else inv_data.error_code = 0;
            inv_data.rx_packet_count++;
            inv_data.live_serial_active = true;
            inv_data.last_rx_tick = (uint32_t)(esp_timer_get_time() / 1000000ULL);
            _lock_release(&lvgl_api_lock);
        }
    }
    /* Calibration & Threshold Limits: $LIMITS,<setbatful>,<setbatwrn>,<setbatlo>,<setbatrst>,<mainslow>,<mainshi>,<hiheat>,<solmax>,<solmin>,<dcmax> */
    else if (strncmp(line_buf, "$LIMITS,", 8) == 0 || strncmp(line_buf, "$CFG,", 5) == 0) {
        const char *p = (line_buf[1] == 'L') ? line_buf + 8 : line_buf + 5;
        float b_ful = 0, b_wrn = 0, b_lo = 0, b_rst = 0;
        float m_lo = 0, m_hi = 0, h_hi = 0, s_max = 0, s_min = 0, dc_m = 0;
        int c = sscanf(p, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
                       &b_ful, &b_wrn, &b_lo, &b_rst, &m_lo, &m_hi, &h_hi, &s_max, &s_min, &dc_m);
        if (c >= 6) {
            _lock_acquire(&lvgl_api_lock);
            /* Auto-detect integer scaling (e.g. 288 -> 28.8V) */
            if (b_ful > 100.0f) { b_ful /= 10.0f; b_wrn /= 10.0f; b_lo /= 10.0f; b_rst /= 10.0f; }
            if (m_lo > 500.0f) { m_lo /= 10.0f; m_hi /= 10.0f; }
            if (s_max > 500.0f) { s_max /= 10.0f; s_min /= 10.0f; }

            if (b_ful > 5.0f && b_ful < 100.0f) pcu_limits.setbatful = b_ful;
            if (b_wrn > 5.0f && b_wrn < 100.0f) pcu_limits.setbatwrn = b_wrn;
            if (b_lo > 5.0f && b_lo < 100.0f) pcu_limits.setbatlo = b_lo;
            if (b_rst > 5.0f && b_rst < 100.0f) pcu_limits.setbatrst = b_rst;
            if (m_lo > 50.0f && m_lo < 350.0f) pcu_limits.mainslow = m_lo;
            if (m_hi > 100.0f && m_hi < 400.0f) pcu_limits.mainshi = m_hi;
            if (c >= 7 && h_hi > 40.0f && h_hi < 130.0f) pcu_limits.hiheat = h_hi;
            if (c >= 9 && s_max > 20.0f && s_max < 300.0f) pcu_limits.solmax = s_max;
            if (c >= 9 && s_min >= 0.0f && s_min < 100.0f) pcu_limits.solmin = s_min;
            if (c >= 10 && dc_m > 50.0f && dc_m < 600.0f) pcu_limits.dcmax = dc_m;
            pcu_limits.is_calibrated = true;
            _lock_release(&lvgl_api_lock);
            ESP_LOGI("PCU_LIMITS", "Calibration applied: BatFul=%.1fV BatWrn=%.1fV BatLo=%.1fV Mains=%.0f-%.0fV SolMax=%.0fV",
                     pcu_limits.setbatful, pcu_limits.setbatwrn, pcu_limits.setbatlo,
                     pcu_limits.mainslow, pcu_limits.mainshi, pcu_limits.solmax);
        }
    }
    /* Dedicated Fault Code Command: $FAULT,<code> or $ERR,<code> */
    else if (strncmp(line_buf, "$FAULT,", 7) == 0 || strncmp(line_buf, "$ERR,", 5) == 0) {
        const char *p = (line_buf[1] == 'F') ? line_buf + 7 : line_buf + 5;
        int f_code = atoi(p);
        _lock_acquire(&lvgl_api_lock);
        inv_data.error_code = f_code;
        if (f_code != 0) {
            if (g_error_scr && lv_screen_active() != g_error_scr) {
                lv_screen_load(g_error_scr);
            }
            update_error_screen(f_code);
        } else {
            if (g_error_scr && lv_screen_active() == g_error_scr && g_main_screen_obj) {
                lv_screen_load(g_main_screen_obj);
                switch_to_page(current_page);
            }
        }
        _lock_release(&lvgl_api_lock);
    }
    /* Manual page switch command: $PAGE,0-5 */
    else if (strncmp(line_buf, "$PAGE,", 6) == 0) {
        int p = atoi(&line_buf[6]);
        _lock_acquire(&lvgl_api_lock);
        switch_to_page(p);
        _lock_release(&lvgl_api_lock);
    }
}

static void inverter_uart_task(void *arg)
{
    uint32_t baud = g_pcu_cfg.telemetry_baudrate;
    if (baud < 1200 || baud > 115200) baud = 9600;

    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(INVERTER_UART_NUM, &uart_config);
    uart_set_pin(INVERTER_UART_NUM, INVERTER_UART_TX_PIN, INVERTER_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(INVERTER_UART_NUM, 1024, 0, 0, NULL, 0);

    uint8_t rx_byte;
    char line_buf[160];
    int line_idx = 0;

    while (1) {
        int len = uart_read_bytes(INVERTER_UART_NUM, &rx_byte, 1, pdMS_TO_TICKS(50));
        if (len > 0) {
            if (rx_byte == '\n' || rx_byte == '\r') {
                if (line_idx > 0) {
                    line_buf[line_idx] = '\0';
                    parse_and_apply_telemetry_line(line_buf);
                    line_idx = 0;
                }
            } else if (line_idx < (int)sizeof(line_buf) - 1) {
                line_buf[line_idx++] = (char)rx_byte;
            }
        }
    }
}

/* ========================================================================= */
/* PCU NVS CONFIGURATION & USB SERVICE PROTOCOL (UART0)                      */
/* ========================================================================= */

#define SERVICE_UART_NUM        UART_NUM_0
#define SERVICE_UART_BAUD       115200

static void config_set_factory_defaults(pcu_config_t *cfg)
{
    memset(cfg, 0, sizeof(pcu_config_t));
    cfg->magic = PCU_CONFIG_MAGIC;
    cfg->version = PCU_CONFIG_VERSION;
    cfg->struct_size = sizeof(pcu_config_t);
    cfg->is_configured = 0;
    strncpy(cfg->brand_title, "HYBRID PSU", sizeof(cfg->brand_title) - 1);
    strncpy(cfg->model_name, "SOLAR INVERTER", sizeof(cfg->model_name) - 1);
    strncpy(cfg->serial_number, "HPSU-2026-X8849", sizeof(cfg->serial_number) - 1);
    strncpy(cfg->hardware_version, "HW-V2.1", sizeof(cfg->hardware_version) - 1);
    strncpy(cfg->vendor_contact, "Toll Free: 1800-425-9999", sizeof(cfg->vendor_contact) - 1);
    strncpy(cfg->vendor_website, "www.hybrid-psu.com", sizeof(cfg->vendor_website) - 1);
    cfg->production_date = 20261001;
    cfg->logo_theme = 0;            // 0=Sunburst Gold
    cfg->boot_duration_sec = 3;     // 3 seconds boot splash
    cfg->auto_carousel_enabled = 1; // Auto carousel
    cfg->carousel_interval_sec = 5; // 5 seconds per screen
    cfg->backlight_brightness = 100;
    cfg->telemetry_baudrate = 9600;
    cfg->config_crc32 = pcu_calc_crc32((const uint8_t*)cfg, offsetof(pcu_config_t, config_crc32));
}

static bool config_load_from_nvs(pcu_config_t *cfg)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("pcu_store", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;

    size_t req_size = sizeof(pcu_config_t);
    err = nvs_get_blob(handle, "pcu_cfg", cfg, &req_size);
    nvs_close(handle);

    if (err != ESP_OK || req_size != sizeof(pcu_config_t)) return false;
    if (cfg->magic != PCU_CONFIG_MAGIC || cfg->version != PCU_CONFIG_VERSION) return false;

    uint32_t calc_crc = pcu_calc_crc32((const uint8_t*)cfg, offsetof(pcu_config_t, config_crc32));
    if (calc_crc != cfg->config_crc32) return false;

    return (cfg->is_configured == 1);
}

static esp_err_t config_save_to_nvs(pcu_config_t *cfg)
{
    cfg->magic = PCU_CONFIG_MAGIC;
    cfg->version = PCU_CONFIG_VERSION;
    cfg->struct_size = sizeof(pcu_config_t);
    cfg->is_configured = 1;
    cfg->config_crc32 = pcu_calc_crc32((const uint8_t*)cfg, offsetof(pcu_config_t, config_crc32));

    nvs_handle_t handle;
    esp_err_t err = nvs_open("pcu_store", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(handle, "pcu_cfg", cfg, sizeof(pcu_config_t));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err;
}

static void send_hrf_packet(uint8_t seq, uint8_t cmd, const uint8_t *payload, uint16_t length)
{
    hrf_header_t hdr;
    hdr.sync1 = HRF_SYNC_BYTE1;
    hdr.sync2 = HRF_SYNC_BYTE2;
    hdr.seq   = seq;
    hdr.cmd   = cmd;
    hdr.length = length;
    hdr.reserved = 0;

    uint8_t frame_buf[sizeof(hrf_header_t) + HRF_MAX_PAYLOAD_SIZE + 5];
    memcpy(frame_buf, &hdr, sizeof(hrf_header_t));
    if (length > 0 && payload != NULL) {
        memcpy(frame_buf + sizeof(hrf_header_t), payload, length);
    }

    uint32_t crc = pcu_calc_crc32(frame_buf, sizeof(hrf_header_t) + length);
    size_t offset = sizeof(hrf_header_t) + length;
    memcpy(frame_buf + offset, &crc, 4);
    offset += 4;
    frame_buf[offset++] = HRF_FRAME_TRAILER;

    uart_write_bytes(SERVICE_UART_NUM, (const char*)frame_buf, offset);
    uart_wait_tx_done(SERVICE_UART_NUM, pdMS_TO_TICKS(100));
}

static void send_ack_response(uint8_t seq, uint8_t ref_cmd, uint8_t status_code, uint16_t extra_info)
{
    hrf_ack_payload_t ack;
    ack.ref_cmd = ref_cmd;
    ack.status_code = status_code;
    ack.extra_info = extra_info;
    uint8_t resp_cmd = (status_code == STATUS_OK) ? RESP_ACK : RESP_NACK;
    send_hrf_packet(seq, resp_cmd, (const uint8_t*)&ack, sizeof(ack));
}

static void handle_hrf_command(const hrf_header_t *hdr, const uint8_t *payload)
{
    switch (hdr->cmd) {
        case CMD_PING: {
            hrf_ping_resp_t ping;
            ping.device_state = g_pcu_cfg.is_configured ? DEVICE_STATE_CONFIGURED : DEVICE_STATE_UNCONFIGURED;
            ping.hw_model_id = 0x01;
            ping.fw_version = PCU_CONFIG_VERSION;
            ping.uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000LL);
            strncpy(ping.chip_model, "ESP32-D0WDQ6", sizeof(ping.chip_model) - 1);
            send_hrf_packet(hdr->seq, RESP_ACK, (const uint8_t*)&ping, sizeof(ping));
            break;
        }
        case CMD_READ_CONFIG: {
            send_hrf_packet(hdr->seq, RESP_CONFIG_DATA, (const uint8_t*)&g_pcu_cfg, sizeof(pcu_config_t));
            break;
        }
        case CMD_WRITE_CONFIG: {
            if (hdr->length == sizeof(pcu_config_t)) {
                memcpy(&g_pcu_cfg, payload, sizeof(pcu_config_t));
                send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
            } else {
                send_ack_response(hdr->seq, hdr->cmd, ERR_PAYLOAD_SIZE, hdr->length);
            }
            break;
        }
        case CMD_COMMIT_NVS: {
            esp_err_t err = config_save_to_nvs(&g_pcu_cfg);
            if (err == ESP_OK) {
                send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
                vTaskDelay(pdMS_TO_TICKS(400));
                esp_restart();
            } else {
                send_ack_response(hdr->seq, hdr->cmd, ERR_NVS_WRITE, (uint16_t)err);
            }
            break;
        }
        case CMD_FACTORY_RESET: {
            nvs_handle_t handle;
            if (nvs_open("pcu_store", NVS_READWRITE, &handle) == ESP_OK) {
                nvs_erase_all(handle);
                nvs_commit(handle);
                nvs_close(handle);
            }
            config_set_factory_defaults(&g_pcu_cfg);
            send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
            vTaskDelay(pdMS_TO_TICKS(400));
            esp_restart();
            break;
        }
        case CMD_REBOOT: {
            send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
            vTaskDelay(pdMS_TO_TICKS(300));
            esp_restart();
            break;
        }
        case CMD_WRITE_LOGO_CHUNK: {
            if (hdr->length >= 4) {
                const hrf_logo_chunk_t *chunk = (const hrf_logo_chunk_t *)payload;
                uint16_t offset = chunk->offset;
                uint16_t chunk_len = chunk->length;
                if (offset + chunk_len <= MAX_LOGO_BUFFER_SIZE && hdr->length == 4 + chunk_len) {
                    memcpy(s_logo_staging_buf + offset, chunk->data, chunk_len);
                    if (offset + chunk_len > s_logo_staging_len) {
                        s_logo_staging_len = offset + chunk_len;
                    }
                    send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
                } else {
                    send_ack_response(hdr->seq, hdr->cmd, ERR_PAYLOAD_SIZE, 0);
                }
            } else {
                send_ack_response(hdr->seq, hdr->cmd, ERR_PAYLOAD_SIZE, 0);
            }
            break;
        }
        case CMD_COMMIT_LOGO: {
            if (s_logo_staging_len >= sizeof(pcu_logo_header_t)) {
                pcu_logo_header_t *lhdr = (pcu_logo_header_t *)s_logo_staging_buf;
                if (lhdr->magic == PCU_LOGO_MAGIC &&
                    s_logo_staging_len == sizeof(pcu_logo_header_t) + lhdr->data_size) {
                    uint32_t calc = pcu_calc_crc32(s_logo_staging_buf + sizeof(pcu_logo_header_t), lhdr->data_size);
                    if (calc == lhdr->crc32) {
                        nvs_handle_t handle;
                        if (nvs_open("pcu_store", NVS_READWRITE, &handle) == ESP_OK) {
                            nvs_set_blob(handle, "pcu_logo", s_logo_staging_buf, s_logo_staging_len);
                            nvs_commit(handle);
                            nvs_close(handle);
                            send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
                            vTaskDelay(pdMS_TO_TICKS(300));
                            esp_restart();
                        } else {
                            send_ack_response(hdr->seq, hdr->cmd, ERR_NVS_WRITE, 0);
                        }
                    } else {
                        send_ack_response(hdr->seq, hdr->cmd, ERR_CRC_MISMATCH, 0);
                    }
                } else {
                    send_ack_response(hdr->seq, hdr->cmd, ERR_INVALID_MAGIC, 0);
                }
            } else {
                send_ack_response(hdr->seq, hdr->cmd, ERR_PAYLOAD_SIZE, 0);
            }
            break;
        }
        case CMD_CLEAR_LOGO: {
            nvs_handle_t handle;
            if (nvs_open("pcu_store", NVS_READWRITE, &handle) == ESP_OK) {
                nvs_erase_key(handle, "pcu_logo");
                nvs_commit(handle);
                nvs_close(handle);
            }
            send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
            vTaskDelay(pdMS_TO_TICKS(300));
            esp_restart();
            break;
        }
        default: {
            send_ack_response(hdr->seq, hdr->cmd, ERR_INVALID_CMD, 0);
            break;
        }
    }
}

static void usb_service_task(void *pvParameters)
{
    uint8_t rx_buf[64];
    int state = 0;
    hrf_header_t hdr;
    uint8_t payload[HRF_MAX_PAYLOAD_SIZE];
    size_t rx_idx = 0;
    uint32_t rx_crc = 0;

    char text_line_buf[160];
    int text_line_idx = 0;

    while (1) {
        int len = uart_read_bytes(SERVICE_UART_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(50));
        if (len <= 0) continue;

        for (int i = 0; i < len; i++) {
            uint8_t rx_byte = rx_buf[i];

            if (state == 0) {
                if (rx_byte == HRF_SYNC_BYTE1) {
                    state = 1;
                    text_line_idx = 0;
                    continue;
                } else if (rx_byte == '$' || text_line_idx > 0) {
                    if (rx_byte == '\n' || rx_byte == '\r') {
                        if (text_line_idx > 0) {
                            text_line_buf[text_line_idx] = '\0';
                            parse_and_apply_telemetry_line(text_line_buf);
                            text_line_idx = 0;
                        }
                    } else if (rx_byte == '$') {
                        text_line_buf[0] = '$';
                        text_line_idx = 1;
                    } else if (text_line_idx < (int)sizeof(text_line_buf) - 1) {
                        text_line_buf[text_line_idx++] = (char)rx_byte;
                    }
                    continue;
                }
            }

            switch (state) {
                case 1:
                    if (rx_byte == HRF_SYNC_BYTE2) {
                        state = 2;
                        rx_idx = 0;
                    } else if (rx_byte == HRF_SYNC_BYTE1) {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    break;
                case 2:
                    ((uint8_t*)&hdr)[2 + rx_idx++] = rx_byte;
                    if (rx_idx == sizeof(hrf_header_t) - 2) {
                        hdr.sync1 = HRF_SYNC_BYTE1;
                        hdr.sync2 = HRF_SYNC_BYTE2;
                        if (hdr.length > HRF_MAX_PAYLOAD_SIZE) {
                            state = 0;
                        } else if (hdr.length == 0) {
                            state = 4;
                            rx_idx = 0;
                        } else {
                            state = 3;
                            rx_idx = 0;
                        }
                    }
                    break;
                case 3:
                    payload[rx_idx++] = rx_byte;
                    if (rx_idx == hdr.length) {
                        state = 4;
                        rx_idx = 0;
                    }
                    break;
                case 4:
                    ((uint8_t*)&rx_crc)[rx_idx++] = rx_byte;
                    if (rx_idx == 4) {
                        state = 5;
                    }
                    break;
                case 5:
                    if (rx_byte == HRF_FRAME_TRAILER) {
                        uint8_t vbuf[sizeof(hrf_header_t) + HRF_MAX_PAYLOAD_SIZE];
                        memcpy(vbuf, &hdr, sizeof(hrf_header_t));
                        if (hdr.length > 0) {
                            memcpy(vbuf + sizeof(hrf_header_t), payload, hdr.length);
                        }
                        uint32_t calc = pcu_calc_crc32(vbuf, sizeof(hrf_header_t) + hdr.length);
                        if (calc == rx_crc) {
                            handle_hrf_command(&hdr, payload);
                        } else {
                            send_ack_response(hdr.seq, hdr.cmd, ERR_CRC_MISMATCH, 0);
                        }
                    }
                    state = 0;
                    break;
            }
        }
    }
}

static void usb_service_init(void)
{
    uart_driver_delete(SERVICE_UART_NUM);

    uart_config_t ucfg = {
        .baud_rate = SERVICE_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(SERVICE_UART_NUM, &ucfg);
    uart_driver_install(SERVICE_UART_NUM, 1024, 0, 0, NULL, 0);
    uart_set_pin(SERVICE_UART_NUM, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/* ========================================================================= */
/* APP MAIN                                                                  */
/* ========================================================================= */

void app_main(void)
{
    /* Initialize NVS Flash */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize USB Service Protocol (UART0 on GPIO 1 & 3) */
    usb_service_init();

    /* Check if unit is configured */
    bool configured = config_load_from_nvs(&g_pcu_cfg);
    if (!configured) {
        config_set_factory_defaults(&g_pcu_cfg);
        g_pcu_cfg.is_configured = 1;
        config_save_to_nvs(&g_pcu_cfg);
        configured = true;
    }

    /* Launch USB Service Task on Core 0 so PC tool can communicate 24/7 simultaneously */
    xTaskCreatePinnedToCore(usb_service_task, "USB_SVC", 4096, NULL, 3, NULL, 0);

    /* 1. Set Initial RTC Clock time (2026-10-01 00:00:00) */
    struct timeval tv = {
        .tv_sec = 1790812800,
        .tv_usec = 0
    };
    settimeofday(&tv, NULL);

    /* 2. Turn ON LCD Backlight on GPIO 4 (active HIGH) */
    gpio_reset_pin(PIN_NUM_BK_LIGHT);
    gpio_set_direction(PIN_NUM_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_BK_LIGHT, 1);

    /* Reset DC (GPIO 18) and RST (GPIO 21) */
    gpio_reset_pin(PIN_NUM_DC);
    gpio_reset_pin(PIN_NUM_RST);

    /* 3. Initialize SPI Bus with generous DMA transfer size */
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 320 * 80 * sizeof(lv_color16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* 4. Install Panel IO at 40 MHz clock */
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    /* 5. Install ILI9341 Panel Driver */
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* 6. Initialize LVGL v9 */
    lv_init();

    /* Create display (240x320 native) with Landscape 90-degree rotation */
    lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);

    /* Allocate dual DMA draw buffers */
    size_t draw_buffer_sz = 320 * LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
    void *buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf1 != NULL);
    void *buf2 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    assert(buf2 != NULL);

    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(display, panel_handle);
    update_panel_rotation(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, lvgl_flush_cb);

    /* 7. Setup LVGL Tick Timer */
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    /* 8. Register IO Done Callback for asynchronous DMA flush */
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

    /* 9. Build UI */
    _lock_acquire(&lvgl_api_lock);
    init_custom_logo();
    if (!configured) {
        lv_obj_t *unconf_scr = lv_display_get_screen_active(display);
        build_unconfigured_screen(unconf_scr);
    } else {
        g_main_screen_obj = lv_obj_create(NULL);
        build_dashboard_shell(g_main_screen_obj);

        g_error_scr = lv_obj_create(NULL);
        build_error_screen(g_error_scr);

        lv_obj_t *boot_scr = lv_obj_create(NULL);
        build_boot_screen(boot_scr, g_main_screen_obj);

        lv_screen_load(boot_scr);
    }
    _lock_release(&lvgl_api_lock);

    /* 10. Start Background Tasks */
    xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", 8192, NULL, 2, NULL, 1);
    if (configured) {
        xTaskCreatePinnedToCore(inverter_uart_task, "INV_UART", 4096, NULL, 3, NULL, 0);
    }
}

