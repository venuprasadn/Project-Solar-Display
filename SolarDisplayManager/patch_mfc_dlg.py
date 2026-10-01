dlg_cpp_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\SolarDisplayManagerDlg.cpp'

with open(dlg_cpp_path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update DoDataExchange
old_ddx = '''    DDX_Control(pDX, IDC_EDIT_BRAND, m_editBrand);
    DDX_Control(pDX, IDC_EDIT_MODEL, m_editModel);
    DDX_Control(pDX, IDC_EDIT_SERIAL, m_editSerial);
    DDX_Control(pDX, IDC_EDIT_HW_REV, m_editHwRev);
    DDX_Control(pDX, IDC_COMBO_BATT_VOLT, m_comboBattVolt);
    DDX_Control(pDX, IDC_COMBO_INV_VA, m_comboInvVa);
    DDX_Control(pDX, IDC_COMBO_BATT_CHEM, m_comboBattChem);
    DDX_Control(pDX, IDC_EDIT_BATT_LOW, m_editBattLow);
    DDX_Control(pDX, IDC_EDIT_BATT_HIGH, m_editBattHigh);
    DDX_Control(pDX, IDC_EDIT_OVERLOAD, m_editOverload);
    DDX_Control(pDX, IDC_EDIT_TEMP_TRIP, m_editTempTrip);
    DDX_Control(pDX, IDC_CHK_CAROUSEL, m_chkCarousel);
    DDX_Control(pDX, IDC_EDIT_CAROUSEL_SEC, m_editCarouselSec);
    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);'''

new_ddx = '''    DDX_Control(pDX, IDC_EDIT_BRAND, m_editBrand);
    DDX_Control(pDX, IDC_EDIT_MODEL, m_editModel);
    DDX_Control(pDX, IDC_EDIT_SERIAL, m_editSerial);
    DDX_Control(pDX, IDC_EDIT_HW_REV, m_editHwRev);
    DDX_Control(pDX, IDC_EDIT_VENDOR_CONTACT, m_editVendorContact);
    DDX_Control(pDX, IDC_EDIT_VENDOR_WEBSITE, m_editVendorWebsite);
    DDX_Control(pDX, IDC_COMBO_LOGO_THEME, m_comboLogoTheme);
    DDX_Control(pDX, IDC_EDIT_BOOT_SEC, m_editBootSec);
    DDX_Control(pDX, IDC_CHK_CAROUSEL, m_chkCarousel);
    DDX_Control(pDX, IDC_EDIT_CAROUSEL_SEC, m_editCarouselSec);
    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);'''

text = text.replace(old_ddx, new_ddx, 1)

# 2. Update OnInitDialog dropdowns and factory defaults
old_init = '''    // 2. Initialize Configuration Dropdowns
    m_comboBattVolt.AddString(_T("12 V"));
    m_comboBattVolt.AddString(_T("24 V"));
    m_comboBattVolt.AddString(_T("36 V"));
    m_comboBattVolt.AddString(_T("48 V"));
    m_comboBattVolt.SetCurSel(1); // 24V

    m_comboInvVa.AddString(_T("1000 VA"));
    m_comboInvVa.AddString(_T("1500 VA"));
    m_comboInvVa.AddString(_T("2500 VA"));
    m_comboInvVa.AddString(_T("3500 VA"));
    m_comboInvVa.AddString(_T("5000 VA"));
    m_comboInvVa.SetCurSel(3); // 3500 VA

    m_comboBattChem.AddString(_T("Tubular Lead-Acid"));
    m_comboBattChem.AddString(_T("SMF / VRLA"));
    m_comboBattChem.AddString(_T("LiFePO4"));
    m_comboBattChem.AddString(_T("Gel Battery"));
    m_comboBattChem.SetCurSel(0); // Tubular

    m_comboTelemBaud.AddString(_T("9600"));
    m_comboTelemBaud.AddString(_T("19200"));
    m_comboTelemBaud.AddString(_T("38400"));
    m_comboTelemBaud.AddString(_T("115200"));
    m_comboTelemBaud.SetCurSel(0); // 9600

    // 3. Load Factory Default Values to UI
    pcu_config_t defCfg;
    memset(&defCfg, 0, sizeof(defCfg));
    strcpy_s(defCfg.brand_title, "DONPOWER HYBRID PCU");
    strcpy_s(defCfg.model_name, "PCU-3500-24V");
    strcpy_s(defCfg.serial_number, "DP-2026-X001");
    strcpy_s(defCfg.hardware_version, "HW-V1.2");
    defCfg.nominal_batt_volt = 24;
    defCfg.rated_inverter_va = 3500;
    defCfg.battery_chemistry = 0;
    defCfg.batt_low_cutoff_v = 21.0f;
    defCfg.batt_high_cutoff_v = 29.2f;
    defCfg.overload_cutoff_pct = 120.0f;
    defCfg.heatsink_trip_temp_c = 75.0f;
    defCfg.auto_carousel_enabled = 1;
    defCfg.carousel_interval_sec = 5;
    defCfg.telemetry_baudrate = 9600;
    LoadConfigToUI(defCfg);'''

new_init = '''    // 2. Initialize Configuration Dropdowns
    m_comboLogoTheme.AddString(_T("0 - Sunburst Gold"));
    m_comboLogoTheme.AddString(_T("1 - Cyber Cyan"));
    m_comboLogoTheme.AddString(_T("2 - Emerald Eco"));
    m_comboLogoTheme.AddString(_T("3 - Crimson Red"));
    m_comboLogoTheme.SetCurSel(0);

    m_comboTelemBaud.AddString(_T("9600"));
    m_comboTelemBaud.AddString(_T("19200"));
    m_comboTelemBaud.AddString(_T("38400"));
    m_comboTelemBaud.AddString(_T("115200"));
    m_comboTelemBaud.SetCurSel(0); // 9600

    // 3. Load Factory Default Values to UI
    pcu_config_t defCfg;
    memset(&defCfg, 0, sizeof(defCfg));
    strcpy_s(defCfg.brand_title, "DONPOWER SOLAR");
    strcpy_s(defCfg.model_name, "HYBRID MPPT PCU");
    strcpy_s(defCfg.serial_number, "DP-2026-X8849");
    strcpy_s(defCfg.hardware_version, "HW-V2.1");
    strcpy_s(defCfg.vendor_contact, "Toll Free: 1800-425-9999");
    strcpy_s(defCfg.vendor_website, "www.donpower.in");
    defCfg.production_date = 20261001;
    defCfg.logo_theme = 0;
    defCfg.boot_duration_sec = 3;
    defCfg.auto_carousel_enabled = 1;
    defCfg.carousel_interval_sec = 5;
    defCfg.telemetry_baudrate = 9600;
    LoadConfigToUI(defCfg);'''

text = text.replace(old_init, new_init, 1)

# 3. Update LoadConfigToUI and CollectConfigFromUI
old_load_collect = '''void CSolarDisplayManagerDlg::LoadConfigToUI(const pcu_config_t& cfg)
{
    m_editBrand.SetWindowText(CString(cfg.brand_title));
    m_editModel.SetWindowText(CString(cfg.model_name));
    m_editSerial.SetWindowText(CString(cfg.serial_number));
    m_editHwRev.SetWindowText(CString(cfg.hardware_version));

    // Battery Volt
    if (cfg.nominal_batt_volt == 12) m_comboBattVolt.SetCurSel(0);
    else if (cfg.nominal_batt_volt == 24) m_comboBattVolt.SetCurSel(1);
    else if (cfg.nominal_batt_volt == 36) m_comboBattVolt.SetCurSel(2);
    else if (cfg.nominal_batt_volt == 48) m_comboBattVolt.SetCurSel(3);

    // Inverter VA
    if (cfg.rated_inverter_va <= 1000) m_comboInvVa.SetCurSel(0);
    else if (cfg.rated_inverter_va <= 1500) m_comboInvVa.SetCurSel(1);
    else if (cfg.rated_inverter_va <= 2500) m_comboInvVa.SetCurSel(2);
    else if (cfg.rated_inverter_va <= 3500) m_comboInvVa.SetCurSel(3);
    else m_comboInvVa.SetCurSel(4);

    m_comboBattChem.SetCurSel(cfg.battery_chemistry % 4);

    CString str;
    str.Format(_T("%.1f"), cfg.batt_low_cutoff_v);
    m_editBattLow.SetWindowText(str);

    str.Format(_T("%.1f"), cfg.batt_high_cutoff_v);
    m_editBattHigh.SetWindowText(str);

    str.Format(_T("%.0f"), cfg.overload_cutoff_pct);
    m_editOverload.SetWindowText(str);

    str.Format(_T("%.1f"), cfg.heatsink_trip_temp_c);
    m_editTempTrip.SetWindowText(str);

    m_chkCarousel.SetCheck(cfg.auto_carousel_enabled ? BST_CHECKED : BST_UNCHECKED);
    str.Format(_T("%u"), cfg.carousel_interval_sec);
    m_editCarouselSec.SetWindowText(str);

    // Telem Baud
    if (cfg.telemetry_baudrate == 9600) m_comboTelemBaud.SetCurSel(0);
    else if (cfg.telemetry_baudrate == 19200) m_comboTelemBaud.SetCurSel(1);
    else if (cfg.telemetry_baudrate == 38400) m_comboTelemBaud.SetCurSel(2);
    else if (cfg.telemetry_baudrate == 115200) m_comboTelemBaud.SetCurSel(3);
}

bool CSolarDisplayManagerDlg::CollectConfigFromUI(pcu_config_t& cfg)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.magic = PCU_CONFIG_MAGIC;
    cfg.version = PCU_CONFIG_VERSION;
    cfg.struct_size = sizeof(pcu_config_t);
    cfg.is_configured = 1;

    CString str;
    m_editBrand.GetWindowText(str);
    strncpy_s(cfg.brand_title, sizeof(cfg.brand_title), CT2A(str), _TRUNCATE);

    m_editModel.GetWindowText(str);
    strncpy_s(cfg.model_name, sizeof(cfg.model_name), CT2A(str), _TRUNCATE);

    m_editSerial.GetWindowText(str);
    strncpy_s(cfg.serial_number, sizeof(cfg.serial_number), CT2A(str), _TRUNCATE);

    m_editHwRev.GetWindowText(str);
    strncpy_s(cfg.hardware_version, sizeof(cfg.hardware_version), CT2A(str), _TRUNCATE);

    int sel = m_comboBattVolt.GetCurSel();
    cfg.nominal_batt_volt = (sel == 0) ? 12 : (sel == 1) ? 24 : (sel == 2) ? 36 : 48;

    sel = m_comboInvVa.GetCurSel();
    cfg.rated_inverter_va = (sel == 0) ? 1000 : (sel == 1) ? 1500 : (sel == 2) ? 2500 : (sel == 3) ? 3500 : 5000;

    cfg.battery_chemistry = (uint8_t)m_comboBattChem.GetCurSel();
    cfg.grid_frequency_hz = 50;

    m_editBattLow.GetWindowText(str);
    cfg.batt_low_cutoff_v = (float)_tstof(str);

    m_editBattHigh.GetWindowText(str);
    cfg.batt_high_cutoff_v = (float)_tstof(str);

    m_editOverload.GetWindowText(str);
    cfg.overload_cutoff_pct = (float)_tstof(str);

    m_editTempTrip.GetWindowText(str);
    cfg.heatsink_trip_temp_c = (float)_tstof(str);

    cfg.auto_carousel_enabled = (m_chkCarousel.GetCheck() == BST_CHECKED) ? 1 : 0;
    m_editCarouselSec.GetWindowText(str);
    cfg.carousel_interval_sec = (uint8_t)_tstoi(str);
    if (cfg.carousel_interval_sec < 1) cfg.carousel_interval_sec = 5;

    sel = m_comboTelemBaud.GetCurSel();
    cfg.telemetry_baudrate = (sel == 0) ? 9600 : (sel == 1) ? 19200 : (sel == 2) ? 38400 : 115200;

    cfg.default_boot_page = 0;
    cfg.backlight_brightness = 100;
    cfg.telemetry_format = 0;

    return true;
}'''

new_load_collect = '''void CSolarDisplayManagerDlg::LoadConfigToUI(const pcu_config_t& cfg)
{
    m_editBrand.SetWindowText(CString(cfg.brand_title));
    m_editModel.SetWindowText(CString(cfg.model_name));
    m_editSerial.SetWindowText(CString(cfg.serial_number));
    m_editHwRev.SetWindowText(CString(cfg.hardware_version));
    m_editVendorContact.SetWindowText(CString(cfg.vendor_contact));
    m_editVendorWebsite.SetWindowText(CString(cfg.vendor_website));

    m_comboLogoTheme.SetCurSel(cfg.logo_theme % 4);

    CString str;
    str.Format(_T("%u"), cfg.boot_duration_sec > 0 ? cfg.boot_duration_sec : 3);
    m_editBootSec.SetWindowText(str);

    m_chkCarousel.SetCheck(cfg.auto_carousel_enabled ? BST_CHECKED : BST_UNCHECKED);
    str.Format(_T("%u"), cfg.carousel_interval_sec > 0 ? cfg.carousel_interval_sec : 5);
    m_editCarouselSec.SetWindowText(str);

    // Telem Baud
    if (cfg.telemetry_baudrate == 9600) m_comboTelemBaud.SetCurSel(0);
    else if (cfg.telemetry_baudrate == 19200) m_comboTelemBaud.SetCurSel(1);
    else if (cfg.telemetry_baudrate == 38400) m_comboTelemBaud.SetCurSel(2);
    else if (cfg.telemetry_baudrate == 115200) m_comboTelemBaud.SetCurSel(3);
    else m_comboTelemBaud.SetCurSel(0);
}

bool CSolarDisplayManagerDlg::CollectConfigFromUI(pcu_config_t& cfg)
{
    memset(&cfg, 0, sizeof(cfg));
    cfg.magic = PCU_CONFIG_MAGIC;
    cfg.version = PCU_CONFIG_VERSION;
    cfg.struct_size = sizeof(pcu_config_t);
    cfg.is_configured = 1;

    CString str;
    m_editBrand.GetWindowText(str);
    strncpy_s(cfg.brand_title, sizeof(cfg.brand_title), CT2A(str), _TRUNCATE);

    m_editModel.GetWindowText(str);
    strncpy_s(cfg.model_name, sizeof(cfg.model_name), CT2A(str), _TRUNCATE);

    m_editSerial.GetWindowText(str);
    strncpy_s(cfg.serial_number, sizeof(cfg.serial_number), CT2A(str), _TRUNCATE);

    m_editHwRev.GetWindowText(str);
    strncpy_s(cfg.hardware_version, sizeof(cfg.hardware_version), CT2A(str), _TRUNCATE);

    m_editVendorContact.GetWindowText(str);
    strncpy_s(cfg.vendor_contact, sizeof(cfg.vendor_contact), CT2A(str), _TRUNCATE);

    m_editVendorWebsite.GetWindowText(str);
    strncpy_s(cfg.vendor_website, sizeof(cfg.vendor_website), CT2A(str), _TRUNCATE);

    SYSTEMTIME st;
    GetLocalTime(&st);
    cfg.production_date = st.wYear * 10000 + st.wMonth * 100 + st.wDay;

    int themeSel = m_comboLogoTheme.GetCurSel();
    cfg.logo_theme = (themeSel >= 0 && themeSel <= 3) ? (uint8_t)themeSel : 0;

    m_editBootSec.GetWindowText(str);
    cfg.boot_duration_sec = (uint8_t)_tstoi(str);
    if (cfg.boot_duration_sec < 1) cfg.boot_duration_sec = 3;

    cfg.auto_carousel_enabled = (m_chkCarousel.GetCheck() == BST_CHECKED) ? 1 : 0;
    m_editCarouselSec.GetWindowText(str);
    cfg.carousel_interval_sec = (uint8_t)_tstoi(str);
    if (cfg.carousel_interval_sec < 1) cfg.carousel_interval_sec = 5;

    int baudSel = m_comboTelemBaud.GetCurSel();
    cfg.telemetry_baudrate = (baudSel == 0) ? 9600 : (baudSel == 1) ? 19200 : (baudSel == 2) ? 38400 : 115200;

    cfg.backlight_brightness = 100;

    return true;
}'''

text = text.replace(old_load_collect, new_load_collect, 1)

with open(dlg_cpp_path, 'w', encoding='utf-8') as f:
    f.write(text)

print('Successfully patched SolarDisplayManagerDlg.cpp!')
