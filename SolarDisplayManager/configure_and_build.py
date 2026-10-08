#!/usr/bin/env python3
"""
=============================================================================
  Industrial OEM Solar PCU Staging Manager - Interactive Wizard & Builder
=============================================================================
Interactive Command-Line Tool to configure:
  - Vendor Branding & Legal Information
  - Operational Hardware Mode (COMBO / TFT ONLY / CLOUD ONLY)
  - Default UI Themes & Display Timers
  - Master OEM PNG Logo from local directory
  - Recompilation into Monolithic Standalone Release .EXE
"""

import os
import sys
import glob
import json
import shutil
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.join(SCRIPT_DIR, "SolarDisplayManager")
SLN_PATH = os.path.join(SCRIPT_DIR, "SolarDisplayManager.sln")
DIST_DIR = os.path.join(SCRIPT_DIR, "SolarDisplayManager_Distribution")
CONFIG_PATH = os.path.join(SCRIPT_DIR, "vendor_config.json")

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

def prompt_val(prompt_text, default_val=""):
    if default_val:
        val = input(f"{prompt_text} [{default_val}]: ").strip()
        return val if val else default_val
    else:
        val = input(f"{prompt_text}: ").strip()
        return val

def encrypt_str(s, k):
    b = s.encode('utf-8')
    res = []
    for i, byte_val in enumerate(b):
        shift = (i % 4) * 8
        k_byte = (k >> shift) & 0xFF
        res.append(byte_val ^ k_byte ^ ((i * 37 + 13) & 0xFF))
    return res

def scan_for_png_logos():
    pngs = glob.glob(os.path.join(SCRIPT_DIR, "*.png"))
    res_pngs = glob.glob(os.path.join(PROJECT_DIR, "res", "*.png"))
    all_pngs = list(dict.fromkeys(pngs + res_pngs))
    return all_pngs

