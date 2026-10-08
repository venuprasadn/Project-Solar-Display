#include "aws_iot_client.h"
#include "wifi_manager.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/lock.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "esp_netif_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "aws_iot_certs.h"

static const char *TAG = "AWS_IOT";

#define AWS_IOT_ENDPOINT   "a15qebuvm1g118-ats.iot.ap-southeast-2.amazonaws.com"
#define AWS_IOT_PORT       8883

#define AWS_PROV_TEMPLATE  "SunGridNova_Provisioning_Template"

#define TOPIC_CERT_CREATE_REQ  "$aws/certificates/create/json"
#define TOPIC_CERT_CREATE_ACC  "$aws/certificates/create/json/accepted"
#define TOPIC_CERT_CREATE_REJ  "$aws/certificates/create/json/rejected"

#define TOPIC_PROV_REQ         "$aws/provisioning-templates/" AWS_PROV_TEMPLATE "/provision/json"
#define TOPIC_PROV_ACC         "$aws/provisioning-templates/" AWS_PROV_TEMPLATE "/provision/json/accepted"
#define TOPIC_PROV_REJ         "$aws/provisioning-templates/" AWS_PROV_TEMPLATE "/provision/json/rejected"

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_is_mqtt_connected = false;
static bool s_is_provisioned = false;
static char s_thing_id[32] = "SunGridNova-XXXX";
static char s_mac_str[18] = {0};
static char s_mac_clean[14] = {0};
static char s_topic_telemetry[64];
static char s_topic_alerts[64];

/* External lock from main.c */
extern _lock_t lvgl_api_lock;

static int s_tls_esp_err = 0;
static int s_tls_stack_err = 0;
static int s_tls_cert_flags = 0;
static int s_tls_error_type = 0;

int aws_iot_get_last_esp_err(void) { return s_tls_esp_err; }
int aws_iot_get_last_stack_err(void) { return s_tls_stack_err; }
int aws_iot_get_last_cert_flags(void) { return s_tls_cert_flags; }
int aws_iot_get_last_error_type(void) { return s_tls_error_type; }

static int64_t s_client_active_until = 0;

bool aws_iot_is_app_client_active(void)
{
    return (s_is_mqtt_connected && (esp_timer_get_time() < s_client_active_until));
}

