import os

rc_content = '''// Microsoft Visual C++ generated resource script.
//
#include "resource.h"

#define APSTUDIO_READONLY_SYMBOLS
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 2 resource.
//
#ifndef APSTUDIO_INVOKED
#include "targetver.h"
#endif
#include "afxres.h"
#include "verrsrc.h"

/////////////////////////////////////////////////////////////////////////////
#undef APSTUDIO_READONLY_SYMBOLS

/////////////////////////////////////////////////////////////////////////////
// English (United States) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_US

/////////////////////////////////////////////////////////////////////////////
//
// Dialog
//

IDD_ABOUTBOX DIALOGEX 0, 0, 170, 62
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "About SolarDisplayManager"
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    ICON            IDR_MAINFRAME,IDC_STATIC,14,14,21,20
    LTEXT           "SolarDisplayManager, Version 2.5",IDC_STATIC,42,14,114,8,SS_NOPREFIX
    LTEXT           "Industrial OEM Staging System",IDC_STATIC,42,26,114,8
    DEFPUSHBUTTON   "OK",IDOK,113,41,50,14,WS_GROUP
END

IDD_SOLARDISPLAYMANAGER_DIALOG DIALOGEX 0, 0, 520, 410
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
EXSTYLE WS_EX_APPWINDOW
CAPTION "Solar PCU Display & IoT Staging Manager - Industrial OEM Tool v2.5"
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    CONTROL         "",IDC_STATIC_OEM_HEADER,"Static",SS_NOTIFY | WS_CHILD | WS_VISIBLE,7,4,506,56


    GROUPBOX        "Target Hardware Connection & Auto-Discovery",IDC_STATIC,7,62,506,29
    LTEXT           "Port:",IDC_STATIC,14,74,18,8
    COMBOBOX        IDC_COMBO_PORT,36,72,180,100,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Auto-Detect",IDC_BTN_REFRESH_PORTS,220,71,52,14
    PUSHBUTTON      "Connect",IDC_BTN_CONNECT,276,71,45,14
    LTEXT           "Status: Disconnected",IDC_STATIC_STATUS,328,74,180,10

    GROUPBOX        "1. Embedded Firmware Flasher (ESP32-S3 16MB Flash, 8MB PSRAM)",IDC_STATIC,7,93,506,42
    LTEXT           "Firmware:",IDC_STATIC,14,105,32,8
    LTEXT           "Integrated Production Release v2.5 (Encrypted Standalone)",IDC_STATIC,50,105,210,8
    LTEXT           "Baud:",IDC_STATIC,266,105,20,8
    COMBOBOX        IDC_COMBO_FLASH_BAUD,288,103,42,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Flash Device",IDC_BTN_FLASH,336,102,68,15
    CONTROL         "Auto-provision NVS",IDC_CHK_AUTO_PROVISION,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,410,104,95,10
    CONTROL         "",IDC_PROGRESS_FLASH,"msctls_progress32",WS_BORDER,14,120,492,9

    CONTROL         "",IDC_TAB_MAIN,"SysTabControl32",WS_CHILD | WS_VISIBLE | WS_TABSTOP,7,138,506,94

    GROUPBOX        "2. Production Operations (ISO 9001 / IEC 62109 / MIL-STD)",IDC_STATIC,7,234,506,30
    PUSHBUTTON      "Read Active NVS",IDC_BTN_READ_CONFIG,14,244,95,15
    PUSHBUTTON      "Write OEM NVS",IDC_BTN_WRITE_CONFIG,114,244,95,15
    PUSHBUTTON      "Reboot Device",IDC_BTN_REBOOT,214,244,80,15
    PUSHBUTTON      "Factory Reset",IDC_BTN_FACTORY_RESET,298,244,85,15

    GROUPBOX        "Operation Log & Live Telemetry Console",IDC_STATIC,7,266,506,138
    EDITTEXT        IDC_EDIT_LOG,14,277,492,108,ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL
    PUSHBUTTON      "Clear Log",IDC_BTN_CLEAR_LOG,14,390,55,12
    PUSHBUTTON      "Close",IDCANCEL,453,390,55,12
END

IDD_TAB_DISPLAY DIALOGEX 0, 0, 500, 80
STYLE DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    CONTROL         "Enable LCD Display Features (ILI9341 320x240 LVGL UI Active)",IDC_CHK_FEATURE_DISPLAY,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,8,2,240,10

    GROUPBOX        "Display Theme Selection & Screen Sequence Timers",IDC_STATIC,4,13,492,36
    LTEXT           "UI Theme:",IDC_STATIC,12,25,36,8
    COMBOBOX        IDC_COMBO_LOGO_THEME,50,23,140,80,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    LTEXT           "Boot Splash:",IDC_STATIC,196,25,42,8
    EDITTEXT        IDC_EDIT_BOOT_SEC,240,23,22,12,ES_AUTOHSCROLL
    LTEXT           "sec",IDC_STATIC,265,25,12,8
    CONTROL         "Auto Screen Cycle",IDC_CHK_CAROUSEL,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,284,24,75,10
    LTEXT           "Interval:",IDC_STATIC,362,25,30,8
    EDITTEXT        IDC_EDIT_CAROUSEL_SEC,394,23,22,12,ES_AUTOHSCROLL
    LTEXT           "sec",IDC_STATIC,419,25,12,8
    LTEXT           "UART:",IDC_STATIC,436,25,24,8
    COMBOBOX        IDC_COMBO_TELEM_BAUD,462,23,30,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    GROUPBOX        "Master OEM Boot Logo (Partition Offset 0x410000)",IDC_STATIC,4,50,492,26
    LTEXT           "Integrated Master OEM Logo (256x256 High-Resolution PNG) is locked and flashed automatically.",IDC_STATIC,12,61,475,10
END

IDD_TAB_APP DIALOGEX 0, 0, 500, 80
STYLE DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    CONTROL         "Enable Mobile App & Cloud / IoT Features (Wi-Fi + AWS IoT Core)",IDC_CHK_FEATURE_APP,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,8,2,250,10
    PUSHBUTTON      "Flash Certificates (mTLS)",IDC_BTN_FLASH_CERTS,360,1,130,13

    GROUPBOX        "1. Direct USB Wi-Fi Provisioning",IDC_STATIC,4,13,492,32
    LTEXT           "SSID:",IDC_STATIC,12,24,22,8
    COMBOBOX        IDC_COMBO_WIFI_SSID,36,22,135,100,CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Scan APs",IDC_BTN_SCAN_WIFI,175,21,48,14
    LTEXT           "Password:",IDC_STATIC,228,24,35,8
    EDITTEXT        IDC_EDIT_WIFI_PASS,266,22,92,12,ES_PASSWORD | ES_AUTOHSCROLL
    CONTROL         "Show",IDC_CHK_SHOW_PASS,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,362,23,32,10
    PUSHBUTTON      "Provision Wi-Fi",IDC_BTN_PROVISION_WIFI,398,21,90,14

    GROUPBOX        "2. Live Cloud & Network Diagnostics",IDC_STATIC,4,47,492,30
    PUSHBUTTON      "Query Status",IDC_BTN_QUERY_IOT_STATUS,12,58,58,14
    LTEXT           "Wi-Fi:",IDC_STATIC,74,60,22,8
    EDITTEXT        IDC_EDIT_IOT_WIFI_STATUS,98,58,80,12,ES_AUTOHSCROLL | ES_READONLY
    LTEXT           "Cloud:",IDC_STATIC,182,60,22,8
    EDITTEXT        IDC_EDIT_IOT_CLOUD_STATUS,206,58,64,12,ES_AUTOHSCROLL | ES_READONLY
    LTEXT           "Thing:",IDC_STATIC,274,60,22,8
    EDITTEXT        IDC_EDIT_IOT_THING_ID,298,58,85,12,ES_AUTOHSCROLL | ES_READONLY
    LTEXT           "MAC:",IDC_STATIC,387,60,20,8
    EDITTEXT        IDC_EDIT_IOT_MAC,410,58,80,12,ES_AUTOHSCROLL | ES_READONLY
END

// Version
//

VS_VERSION_INFO VERSIONINFO
 FILEVERSION 2,5,0,0
 PRODUCTVERSION 2,5,0,0
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x1L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName", "Industrial OEM Solutions"
            VALUE "FileDescription", "Solar PCU Display & IoT Manager"
            VALUE "FileVersion", "2.5.0.0"
            VALUE "InternalName", "SolarDisplayManager.exe"
            VALUE "LegalCopyright", "Copyright (C) 2026. All rights reserved."
            VALUE "OriginalFilename", "SolarDisplayManager.exe"
            VALUE "ProductName", "SolarDisplayManager"
            VALUE "ProductVersion", "2.5.0.0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x409, 1200
    END
END

/////////////////////////////////////////////////////////////////////////////
//
// DESIGNINFO
//

#ifdef APSTUDIO_INVOKED
GUIDELINES DESIGNINFO
BEGIN
    IDD_ABOUTBOX, DIALOG
    BEGIN
        LEFTMARGIN, 7
        RIGHTMARGIN, 163
        TOPMARGIN, 7
        BOTTOMMARGIN, 55
    END

    IDD_SOLARDISPLAYMANAGER_DIALOG, DIALOG
    BEGIN
        LEFTMARGIN, 7
        RIGHTMARGIN, 513
        TOPMARGIN, 4
        BOTTOMMARGIN, 404
    END

    IDD_TAB_DISPLAY, DIALOG
    BEGIN
        LEFTMARGIN, 4
        RIGHTMARGIN, 496
        TOPMARGIN, 2
        BOTTOMMARGIN, 78
    END

    IDD_TAB_APP, DIALOG
    BEGIN
        LEFTMARGIN, 4
        RIGHTMARGIN, 496
        TOPMARGIN, 2
        BOTTOMMARGIN, 78
    END
END
#endif    // APSTUDIO_INVOKED

/////////////////////////////////////////////////////////////////////////////
//
// AFX_DIALOG_LAYOUT
//

IDD_SOLARDISPLAYMANAGER_DIALOG AFX_DIALOG_LAYOUT
BEGIN
    0
END

/////////////////////////////////////////////////////////////////////////////
//
// String Table
//

STRINGTABLE
BEGIN
    IDS_ABOUTBOX            "&About SolarDisplayManager..."
END

#endif    // English (United States) resources
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// English (India) resources

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENN)
LANGUAGE LANG_ENGLISH, SUBLANG_ENGLISH_INDIA

#ifdef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// TEXTINCLUDE
//

1 TEXTINCLUDE 
BEGIN
    "resource.h\\0"
END

2 TEXTINCLUDE 
BEGIN
    "#ifndef APSTUDIO_INVOKED\\r\\n"
    "#include ""targetver.h""\\r\\n"
    "#endif\\r\\n"
    "#include ""afxres.h""\\r\\n"
    "#include ""verrsrc.h""\\r\\n"
    "\\0"
END

3 TEXTINCLUDE 
BEGIN
    "#define _AFX_NO_SPLITTER_RESOURCES\\r\\n"
    "#define _AFX_NO_OLE_RESOURCES\\r\\n"
    "#define _AFX_NO_TRACKER_RESOURCES\\r\\n"
    "#define _AFX_NO_PROPERTY_RESOURCES\\r\\n"
    "\\r\\n"
    "#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)\\r\\n"
    "LANGUAGE 9, 1\\r\\n"
    "#include ""res\\\\SolarDisplayManager.rc2""  // non-Microsoft Visual C++ edited resources\\r\\n"
    "#include ""afxres.rc""      // Standard components\\r\\n"
    "#if !defined(_AFXDLL)\\r\\n"
    "#include ""afxribbon.rc""   // MFC ribbon and control bar resources\\r\\n"
    "#endif\\r\\n"
    "#endif\\r\\n"
    "\\0"
END

#endif    // APSTUDIO_INVOKED

/////////////////////////////////////////////////////////////////////////////
//
// Icon
//

IDR_MAINFRAME           ICON                    "res\\\\SolarDisplayManager.ico"

/////////////////////////////////////////////////////////////////////////////
//
// Binary RCDATA Resources (Standalone Monolithic Firmware & Flasher)
//

IDR_BIN_BOOTLOADER      RCDATA                  "res\\\\bootloader.bin"
IDR_BIN_PARTITIONS      RCDATA                  "res\\\\partition-table.bin"
IDR_BIN_FIRMWARE        RCDATA                  "res\\\\firmware.bin"
IDR_BIN_ESPTOOL         RCDATA                  "res\\\\esptool.bin"

#endif    // English (India) resources
/////////////////////////////////////////////////////////////////////////////

#ifndef APSTUDIO_INVOKED
/////////////////////////////////////////////////////////////////////////////
//
// Generated from the TEXTINCLUDE 3 resource.
//
#define _AFX_NO_SPLITTER_RESOURCES
#define _AFX_NO_OLE_RESOURCES
#define _AFX_NO_TRACKER_RESOURCES
#define _AFX_NO_PROPERTY_RESOURCES

#if !defined(AFX_RESOURCE_DLL) || defined(AFX_TARG_ENU)
LANGUAGE 9, 1
#include "res\\SolarDisplayManager.rc2"
#include "afxres.rc"
#if !defined(_AFXDLL)
#include "afxribbon.rc"
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
#endif    // not APSTUDIO_INVOKED
'''

rc_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\SolarDisplayManager.rc'
with open(rc_path, 'w', encoding='utf-16le') as f:
    f.write(rc_content)

print('SolarDisplayManager.rc generated cleanly.')
