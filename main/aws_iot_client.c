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

static const char *TAG = "AWS_IOT";

#define AWS_IOT_ENDPOINT   "a15qebuvm1g118-ats.iot.ap-southeast-2.amazonaws.com"
#define AWS_IOT_PORT       8883

static const char *s_root_ca =
"-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----\n";

static const char *s_client_cert =
"-----BEGIN CERTIFICATE-----\n"
"MIICFjCCAbygAwIBAgIUU+zmCtwJr5LiZE4Ock9YzJulpQkwCgYIKoZIzj0EAwIw\n"
"cTELMAkGA1UEBhMCSU4xDzANBgNVBAgMBktlcmFsYTEPMA0GA1UEBwwGS2FubnVy\n"
"MRQwEgYDVQQKDAtTdW5HcmlkTm92YTEMMAoGA1UECwwDSW9UMRwwGgYDVQQDDBNT\n"
"dW5HcmlkTm92YSBSb290IENBMCAXDTI1MDcwMjE4MjgwM1oYDzIxMjQwNjA4MTgy\n"
"ODAzWjBfMQswCQYDVQQGEwJJTjEPMA0GA1UECAwGS2VyYWxhMQ8wDQYDVQQHDAZL\n"
"YW5udXIxFDASBgNVBAoMC1N1bkdyaWROb3ZhMQwwCgYDVQQLDANJb1QxCjAIBgNV\n"
"BAMMATEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS4O/fxZ3FQSAJU5vUPmhJ0\n"
"FPpRjDaZ8cBYurYGPjzPVTOIKaswRdKwgpaw0P6fZtoy41E/hiyJ0a1BE4Mr0gFv\n"
"o0IwQDAdBgNVHQ4EFgQUpenLX3F72HXstrVVRZBKQnvWOxwwHwYDVR0jBBgwFoAU\n"
"rrrDrznIbmEJXH0Ev4t2qZqpi78wCgYIKoZIzj0EAwIDSAAwRQIhAJrwVfW/Apin\n"
"NZ5pmfR70NYGE+L+X0QutHnSQQRE/SCHAiAqL/yw8SL3steM5S6tmfp1PmE4pZOV\n"
"HFInMT64gm7ukw==\n"
"-----END CERTIFICATE-----\n";

static const char *s_client_key =
"-----BEGIN EC PRIVATE KEY-----\n"
"MHcCAQEEIAeDoK8SV4NakJNmBp5XZtvVZlqVoTPNUqC4EKAk6qteoAoGCCqGSM49\n"
"AwEHoUQDQgAEuDv38WdxUEgCVOb1D5oSdBT6UYw2mfHAWLq2Bj48z1UziCmrMEXS\n"
"sIKWsND+n2baMuNRP4YsidGtQRODK9IBbw==\n"
"-----END EC PRIVATE KEY-----\n";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_is_mqtt_connected = false;
static char s_thing_id[32] = "SunGridNova-XXXX";
static char s_topic_telemetry[64];
static char s_topic_alerts[64];

/* External lock from main.c */
extern _lock_t lvgl_api_lock;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            s_is_mqtt_connected = true;
            ESP_LOGI(TAG, "✅ Connected to AWS IoT Core securely (mTLS 1.2 / Port 8883)");
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_is_mqtt_connected = false;
            ESP_LOGW(TAG, "AWS IoT Core disconnected. Auto-reconnecting in background...");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT TLS transport error");
            break;
        default:
            break;
    }
}

static void aws_iot_publisher_task(void *pvParameters)
{
    esp_task_wdt_add(NULL);

    uint32_t seq = 0;
    while (1) {
        esp_task_wdt_reset();

        /* Only publish if Wi-Fi and AWS MQTT are connected */
        if (s_is_mqtt_connected) {
            inverter_telemetry_snapshot_t snap;
            get_inverter_data_snapshot(&snap);

            char payload[320];
            uint32_t uptime = (uint32_t)(esp_timer_get_time() / 1000000ULL);

            snprintf(payload, sizeof(payload),
                     "{\"thing\":\"%s\",\"seq\":%lu,\"up\":%lu,\"sol_v\":%.1f,\"bat_v\":%.1f,"
                     "\"grid_v\":%.0f,\"ac_out\":%.0f,\"load_pct\":%.0f,\"chg_a\":%.1f,"
                     "\"dis_a\":%.1f,\"dc_b\":%.0f,\"heat\":%.1f,\"err\":%d}",
                     s_thing_id, seq++, uptime, snap.solarvolt, snap.battvolts,
                     snap.mainsvolt, snap.acout, snap.loaddisp, snap.chrampsdisp,
                     snap.dischdisp, snap.dcboost, snap.upsheat, snap.error_code);

            int msg_id = esp_mqtt_client_publish(s_mqtt_client, s_topic_telemetry, payload, 0, 1, 0);
            ESP_LOGI(TAG, "📤 Published Telemetry to AWS IoT [ID:%d] (Payload: %d bytes)", msg_id, (int)strlen(payload));

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

        /* 15-Second interval between regular telemetry publications */
        for (int i = 0; i < 150; i++) {
            vTaskDelay(pdMS_TO_TICKS(100));
            esp_task_wdt_reset();
        }
    }
}

esp_err_t aws_iot_client_init(void)
{
    /* 1. Generate unique Hardware Thing ID from eFuse MAC */
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(s_thing_id, sizeof(s_thing_id), "SunGridNova-%02X%02X", mac[4], mac[5]);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "solar/%s/telemetry", s_thing_id);
    snprintf(s_topic_alerts, sizeof(s_topic_alerts), "solar/%s/alerts", s_thing_id);

    ESP_LOGI(TAG, "Initializing AWS IoT Client for Thing: %s", s_thing_id);

    /* 2. Configure MQTT Client for AWS IoT Core (mTLS 1.2) */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .hostname = AWS_IOT_ENDPOINT,
                .transport = MQTT_TRANSPORT_OVER_SSL,
                .port = AWS_IOT_PORT,
            },
            .verification = {
                .certificate = s_root_ca,
            },
        },
        .credentials = {
            .client_id = s_thing_id,
            .authentication = {
                .certificate = s_client_cert,
                .key = s_client_key,
            },
        },
        .session = {
            .keepalive = 60,
        },
        .network = {
            .reconnect_timeout_ms = 5000,
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
    xTaskCreatePinnedToCore(aws_iot_publisher_task, "AWS_PUB", 4096, NULL, 3, NULL, 0);

    return ESP_OK;
}

bool aws_iot_client_is_connected(void)
{
    return s_is_mqtt_connected;
}

const char* aws_iot_get_thing_id(void)
{
    return s_thing_id;
}
