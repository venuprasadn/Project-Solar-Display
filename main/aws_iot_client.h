#ifndef AWS_IOT_CLIENT_H
#define AWS_IOT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float mainsvolt;
    float solarvolt;
    float battvolts;
    float acout;
    float loaddisp;
    float chrampsdisp;
    float dischdisp;
    float dcboost;
    float upsheat;
    int   error_code;
} inverter_telemetry_snapshot_t;

void get_inverter_data_snapshot(inverter_telemetry_snapshot_t *snap);

/**
 * @brief Initialize the AWS IoT Core TLS 1.2 client and telemetry publishing task.
 *        Runs securely on Core 0, Watchdog protected, non-blocking to LVGL UI.
 */
esp_err_t aws_iot_client_init(void);

/**
 * @brief Returns true if MQTT client is currently connected to AWS IoT Core.
 */
bool aws_iot_client_is_connected(void);

/**
 * @brief Get the unique hardware Thing ID (e.g. "SunGridNova-5B1C").
 */
const char* aws_iot_get_thing_id(void);

#ifdef __cplusplus
}
#endif

#endif // AWS_IOT_CLIENT_H
