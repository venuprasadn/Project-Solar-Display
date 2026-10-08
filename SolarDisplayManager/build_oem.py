#!/usr/bin/env python3
"""
Industrial OEM Staging System - Monolithic Package Builder
Automates:
1. Vendor descriptor dynamic byte protection
2. Hardware Operational Mode configuration (COMBO / TFT_ONLY / CLOUD_ONLY)
3. Master Logo PNG embedding
4. MSBuild compilation (Release | x64)
5. Distribution package generation
"""

import os
import sys
import json
import shutil
import argparse
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "SolarDisplayManager")
SLN_PATH = os.path.join(SCRIPT_DIR, "SolarDisplayManager.sln")
CONFIG_PATH = os.path.join(SCRIPT_DIR, "vendor_config.json")
DIST_DIR = os.path.join(SCRIPT_DIR, "SolarDisplayManager_Distribution")

VAULT_KEY = 0x5A9E3C71

def find_msbuild():
    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\amd64\MSBuild.exe",
    ]
    for p in candidates:
        if os.path.isfile(p):
            return p
    return "MSBuild.exe"

def encrypt_str(s, k):
    b = s.encode('utf-8')
    res = []
    for i, byte_val in enumerate(b):
        shift = (i % 4) * 8
        k_byte = (k >> shift) & 0xFF
        res.append(byte_val ^ k_byte ^ ((i * 37 + 13) & 0xFF))
    return res

