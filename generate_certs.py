#!/usr/bin/env python3
"""
Dynamic Industrial TLS Certificate and Key Generator for SunGridNova
Generates SECP256R1 ECDSA Keys and X.509 Client Certificates.
"""

import os
import datetime
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import serialization

def generate_credentials(thing_id="SunGridNova-FB04"):
    print(f"Generating dynamic ECDSA SECP256R1 keypair for {thing_id}...")
    
    # 1. Generate EC Private Key
    private_key = ec.generate_private_key(ec.SECP256R1())
    
    # 2. Build X.509 Subject and Issuer
    subject = issuer = x509.Name([
        x509.NameAttribute(NameOID.COUNTRY_NAME, "IN"),
        x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Karnataka"),
        x509.NameAttribute(NameOID.LOCALITY_NAME, "Bengaluru"),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME, "SunGridNova Energy Systems"),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, "Industrial Inverter Security Core"),
        x509.NameAttribute(NameOID.COMMON_NAME, thing_id),
    ])
    
    # 3. Build X.509 Certificate (Valid 2026 to 2040)
    not_valid_before = datetime.datetime(2026, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)
    not_valid_after = datetime.datetime(2040, 1, 1, 0, 0, 0, tzinfo=datetime.timezone.utc)
    
    cert = (
        x509.CertificateBuilder()
        .subject_name(subject)
        .issuer_name(issuer)
        .public_key(private_key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(not_valid_before)
        .not_valid_after(not_valid_after)
        .add_extension(
            x509.BasicConstraints(ca=True, path_length=None),
            critical=True,
        )
        .sign(private_key, hashes.SHA256())
    )
    
    # 4. Serialize to PEM format
    cert_pem = cert.public_bytes(serialization.Encoding.PEM).decode("utf-8")
    key_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.TraditionalOpenSSL,
        encryption_algorithm=serialization.NoEncryption(),
    ).decode("utf-8")
    
    # 5. Amazon Root CA 1
    root_ca_pem = (
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
        "-----END CERTIFICATE-----\n"
    )
    
    # 6. Generate C Header
    header_content = f"""/*
 * Auto-Generated Industrial AWS IoT Core mTLS Certificates & Key
 * Target Device: {thing_id}
 * Generation Timestamp: {datetime.datetime.now(datetime.timezone.utc).isoformat()}
 * Security Standard: ECDSA SECP256R1 / SHA256
 */

#pragma once

#ifndef AWS_IOT_CERTS_H
#define AWS_IOT_CERTS_H

/* Amazon Root CA 1 Certificate */
static const char *AWS_ROOT_CA_CERT =
{format_c_string(root_ca_pem)}

/* Dynamic Client Device X.509 Certificate */
static const char *AWS_CLIENT_CERT =
{format_c_string(cert_pem)}

/* Dynamic Client Device SECP256R1 EC Private Key */
static const char *AWS_CLIENT_KEY =
{format_c_string(key_pem)}

#endif /* AWS_IOT_CERTS_H */
"""
    return header_content

def format_c_string(pem_str):
    lines = pem_str.strip().split("\n")
    return "\n".join(f'"{line}\\n"' for line in lines) + ";"

if __name__ == "__main__":
    c_header = generate_credentials("SunGridNova-FB04")
    
    out_paths = [
        r"D:\Project_2026\Project-ESP32-LCD\main\aws_iot_certs.h",
        r"D:\Project_Solar_Display\Project-ESP32-LCD\main\aws_iot_certs.h",
    ]
    
    for p in out_paths:
        with open(p, "w", encoding="utf-8") as f:
            f.write(c_header)
        print(f"Generated certificate header: {p}")
