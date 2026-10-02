#include "wifi_manager.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "WIFI_MGR";

static bool s_is_connected = false;
static char s_ip_str[16] = "0.0.0.0";
static int s_retry_num = 0;
static uint32_t s_backoff_ms = 1000;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_is_connected = false;
        s_retry_num++;
        
        /* Exponential backoff: 1s, 2s, 4s, 8s, 16s... capped at 60s */
        if (s_backoff_ms < 60000) {
            s_backoff_ms *= 2;
            if (s_backoff_ms > 60000) s_backoff_ms = 60000;
        }
        ESP_LOGW(TAG, "Wi-Fi disconnected. Reconnecting in %lu ms (attempt %d)...", s_backoff_ms, s_retry_num);
        vTaskDelay(pdMS_TO_TICKS(s_backoff_ms));
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        s_is_connected = true;
        s_retry_num = 0;
        s_backoff_ms = 1000;
        ESP_LOGI(TAG, "✅ Wi-Fi Connected! IP Address: %s", s_ip_str);
    }
}

esp_err_t wifi_manager_init(void)
{
    /* 1. Initialize Network Interface and Event Loop */
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);

    /* 2. Load SSID and Password from NVS */
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    nvs_handle_t nvs_h;
    char ssid_buf[32] = "Prasadam-BSNL";
    char pass_buf[64] = "venu6076!";
    size_t len;

    if (nvs_open("wifi", NVS_READONLY, &nvs_h) == ESP_OK) {
        len = sizeof(ssid_buf);
        if (nvs_get_str(nvs_h, "ssid", ssid_buf, &len) == ESP_OK) {
            len = sizeof(pass_buf);
            nvs_get_str(nvs_h, "password", pass_buf, &len);
        }
        nvs_close(nvs_h);
    }

    strncpy((char*)wifi_config.sta.ssid, ssid_buf, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, pass_buf, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi Manager started (SSID: %s)", ssid_buf);
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    return s_is_connected;
}

const char* wifi_manager_get_ip_str(void)
{
    return s_ip_str;
}
