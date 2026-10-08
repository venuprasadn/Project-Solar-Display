#pragma once

#include "pcu_protocol.h"
#include <vector>

class CSolarDisplayManagerDlg;

class CTabAppDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CTabAppDlg)

public:
    CTabAppDlg(CWnd* pParent = nullptr);
    virtual ~CTabAppDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_TAB_APP };
#endif

    void SetMainDlg(CSolarDisplayManagerDlg* pMainDlg) { m_pMainDlg = pMainDlg; }
    void SetUIEnabled(BOOL bEnable);

    bool IsAppFeatureEnabled() const;
    void SetAppFeatureEnabled(bool bEnable);
    void UpdateFeaturesFromConfig(uint8_t modelFeatures);

    void LoadVendorDetails(const pcu_config_t& cfg);
    void CollectVendorDetails(pcu_config_t& cfg);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}

    CSolarDisplayManagerDlg* m_pMainDlg;

    // Feature Flags & Cert Controls
    CButton      m_chkFeatureApp;
    CButton      m_btnFlashCerts;

    // Wi-Fi Controls
    CComboBox    m_comboWifiSsid;
    CEdit        m_editWifiPass;
    CButton      m_chkShowPass;

    // Cloud / IoT Status Readouts
    CEdit        m_editWifiStatus;
    CEdit        m_editCloudStatus;
    CEdit        m_editThingId;
    CEdit        m_editMac;

    afx_msg void OnBnClickedChkFeatureApp();
    afx_msg void OnBnClickedBtnFlashCerts();
    afx_msg void OnBnClickedBtnScanWifi();
    afx_msg void OnBnClickedBtnProvisionWifi();
    afx_msg void OnBnClickedChkShowPass();
    afx_msg void OnBnClickedBtnQueryIotStatus();

    DECLARE_MESSAGE_MAP()
};
