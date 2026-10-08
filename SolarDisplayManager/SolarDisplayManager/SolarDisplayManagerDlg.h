#pragma once

#include "SerialComm.h"
#include "EspFlasher.h"
#include "pcu_protocol.h"
#include "TabDisplayDlg.h"
#include "TabAppDlg.h"
#include "OEMHeaderCtrl.h"
#include "OEMPayload.h"

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

    // Top GDI+ Banner Control
    COEMHeaderCtrl m_oemHeader;

    // Common Header Controls
    CComboBox     m_comboPort;
    CStatic       m_staticStatus;
    CComboBox     m_comboFlashBaud;
    CProgressCtrl m_progressFlash;
    CButton       m_chkAutoProvision;

    // Tab Control & Child Dialogs
    CTabCtrl        m_tabCtrl;
    CTabDisplayDlg  m_tabDisplay;
    CTabAppDlg      m_tabApp;

    // Common Action Buttons (Out of Tab)
    CButton       m_btnReadConfig;
    CButton       m_btnWriteConfig;
    CButton       m_btnReboot;
    CButton       m_btnFactoryReset;

    // Common Footer Controls
    CEdit         m_editLog;

    // Logic Engines
    CSerialComm     m_comm;
    CEspFlasher     m_flasher;
    bool            m_isFlashing;

public:
    void AppendLog(const CString& text, bool isError = false);
    void SetUIEnabled(BOOL bEnable);
    bool EnsureConnected();
    CSerialComm& GetComm() { return m_comm; }

    // Inter-dialog communication & Device Commands
    bool ReadConfig(pcu_config_t& cfg);
    bool WriteConfig(const pcu_config_t& cfg);
    bool RebootDevice();
    bool FactoryResetDevice();
    bool ProvisionWifi(const CString& ssid, const CString& pass);
    bool ScanWifi(std::vector<wifi_scan_ap_record_t>& outAps);
    bool QueryIotStatus(hrf_iot_status_payload_t& outStatus);

protected:
    void RefreshComPorts();

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    // Event Handlers
    afx_msg void OnBnClickedBtnRefreshPorts();
    afx_msg void OnBnClickedBtnConnect();
    afx_msg void OnBnClickedBtnFlash();
    afx_msg void OnTcnSelchangeTabMain(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedBtnReadConfig();
    afx_msg void OnBnClickedBtnWriteConfig();
    afx_msg void OnBnClickedBtnReboot();
    afx_msg void OnBnClickedBtnFactoryReset();
    afx_msg void OnBnClickedBtnClearLog();
    afx_msg LRESULT OnFlashProgress(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnFlashComplete(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};
