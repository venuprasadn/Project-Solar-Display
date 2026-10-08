#ifndef BLE_PROVISIONING_H
#define BLE_PROVISIONING_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Industrial BLE Provisioning GATT Server (NimBLE stack)
 *        Advertises as "SunGridNova-XXXX" with service UUID 12345678-1234-5678-1234-56789abcdef0.
 *        Accepts "<SSID>,<PASSWORD>" on RX char, notifies "<MAC>,<0|1>" on TX char.
 */
esp_err_t ble_provisioning_init(void);

/**
 * @brief Notify connected BLE client of current Wi-Fi status update
 */
void ble_provisioning_notify_status(bool connected);

/**
 * @brief Start or resume BLE advertising
 */
void ble_provisioning_start_advertising(void);

/**
 * @brief Pause BLE advertising
 */
void ble_provisioning_stop(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_PROVISIONING_H