static bool check_nvs_provisioned(void)
{
    nvs_handle_t nvs;
    uint8_t val = 0;
    if (nvs_open("aws_prov", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u8(nvs, "reg_done", &val);
        nvs_close(nvs);
    }
    return (val == 1);
}

static void set_nvs_provisioned(bool done)
{
    nvs_handle_t nvs;
    if (nvs_open("aws_prov", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_u8(nvs, "reg_done", done ? 1 : 0);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "💾 Saved provisioning status to NVS: %d", done ? 1 : 0);
    }
}

static void start_fleet_provisioning(void)
{
    ESP_LOGI(TAG, "🚀 Initiating AWS IoT Fleet Provisioning by Claim for %s...", s_thing_id);
    esp_mqtt_client_subscribe(s_mqtt_client, TOPIC_CERT_CREATE_ACC, 1);
    esp_mqtt_client_subscribe(s_mqtt_client, TOPIC_CERT_CREATE_REJ, 1);
    esp_mqtt_client_subscribe(s_mqtt_client, TOPIC_PROV_ACC, 1);
    esp_mqtt_client_subscribe(s_mqtt_client, TOPIC_PROV_REJ, 1);

    /* Step 1: Request Certificate Creation via Claim */
    esp_mqtt_client_publish(s_mqtt_client, TOPIC_CERT_CREATE_REQ, "{}", 2, 1, 0);
    ESP_LOGI(TAG, "📤 Sent certificate request to %s", TOPIC_CERT_CREATE_REQ);
}

static void handle_provisioning_data(const char *topic, const char *data, int data_len)
{
    if (strcmp(topic, TOPIC_CERT_CREATE_ACC) == 0) {
        ESP_LOGI(TAG, "📥 Certificate creation accepted by AWS IoT!");
        cJSON *root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON *token_item = cJSON_GetObjectItem(root, "certificateOwnershipToken");
            if (token_item && token_item->valuestring) {
                pcu_vendor_snapshot_t v_snap = {0};
                get_pcu_vendor_snapshot(&v_snap);
                char v_group[64];
                snprintf(v_group, sizeof(v_group), "Vendor_%s", v_snap.brand_title);
                for (int i = 0; v_group[i]; i++) {
                    if (v_group[i] == ' ') v_group[i] = '_';
                }

                cJSON *req = cJSON_CreateObject();
                cJSON_AddStringToObject(req, "certificateOwnershipToken", token_item->valuestring);
                cJSON *params = cJSON_CreateObject();
                cJSON_AddStringToObject(params, "SerialNumber", s_mac_clean);
                cJSON_AddStringToObject(params, "MAC", s_mac_str);
                cJSON_AddStringToObject(params, "VendorGroup", v_group);
                cJSON_AddItemToObject(req, "parameters", params);

                char *req_json = cJSON_PrintUnformatted(req);
                if (req_json) {
                    ESP_LOGI(TAG, "📤 Registering Thing in Template '%s' under '%s'...", AWS_PROV_TEMPLATE, v_group);
                    esp_mqtt_client_publish(s_mqtt_client, TOPIC_PROV_REQ, req_json, strlen(req_json), 1, 0);
                    free(req_json);
                }
                cJSON_Delete(req);
            }
            cJSON_Delete(root);
        }
    } else if (strcmp(topic, TOPIC_PROV_ACC) == 0) {
        ESP_LOGI(TAG, "🎉 AWS IoT Fleet Provisioning SUCCESS! Thing '%s' registered in AWS Registry.", s_thing_id);
        s_is_provisioned = true;
        set_nvs_provisioned(true);

        /* Unsubscribe from provisioning topics */
        esp_mqtt_client_unsubscribe(s_mqtt_client, TOPIC_CERT_CREATE_ACC);
        esp_mqtt_client_unsubscribe(s_mqtt_client, TOPIC_CERT_CREATE_REJ);
        esp_mqtt_client_unsubscribe(s_mqtt_client, TOPIC_PROV_ACC);
        esp_mqtt_client_unsubscribe(s_mqtt_client, TOPIC_PROV_REJ);
    } else if (strcmp(topic, TOPIC_CERT_CREATE_REJ) == 0 || strcmp(topic, TOPIC_PROV_REJ) == 0) {
        ESP_LOGW(TAG, "⚠️ AWS IoT Provisioning rejected on %s: %.*s", topic, data_len, data);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_is_mqtt_connected = true;
            s_tls_esp_err = 0;
            s_tls_stack_err = 0;
            s_tls_cert_flags = 0;
            s_tls_error_type = 0;
            ESP_LOGI(TAG, "✅ Connected to AWS IoT Core securely (mTLS 1.2 / Port 8883)");
            esp_mqtt_client_subscribe(s_mqtt_client, "solar/presence", 1);
            char cmd_topic[64];
            snprintf(cmd_topic, sizeof(cmd_topic), "solar/%s/cmd", s_thing_id);
            esp_mqtt_client_subscribe(s_mqtt_client, cmd_topic, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, "solar/+/cmd", 1);

            if (!s_is_provisioned) {
                start_fleet_provisioning();
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_is_mqtt_connected = false;
            ESP_LOGW(TAG, "AWS IoT Core disconnected. Auto-reconnecting in background...");
            break;
        case MQTT_EVENT_DATA:
            if (event->topic && event->topic_len > 0) {
                char topic_buf[128] = {0};
                int tlen = (event->topic_len < 127) ? event->topic_len : 127;
                memcpy(topic_buf, event->topic, tlen);
                topic_buf[tlen] = '\0';

                if (strstr(topic_buf, "presence") != NULL) {
                    s_client_active_until = esp_timer_get_time() + (35 * 1000000ULL);
                    ESP_LOGI(TAG, "📱 Mobile App client presence detected! Real-time telemetry engaged.");
                } else if (strstr(topic_buf, "cmd") != NULL) {
                    if (event->data && (strstr(event->data, "factory_reset") || strstr(event->data, "FACTORY_RESET"))) {
                        ESP_LOGW(TAG, "🚨 Remote Factory Reset received via AWS IoT! Erasing NVS and rebooting...");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        nvs_flash_erase();
                        vTaskDelay(pdMS_TO_TICKS(300));
                        esp_restart();
                    }
                } else if (strstr(topic_buf, "$aws/") != NULL) {
                    handle_provisioning_data(topic_buf, event->data, event->data_len);
                }
            }
            break;
        case MQTT_EVENT_ERROR:
            if (event->error_handle) {
                s_tls_error_type = (int)event->error_handle->error_type;
                s_tls_esp_err = event->error_handle->esp_tls_last_esp_err;
                s_tls_stack_err = event->error_handle->esp_tls_stack_err;
                s_tls_cert_flags = event->error_handle->esp_tls_cert_verify_flags;
                ESP_LOGE(TAG, "MQTT TLS transport error: type=%d, esp_err=%d, stack_err=%d, cert_flags=0x%x",
                         s_tls_error_type, s_tls_esp_err, s_tls_stack_err, s_tls_cert_flags);
            }
            break;
        default:
            break;
    }
}

static void aws_iot_publisher_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    uint32_t seq = 0;
    inverter_telemetry_snapshot_t last_snap = {0};
    uint32_t last_pub_time = 0;
    bool has_published_first = false;

    while (1) {
        esp_task_wdt_reset();

        /* Only publish if Wi-Fi and AWS MQTT are connected */
        if (s_is_mqtt_connected) {
            inverter_telemetry_snapshot_t snap;
            get_inverter_data_snapshot(&snap);

            uint32_t uptime = (uint32_t)(esp_timer_get_time() / 1000000ULL);
            bool is_client_active = (esp_timer_get_time() < s_client_active_until);

            bool should_publish = false;

            if (!has_published_first) {
                should_publish = true;
            } else if (snap.error_code != last_snap.error_code) {
                /* Instant transmission on protection fault trigger/cleared */
                should_publish = true;
            } else if (is_client_active) {
                /* Report-by-Exception: send on meaningful deadband change */
                bool changed = false;
                if (fabsf(snap.solarvolt - last_snap.solarvolt) >= 0.5f ||
                    fabsf(snap.battvolts - last_snap.battvolts) >= 0.2f ||
                    fabsf(snap.mainsvolt - last_snap.mainsvolt) >= 1.0f ||
                    fabsf(snap.acout - last_snap.acout) >= 1.0f ||
                    fabsf(snap.loaddisp - last_snap.loaddisp) >= 10.0f ||
                    fabsf(snap.chrampsdisp - last_snap.chrampsdisp) >= 0.5f ||
                    fabsf(snap.dischdisp - last_snap.dischdisp) >= 0.5f ||
                    fabsf(snap.dcboost - last_snap.dcboost) >= 2.0f ||
                    fabsf(snap.upsheat - last_snap.upsheat) >= 1.0f) {
                    changed = true;
                }

                /* Active client: transmit on deadband change OR 3-second live heartbeat */
                if (changed || (uptime - last_pub_time >= 3)) {
                    should_publish = true;
                }
            } else {
                /* Standby mode: 15-second background sync for responsive cloud tracking */
                if (uptime - last_pub_time >= 15) {
                    should_publish = true;
                }
            }

            if (should_publish) {
                pcu_vendor_snapshot_t v_snap = {0};
                get_pcu_vendor_snapshot(&v_snap);

                char v_group[64];
                snprintf(v_group, sizeof(v_group), "Vendor_%s", v_snap.brand_title);
                for (int i = 0; v_group[i]; i++) {
                    if (v_group[i] == ' ') v_group[i] = '_';
                }

                char payload[1024];
                snprintf(payload, sizeof(payload),
                         "{\"thing\":\"%s\",\"mac\":\"%s\",\"vendor_group\":\"%s\",\"seq\":%lu,\"up\":%lu,\"sol_v\":%.1f,\"bat_v\":%.1f,"
                         "\"grid_v\":%.0f,\"ac_out\":%.0f,\"load_pct\":%.0f,\"chg_a\":%.1f,"
                         "\"dis_a\":%.1f,\"dc_b\":%.0f,\"heat\":%.1f,\"err\":%d,\"ssid\":\"%s\",\"ip\":\"%s\","
                         "\"vendor\":\"%s\",\"model\":\"%s\",\"sn\":\"%s\",\"hw\":\"%s\",\"contact\":\"%s\",\"site\":\"%s\"}",
                         s_thing_id, s_mac_str, v_group, seq++, uptime, snap.solarvolt, snap.battvolts,
                         snap.mainsvolt, snap.acout, snap.loaddisp, snap.chrampsdisp,
                         snap.dischdisp, snap.dcboost, snap.upsheat, snap.error_code,
                         wifi_manager_get_ssid(), wifi_manager_get_ip_str(),
                         v_snap.brand_title, v_snap.model_name, v_snap.serial_number,
                         v_snap.hardware_version, v_snap.vendor_contact, v_snap.vendor_website);

                int msg_id = esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, payload, 0, 0, 0);
                ESP_LOGI(TAG, "📤 Published Telemetry to AWS IoT [ID:%d] (Thing:%s, Group:%s)",
                         msg_id, s_thing_id, v_group);

                last_snap = snap;
                last_pub_time = uptime;
                has_published_first = true;

                /* If there is an active protection fault, publish instant alert topic */
                if (snap.error_code != 0) {
                    char alert_payload[128];
                    snprintf(alert_payload, sizeof(alert_payload),
                             "{\"thing\":\"%s\",\"alert\":\"PROTECTION_TRIP\",\"code\":%d,\"time\":%lu}",
                             s_thing_id, snap.error_code, uptime);
                    esp_mqtt_client_publish(s_mqtt_client, s_topic_alerts, alert_payload, 0, 1, 0);
                    ESP_LOGW(TAG, "🚨 High-Priority Safety Alert pushed to AWS IoT [Code %d]!", snap.error_code);
                }
            }
        }

        /* 1-Second evaluation cycle */
        for (int i = 0; i < 10; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_task_wdt_reset();
        }
    }
}

