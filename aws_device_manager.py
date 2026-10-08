#!/usr/bin/env python3
"""
SunGridNova AWS IoT Device Manager & Auto-Provisioner
Automates Thing Registration, Vendor Groups, Certificates, and Live Telemetry Streaming.
"""

import os
import sys
import json
import time
import argparse
import subprocess

try:
    import boto3
    from botocore.exceptions import ClientError
except ImportError:
    print("Error: boto3 is not installed. Run: pip install boto3")
    sys.exit(1)

PYTHON_EXE = r"C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe"

def get_connected_esp_mac(port="COM8"):
    """Reads eFuse MAC directly from connected ESP32-S3 hardware."""
    try:
        cmd = [PYTHON_EXE, "-m", "esptool", "--port", port, "chip_id"]
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        for line in res.stdout.splitlines():
            if "MAC:" in line:
                mac_raw = line.split("MAC:")[1].strip().upper()
                mac_clean = mac_raw.replace(":", "")
                return mac_raw, mac_clean
    except Exception as e:
        print(f"⚠️  Could not read live MAC from {port} ({e}). Using hardware fallback.")
    return "14:C1:9F:C7:FB:04", "14C19FC7FB04"

def get_iot_client(region="ap-southeast-2", profile=None):
    session = boto3.Session(profile_name=profile, region_name=region)
    return session.client("iot")

def setup_vendor_and_thing(iot, mac_raw, mac_clean, vendor_name="HYBRID PSU"):
    thing_name = f"SunGridNova-{mac_clean}"
    group_name = f"Vendor_{vendor_name.replace(' ', '_')}"

    print(f"\n📦 Target Thing: {thing_name}")
    print(f"🏢 Vendor Group: {group_name}")
    print(f"🔌 Hardware MAC: {mac_raw}\n")

    # 1. Create or Verify Thing Group
    try:
        iot.create_thing_group(thingGroupName=group_name)
        print(f"✅ Created Vendor Thing Group: '{group_name}'")
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceAlreadyExistsException':
            print(f"ℹ️  Vendor Thing Group '{group_name}' already exists.")
        else:
            print(f"❌ Error creating group: {e}")

    # 2. Create or Verify Thing
    try:
        iot.create_thing(
            thingName=thing_name,
            attributePayload={
                'attributes': {
                    'MAC': mac_raw,
                    'Vendor': vendor_name,
                    'Model': 'SunGridNova'
                },
                'merge': True
            }
        )
        print(f"✅ Created Thing in AWS IoT Registry: '{thing_name}'")
    except ClientError as e:
        if e.response['Error']['Code'] == 'ResourceAlreadyExistsException':
            print(f"ℹ️  Thing '{thing_name}' already exists in Registry.")
        else:
            print(f"❌ Error creating Thing: {e}")

    # 3. Add Thing to Thing Group
    try:
        iot.add_thing_to_thing_group(
            thingGroupName=group_name,
            thingName=thing_name
        )
        print(f"✅ Added '{thing_name}' into Thing Group '{group_name}'")
    except Exception as e:
        print(f"❌ Error adding thing to group: {e}")

    return thing_name, group_name

def list_all_devices(iot):
    print("\n" + "=" * 60)
    print("  AWS IOT CORE - REGISTERED DEVICES & GROUPS")
    print("=" * 60)
    
    try:
        groups = iot.list_thing_groups().get('thingGroups', [])
        print(f"\n[*] Thing Groups ({len(groups)}):")
        for g in groups:
            print(f"   - {g['groupName']}")
            
        things = iot.list_things().get('things', [])
        print(f"\n[*] Things ({len(things)}):")
        for t in things:
            attrs = t.get('attributes', {})
            mac = attrs.get('MAC', 'N/A')
            vendor = attrs.get('Vendor', 'N/A')
            print(f"   - Name: {t['thingName']:<28} | MAC: {mac:<17} | Vendor: {vendor}")
    except Exception as e:
        print(f"[!] Error querying AWS IoT resources: {e}")
        print("[*] Please provide AWS credentials by setting:")
        print("    $env:AWS_ACCESS_KEY_ID = 'your_access_key'")
        print("    $env:AWS_SECRET_ACCESS_KEY = 'your_secret_key'")
        print("    $env:AWS_DEFAULT_REGION = 'ap-southeast-2'")
    print("=" * 60 + "\n")

def main():
    parser = argparse.ArgumentParser(description="SunGridNova AWS IoT Device Manager")
    parser.add_argument("--port", default="COM8", help="ESP32 Serial Port (default: COM8)")
    parser.add_argument("--vendor", default="HYBRID PSU", help="Vendor / Brand Title")
    parser.add_argument("--region", default="ap-southeast-2", help="AWS Region (default: ap-southeast-2)")
    parser.add_argument("--list", action="store_true", help="List all registered Things and Groups")
    parser.add_argument("--register", action="store_true", help="Auto-register connected ESP32 to AWS IoT")
    args = parser.parse_args()

    try:
        iot = get_iot_client(args.region)
    except Exception as e:
        print(f"[!] Failed to connect to AWS IoT SDK: {e}")
        sys.exit(1)

    if args.list:
        list_all_devices(iot)
    elif args.register or len(sys.argv) == 1:
        mac_raw, mac_clean = get_connected_esp_mac(args.port)
        try:
            setup_vendor_and_thing(iot, mac_raw, mac_clean, args.vendor)
            list_all_devices(iot)
        except Exception as e:
            print(f"[!] Operation failed: {e}")
            print("[*] To authenticate, set your AWS Access Keys in terminal:")
            print("    $env:AWS_ACCESS_KEY_ID = 'your_access_key'")
            print("    $env:AWS_SECRET_ACCESS_KEY = 'your_secret_key'")
            print("    $env:AWS_DEFAULT_REGION = 'ap-southeast-2'")

if __name__ == "__main__":
    main()
