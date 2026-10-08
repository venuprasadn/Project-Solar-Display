import re

rc_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\SolarDisplayManager.rc'
with open(rc_path, 'r', encoding='utf-16le') as f:
    content = f.read()

new_dialog = '''IDD_SOLARDISPLAYMANAGER_DIALOG DIALOGEX 0, 0, 520, 352
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
EXSTYLE WS_EX_APPWINDOW
CAPTION "DONPOWER Solar Display Manager - Brand & Display Provisioning Tool v2.0"
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    GROUPBOX        "Target Connection",IDC_STATIC,7,4,506,32
    LTEXT           "COM Port:",IDC_STATIC,14,16,36,8
    COMBOBOX        IDC_COMBO_PORT,52,14,75,100,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Refresh",IDC_BTN_REFRESH_PORTS,132,13,42,14
    PUSHBUTTON      "Connect",IDC_BTN_CONNECT,178,13,48,14
    LTEXT           "Status: Disconnected",IDC_STATIC_STATUS,235,16,270,10

    GROUPBOX        "1. Firmware Flasher (Standalone esptool)",IDC_STATIC,7,38,506,64
    LTEXT           "App Binary:",IDC_STATIC,14,52,42,8
    EDITTEXT        IDC_EDIT_APP_BIN,58,50,336,12,ES_AUTOHSCROLL
    PUSHBUTTON      "Browse...",IDC_BTN_BROWSE_APP,398,49,45,14
    LTEXT           "Baud:",IDC_STATIC,448,52,20,8
    COMBOBOX        IDC_COMBO_FLASH_BAUD,470,50,38,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    CONTROL         "",IDC_PROGRESS_FLASH,"msctls_progress32",WS_BORDER,14,68,380,12
    PUSHBUTTON      "Flash Firmware",IDC_BTN_FLASH,398,67,110,14
    CONTROL         "Auto-provision configuration to NVS immediately after flashing",IDC_CHK_AUTO_PROVISION,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,86,300,10

    GROUPBOX        "2. Brand Identity & Display Customization (Permanent NVS Store)",IDC_STATIC,7,104,506,125
    LTEXT           "Brand Title:",IDC_STATIC,14,117,44,8
    EDITTEXT        IDC_EDIT_BRAND,60,115,130,12,ES_AUTOHSCROLL
    LTEXT           "Model Name:",IDC_STATIC,196,117,46,8
    EDITTEXT        IDC_EDIT_MODEL,244,115,100,12,ES_AUTOHSCROLL
    LTEXT           "Serial No:",IDC_STATIC,350,117,34,8
    EDITTEXT        IDC_EDIT_SERIAL,386,115,122,12,ES_AUTOHSCROLL

    LTEXT           "Hardware Rev:",IDC_STATIC,14,133,48,8
    EDITTEXT        IDC_EDIT_HW_REV,64,131,56,12,ES_AUTOHSCROLL
    LTEXT           "Support Contact:",IDC_STATIC,124,133,58,8
    EDITTEXT        IDC_EDIT_VENDOR_CONTACT,184,131,160,12,ES_AUTOHSCROLL
    LTEXT           "Website:",IDC_STATIC,350,133,32,8
    EDITTEXT        IDC_EDIT_VENDOR_WEBSITE,386,131,122,12,ES_AUTOHSCROLL

    LTEXT           "Custom Logo:",IDC_STATIC,14,149,46,8
    EDITTEXT        IDC_EDIT_LOGO_PATH,62,147,270,12,ES_AUTOHSCROLL
    PUSHBUTTON      "Browse...",IDC_BTN_BROWSE_LOGO,336,146,46,14
    PUSHBUTTON      "Flash Logo",IDC_BTN_FLASH_LOGO,386,146,65,14
    PUSHBUTTON      "Clear Logo",IDC_BTN_CLEAR_LOGO,455,146,53,14

    LTEXT           "Logo Theme:",IDC_STATIC,14,166,44,8
    COMBOBOX        IDC_COMBO_LOGO_THEME,60,164,82,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    LTEXT           "Boot Splash (s):",IDC_STATIC,148,166,54,8
    EDITTEXT        IDC_EDIT_BOOT_SEC,204,164,26,12,ES_AUTOHSCROLL
    CONTROL         "Auto Carousel Display",IDC_CHK_CAROUSEL,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,240,165,85,10
    LTEXT           "Cycle (sec):",IDC_STATIC,328,166,40,8
    EDITTEXT        IDC_EDIT_CAROUSEL_SEC,370,164,26,12,ES_AUTOHSCROLL
    LTEXT           "Telem Baud:",IDC_STATIC,406,166,42,8
    COMBOBOX        IDC_COMBO_TELEM_BAUD,450,164,58,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    PUSHBUTTON      "Read from Unit",IDC_BTN_READ_CONFIG,14,188,85,15
    PUSHBUTTON      "Write & Commit NVS",IDC_BTN_WRITE_CONFIG,105,188,95,15
    PUSHBUTTON      "Reboot Unit",IDC_BTN_REBOOT,206,188,75,15
    PUSHBUTTON      "Factory Reset",IDC_BTN_FACTORY_RESET,287,188,75,15

    GROUPBOX        "3. Operation Log & Diagnostics",IDC_STATIC,7,232,506,114
    EDITTEXT        IDC_EDIT_LOG,14,244,492,82,ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL
    PUSHBUTTON      "Clear Log",IDC_BTN_CLEAR_LOG,14,330,60,12
    PUSHBUTTON      "Close",IDCANCEL,453,330,60,12
END'''

pattern = r'IDD_SOLARDISPLAYMANAGER_DIALOG DIALOGEX.*?\nEND'
content, count = re.subn(pattern, new_dialog, content, flags=re.DOTALL)
print('Replaced count:', count)

with open(rc_path, 'w', encoding='utf-16le') as f:
    f.write(content)
print('Successfully updated SolarDisplayManager.rc with custom logo controls!')
