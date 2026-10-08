#pragma once

#include "pcu_protocol.h"
#include "OEMPayload.h"

class CSolarDisplayManagerDlg;

class CTabDisplayDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CTabDisplayDlg)

public:
    CTabDisplayDlg(CWnd* pParent = nullptr);
    virtual ~CTabDisplayDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_TAB_DISPLAY };
#endif

    void SetMainDlg(CSolarDisplayManagerDlg* pMainDlg) { m_pMainDlg = pMainDlg; }
    void SetUIEnabled(BOOL bEnable);

    bool IsDisplayFeatureEnabled() const;
    void SetDisplayFeatureEnabled(bool bEnable);

    bool CollectConfigFromUI(pcu_config_t& cfg);
    void LoadConfigToUI(const pcu_config_t& cfg);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}

    CSolarDisplayManagerDlg* m_pMainDlg;

    // Control Variables
    CButton      m_chkFeatureDisplay;
    CComboBox    m_comboLogoTheme;
    CEdit        m_editBootSec;
    CButton      m_chkCarousel;
    CEdit        m_editCarouselSec;
    CComboBox    m_comboTelemBaud;

    afx_msg void OnBnClickedChkFeatureDisplay();

    DECLARE_MESSAGE_MAP()
};
