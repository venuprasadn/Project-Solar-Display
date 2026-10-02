#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1

/**
 * @brief Initialize the Wi-Fi Station Manager with industrial auto-reconnect backoff.
 *        Non-blocking, isolated to Core 0, failsafe if no AP is found.
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Check if Wi-Fi currently has an active IP connection.
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Get the local IP address as a string (e.g. "192.168.1.100").
 */
const char* wifi_manager_get_ip_str(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
