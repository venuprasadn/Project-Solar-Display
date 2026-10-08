#!/usr/bin/env python3
"""
SunGridNova AWS IoT Thing Auto-Registration & Provisioning Tool
Registers individual hardware units under their respective Vendor Thing Groups based on eFuse MAC.
"""

import sys
import json
import subprocess
import argparse

def get_device_mac(port="COM8"):
    """Read eFuse MAC from ESP32 via esptool."""
    python_exe = r"C:\Espressif\python_env\idf6.1_py3.10_env\Scripts\python.exe"
    try:
        cmd = [python_exe, "-m", "esptool", "--port", port, "chip_id"]
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        for line in res.stdout.splitlines():
            if "MAC:" in line:
                mac_raw = line.split("MAC:")[1].strip()
                mac_clean = mac_raw.replace(":", "").upper()
                return mac_raw.upper(), mac_clean
    except Exception as e:
        print(f"Warning: Could not read directly from {port}: {e}")
    
    # Fallback to default connected board MAC
    return "14:C1:9F:C7:FB:04", "14C19FC7FB04"

def generate_aws_cli_commands(mac_raw, mac_clean, vendor_name="HYBRID PSU", region="ap-southeast-2", cert_arn=None):
    thing_name = f"SunGridNova-{mac_clean}"
    vendor_group = f"Vendor_{vendor_name.replace(' ', '_')}"
    policy_name = "SunGridNova_Inverter_Policy"
    
    commands = [
        f"# 1. Create Vendor Thing Group: {vendor_group}",
        f"aws iot create-thing-group --thing-group-name \"{vendor_group}\" --region {region}",
        "",
        f"# 2. Create AWS IoT Thing based on eFuse MAC: {thing_name}",
        f"aws iot create-thing --thing-name \"{thing_name}\" --attribute-payload \"attributes={{MAC={mac_raw},Vendor={vendor_name}}}\" --region {region}",
        "",
        f"# 3. Add Thing to Vendor Group",
        f"aws iot add-thing-to-thing-group --thing-group-name \"{vendor_group}\" --thing-name \"{thing_name}\" --region {region}",
        "",
        f"# 4. Attach Security Certificate to Thing",
    ]
    
    if cert_arn:
        commands.append(f"aws iot attach-thing-principal --thing-name \"{thing_name}\" --principal \"{cert_arn}\" --region {region}")
    else:
        commands.append(f"# (Replace <CERT_ARN> with your active Certificate ARN from AWS IoT Console)")
        commands.append(f"aws iot attach-thing-principal --thing-name \"{thing_name}\" --principal \"<CERT_ARN>\" --region {region}")
        
    commands.extend([
        "",
        f"# 5. Create & Attach Inverter Scoped Policy (Publish/Subscribe Telemetry & Alerts)",
        f"aws iot attach-policy --policy-name \"{policy_name}\" --target \"<CERT_ARN>\" --region {region}"
    ])
    
    return thing_name, vendor_group, "\n".join(commands)

def main():
    parser = argparse.ArgumentParser(description="SunGridNova AWS IoT Thing Registration Utility")
    parser.add_argument("--port", default="COM8", help="Serial COM port for ESP32")
    parser.add_argument("--vendor", default="HYBRID PSU", help="Vendor / Brand Title")
    parser.add_argument("--region", default="ap-southeast-2", help="AWS Region")
    parser.add_argument("--cert-arn", default="", help="AWS Certificate ARN")
    args = parser.parse_args()

    mac_raw, mac_clean = get_device_mac(args.port)
    thing_name, vendor_group, script = generate_aws_cli_commands(mac_raw, mac_clean, args.vendor, args.region, args.cert_arn)

    print("=" * 70)
    print("  SUNGRIDNOVA AWS IOT CORE THING PROVISIONING SPECIFICATION")
    print("=" * 70)
    print(f"  eFuse MAC Address   : {mac_raw}")
    print(f"  AWS Thing Name      : {thing_name}")
    print(f"  Vendor Thing Group  : {vendor_group}")
    print(f"  AWS Region          : {args.region}")
    print("=" * 70)
    print("\nGenerated Provisioning Script (Run in AWS CLI or CloudShell):\n")
    print(script)
    print("=" * 70)

if __name__ == "__main__":
    main()
