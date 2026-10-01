import re

rc_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\SolarDisplayManager.rc'
with open(rc_path, 'r', encoding='utf-16le') as f:
    content = f.read()

new_dialog = '''IDD_SOLARDISPLAYMANAGER_DIALOG DIALOGEX 0, 0, 520, 352
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
EXSTYLE WS_EX_APPWINDOW
CAPTION "DONPOWER Solar Display Manager - Production & Provisioning Tool"
FONT 8, "MS Shell Dlg", 0, 0, 0x1
BEGIN
    GROUPBOX        "Target Connection",IDC_STATIC,7,4,506,32
    LTEXT           "COM Port:",IDC_STATIC,14,16,36,8
    COMBOBOX        IDC_COMBO_PORT,52,14,75,100,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    PUSHBUTTON      "Refresh",IDC_BTN_REFRESH_PORTS,132,13,42,14
    PUSHBUTTON      "Connect",IDC_BTN_CONNECT,178,13,48,14
    LTEXT           "Status: Disconnected",IDC_STATIC_STATUS,235,16,270,10

    GROUPBOX        "1. Firmware Flasher (esptool)",IDC_STATIC,7,38,506,64
    LTEXT           "App Binary:",IDC_STATIC,14,52,42,8
    EDITTEXT        IDC_EDIT_APP_BIN,58,50,336,12,ES_AUTOHSCROLL
    PUSHBUTTON      "Browse...",IDC_BTN_BROWSE_APP,398,49,45,14
    LTEXT           "Baud:",IDC_STATIC,448,52,20,8
    COMBOBOX        IDC_COMBO_FLASH_BAUD,470,50,38,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    CONTROL         "",IDC_PROGRESS_FLASH,"msctls_progress32",WS_BORDER,14,68,380,12
    PUSHBUTTON      "Flash Firmware",IDC_BTN_FLASH,398,67,110,14
    CONTROL         "Auto-provision configuration to NVS immediately after flashing",IDC_CHK_AUTO_PROVISION,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,86,300,10

    GROUPBOX        "2. Unit Configuration & Provisioning (NVS Permanent Store)",IDC_STATIC,7,104,506,124
    LTEXT           "Brand Title:",IDC_STATIC,14,118,44,8
    EDITTEXT        IDC_EDIT_BRAND,60,116,130,12,ES_AUTOHSCROLL
    LTEXT           "Model Name:",IDC_STATIC,196,118,46,8
    EDITTEXT        IDC_EDIT_MODEL,244,116,100,12,ES_AUTOHSCROLL
    LTEXT           "Serial No:",IDC_STATIC,350,118,34,8
    EDITTEXT        IDC_EDIT_SERIAL,386,116,122,12,ES_AUTOHSCROLL

    LTEXT           "Hardware Rev:",IDC_STATIC,14,134,48,8
    EDITTEXT        IDC_EDIT_HW_REV,64,132,60,12,ES_AUTOHSCROLL
    LTEXT           "Battery Voltage:",IDC_STATIC,130,134,54,8
    COMBOBOX        IDC_COMBO_BATT_VOLT,186,132,45,50,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    LTEXT           "Inverter Capacity:",IDC_STATIC,238,134,60,8
    COMBOBOX        IDC_COMBO_INV_VA,300,132,60,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
    LTEXT           "Battery Type:",IDC_STATIC,368,134,46,8
    COMBOBOX        IDC_COMBO_BATT_CHEM,416,132,92,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    LTEXT           "Low Cutoff (V):",IDC_STATIC,14,150,50,8
    EDITTEXT        IDC_EDIT_BATT_LOW,66,148,42,12,ES_AUTOHSCROLL
    LTEXT           "High Cutoff (V):",IDC_STATIC,114,150,52,8
    EDITTEXT        IDC_EDIT_BATT_HIGH,168,148,42,12,ES_AUTOHSCROLL
    LTEXT           "Overload (%):",IDC_STATIC,216,150,46,8
    EDITTEXT        IDC_EDIT_OVERLOAD,264,148,38,12,ES_AUTOHSCROLL
    LTEXT           "Thermal Trip (C):",IDC_STATIC,308,150,56,8
    EDITTEXT        IDC_EDIT_TEMP_TRIP,366,148,36,12,ES_AUTOHSCROLL
    LTEXT           "Telem Baud:",IDC_STATIC,408,150,44,8
    COMBOBOX        IDC_COMBO_TELEM_BAUD,454,148,54,60,CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP

    CONTROL         "Auto Carousel Display",IDC_CHK_CAROUSEL,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,166,90,10
    LTEXT           "Cycle (sec):",IDC_STATIC,108,167,40,8
    EDITTEXT        IDC_EDIT_CAROUSEL_SEC,150,165,26,12,ES_AUTOHSCROLL

    PUSHBUTTON      "Read from Unit",IDC_BTN_READ_CONFIG,14,186,85,15
    PUSHBUTTON      "Write & Commit NVS",IDC_BTN_WRITE_CONFIG,105,186,95,15
    PUSHBUTTON      "Reboot Unit",IDC_BTN_REBOOT,206,186,75,15
    PUSHBUTTON      "Factory Reset",IDC_BTN_FACTORY_RESET,287,186,75,15

    GROUPBOX        "3. Operation Log & Diagnostics",IDC_STATIC,7,230,506,116
    EDITTEXT        IDC_EDIT_LOG,14,242,492,84,ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL
    PUSHBUTTON      "Clear Log",IDC_BTN_CLEAR_LOG,14,330,60,12
    PUSHBUTTON      "Close",IDCANCEL,453,330,60,12
END'''

pattern = r'IDD_SOLARDISPLAYMANAGER_DIALOG DIALOGEX.*?\nEND'
content, count = re.subn(pattern, new_dialog, content, flags=re.DOTALL)
print('Replaced count:', count)

with open(rc_path, 'w', encoding='utf-16le') as f:
    f.write(content)
print('Successfully wrote new dialog into SolarDisplayManager.rc!')
