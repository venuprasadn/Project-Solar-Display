#include "pch.h"
#include "SolarDisplayManager.h"
#include "TabDisplayDlg.h"
#include "SolarDisplayManagerDlg.h"
#include "OEMPayload.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CTabDisplayDlg, CDialogEx)

CTabDisplayDlg::CTabDisplayDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_TAB_DISPLAY, pParent)
    , m_pMainDlg(nullptr)
{
}

CTabDisplayDlg::~CTabDisplayDlg()
{
}

void CTabDisplayDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHK_FEATURE_DISPLAY, m_chkFeatureDisplay);
    DDX_Control(pDX, IDC_COMBO_LOGO_THEME, m_comboLogoTheme);
    DDX_Control(pDX, IDC_EDIT_BOOT_SEC, m_editBootSec);
    DDX_Control(pDX, IDC_CHK_CAROUSEL, m_chkCarousel);
    DDX_Control(pDX, IDC_EDIT_CAROUSEL_SEC, m_editCarouselSec);
    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);
}

BEGIN_MESSAGE_MAP(CTabDisplayDlg, CDialogEx)
    ON_BN_CLICKED(IDC_CHK_FEATURE_DISPLAY, &CTabDisplayDlg::OnBnClickedChkFeatureDisplay)
END_MESSAGE_MAP()

BOOL CTabDisplayDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Default to Display Feature Active
    m_chkFeatureDisplay.SetCheck(BST_CHECKED);

    // Initialize Theme Combo with the 4 Production Themes
    m_comboLogoTheme.ResetContent();
    m_comboLogoTheme.AddString(_T("0 - Tactical Amber (Solar Gold)"));
    m_comboLogoTheme.AddString(_T("1 - Cyber Cyan (Cockpit HUD)"));
    m_comboLogoTheme.AddString(_T("2 - Emerald Defense (Military)"));
    m_comboLogoTheme.AddString(_T("3 - Crimson Alert (Warning Red)"));
    m_comboLogoTheme.SetCurSel(OEMPayload::GetDefaultTheme() % 4);

    // Initialize Telemetry Baud Rate Combo
    m_comboTelemBaud.ResetContent();
    m_comboTelemBaud.AddString(_T("9600"));
    m_comboTelemBaud.AddString(_T("19200"));
    m_comboTelemBaud.AddString(_T("38400"));
    m_comboTelemBaud.AddString(_T("115200"));
    m_comboTelemBaud.SetCurSel(0); // 9600 baud standard for PCU

    // Populate factory timing defaults
    CString strBoot;
    strBoot.Format(_T("%u"), OEMPayload::GetDefaultBootSec());
    m_editBootSec.SetWindowText(strBoot);

    m_chkCarousel.SetCheck(BST_CHECKED);

    CString strCycle;
    strCycle.Format(_T("%u"), OEMPayload::GetDefaultCarouselSec());
    m_editCarouselSec.SetWindowText(strCycle);

    return TRUE;
}

bool CTabDisplayDlg::IsDisplayFeatureEnabled() const
{
    return (m_chkFeatureDisplay.GetCheck() == BST_CHECKED);
}

void CTabDisplayDlg::SetDisplayFeatureEnabled(bool bEnable)
{
    m_chkFeatureDisplay.SetCheck(bEnable ? BST_CHECKED : BST_UNCHECKED);
    OnBnClickedChkFeatureDisplay();
}

void CTabDisplayDlg::OnBnClickedChkFeatureDisplay()
{
    BOOL bActive = (m_chkFeatureDisplay.GetCheck() == BST_CHECKED);
    m_comboLogoTheme.EnableWindow(bActive);
    m_editBootSec.EnableWindow(bActive);
    m_chkCarousel.EnableWindow(bActive);
    m_editCarouselSec.EnableWindow(bActive);
    m_comboTelemBaud.EnableWindow(bActive);
}

