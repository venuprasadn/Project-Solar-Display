#include "wifi_manager.h"
#include "aws_iot_client.h"
#include "ble_provisioning.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_timer.h"

static const char *TAG = "WIFI_MGR";

static bool s_is_connected = false;
static char s_ip_str[16] = "0.0.0.0";
static char s_active_ssid[33] = "Prasadam-BSNL-2.4G";
static int s_retry_num = 0;
static uint16_t s_last_disconnect_reason = 0;
static esp_timer_handle_t s_reconnect_timer = NULL;

static void wifi_reconnect_timer_cb(void *arg)
{
    if (!s_is_connected) {
        ESP_LOGI(TAG, "Attempting Wi-Fi reconnection...");
        esp_wifi_connect();
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *dis = (wifi_event_sta_disconnected_t*) event_data;
        if (dis) {
            s_last_disconnect_reason = dis->reason;
        }
        s_is_connected = false;
        s_retry_num++;
        ESP_LOGW(TAG, "Wi-Fi disconnected (reason %d). Scheduling reconnect in 3s (attempt %d)...",
                 dis ? dis->reason : -1, s_retry_num);
        if (s_reconnect_timer) {
            esp_timer_stop(s_reconnect_timer);
            esp_timer_start_once(s_reconnect_timer, 3000 * 1000);
        }
        /* Re-init BLE Provisioning so user can immediately re-provision over Bluetooth */
        ble_provisioning_init();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        s_is_connected = true;
        s_retry_num = 0;
        if (s_reconnect_timer) {
            esp_timer_stop(s_reconnect_timer);
        }
        ESP_LOGI(TAG, "✅ Wi-Fi Connected! IP Address: %s", s_ip_str);
        ble_provisioning_notify_status(true);
        /* Keep BLE provisioning active so user can re-provision over Bluetooth anytime */
        aws_iot_client_init();
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

    esp_timer_create_args_t timer_args = {
        .callback = &wifi_reconnect_timer_cb,
        .name = "wifi_reconn"
    };
    esp_timer_create(&timer_args, &s_reconnect_timer);

    /* 2. Load SSID and Password from NVS */
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    nvs_handle_t nvs_h;
    char ssid_buf[32] = "Prasadam-BSNL-2.4G";
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

    if (strlen(ssid_buf) == 0) {
        strncpy(ssid_buf, "Prasadam-BSNL-2.4G", sizeof(ssid_buf));
        strncpy(pass_buf, "venu6076!", sizeof(pass_buf));
    }

    strncpy((char*)wifi_config.sta.ssid, ssid_buf, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, pass_buf, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));


    strncpy(s_active_ssid, ssid_buf, sizeof(s_active_ssid) - 1);
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

const char* wifi_manager_get_ssid(void)
{
    return s_active_ssid;
}

esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0 || strlen(ssid) >= 32) {
        ESP_LOGE(TAG, "Invalid SSID parameter");
        return ESP_ERR_INVALID_ARG;
    }
    if (password && strlen(password) >= 64) {
        ESP_LOGE(TAG, "Password exceeds max 63 characters");
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_h;
    esp_err_t err = nvs_open("wifi", NVS_READWRITE, &nvs_h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS wifi namespace: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_str(nvs_h, "ssid", ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(nvs_h, "password", password ? password : "");
    }
    if (err == ESP_OK) {
        err = nvs_commit(nvs_h);
    }
    nvs_close(nvs_h);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit wifi credentials to NVS: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "✅ New Wi-Fi credentials saved to NVS (SSID: %s). Reconnecting...", ssid);

    strncpy(s_active_ssid, ssid, sizeof(s_active_ssid) - 1);
    /* Update live Wi-Fi configuration and reconnect */
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    if (s_reconnect_timer) {
        esp_timer_stop(s_reconnect_timer);
    }
    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    return ESP_OK;
}

uint16_t wifi_manager_get_last_disconnect_reason(void)
{
    return s_last_disconnect_reason;
}

void wifi_manager_stop_reconnect_timer(void)
{
    if (s_reconnect_timer) {
        esp_timer_stop(s_reconnect_timer);
    }
}

void wifi_manager_start_reconnect_timer(void)
{
    if (s_reconnect_timer && !s_is_connected) {
        esp_timer_start_once(s_reconnect_timer, 1000 * 1000);
    }
}