def generate_payload_files(cfg):
    mode_str = cfg.get("hardware_mode", "COMBO").upper()
    if mode_str == "TFT_ONLY":
        mode_val = "OEM_HW_MODE_TFT_ONLY"
    elif mode_str == "CLOUD_ONLY":
        mode_val = "OEM_HW_MODE_CLOUD_ONLY"
    else:
        mode_val = "OEM_HW_MODE_COMBO"

    # 1. Update OEMPayload.h
    h_path = os.path.join(PROJECT_DIR, "OEMPayload.h")
    h_content = f"""#pragma once

#include <windows.h>
#include <afxstr.h>
#include <vector>
#include <gdiplus.h>

// Target Hardware Operational Modes
#define OEM_HW_MODE_COMBO       0   // Both TFT Display and Mobile App / Cloud enabled
#define OEM_HW_MODE_TFT_ONLY    1   // TFT Display dedicated unit (Cloud disabled)
#define OEM_HW_MODE_CLOUD_ONLY  2   // Mobile App / Cloud dedicated unit (No TFT LCD)

// Active Pre-Compilation Configuration Switch
#ifndef ACTIVE_OEM_HW_MODE
#define ACTIVE_OEM_HW_MODE {mode_val}
#endif

class OEMPayload
{{
public:
    // Core Protected Vendor Descriptors
    static CString GetBrandTitle();
    static CString GetModelName();
    static CString GetSerialPrefix();
    static CString GetHardwareVersion();
    static CString GetVendorContact();
    static CString GetVendorWebsite();
    static CString GetCompanyName();
    static CString GetModeDescription();

    // Default Operational Parameters
    static uint8_t GetDefaultTheme();
    static uint8_t GetDefaultBootSec();
    static uint8_t GetDefaultCarouselSec();
    static uint8_t GetTargetHardwareMode();

    // Master High-Resolution Logo Resources
    static const uint8_t* GetMasterLogoPng(size_t& outLen);
    static Gdiplus::Bitmap* GetMasterLogoBitmap();

    // Protected Payload Retrieval Utility
    static std::vector<uint8_t> DecryptPayload(const uint8_t* cipherData, size_t length, uint32_t key);
}};
"""
    with open(h_path, "w", encoding="utf-8") as f:
        f.write(h_content)

    # 2. Read PNG Logo
    logo_rel = cfg.get("logo_png_path", "SolarDisplayManager/res/logo_master.png")
    logo_abs = os.path.join(SCRIPT_DIR, logo_rel)
    if not os.path.isfile(logo_abs):
        logo_abs = os.path.join(PROJECT_DIR, "res", "logo_master.png")

    with open(logo_abs, "rb") as f:
        png_bytes = f.read()

    # 3. Generate OEMPayload.cpp
    strings = {
        'BRAND': cfg.get('brand_title', 'SOLAR PCU'),
        'MODEL': cfg.get('model_name', 'HYBRID MPPT'),
        'SERIAL': cfg.get('serial_prefix', 'SN-2026-0001'),
        'HWREV': cfg.get('hardware_version', 'HW-V1.0'),
        'CONTACT': cfg.get('vendor_contact', 'Support: 1800-000-0000'),
        'WEBSITE': cfg.get('vendor_website', 'www.solarpcu.com'),
        'COMPANY': cfg.get('company_name', 'Solar Power Systems Ltd')
    }

    lines = [
        '#include "pch.h"',
        '#include "OEMPayload.h"',
        '#include <string>',
        '#include <ole2.h>',
        '',
        '// Dynamic protection constants',
        f'static const uint32_t s_vaultKey = 0x{VAULT_KEY:08X};',
        ''
    ]

    for name, val in strings.items():
        enc = encrypt_str(val, VAULT_KEY)
        hex_str = ', '.join(f'0x{b:02X}' for b in enc)
        lines.append(f'static const uint8_t s_enc_{name}[] = {{ {hex_str} }};')
        lines.append(f'static const size_t s_len_{name} = {len(enc)};')
        lines.append('')

    lines.extend([
        'static CString DecryptString(const uint8_t* encData, size_t len, uint32_t key)',
        '{',
        '    std::string s;',
        '    s.reserve(len);',
        '    for (size_t i = 0; i < len; i++) {',
        '        uint8_t shift = (uint8_t)((i % 4) * 8);',
        '        uint8_t k_byte = (uint8_t)((key >> shift) & 0xFF);',
        '        uint8_t orig = (uint8_t)(encData[i] ^ k_byte ^ ((i * 37 + 13) & 0xFF));',
        '        s.push_back((char)orig);',
        '    }',
        '    return CString(s.c_str());',
        '}',
        '',
        'CString OEMPayload::GetBrandTitle()       { return DecryptString(s_enc_BRAND, s_len_BRAND, s_vaultKey); }',
        'CString OEMPayload::GetModelName()        { return DecryptString(s_enc_MODEL, s_len_MODEL, s_vaultKey); }',
        'CString OEMPayload::GetSerialPrefix()     { return DecryptString(s_enc_SERIAL, s_len_SERIAL, s_vaultKey); }',
        'CString OEMPayload::GetHardwareVersion()  { return DecryptString(s_enc_HWREV, s_len_HWREV, s_vaultKey); }',
        'CString OEMPayload::GetVendorContact()    { return DecryptString(s_enc_CONTACT, s_len_CONTACT, s_vaultKey); }',
        'CString OEMPayload::GetVendorWebsite()    { return DecryptString(s_enc_WEBSITE, s_len_WEBSITE, s_vaultKey); }',
        'CString OEMPayload::GetCompanyName()      { return DecryptString(s_enc_COMPANY, s_len_COMPANY, s_vaultKey); }',
        '',
        'CString OEMPayload::GetModeDescription()',
        '{',
        '#if ACTIVE_OEM_HW_MODE == OEM_HW_MODE_COMBO',
        '    return _T("TFT LCD Display + Mobile App / Cloud [COMBO]");',
        '#elif ACTIVE_OEM_HW_MODE == OEM_HW_MODE_TFT_ONLY',
        '    return _T("TFT LCD Display [DEDICATED]");',
        '#elif ACTIVE_OEM_HW_MODE == OEM_HW_MODE_CLOUD_ONLY',
        '    return _T("Mobile App & Cloud Platform [DEDICATED]");',
        '#endif',
        '}',
        '',
        f'uint8_t OEMPayload::GetDefaultTheme()         {{ return {cfg.get("default_theme", 0)}; }}',
        f'uint8_t OEMPayload::GetDefaultBootSec()       {{ return {cfg.get("default_boot_sec", 3)}; }}',
        f'uint8_t OEMPayload::GetDefaultCarouselSec()   {{ return {cfg.get("default_carousel_sec", 5)}; }}',
        'uint8_t OEMPayload::GetTargetHardwareMode()   { return ACTIVE_OEM_HW_MODE; }',
        ''
    ])

    # PNG chunks
    png_lines = []
    for i in range(0, len(png_bytes), 16):
        chunk = png_bytes[i:i+16]
        png_lines.append('    ' + ', '.join(f'0x{b:02X}' for b in chunk) + ',')

    lines.extend([
        'static const uint8_t s_masterLogoPngData[] = {',
        *png_lines,
        '};',
        'static const size_t s_masterLogoPngSize = sizeof(s_masterLogoPngData);',
        '',
        'const uint8_t* OEMPayload::GetMasterLogoPng(size_t& outLen)',
        '{',
        '    outLen = s_masterLogoPngSize;',
        '    return s_masterLogoPngData;',
        '}',
        '',
        'Gdiplus::Bitmap* OEMPayload::GetMasterLogoBitmap()',
        '{',
        '    static Gdiplus::Bitmap* s_pCachedBitmap = nullptr;',
        '    if (s_pCachedBitmap != nullptr) return s_pCachedBitmap;',
        '',
        '    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, s_masterLogoPngSize);',
        '    if (!hMem) return nullptr;',
        '    void* pMem = GlobalLock(hMem);',
        '    if (pMem) {',
        '        memcpy(pMem, s_masterLogoPngData, s_masterLogoPngSize);',
        '        GlobalUnlock(hMem);',
        '    }',
        '    IStream* pStream = nullptr;',
        '    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK && pStream != nullptr) {',
        '        s_pCachedBitmap = Gdiplus::Bitmap::FromStream(pStream);',
        '        pStream->Release();',
        '    }',
        '    return s_pCachedBitmap;',
        '}',
        '',
        'std::vector<uint8_t> OEMPayload::DecryptPayload(const uint8_t* cipherData, size_t length, uint32_t key)',
        '{',
        '    std::vector<uint8_t> plain(length);',
        '    for (size_t i = 0; i < length; i++) {',
        '        uint8_t shift = (uint8_t)((i % 4) * 8);',
        '        uint8_t k_byte = (uint8_t)((key >> shift) & 0xFF);',
        '        plain[i] = (uint8_t)(cipherData[i] ^ k_byte ^ ((i * 37 + 13) & 0xFF));',
        '    }',
        '    return plain;',
        '}'
    ])

    cpp_path = os.path.join(PROJECT_DIR, "OEMPayload.cpp")
    with open(cpp_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print(f"[OK] Generated {cpp_path}")
    print(f"[OK] Mode set to: {mode_val}")

def build_solution():
    msbuild = find_msbuild()
    print(f"[BUILD] Using MSBuild: {msbuild}")
    cmd = [
        msbuild,
        SLN_PATH,
        "/p:Configuration=Release",
        "/p:Platform=x64",
        "/t:Rebuild"
    ]
    ret = subprocess.run(cmd, check=False)
    if ret.returncode != 0:
        print("[ERROR] Build failed with exit code:", ret.returncode)
        sys.exit(ret.returncode)

    # Close running instances if any
    subprocess.run(["taskkill", "/F", "/IM", "SolarDisplayManager.exe"], capture_output=True, check=False)

    # Copy output
    out_exe = os.path.join(SCRIPT_DIR, "x64", "Release", "SolarDisplayManager.exe")
    dist_exe = os.path.join(DIST_DIR, "SolarDisplayManager.exe")
    os.makedirs(DIST_DIR, exist_ok=True)
    shutil.copy2(out_exe, dist_exe)
    print(f"[SUCCESS] Standalone executable created at: {dist_exe}")
    print(f"[SIZE] Executable size: {os.path.getsize(dist_exe):,} bytes")

def main():
    parser = argparse.ArgumentParser(description="Solar Display OEM Package & Staging Builder")
    parser.add_argument("--config", default=CONFIG_PATH, help="Path to vendor_config.json")
    parser.add_argument("--brand", help="Override Brand Title")
    parser.add_argument("--model", help="Override Model Name")
    parser.add_argument("--mode", choices=["COMBO", "TFT_ONLY", "CLOUD_ONLY"], help="Operational mode")
    parser.add_argument("--no-build", action="store_true", help="Only generate payload files without compiling")

    args = parser.parse_args()

    cfg = {}
    if os.path.isfile(args.config):
        with open(args.config, "r", encoding="utf-8") as f:
            cfg = json.load(f)
    else:
        print(f"[WARN] Config file {args.config} not found. Using defaults.")

    if args.brand: cfg["brand_title"] = args.brand
    if args.model: cfg["model_name"] = args.model
    if args.mode: cfg["hardware_mode"] = args.mode

    print("======================================================")
    print("       Solar PCU OEM Staging Package Builder          ")
    print("======================================================")
    print(f" Brand:     {cfg.get('brand_title')}")
    print(f" Model:     {cfg.get('model_name')}")
    print(f" Mode:      {cfg.get('hardware_mode')}")
    print(f" Company:   {cfg.get('company_name')}")
    print(f" Contact:   {cfg.get('vendor_contact')}")
    print(f" Web:       {cfg.get('vendor_website')}")
    print("------------------------------------------------------")

    generate_payload_files(cfg)

    if not args.no_build:
        build_solution()

if __name__ == "__main__":
    main()