void CTabDisplayDlg::SetUIEnabled(BOOL bEnable)
{
    m_chkFeatureDisplay.EnableWindow(bEnable);

    BOOL bDisplayActive = (m_chkFeatureDisplay.GetCheck() == BST_CHECKED) && bEnable;
    m_comboLogoTheme.EnableWindow(bDisplayActive);
    m_editBootSec.EnableWindow(bDisplayActive);
    m_chkCarousel.EnableWindow(bDisplayActive);
    m_editCarouselSec.EnableWindow(bDisplayActive);
    m_comboTelemBaud.EnableWindow(bDisplayActive);
}

bool CTabDisplayDlg::CollectConfigFromUI(pcu_config_t& cfg)
{
    // 1. Fill locked OEM vendor parameters from vault
    CString strBrand = OEMPayload::GetBrandTitle();
    strncpy_s(cfg.brand_title, sizeof(cfg.brand_title), (CT2A)strBrand, _TRUNCATE);

    CString strModel = OEMPayload::GetModelName();
    strncpy_s(cfg.model_name, sizeof(cfg.model_name), (CT2A)strModel, _TRUNCATE);

    CString strSerial = OEMPayload::GetSerialPrefix();
    strncpy_s(cfg.serial_number, sizeof(cfg.serial_number), (CT2A)strSerial, _TRUNCATE);

    CString strHw = OEMPayload::GetHardwareVersion();
    strncpy_s(cfg.hardware_version, sizeof(cfg.hardware_version), (CT2A)strHw, _TRUNCATE);

    CString strContact = OEMPayload::GetVendorContact();
    strncpy_s(cfg.vendor_contact, sizeof(cfg.vendor_contact), (CT2A)strContact, _TRUNCATE);

    CString strWeb = OEMPayload::GetVendorWebsite();
    strncpy_s(cfg.vendor_website, sizeof(cfg.vendor_website), (CT2A)strWeb, _TRUNCATE);

    // 2. Fill UI configurable parameters
    int theme = m_comboLogoTheme.GetCurSel();
    cfg.logo_theme = (theme >= 0 && theme <= 3) ? (uint8_t)theme : 0;

    CString str;
    m_editBootSec.GetWindowText(str);
    int bootSec = _ttoi(str);
    cfg.boot_duration_sec = (bootSec >= 1 && bootSec <= 15) ? (uint8_t)bootSec : 3;

    cfg.auto_carousel_enabled = (m_chkCarousel.GetCheck() == BST_CHECKED) ? 1 : 0;

    m_editCarouselSec.GetWindowText(str);
    int carSec = _ttoi(str);
    cfg.carousel_interval_sec = (carSec >= 2 && carSec <= 60) ? (uint8_t)carSec : 5;

    int baudIdx = m_comboTelemBaud.GetCurSel();
    switch (baudIdx)
    {
    case 1: cfg.telemetry_baudrate = 19200; break;
    case 2: cfg.telemetry_baudrate = 38400; break;
    case 3: cfg.telemetry_baudrate = 115200; break;
    case 0:
    default: cfg.telemetry_baudrate = 9600; break;
    }

    return true;
}

void CTabDisplayDlg::LoadConfigToUI(const pcu_config_t& cfg)
{
    bool hasDisplay = (cfg.model_features & MODEL_FEATURE_DISPLAY) != 0;
    m_chkFeatureDisplay.SetCheck(hasDisplay ? BST_CHECKED : BST_UNCHECKED);
    OnBnClickedChkFeatureDisplay();

    m_comboLogoTheme.SetCurSel(cfg.logo_theme <= 3 ? cfg.logo_theme : 0);

    CString str;
    str.Format(_T("%u"), cfg.boot_duration_sec);
    m_editBootSec.SetWindowText(str);

    m_chkCarousel.SetCheck(cfg.auto_carousel_enabled ? BST_CHECKED : BST_UNCHECKED);

    str.Format(_T("%u"), cfg.carousel_interval_sec);
    m_editCarouselSec.SetWindowText(str);

    switch (cfg.telemetry_baudrate)
    {
    case 19200: m_comboTelemBaud.SetCurSel(1); break;
    case 38400: m_comboTelemBaud.SetCurSel(2); break;
    case 115200: m_comboTelemBaud.SetCurSel(3); break;
    case 9600:
    default: m_comboTelemBaud.SetCurSel(0); break;
    }
}
