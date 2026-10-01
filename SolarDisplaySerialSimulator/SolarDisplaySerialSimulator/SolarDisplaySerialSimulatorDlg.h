#pragma once

#include "SerialComm.h"
#include "pcu_protocol.h"
#include <afxwin.h>
#include <afxcmn.h>

class CSolarDisplaySerialSimulatorDlg : public CDialogEx
{
public:
    CSolarDisplaySerialSimulatorDlg(CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SOLARDISPLAYSERIALSIMULATOR_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnDestroy();

    // Event Handlers
    afx_msg void OnBnClickedBtnRefresh();
    afx_msg void OnBnClickedBtnConnect();
    afx_msg void OnCbnSelchangeComboPort();
    afx_msg void OnRadioModeChanged();
    afx_msg void OnBnClickedBtnSendNow();
    afx_msg void OnBnClickedChkAutoSend();
    afx_msg void OnCbnSelchangeComboInterval();
    afx_msg void OnBnClickedPresetSunny();
    afx_msg void OnBnClickedPresetNight();
    afx_msg void OnBnClickedPresetOutage();
    afx_msg void OnBnClickedPresetOverload();
    afx_msg void OnBnClickedPresetSweep();
    afx_msg void OnBnClickedPresetShare();
    afx_msg void OnBnClickedPresetGridFailLow();
    afx_msg void OnBnClickedPresetGridFailHigh();
    afx_msg void OnBnClickedBtnPage0();
    afx_msg void OnBnClickedBtnPage1();
    afx_msg void OnBnClickedBtnPage2();
    afx_msg void OnBnClickedBtnPage3();
    afx_msg void OnBnClickedBtnPage4();
    afx_msg void OnBnClickedBtnPage5();
    afx_msg void OnBnClickedBtnClearLog();
    afx_msg void OnBnClickedBtnTriggerFault();
    afx_msg void OnBnClickedBtnClearFault();
    afx_msg void OnBnClickedBtnSendLimits();
    afx_msg void OnCbnSelchangeComboFaultCode();

    DECLARE_MESSAGE_MAP()

private:
    HICON m_hIcon;

    // Controls
    CComboBox    m_comboPort;
    CComboBox    m_comboBaud;
    CStatic      m_staticStatus;
    CButton      m_btnConnect;
    int          m_nMode; // 0 = Inverter TX, 1 = Virtual ESP32 Echo

    CSliderCtrl  m_sliderMains;
    CEdit        m_editMains;
    CSliderCtrl  m_sliderSolar;
    CEdit        m_editSolar;
    CSliderCtrl  m_sliderBatt;
    CEdit        m_editBatt;
    CSliderCtrl  m_sliderAcOut;
    CEdit        m_editAcOut;
    CSliderCtrl  m_sliderLoad;
    CEdit        m_editLoad;
    CSliderCtrl  m_sliderChg;
    CEdit        m_editChg;
    CSliderCtrl  m_sliderDisch;
    CEdit        m_editDisch;
    CSliderCtrl  m_sliderDcBoost;
    CEdit        m_editDcBoost;
    CSliderCtrl  m_sliderHeat;
    CEdit        m_editHeat;

    CComboBox    m_comboOnFlag;
    CComboBox    m_comboSolarState;
    CComboBox    m_comboChgState;
    CComboBox    m_comboFeedMode;
    CComboBox    m_comboSwitchState;
    CButton      m_chkDcBoostMode;
    CButton      m_chkDcOk;
    CButton      m_chkShareMode;
    CButton      m_chkBatGravity;

    // Fault Injection Controls
    CComboBox    m_comboFaultCode;

    // Dynamic Limits ($LIMITS) Controls
    CEdit        m_editBatFul;
    CEdit        m_editBatWrn;
    CEdit        m_editBatLo;
    CEdit        m_editBatRst;
    CEdit        m_editMainsLo;
    CEdit        m_editMainsHi;
    CEdit        m_editHiHeat;
    CEdit        m_editSolMax;
    CEdit        m_editSolMin;
    CEdit        m_editDcMax;

    CButton      m_chkAutoSend;
    CComboBox    m_comboInterval;
    CStatic      m_staticPreview;
    CStatic      m_staticCalcStatus;

    CEdit        m_editLog;
    CButton      m_chkAutoScroll;

    // State Variables
    float        m_fMains;
    float        m_fSolar;
    float        m_fBatt;
    float        m_fAcOut;
    float        m_fLoad;
    float        m_fChg;
    float        m_fDisch;
    float        m_fDcBoost;
    float        m_fHeat;

    bool         m_bDynamicSweep;
    float        m_fSweepAngle;

    // Serial Communication
    CSerialComm  m_comm;
    CWinThread*  m_pRxThread;
    HANDLE       m_hStopRxEvent;

    // Virtual Target (ESP32) Simulated State
    pcu_config_t m_virtualCfg;

    // Methods
    void RefreshComPorts();
    void UpdateSliderFromEdit(int sliderId, int editId, float val, float minVal, float maxVal, float scale);
    void UpdateEditFromSlider(int sliderId, int editId, float minVal, float maxVal, float scale, float& outVal);
    void SyncAllValuesFromSliders();
    CStringA GenerateTelemetryPacket();
    void UpdatePacketPreview();
    void SendCurrentTelemetry();
    void SendPageCommand(int pageNum);
    void AppendLog(const CString& text, bool isError = false, bool isTx = false, bool isRx = false);

    static UINT RxWorkerThreadProc(LPVOID pParam);
    void ProcessRxBuffer(const uint8_t* buf, DWORD len);
};
