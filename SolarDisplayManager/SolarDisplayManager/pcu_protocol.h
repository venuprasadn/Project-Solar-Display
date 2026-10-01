#ifndef PCU_PROTOCOL_H
#define PCU_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#pragma pack(push, 1)

#define HRF_SYNC_BYTE1          0x53    // 'S'
#define HRF_SYNC_BYTE2          0x44    // 'D'
#define HRF_FRAME_TRAILER       0x0D    // '\r'

#define HRF_MAX_PAYLOAD_SIZE    512

/* Commands */
#define CMD_PING                0x01
#define CMD_READ_CONFIG         0x02
#define CMD_WRITE_CONFIG        0x03
#define CMD_COMMIT_NVS          0x04
#define CMD_FACTORY_RESET       0x05
#define CMD_READ_TELEMETRY      0x06
#define CMD_REBOOT              0x07
#define CMD_WRITE_LOGO_CHUNK    0x08
#define CMD_COMMIT_LOGO         0x09
#define CMD_CLEAR_LOGO          0x0A

#define PCU_LOGO_MAGIC          0x474F4C50  // 'PLOG'
#define PCU_LOGO_FLASH_ADDR     0x110000

typedef struct {
    uint32_t magic;         // PCU_LOGO_MAGIC
    uint16_t width;         // Image width (e.g. 48)
    uint16_t height;        // Image height (e.g. 48)
    uint16_t cf;            // 1 = RGB565
    uint16_t reserved;      // 0
    uint32_t data_size;     // width * height * 2
    uint32_t crc32;         // CRC32 of pixel data
} pcu_logo_header_t;

typedef struct {
    uint16_t offset;        // Byte offset within logo buffer
    uint16_t length;        // Number of bytes in this chunk
    uint8_t  data[];
} hrf_logo_chunk_t;

/* Responses */
#define RESP_ACK                0x80
#define RESP_NACK               0x81
#define RESP_CONFIG_DATA        0x82
#define RESP_TELEMETRY_DATA     0x83

/* Status / Error Codes */
#define STATUS_OK               0x00
#define ERR_CRC_MISMATCH        0x01
#define ERR_INVALID_CMD         0x02
#define ERR_PAYLOAD_SIZE        0x03
#define ERR_NVS_WRITE           0x04
#define ERR_NOT_CONFIGURED      0x05
#define ERR_INVALID_MAGIC       0x06

/* Device Operating State */
#define DEVICE_STATE_UNCONFIGURED   0x00
#define DEVICE_STATE_CONFIGURED     0x01
#define DEVICE_STATE_SERVICE_MODE   0x02

/* Packet Header (8 bytes) */
typedef struct {
    uint8_t  sync1;     // 'S'
    uint8_t  sync2;     // 'D'
    uint8_t  seq;       // Packet sequence counter
    uint8_t  cmd;       // Command / Response code
    uint16_t length;    // Payload length in bytes
    uint16_t reserved;  // 0x0000
} hrf_header_t;

/* Ping Response Payload */
typedef struct {
    uint8_t  device_state;       // DEVICE_STATE_UNCONFIGURED / CONFIGURED
    uint8_t  hw_model_id;        // 0x01 = ESP32-DevKit-2.8TFT
    uint16_t fw_version;         // 0x0100 (v1.0)
    uint32_t uptime_sec;
    char     chip_model[16];     // "ESP32-D0WDQ6"
} hrf_ping_resp_t;

/* ACK / NACK Payload */
typedef struct {
    uint8_t  ref_cmd;            // Command that this is acknowledging
    uint8_t  status_code;        // STATUS_OK or ERR_*
    uint16_t extra_info;
} hrf_ack_payload_t;

/* The Complete Production Configuration Schema */
#define PCU_CONFIG_MAGIC        0x50435532  // "PCU2"
#define PCU_CONFIG_VERSION      0x0200      // v2.0

typedef struct {
    /* Header & Verification */
    uint32_t magic;                 // PCU_CONFIG_MAGIC
    uint16_t version;               // PCU_CONFIG_VERSION
    uint16_t struct_size;           // sizeof(pcu_config_t)
    uint8_t  is_configured;         // 1 = Configured & ready to run display, 0 = Unconfigured

    /* Manufacturing & Brand Identity */
    char     brand_title[32];       // e.g. "DONPOWER SOLAR"
    char     model_name[32];        // e.g. "HYBRID MPPT PCU"
    char     serial_number[32];     // e.g. "DP-2026-X8849"
    char     hardware_version[16];  // e.g. "HW-V2.1"
    char     vendor_contact[32];    // e.g. "Toll Free: 1800-425-9999"
    char     vendor_website[32];    // e.g. "www.donpower.in"
    uint32_t production_date;       // YYYYMMDD (e.g. 20261001)

    /* Visual Theme & Boot Options */
    uint8_t  logo_theme;            // 0=Amber, 1=Cyan, 2=Emerald, 3=Crimson, 4=Cobalt, 5=Stealth
    uint8_t  boot_duration_sec;     // 2 to 6 seconds (default 3s)
    uint8_t  auto_carousel_enabled; // 1 = Yes, 0 = No
    uint8_t  carousel_interval_sec; // 3 to 30 seconds (default 5s)
    uint8_t  backlight_brightness;  // 10 to 100%

    /* Field Telemetry Port (UART2) Configuration */
    uint32_t telemetry_baudrate;    // 9600, 19200, 38400, 115200 (default 9600)
    uint8_t  reserved_align[3];

    /* CRC Checksum over all prior bytes in struct */
    uint32_t config_crc32;
} pcu_config_t;

#pragma pack(pop)

/* Standard CRC32 (IEEE 802.3) Calculation */
static inline uint32_t pcu_calc_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

#endif // PCU_PROTOCOL_H
