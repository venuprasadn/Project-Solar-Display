#!/usr/bin/env python3
"""
Package genuine Amazon-signed device certificate and private key from AWSMobSim
into main/aws_iot_certs.h
"""

import os

def format_c_string(pem_str):
    lines = pem_str.strip().split("\n")
    return "\n".join(f'"{line}\\n"' for line in lines) + ";"

def main():
    root_ca_path = r"D:\Project_2025\AWSMobSim\x64\Release\AmazonRootCA1.pem"
    device_crt_path = r"D:\Project_2025\AWSMobSim\x64\Release\device.crt"
    device_key_path = r"D:\Project_2025\AWSMobSim\x64\Release\device.key"
    
    with open(root_ca_path, "r", encoding="utf-8") as f:
        root_ca_pem = f.read()
    with open(device_crt_path, "r", encoding="utf-8") as f:
        device_crt_pem = f.read()
    with open(device_key_path, "r", encoding="utf-8") as f:
        device_key_pem = f.read()
        
    header_content = f"""/*
 * Genuine Amazon-Signed AWS IoT Core mTLS Certificates & Private Key
 * Issuer: Amazon Web Services (Amazon.com Inc.)
 * Endpoint: a15qebuvm1g118-ats.iot.ap-southeast-2.amazonaws.com
 * Serial: 0x4cafa97f2f629b0c9d209741cb9e1cfb56f10b24
 */

#pragma once

#ifndef AWS_IOT_CERTS_H
#define AWS_IOT_CERTS_H

/* Amazon Root CA 1 Certificate */
static const char *AWS_ROOT_CA_CERT =
{format_c_string(root_ca_pem)}

/* Amazon Web Services Signed Device Certificate */
static const char *AWS_CLIENT_CERT =
{format_c_string(device_crt_pem)}

/* Device RSA Private Key */
static const char *AWS_CLIENT_KEY =
{format_c_string(device_key_pem)}

#endif /* AWS_IOT_CERTS_H */
"""

    targets = [
        r"D:\Project_2026\Project-ESP32-LCD\main\aws_iot_certs.h",
        r"D:\Project_Solar_Display\Project-ESP32-LCD\main\aws_iot_certs.h",
    ]
    
    for t in targets:
        with open(t, "w", encoding="utf-8") as f:
            f.write(header_content)
        print(f"Updated Amazon-signed credentials in: {t}")

if __name__ == "__main__":
    main()