def main():
    print("=====================================================================")
    print("       SOLAR PCU OEM STAGING MANAGER - CONFIGURATION WIZARD          ")
    print("=====================================================================")
    print("Enter custom vendor details below. Press [Enter] to accept defaults.\n")

    cfg = {}
    if os.path.isfile(CONFIG_PATH):
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                cfg = json.load(f)
        except Exception:
            cfg = {}

    def_brand = cfg.get("brand_title", "SOLAR PCU")
    def_model = cfg.get("model_name", "HYBRID MPPT")
    def_company = cfg.get("company_name", "Solar Power Systems Ltd")
    def_hw = cfg.get("hardware_version", "HW-V1.0")
    def_serial = cfg.get("serial_prefix", "SN-2026-0001")
    def_contact = cfg.get("vendor_contact", "Support: 1800-000-0000")
    def_web = cfg.get("vendor_website", "www.solarpcu.com")
    def_mode = cfg.get("hardware_mode", "COMBO")
    def_theme = str(cfg.get("default_theme", 0))
    def_boot = str(cfg.get("default_boot_sec", 3))
    def_cycle = str(cfg.get("default_carousel_sec", 5))

    brand = prompt_val("1. Brand Title", def_brand)
    model = prompt_val("2. Model Name", def_model)
    company = prompt_val("3. Company Legal Name", def_company)
    hw_rev = prompt_val("4. Hardware Version", def_hw)
    serial = prompt_val("5. Serial Number Prefix", def_serial)
    contact = prompt_val("6. Vendor Contact", def_contact)
    website = prompt_val("7. Vendor Website", def_web)

    print("\nOperational Hardware Mode:")
    print("   1. COMBO      -> Both TFT Display + Mobile App / Cloud Tabs")
    print("   2. TFT_ONLY   -> Dedicated TFT LCD Display (Cloud Tab Hidden)")
    print("   3. CLOUD_ONLY -> Dedicated Mobile App / Cloud (TFT Tab Hidden)")
    mode_choice = prompt_val("Select Mode (1/2/3 or COMBO/TFT_ONLY/CLOUD_ONLY)", "1")

    if mode_choice in ["1", "COMBO"]:
        hw_mode = "COMBO"
        mode_val = "OEM_HW_MODE_COMBO"
    elif mode_choice in ["2", "TFT_ONLY"]:
        hw_mode = "TFT_ONLY"
        mode_val = "OEM_HW_MODE_TFT_ONLY"
    elif mode_choice in ["3", "CLOUD_ONLY"]:
        hw_mode = "CLOUD_ONLY"
        mode_val = "OEM_HW_MODE_CLOUD_ONLY"
    else:
        hw_mode = "COMBO"
        mode_val = "OEM_HW_MODE_COMBO"

    print("\nDefault Production UI Theme:")
    print("   0: Tactical Amber (Solar Gold)")
    print("   1: Cyber Cyan (Cockpit HUD)")
    print("   2: Emerald Defense (Military)")
    print("   3: Crimson Alert (Warning Red)")
    theme_idx = int(prompt_val("Select Theme (0-3)", def_theme)) % 4

    boot_sec = int(prompt_val("Boot Splash Screen Time (seconds)", def_boot))
    cycle_sec = int(prompt_val("Auto Screen Cycle Interval (seconds)", def_cycle))

    print("\nMaster OEM Logo Selection (256x256 PNG):")
    available_pngs = scan_for_png_logos()
    logo_path = ""
    if available_pngs:
        print("Available PNG logos in directory:")
        for idx, p in enumerate(available_pngs, 1):
            rel_p = os.path.relpath(p, SCRIPT_DIR)
            print(f"   {idx}. {rel_p}")
        logo_sel = prompt_val(f"Select Logo (1-{len(available_pngs)} or type path)", "1")
        try:
            sel_i = int(logo_sel) - 1
            if 0 <= sel_i < len(available_pngs):
                logo_path = available_pngs[sel_i]
        except ValueError:
            logo_path = logo_sel
    else:
        logo_path = prompt_val("Enter Logo PNG path", os.path.join(PROJECT_DIR, "res", "logo_master.png"))

    if not os.path.isfile(logo_path):
        fallback = os.path.join(PROJECT_DIR, "res", "logo_master.png")
        if os.path.isfile(fallback):
            print(f"[NOTE] Using default master logo: {fallback}")
            logo_path = fallback
        else:
            print(f"[ERROR] Logo file not found: {logo_path}")
            input("\nPress Enter to exit...")
            sys.exit(1)

    saved_cfg = {
        "brand_title": brand,
        "model_name": model,
        "company_name": company,
        "hardware_version": hw_rev,
        "serial_prefix": serial,
        "vendor_contact": contact,
        "vendor_website": website,
        "hardware_mode": hw_mode,
        "default_theme": theme_idx,
        "default_boot_sec": boot_sec,
        "default_carousel_sec": cycle_sec,
        "logo_png_path": os.path.relpath(logo_path, SCRIPT_DIR)
    }

    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        json.dump(saved_cfg, f, indent=2)

    print("\n=====================================================================")
    print("                     CONFIGURATION SUMMARY                           ")
    print("=====================================================================")
    print(f" Brand:           {brand}")
    print(f" Model:           {model}")
    print(f" Company:         {company}")
    print(f" HW Revision:     {hw_rev}")
    print(f" Serial Prefix:   {serial}")
    print(f" Contact:         {contact}")
    print(f" Website:         {website}")
    print(f" Operational Mode:{hw_mode} ({mode_val})")
    print(f" UI Theme:        {theme_idx}")
    print(f" Boot Splash:     {boot_sec}s | Cycle Interval: {cycle_sec}s")
    print(f" Logo PNG:        {logo_path}")
    print("=====================================================================")

    confirm = prompt_val("Proceed to compile and build standalone monolithic .EXE? (Y/n)", "Y")
    if confirm.lower().startswith("n"):
        print("[CANCELLED] Configuration saved to vendor_config.json. No build executed.")
        input("\nPress Enter to exit...")
        return

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

    # 2. Read PNG
    with open(logo_path, "rb") as f:
        png_bytes = f.read()

    # 3. Generate OEMPayload.cpp with Obfuscated Strings
    strings = {
        'BRAND': brand,
        'MODEL': model,
        'SERIAL': serial,
        'HWREV': hw_rev,
        'CONTACT': contact,
        'WEBSITE': website,
        'COMPANY': company
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
        f'uint8_t OEMPayload::GetDefaultTheme()         {{ return {theme_idx}; }}',
        f'uint8_t OEMPayload::GetDefaultBootSec()       {{ return {boot_sec}; }}',
        f'uint8_t OEMPayload::GetDefaultCarouselSec()   {{ return {cycle_sec}; }}',
        'uint8_t OEMPayload::GetTargetHardwareMode()   { return ACTIVE_OEM_HW_MODE; }',
        ''
    ])

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

    print(f"[OK] Dynamic payload generated & written to: {cpp_path}")

    # Build Solution
    msbuild = find_msbuild()
    print(f"\n[BUILDING] Compiling Release x64 binary via MSBuild...")
    cmd = [
        msbuild,
        SLN_PATH,
        "/p:Configuration=Release",
        "/p:Platform=x64",
        "/t:Rebuild"
    ]
    ret = subprocess.run(cmd, check=False)
    if ret.returncode != 0:
        print(f"\n[ERROR] Compilation failed with error code: {ret.returncode}")
        input("\nPress Enter to exit...")
        sys.exit(ret.returncode)

    # Close running instances if any
    subprocess.run(["taskkill", "/F", "/IM", "SolarDisplayManager.exe"], capture_output=True, check=False)

    # Copy output
    out_exe = os.path.join(SCRIPT_DIR, "x64", "Release", "SolarDisplayManager.exe")
    dist_exe = os.path.join(DIST_DIR, "SolarDisplayManager.exe")
    os.makedirs(DIST_DIR, exist_ok=True)
    shutil.copy2(out_exe, dist_exe)

    print("\n=====================================================================")
    print("                     BUILD COMPLETED SUCCESSFULLY!                   ")
    print("=====================================================================")
    print(f" Standalone Binary: {dist_exe}")
    print(f" File Size:         {os.path.getsize(dist_exe):,} bytes")
    print("=====================================================================")
    input("\nPress Enter to finish...")

if __name__ == "__main__":
    main()
