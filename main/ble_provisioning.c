#include "ble_provisioning.h"
#include "wifi_manager.h"
#include "aws_iot_client.h"
#include "esp_wifi.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_err.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "esp_bt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE_PROV";

/* Device identity */
static char s_device_name[32] = "SunGridNova-Setup";
static uint16_t s_tx_char_val_handle = 0;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;

/* 
 * 128-bit UUIDs (Little-Endian / LSB first for NimBLE):
 * Service: 12345678-1234-5678-1234-56789abcdef0
 * TX:      12345678-1234-5678-1234-56789abcdef1 (Read / Notify)
 * RX:      12345678-1234-5678-1234-56789abcdef2 (Write)
 */
static const ble_uuid128_t gatt_svr_svc_uuid =
    BLE_UUID128_INIT(0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const ble_uuid128_t gatt_svr_chr_tx_uuid =
    BLE_UUID128_INIT(0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static const ble_uuid128_t gatt_svr_chr_rx_uuid =
    BLE_UUID128_INIT(0xf2, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
                     0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

void ble_provisioning_start_advertising(void);

/* GATT Characteristic Access Callback */
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (attr_handle == s_tx_char_val_handle) {
            char resp_buf[256];
            uint8_t mac[6];
            esp_efuse_mac_get_default(mac);
            wifi_ap_record_t ap_info;
            int8_t rssi = -100;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                rssi = ap_info.rssi;
            }
            pcu_vendor_snapshot_t vsnap;
            memset(&vsnap, 0, sizeof(vsnap));
            get_pcu_vendor_snapshot(&vsnap);
            snprintf(resp_buf, sizeof(resp_buf), "%02X%02X%02X%02X%02X%02X,%d,%s,%s,%d,%s,%s,%s,%s,%s,%s",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                     wifi_manager_is_connected() ? 1 : 0,
                     wifi_manager_get_ssid(),
                     wifi_manager_get_ip_str(),
                     rssi,
                     vsnap.brand_title,
                     vsnap.model_name,
                     vsnap.serial_number,
                     vsnap.hardware_version,
                     vsnap.vendor_contact,
                     vsnap.vendor_website);

            int rc = os_mbuf_append(ctxt->om, resp_buf, strlen(resp_buf));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
    } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len == 0 || len >= 128) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        char buf[128];
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf) - 1, &len);
        if (rc != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        buf[len] = '\0';

        /* Parse <SSID>,<PASSWORD> */
        char *comma = strchr(buf, ',');
        if (!comma) {
            ESP_LOGW(TAG, "Invalid BLE payload format: missing comma separator");
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        *comma = '\0';
        const char *ssid = buf;
        const char *pass = comma + 1;

        /* Strip optional CRLF */
        char *eol = strpbrk(pass, "\r\n");
        if (eol) *eol = '\0';

        ESP_LOGI(TAG, "📲 Received Wi-Fi Provisioning via BLE: SSID='%s'", ssid);

        esp_err_t err = wifi_manager_set_credentials(ssid, pass);
        if (err == ESP_OK) {
            ble_provisioning_notify_status(true);
            return 0;
        } else {
            ble_provisioning_notify_status(false);
            return BLE_ATT_ERR_UNLIKELY;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                /* TX Characteristic: Read / Notify */
                .uuid = &gatt_svr_chr_tx_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_tx_char_val_handle,
            },
            {
                /* RX Characteristic: Write */
                .uuid = &gatt_svr_chr_rx_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                0, /* Terminator */
            }
        },
    },
    {
        0, /* Terminator */
    },
};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                s_conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "📱 BLE Client Connected (Handle: %d)", s_conn_handle);
            } else {
                s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
                ble_provisioning_start_advertising();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "BLE Client Disconnected. Resuming advertisement...");
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_provisioning_start_advertising();
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI(TAG, "Client subscribed to TX status notifications");
            break;

        default:
            break;
    }
    return 0;
}

void ble_provisioning_start_advertising(void)
{
    if (ble_gap_adv_active()) {
        return; // Already advertising
    }

    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    int rc;

    /* 1. Primary Advertising Packet: Flags + 128-bit Service UUID */
    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = &gatt_svr_svc_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set adv fields: rc=%d", rc);
    }

    /* 2. Scan Response Packet: Complete Local Name */
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.name = (uint8_t *)s_device_name;
    rsp_fields.name_len = strlen(s_device_name);
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set scan rsp fields: rc=%d", rc);
    }

    /* 3. Start Advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0 && rc != BLE_HS_EALREADY) {
        ESP_LOGE(TAG, "Failed to start advertising: rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "📡 BLE Advertising active as '%s'", s_device_name);
    }
}

static void ble_provisioning_on_sync(void)
{
    /* Start advertising once NimBLE host synchronizes */
    ble_provisioning_start_advertising();
}

static void ble_provisioning_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static bool s_ble_active = false;

esp_err_t ble_provisioning_init(void)
{
    if (s_ble_active) {
        ble_provisioning_start_advertising();
        return ESP_OK;
    }

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(s_device_name, sizeof(s_device_name), "SunGridNova-%02X%02X", mac[4], mac[5]);

    ESP_LOGI(TAG, "Initializing BLE Provisioning Service (%s)", s_device_name);

    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble port: %s", esp_err_to_name(ret));
        return ret;
    }

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(s_device_name);

    ble_gatts_count_cfg(gatt_svr_svcs);
    ble_gatts_add_svcs(gatt_svr_svcs);

    ble_hs_cfg.sync_cb = ble_provisioning_on_sync;

    nimble_port_freertos_init(ble_provisioning_host_task);
    s_ble_active = true;

    return ESP_OK;
}

void ble_provisioning_notify_status(bool connected)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_tx_char_val_handle == 0) {
        return;
    }

    char resp_buf[256];
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    wifi_ap_record_t ap_info;
    int8_t rssi = -100;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }
    pcu_vendor_snapshot_t vsnap;
    memset(&vsnap, 0, sizeof(vsnap));
    get_pcu_vendor_snapshot(&vsnap);
    snprintf(resp_buf, sizeof(resp_buf), "%02X%02X%02X%02X%02X%02X,%d,%s,%s,%d,%s,%s,%s,%s,%s,%s",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             connected ? 1 : 0,
             wifi_manager_get_ssid(),
             wifi_manager_get_ip_str(),
             rssi,
             vsnap.brand_title,
             vsnap.model_name,
             vsnap.serial_number,
             vsnap.hardware_version,
             vsnap.vendor_contact,
             vsnap.vendor_website);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(resp_buf, strlen(resp_buf));
    if (om) {
        ble_gatts_notify_custom(s_conn_handle, s_tx_char_val_handle, om);
        ESP_LOGI(TAG, "Sent BLE Status Notification: %s", resp_buf);
    }
}

void ble_provisioning_stop(void)
{
    if (!s_ble_active) return;
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ble_gap_adv_stop();
    nimble_port_stop();
    nimble_port_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    s_ble_active = false;
    ESP_LOGI(TAG, "BLE stopped and memory reclaimed for AWS IoT TLS");
}

