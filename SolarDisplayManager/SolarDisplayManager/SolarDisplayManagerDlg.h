#pragma once

#include "SerialComm.h"
#include "EspFlasher.h"
#include "pcu_protocol.h"

// CSolarDisplayManagerDlg dialog
class CSolarDisplayManagerDlg : public CDialogEx
{
// Construction
public:
    CSolarDisplayManagerDlg(CWnd* pParent = nullptr);

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SOLARDISPLAYMANAGER_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

// Implementation
protected:
    HICON m_hIcon;

    // Control Variables
    CComboBox    m_comboPort;
    CStatic      m_staticStatus;
    CEdit        m_editAppBin;
    CComboBox    m_comboFlashBaud;
    CProgressCtrl m_progressFlash;
    CButton      m_chkAutoProvision;

    CEdit        m_editBrand;
    CEdit        m_editModel;
    CEdit        m_editSerial;
    CEdit        m_editHwRev;
    CEdit        m_editVendorContact;
    CEdit        m_editVendorWebsite;
    CComboBox    m_comboLogoTheme;
    CEdit        m_editBootSec;
    CButton      m_chkCarousel;
    CEdit        m_editCarouselSec;
    CComboBox    m_comboTelemBaud;
    CEdit        m_editLogoPath;

    CEdit        m_editLog;

    // Logic Engines
    CSerialComm  m_comm;
    CEspFlasher  m_flasher;
    bool         m_isFlashing;

public:
    void AppendLog(const CString& text, bool isError = false);
    bool CollectConfigFromUI(pcu_config_t& cfg);
    void SetUIEnabled(BOOL bEnable);
    static bool ConvertImageToLogoBin(const CString& imagePath, std::vector<uint8_t>& outLogoData, CString& outError);

protected:
    bool EnsureConnected();
    void RefreshComPorts();
    void LoadConfigToUI(const pcu_config_t& cfg);

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    // Event Handlers
    afx_msg void OnBnClickedBtnRefreshPorts();
    afx_msg void OnBnClickedBtnConnect();
    afx_msg void OnBnClickedBtnBrowseApp();
    afx_msg void OnBnClickedBtnFlash();
    afx_msg void OnBnClickedBtnBrowseLogo();
    afx_msg void OnBnClickedBtnFlashLogo();
    afx_msg void OnBnClickedBtnClearLogo();
    afx_msg void OnBnClickedBtnReadConfig();
    afx_msg void OnBnClickedBtnWriteConfig();
    afx_msg void OnBnClickedBtnReboot();
    afx_msg void OnBnClickedBtnFactoryReset();
    afx_msg void OnBnClickedBtnClearLog();
    afx_msg LRESULT OnFlashProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFlashComplete(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};
