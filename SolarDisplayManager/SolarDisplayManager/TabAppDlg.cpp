#include "pch.h"
#include "SolarDisplayManager.h"
#include "TabAppDlg.h"
#include "SolarDisplayManagerDlg.h"
#include "OEMPayload.h"
#include "afxdialogex.h"

IMPLEMENT_DYNAMIC(CTabAppDlg, CDialogEx)

CTabAppDlg::CTabAppDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_TAB_APP, pParent)
    , m_pMainDlg(nullptr)
{
}

CTabAppDlg::~CTabAppDlg()
{
}

void CTabAppDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CHK_FEATURE_APP, m_chkFeatureApp);
    DDX_Control(pDX, IDC_BTN_FLASH_CERTS, m_btnFlashCerts);

    DDX_Control(pDX, IDC_COMBO_WIFI_SSID, m_comboWifiSsid);
    DDX_Control(pDX, IDC_EDIT_WIFI_PASS, m_editWifiPass);
    DDX_Control(pDX, IDC_CHK_SHOW_PASS, m_chkShowPass);

    DDX_Control(pDX, IDC_EDIT_IOT_WIFI_STATUS, m_editWifiStatus);
    DDX_Control(pDX, IDC_EDIT_IOT_CLOUD_STATUS, m_editCloudStatus);
    DDX_Control(pDX, IDC_EDIT_IOT_THING_ID, m_editThingId);
    DDX_Control(pDX, IDC_EDIT_IOT_MAC, m_editMac);
}

BEGIN_MESSAGE_MAP(CTabAppDlg, CDialogEx)
    ON_BN_CLICKED(IDC_CHK_FEATURE_APP, &CTabAppDlg::OnBnClickedChkFeatureApp)
    ON_BN_CLICKED(IDC_BTN_FLASH_CERTS, &CTabAppDlg::OnBnClickedBtnFlashCerts)
    ON_BN_CLICKED(IDC_BTN_SCAN_WIFI, &CTabAppDlg::OnBnClickedBtnScanWifi)
    ON_BN_CLICKED(IDC_BTN_PROVISION_WIFI, &CTabAppDlg::OnBnClickedBtnProvisionWifi)
    ON_BN_CLICKED(IDC_CHK_SHOW_PASS, &CTabAppDlg::OnBnClickedChkShowPass)
    ON_BN_CLICKED(IDC_BTN_QUERY_IOT_STATUS, &CTabAppDlg::OnBnClickedBtnQueryIotStatus)
END_MESSAGE_MAP()

BOOL CTabAppDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Default to Mobile App & Cloud Active
    m_chkFeatureApp.SetCheck(BST_CHECKED);

    // Initial Status Fields
    m_editWifiStatus.SetWindowText(_T("Not Queried"));
    m_editCloudStatus.SetWindowText(_T("Not Queried"));
    m_editThingId.SetWindowText(_T("Not Queried"));
    m_editMac.SetWindowText(_T("Not Queried"));

    return TRUE;
}

bool CTabAppDlg::IsAppFeatureEnabled() const
{
    return (m_chkFeatureApp.GetCheck() == BST_CHECKED);
}

void CTabAppDlg::SetAppFeatureEnabled(bool bEnable)
{
    m_chkFeatureApp.SetCheck(bEnable ? BST_CHECKED : BST_UNCHECKED);
    OnBnClickedChkFeatureApp();
}

void CTabAppDlg::OnBnClickedChkFeatureApp()
{
    BOOL bActive = (m_chkFeatureApp.GetCheck() == BST_CHECKED);

    m_btnFlashCerts.EnableWindow(bActive);
    m_comboWifiSsid.EnableWindow(bActive);
    m_editWifiPass.EnableWindow(bActive);
    m_chkShowPass.EnableWindow(bActive);

    CWnd* pBtn;
    pBtn = GetDlgItem(IDC_BTN_SCAN_WIFI);        if (pBtn) pBtn->EnableWindow(bActive);
    pBtn = GetDlgItem(IDC_BTN_PROVISION_WIFI);   if (pBtn) pBtn->EnableWindow(bActive);
    pBtn = GetDlgItem(IDC_BTN_QUERY_IOT_STATUS); if (pBtn) pBtn->EnableWindow(bActive);
}

