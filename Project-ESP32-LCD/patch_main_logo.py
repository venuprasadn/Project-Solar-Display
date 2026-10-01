main_path = r'D:\Project_Solar_Display\Project-ESP32-LCD\main\main.c'

with open(main_path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Add #include "esp_flash.h"
text = text.replace('#include "esp_system.h"', '#include "esp_system.h"\n#include "esp_flash.h"')

# 2. Add custom logo data structures and functions
logo_code = '''/* Typography Font Declarations */
LV_FONT_DECLARE(lv_font_montserrat_10);
LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);

/* Custom Logo Storage & LVGL Image Descriptor */
static lv_image_dsc_t custom_logo_dsc;
static uint8_t *custom_logo_pixels = NULL;
static bool custom_logo_available = false;

#define MAX_LOGO_BUFFER_SIZE  8192
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
                        custom_logo_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
                        custom_logo_dsc.header.w = hdr->width;
                        custom_logo_dsc.header.h = hdr->height;
                        custom_logo_dsc.header.stride = hdr->width * 2;
                        custom_logo_dsc.header.flags = 0;
                        custom_logo_dsc.header.reserved_2 = 0;
                        custom_logo_dsc.data_size = hdr->data_size;
                        custom_logo_dsc.data = custom_logo_pixels;
                        custom_logo_available = true;
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
                    custom_logo_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
                    custom_logo_dsc.header.w = flash_hdr.width;
                    custom_logo_dsc.header.h = flash_hdr.height;
                    custom_logo_dsc.header.stride = flash_hdr.width * 2;
                    custom_logo_dsc.header.flags = 0;
                    custom_logo_dsc.header.reserved_2 = 0;
                    custom_logo_dsc.data_size = flash_hdr.data_size;
                    custom_logo_dsc.data = custom_logo_pixels;
                    custom_logo_available = true;
                } else {
                    free(buf);
                }
            }
        }
    }
}

static const lv_image_dsc_t *get_active_logo_dsc(void)
{
    if (custom_logo_available) return &custom_logo_dsc;
    return &img_solar;
}'''

old_fonts = '''/* Typography Font Declarations */
LV_FONT_DECLARE(lv_font_montserrat_10);
LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);'''

text = text.replace(old_fonts, logo_code, 1)

# 3. Update build_page_5 emblem icon src
old_p5_icon = '''    /* Embedded solar icon centered inside emblem */
    lv_obj_t *emb_icon = lv_image_create(emblem_box);
    lv_image_set_src(emb_icon, &img_solar);
    lv_obj_center(emb_icon);'''

new_p5_icon = '''    /* Embedded solar icon centered inside emblem */
    lv_obj_t *emb_icon = lv_image_create(emblem_box);
    lv_image_set_src(emb_icon, get_active_logo_dsc());
    lv_obj_center(emb_icon);'''

text = text.replace(old_p5_icon, new_p5_icon, 1)

# 4. Update build_boot_screen icon src
old_boot_icon = '''    lv_obj_t *icon = lv_image_create(circle);
    lv_image_set_src(icon, &img_solar);
    lv_obj_center(icon);'''

new_boot_icon = '''    lv_obj_t *icon = lv_image_create(circle);
    lv_image_set_src(icon, get_active_logo_dsc());
    lv_obj_center(icon);'''

text = text.replace(old_boot_icon, new_boot_icon, 1)

# 5. Add init_custom_logo call in app_main
old_app_main_ui = '''    /* 9. Build UI */
    _lock_acquire(&lvgl_api_lock);
    if (!configured) {'''

new_app_main_ui = '''    /* 9. Build UI */
    _lock_acquire(&lvgl_api_lock);
    init_custom_logo();
    if (!configured) {'''

text = text.replace(old_app_main_ui, new_app_main_ui, 1)

# 6. Add CMD_WRITE_LOGO_CHUNK, CMD_COMMIT_LOGO, CMD_CLEAR_LOGO to handle_hrf_command
old_cmd_reboot = '''        case CMD_REBOOT: {
            send_ack_response(hdr->seq, hdr->cmd, STATUS_OK, 0);
            vTaskDelay(pdMS_TO_TICKS(300));
            esp_restart();
            break;
        }'''

new_cmds = '''        case CMD_REBOOT: {
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
        }'''

text = text.replace(old_cmd_reboot, new_cmds, 1)

with open(main_path, 'w', encoding='utf-8') as f:
    f.write(text)

print('Successfully patched main.c with custom logo support!')
