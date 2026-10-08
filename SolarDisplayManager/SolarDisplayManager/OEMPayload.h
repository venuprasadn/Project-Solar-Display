#pragma once

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
#define ACTIVE_OEM_HW_MODE OEM_HW_MODE_TFT_ONLY
#endif

class OEMPayload
{
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
};