void CTabAppDlg::SetUIEnabled(BOOL bEnable)
{
    m_chkFeatureApp.EnableWindow(bEnable);

    BOOL bAppActive = (m_chkFeatureApp.GetCheck() == BST_CHECKED) && bEnable;

    m_btnFlashCerts.EnableWindow(bAppActive);
    m_comboWifiSsid.EnableWindow(bAppActive);
    m_editWifiPass.EnableWindow(bAppActive);
    m_chkShowPass.EnableWindow(bAppActive);

    CWnd* pBtn;
    pBtn = GetDlgItem(IDC_BTN_SCAN_WIFI);        if (pBtn) pBtn->EnableWindow(bAppActive);
    pBtn = GetDlgItem(IDC_BTN_PROVISION_WIFI);   if (pBtn) pBtn->EnableWindow(bAppActive);
    pBtn = GetDlgItem(IDC_BTN_QUERY_IOT_STATUS); if (pBtn) pBtn->EnableWindow(bAppActive);
}

void CTabAppDlg::UpdateFeaturesFromConfig(uint8_t modelFeatures)
{
    bool hasApp = (modelFeatures & MODEL_FEATURE_MOBILE_APP) != 0;
    m_chkFeatureApp.SetCheck(hasApp ? BST_CHECKED : BST_UNCHECKED);
    OnBnClickedChkFeatureApp();
}

void CTabAppDlg::LoadVendorDetails(const pcu_config_t& /*cfg*/)
{
}

void CTabAppDlg::CollectVendorDetails(pcu_config_t& cfg)
{
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
}

void CTabAppDlg::OnBnClickedBtnFlashCerts()
{
    if (m_pMainDlg) {
        m_pMainDlg->AppendLog(_T("[CERT_FLASH] Certificate Flasher button triggered. Staging logic hook ready."));
    }
    AfxMessageBox(_T("Certificate Flasher:\nTLS 1.2 mTLS Device Certificate & Private Key flashing interface.\n(Integration logic hook ready for production staging)"),
        MB_ICONINFORMATION);
}

void CTabAppDlg::OnBnClickedBtnScanWifi()
{
    if (!m_pMainDlg) return;

    m_pMainDlg->AppendLog(_T("[WIFI_SCAN] Requesting ESP32 to scan surrounding Wi-Fi networks..."));
    std::vector<wifi_scan_ap_record_t> aps;
    if (m_pMainDlg->ScanWifi(aps))
    {
        CString curText;
        m_comboWifiSsid.GetWindowText(curText);

        m_comboWifiSsid.ResetContent();
        for (const auto& ap : aps)
        {
            CString item;
            item.Format(_T("%s (%d dBm)"), (LPCTSTR)CA2T(ap.ssid), ap.rssi);
            m_comboWifiSsid.AddString(item);
        }

        if (!curText.IsEmpty())
            m_comboWifiSsid.SetWindowText(curText);

        m_pMainDlg->AppendLog(_T("SUCCESS: Wi-Fi networks populated in SSID dropdown."));
    }
}

void CTabAppDlg::OnBnClickedBtnProvisionWifi()
{
    if (!m_pMainDlg) return;

    CString ssid;
    m_comboWifiSsid.GetWindowText(ssid);

    // If formatted like "SSID (-50 dBm)", extract base SSID
    int paren = ssid.Find(_T(" ("));
    if (paren > 0) {
        ssid = ssid.Left(paren);
    }

    CString pass;
    m_editWifiPass.GetWindowText(pass);

    if (ssid.IsEmpty()) {
        AfxMessageBox(_T("Please select or enter a Wi-Fi SSID."), MB_ICONWARNING);
        return;
    }

    if (m_pMainDlg->ProvisionWifi(ssid, pass)) {
        AfxMessageBox(_T("Wi-Fi credentials successfully provisioned to ESP32 NVS!"), MB_ICONINFORMATION);
    }
}

void CTabAppDlg::OnBnClickedChkShowPass()
{
    BOOL show = (m_chkShowPass.GetCheck() == BST_CHECKED);
    m_editWifiPass.SetPasswordChar(show ? 0 : '*');
    m_editWifiPass.Invalidate();
}

void CTabAppDlg::OnBnClickedBtnQueryIotStatus()
{
    if (!m_pMainDlg) return;

    hrf_iot_status_payload_t status;
    if (m_pMainDlg->QueryIotStatus(status))
    {
        CString wifiStr;
        if (status.wifi_state) {
            wifiStr.Format(_T("Connected (%d dBm, IP: %s)"), status.wifi_rssi, (LPCTSTR)CA2T(status.ip_addr));
        } else {
            wifiStr = _T("Disconnected");
        }
        m_editWifiStatus.SetWindowText(wifiStr);
        m_editCloudStatus.SetWindowText(status.aws_mqtt_state ? _T("Connected") : _T("Disconnected"));
        m_editThingId.SetWindowText(CA2T(status.thing_id));
        m_editMac.SetWindowText(CA2T(status.mac_addr));
    }
}