static bool s_client_inited = false;

esp_err_t aws_iot_client_init(void)
{
    if (s_client_inited) {
        if (s_mqtt_client && !s_is_mqtt_connected) {
            ESP_LOGI(TAG, "Reconnecting MQTT client on Wi-Fi IP update...");
            esp_mqtt_client_reconnect(s_mqtt_client);
        }
        return ESP_OK;
    }
    s_client_inited = true;

    /* 0. Seed RTC baseline into 2026 for immediate mTLS certificate validation */
    time_t now;
    time(&now);
    if (now < 1767225600) { // Jan 1, 2026 UTC
        struct timeval tv = { .tv_sec = 1775000000, .tv_usec = 0 }; // Year 2026
        settimeofday(&tv, NULL);
        ESP_LOGI(TAG, "Baseline RTC clock seeded to 2026 for TLS 1.2 mTLS validation");
    }

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&sntp_cfg);

    /* 1. Generate unique Hardware Thing ID from full 6-byte eFuse MAC */
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(s_mac_str, sizeof(s_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(s_mac_clean, sizeof(s_mac_clean), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(s_thing_id, sizeof(s_thing_id), "SunGridNova-%s", s_mac_clean);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "solar/%s/telemetry", s_thing_id);
    snprintf(s_topic_alerts, sizeof(s_topic_alerts), "solar/%s/alerts", s_thing_id);

    s_is_provisioned = check_nvs_provisioned();

    ESP_LOGI(TAG, "Initializing AWS IoT Client for Thing: %s [Provisioned: %s] (Free Heap: %lu, Largest Block: %lu)",
             s_thing_id, s_is_provisioned ? "YES" : "NO",
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    /* 2. Configure MQTT Client for AWS IoT Core (mTLS 1.2) */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .hostname = AWS_IOT_ENDPOINT,
                .transport = MQTT_TRANSPORT_OVER_SSL,
                .port = AWS_IOT_PORT,
            },
            .verification = {
                .certificate = AWS_ROOT_CA_CERT,
                .certificate_len = 0,
            },
        },
        .credentials = {
            .client_id = s_thing_id,
            .authentication = {
                .certificate = AWS_CLIENT_CERT,
                .certificate_len = 0,
                .key = AWS_CLIENT_KEY,
                .key_len = 0,
            },
        },
        .session = {
            .keepalive = 60,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
        },
        .task = {
            .stack_size = 8192,
        },
        .buffer = {
            .size = 5120,
            .out_size = 2048,
        },
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_mqtt_client) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client handle");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt_client);

    /* 3. Launch Telemetry Publisher Task on Core 0 */
    xTaskCreatePinnedToCore(aws_iot_publisher_task, "AWS_PUB", 6144, NULL, 3, NULL, 0);

    return ESP_OK;
}

bool aws_iot_client_is_connected(void)
{
    return s_is_mqtt_connected;
}

void aws_iot_client_on_wifi_connected(void)
{
    if (s_mqtt_client && !s_is_mqtt_connected) {
        ESP_LOGI(TAG, "Wi-Fi up - reconnecting AWS IoT MQTT broker...");
        esp_mqtt_client_reconnect(s_mqtt_client);
    }
}

const char* aws_iot_get_thing_id(void)
{
    return s_thing_id;
}
