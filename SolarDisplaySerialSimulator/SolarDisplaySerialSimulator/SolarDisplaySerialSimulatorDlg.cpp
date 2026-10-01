#include "pch.h"
#include "framework.h"
#include "SolarDisplaySerialSimulator.h"
#include "SolarDisplaySerialSimulatorDlg.h"
#include "afxdialogex.h"
#include <cmath>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
void CAboutDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CSolarDisplaySerialSimulatorDlg
CSolarDisplaySerialSimulatorDlg::CSolarDisplaySerialSimulatorDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SOLARDISPLAYSERIALSIMULATOR_DIALOG, pParent)
    , m_nMode(0)
    , m_fMains(230.0f)
    , m_fSolar(76.5f)
    , m_fBatt(26.8f)
    , m_fAcOut(230.0f)
    , m_fLoad(42.0f)
    , m_fChg(16.4f)
    , m_fDisch(0.0f)
    , m_fDcBoost(385.0f)
    , m_fHeat(38.5f)
    , m_bDynamicSweep(false)
    , m_fSweepAngle(0.0f)
    , m_pRxThread(nullptr)
    , m_hStopRxEvent(nullptr)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    // Initialize Default Virtual Configuration (for Virtual Target Echo mode)
    memset(&m_virtualCfg, 0, sizeof(m_virtualCfg));
    m_virtualCfg.magic = PCU_CONFIG_MAGIC;
    m_virtualCfg.version = PCU_CONFIG_VERSION;
    m_virtualCfg.struct_size = sizeof(pcu_config_t);
    m_virtualCfg.is_configured = 1;
    strcpy_s(m_virtualCfg.brand_title, "DONPOWER SOLAR");
    strcpy_s(m_virtualCfg.model_name, "HYBRID MPPT PCU");
    strcpy_s(m_virtualCfg.serial_number, "DP-2026-X8849");
    strcpy_s(m_virtualCfg.hardware_version, "HW-V2.1");
    strcpy_s(m_virtualCfg.vendor_contact, "Toll Free: 1800-425-9999");
    strcpy_s(m_virtualCfg.vendor_website, "www.donpower.in");
    m_virtualCfg.production_date = 20261001;
    m_virtualCfg.logo_theme = 2; // Emerald Defense
    m_virtualCfg.boot_duration_sec = 3;
    m_virtualCfg.auto_carousel_enabled = 1;
    m_virtualCfg.carousel_interval_sec = 5;
    m_virtualCfg.telemetry_baudrate = 9600;
    m_virtualCfg.backlight_brightness = 100;
    m_virtualCfg.config_crc32 = pcu_calc_crc32((const uint8_t*)&m_virtualCfg, offsetof(pcu_config_t, config_crc32));
}

void CSolarDisplaySerialSimulatorDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_PORT, m_comboPort);
    DDX_Control(pDX, IDC_COMBO_BAUD, m_comboBaud);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_BTN_CONNECT, m_btnConnect);
    DDX_Radio(pDX, IDC_RADIO_MODE_INVERTER, m_nMode);

    DDX_Control(pDX, IDC_SLIDER_MAINS, m_sliderMains);
    DDX_Control(pDX, IDC_EDIT_MAINS, m_editMains);
    DDX_Control(pDX, IDC_SLIDER_SOLAR, m_sliderSolar);
    DDX_Control(pDX, IDC_EDIT_SOLAR, m_editSolar);
    DDX_Control(pDX, IDC_SLIDER_BATT, m_sliderBatt);
    DDX_Control(pDX, IDC_EDIT_BATT, m_editBatt);
    DDX_Control(pDX, IDC_SLIDER_ACOUT, m_sliderAcOut);
    DDX_Control(pDX, IDC_EDIT_ACOUT, m_editAcOut);
    DDX_Control(pDX, IDC_SLIDER_LOAD, m_sliderLoad);
    DDX_Control(pDX, IDC_EDIT_LOAD, m_editLoad);
    DDX_Control(pDX, IDC_SLIDER_CHG, m_sliderChg);
    DDX_Control(pDX, IDC_EDIT_CHG, m_editChg);
    DDX_Control(pDX, IDC_SLIDER_DISCH, m_sliderDisch);
    DDX_Control(pDX, IDC_EDIT_DISCH, m_editDisch);
    DDX_Control(pDX, IDC_SLIDER_DCBOOST, m_sliderDcBoost);
    DDX_Control(pDX, IDC_EDIT_DCBOOST, m_editDcBoost);
    DDX_Control(pDX, IDC_SLIDER_HEAT, m_sliderHeat);
    DDX_Control(pDX, IDC_EDIT_HEAT, m_editHeat);

    DDX_Control(pDX, IDC_COMBO_ONFLAG, m_comboOnFlag);
    DDX_Control(pDX, IDC_COMBO_SOLARSTATE, m_comboSolarState);
    DDX_Control(pDX, IDC_COMBO_CHGSTATE, m_comboChgState);
    DDX_Control(pDX, IDC_COMBO_FEEDMODE, m_comboFeedMode);
    DDX_Control(pDX, IDC_COMBO_SWITCHSTATE, m_comboSwitchState);
    DDX_Control(pDX, IDC_CHK_DCBOOSTMODE, m_chkDcBoostMode);
    DDX_Control(pDX, IDC_CHK_DCOK, m_chkDcOk);
    DDX_Control(pDX, IDC_CHK_SHAREMODE, m_chkShareMode);
    DDX_Control(pDX, IDC_CHK_BATGRAVITY, m_chkBatGravity);

    DDX_Control(pDX, IDC_CHK_AUTO_SEND, m_chkAutoSend);
    DDX_Control(pDX, IDC_COMBO_INTERVAL, m_comboInterval);
    DDX_Control(pDX, IDC_STATIC_PACKET_PREVIEW, m_staticPreview);
    DDX_Control(pDX, IDC_STATIC_CALC_STATUS, m_staticCalcStatus);

    // Fault Injection Controls
    DDX_Control(pDX, IDC_COMBO_FAULTCODE, m_comboFaultCode);

    // Dynamic Limits Controls ($LIMITS)
    DDX_Control(pDX, IDC_EDIT_BAT_FUL, m_editBatFul);
    DDX_Control(pDX, IDC_EDIT_BAT_WRN, m_editBatWrn);
    DDX_Control(pDX, IDC_EDIT_BAT_LO, m_editBatLo);
    DDX_Control(pDX, IDC_EDIT_BAT_RST, m_editBatRst);
    DDX_Control(pDX, IDC_EDIT_MAINS_LO, m_editMainsLo);
    DDX_Control(pDX, IDC_EDIT_MAINS_HI, m_editMainsHi);
    DDX_Control(pDX, IDC_EDIT_HI_HEAT, m_editHiHeat);
    DDX_Control(pDX, IDC_EDIT_SOL_MAX, m_editSolMax);
    DDX_Control(pDX, IDC_EDIT_SOL_MIN, m_editSolMin);
    DDX_Control(pDX, IDC_EDIT_DC_MAX, m_editDcMax);

    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
    DDX_Control(pDX, IDC_CHK_AUTOSCROLL, m_chkAutoScroll);
}

BEGIN_MESSAGE_MAP(CSolarDisplaySerialSimulatorDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_HSCROLL()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnRefresh)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnConnect)
    ON_CBN_SELCHANGE(IDC_COMBO_PORT, &CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboPort)
    ON_BN_CLICKED(IDC_RADIO_MODE_INVERTER, &CSolarDisplaySerialSimulatorDlg::OnRadioModeChanged)
    ON_BN_CLICKED(IDC_RADIO_MODE_ECHO, &CSolarDisplaySerialSimulatorDlg::OnRadioModeChanged)
    ON_BN_CLICKED(IDC_BTN_SEND_NOW, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_BN_CLICKED(IDC_CHK_AUTO_SEND, &CSolarDisplaySerialSimulatorDlg::OnBnClickedChkAutoSend)
    ON_CBN_SELCHANGE(IDC_COMBO_INTERVAL, &CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboInterval)
    ON_BN_CLICKED(IDC_BTN_PRESET_SUNNY, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetSunny)
    ON_BN_CLICKED(IDC_BTN_PRESET_NIGHT, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetNight)
    ON_BN_CLICKED(IDC_BTN_PRESET_OUTAGE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetOutage)
    ON_BN_CLICKED(IDC_BTN_PRESET_OVERLOAD, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetOverload)
    ON_BN_CLICKED(IDC_BTN_PRESET_SWEEP, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetSweep)
    ON_BN_CLICKED(IDC_BTN_PRESET_SHARE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetShare)
    ON_BN_CLICKED(IDC_BTN_PRESET_GRIDFAIL_LOW, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetGridFailLow)
    ON_BN_CLICKED(IDC_BTN_PRESET_GRIDFAIL_HIGH, &CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetGridFailHigh)
    ON_BN_CLICKED(IDC_BTN_PAGE0, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage0)
    ON_BN_CLICKED(IDC_BTN_PAGE1, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage1)
    ON_BN_CLICKED(IDC_BTN_PAGE2, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage2)
    ON_BN_CLICKED(IDC_BTN_PAGE3, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage3)
    ON_BN_CLICKED(IDC_BTN_PAGE4, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage4)
    ON_BN_CLICKED(IDC_BTN_PAGE5, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage5)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnClearLog)
    ON_BN_CLICKED(IDC_BTN_TRIGGER_FAULT, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnTriggerFault)
    ON_BN_CLICKED(IDC_BTN_CLEAR_FAULT, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnClearFault)
    ON_BN_CLICKED(IDC_BTN_SEND_LIMITS, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendLimits)
    ON_CBN_SELCHANGE(IDC_COMBO_FAULTCODE, &CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboFaultCode)
    ON_CBN_SELCHANGE(IDC_COMBO_ONFLAG, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_CBN_SELCHANGE(IDC_COMBO_SOLARSTATE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_CBN_SELCHANGE(IDC_COMBO_CHGSTATE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_CBN_SELCHANGE(IDC_COMBO_FEEDMODE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_CBN_SELCHANGE(IDC_COMBO_SWITCHSTATE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_BN_CLICKED(IDC_CHK_DCBOOSTMODE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_BN_CLICKED(IDC_CHK_DCOK, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_BN_CLICKED(IDC_CHK_SHAREMODE, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
    ON_BN_CLICKED(IDC_CHK_BATGRAVITY, &CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow)
END_MESSAGE_MAP()

BOOL CSolarDisplaySerialSimulatorDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // 1. Baud Rate Dropdown
    m_comboBaud.AddString(_T("9600"));
    m_comboBaud.AddString(_T("19200"));
    m_comboBaud.AddString(_T("38400"));
    m_comboBaud.AddString(_T("57600"));
    m_comboBaud.AddString(_T("115200"));
    m_comboBaud.SetCurSel(0); // 9600 default (Inverter port)

    // 2. Broadcast Interval Dropdown
    m_comboInterval.AddString(_T("1000 ms (1 Hz)"));
    m_comboInterval.AddString(_T("500 ms (2 Hz)"));
    m_comboInterval.AddString(_T("250 ms (4 Hz)"));
    m_comboInterval.AddString(_T("100 ms (10 Hz)"));
    m_comboInterval.SetCurSel(0); // 1000ms

    // 3. Dropdown Options for Operational States
    m_comboFeedMode.AddString(_T("0 - Auto (Physics-Based Routing)"));
    m_comboFeedMode.AddString(_T("1 - FEED: SOLAR PV"));
    m_comboFeedMode.AddString(_T("2 - FEED: BATTERY"));
    m_comboFeedMode.AddString(_T("3 - FEED: MAINS GRID"));
    m_comboFeedMode.AddString(_T("4 - FEED: SMART SHARE"));
    m_comboFeedMode.SetCurSel(0); // Auto

    m_comboOnFlag.AddString(_T("0 - Mains Grid Bypass"));
    m_comboOnFlag.AddString(_T("1 - Inverter Active"));
    m_comboOnFlag.SetCurSel(1); // Inverter

    m_comboSwitchState.AddString(_T("1 - Inverter Switch ON (Active)"));
    m_comboSwitchState.AddString(_T("0 - Inverter Switch OFF (Shutdown)"));
    m_comboSwitchState.SetCurSel(0); // Switch ON

    m_comboSolarState.AddString(_T("0 - Solar PV Off"));
    m_comboSolarState.AddString(_T("1 - Solar PV Active"));
    m_comboSolarState.SetCurSel(1); // Solar Active

    m_comboChgState.AddString(_T("0 - Charger Off"));
    m_comboChgState.AddString(_T("1 - AC Mains Charger"));
    m_comboChgState.AddString(_T("2 - Solar MPPT Charger"));
    m_comboChgState.AddString(_T("3 - Smart Share Charger"));
    m_comboChgState.SetCurSel(2); // Solar MPPT

    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);
    m_chkAutoScroll.SetCheck(BST_CHECKED);

    // Initialize Fault Codes Dropdown
    m_comboFaultCode.ResetContent();
    m_comboFaultCode.AddString(_T("00: FAULT_CLEARED (Normal)"));
    m_comboFaultCode.AddString(_T("01: SHORT_TRIP (Output Short)"));
    m_comboFaultCode.AddString(_T("02: NOFEED_TRIP (Grid Sync Lost)"));
    m_comboFaultCode.AddString(_T("03: HEATOVER_TRIP (Heatsink Overtemp)"));
    m_comboFaultCode.AddString(_T("04: OVERLOAD_TRIP (Inverter Overload)"));
    m_comboFaultCode.AddString(_T("05: DC_LO_TRIP (DC Bus Undervolt)"));
    m_comboFaultCode.AddString(_T("06: HI_CURRENT_TRIP (Peak High Current)"));
    m_comboFaultCode.AddString(_T("07: SOLAR_HIGH (Solar Voc Overvolt)"));
    m_comboFaultCode.AddString(_T("08: DC_HI_TRIP (DC Bus Overvolt)"));
    m_comboFaultCode.AddString(_T("09: DC_FAIL_TRIP (DC Boost Fail)"));
    m_comboFaultCode.AddString(_T("20: OVERLOAD_WARN (Load Warning)"));
    m_comboFaultCode.AddString(_T("30: LOWBATT_WARN (Low Battery Warning)"));
    m_comboFaultCode.AddString(_T("40: LOWBAT_TRIP (Battery Deep Cutoff)"));
    m_comboFaultCode.SetCurSel(0);

    // Initialize Dynamic Calibration Limits Defaults
    m_editBatFul.SetWindowText(_T("28.8"));
    m_editBatWrn.SetWindowText(_T("23.5"));
    m_editBatLo.SetWindowText(_T("21.0"));
    m_editBatRst.SetWindowText(_T("24.5"));
    m_editMainsLo.SetWindowText(_T("185.0"));
    m_editMainsHi.SetWindowText(_T("265.0"));
    m_editHiHeat.SetWindowText(_T("85.0"));
    m_editSolMax.SetWindowText(_T("115.0"));
    m_editSolMin.SetWindowText(_T("15.0"));
    m_editDcMax.SetWindowText(_T("450.0"));

    // 4. Configure Sliders (Ranges with 1-decimal precision using scale factor 10)
    m_sliderMains.SetRange(0, 3000);   // 0.0 to 300.0 V
    m_sliderSolar.SetRange(0, 1200);   // 0.0 to 120.0 V
    m_sliderBatt.SetRange(180, 320);   // 18.0 to 32.0 V
    m_sliderAcOut.SetRange(0, 2500);   // 0.0 to 250.0 V
    m_sliderLoad.SetRange(0, 120);     // 0 to 120 %
    m_sliderChg.SetRange(0, 600);      // 0.0 to 60.0 A
    m_sliderDisch.SetRange(0, 600);    // 0.0 to 60.0 A
    m_sliderDcBoost.SetRange(2000, 4500); // 200.0 to 450.0 V
    m_sliderHeat.SetRange(150, 950);   // 15.0 to 95.0 C

    // Set Slider positions from default values
    m_sliderMains.SetPos((int)(m_fMains * 10.0f));
    m_sliderSolar.SetPos((int)(m_fSolar * 10.0f));
    m_sliderBatt.SetPos((int)(m_fBatt * 10.0f));
    m_sliderAcOut.SetPos((int)(m_fAcOut * 10.0f));
    m_sliderLoad.SetPos((int)m_fLoad);
    m_sliderChg.SetPos((int)(m_fChg * 10.0f));
    m_sliderDisch.SetPos((int)(m_fDisch * 10.0f));
    m_sliderDcBoost.SetPos((int)(m_fDcBoost * 10.0f));
    m_sliderHeat.SetPos((int)(m_fHeat * 10.0f));

    SyncAllValuesFromSliders();
    UpdatePacketPreview();

    // 5. Enumerate COM Ports
    RefreshComPorts();

    AppendLog(_T("=== Solar PCU Serial Simulator Initialized ==="));
    AppendLog(_T("Ready. Select target COM port and click 'Connect'."));

    return TRUE;
}

void CSolarDisplaySerialSimulatorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CSolarDisplaySerialSimulatorDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CSolarDisplaySerialSimulatorDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CSolarDisplaySerialSimulatorDlg::OnDestroy()
{
    KillTimer(1);

    if (m_hStopRxEvent) {
        SetEvent(m_hStopRxEvent);
        if (m_pRxThread) {
            WaitForSingleObject(m_pRxThread->m_hThread, 1000);
            m_pRxThread = nullptr;
        }
        CloseHandle(m_hStopRxEvent);
        m_hStopRxEvent = nullptr;
    }

    m_comm.Close();
    CDialogEx::OnDestroy();
}

void CSolarDisplaySerialSimulatorDlg::RefreshComPorts()
{
    m_comboPort.ResetContent();
    std::vector<CString> ports = CSerialComm::EnumeratePorts();
    for (const auto& p : ports) {
        m_comboPort.AddString(p);
    }
    if (m_comboPort.GetCount() > 0) {
        m_comboPort.SetCurSel(0);
        OnCbnSelchangeComboPort();
    }
}

void CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboPort()
{
    CString portName;
    m_comboPort.GetWindowText(portName);
    if (portName.IsEmpty()) {
        int sel = m_comboPort.GetCurSel();
        if (sel >= 0) m_comboPort.GetLBText(sel, portName);
    }

    if (portName.CompareNoCase(_T("COM3")) == 0) {
        // COM3 is ESP32 Onboard Direct USB (UART0) -> 115200 baud default
        int idx = m_comboBaud.FindStringExact(0, _T("115200"));
        if (idx != CB_ERR) m_comboBaud.SetCurSel(idx);
        AppendLog(_T("Target: COM3 (ESP32 USB Service Port) -> Defaulted to 115200 baud."));
    } else {
        // COM4 or other serial adapter connected to UART2 (GPIO 16/17) -> 9600 baud default
        int idx = m_comboBaud.FindStringExact(0, _T("9600"));
        if (idx != CB_ERR) m_comboBaud.SetCurSel(idx);
        AppendLog(_T("Target: ") + portName + _T(" (Inverter Field Port / UART2) -> Defaulted to 9600 baud."));
    }
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnRefresh()
{
    RefreshComPorts();
    AppendLog(_T("Refreshed COM port list."));
}

void CSolarDisplaySerialSimulatorDlg::OnRadioModeChanged()
{
    UpdateData(TRUE);
    if (m_nMode == 0) {
        OnCbnSelchangeComboPort();
        AppendLog(_T("Mode changed to: TRANSMIT TELEMETRY STREAM ($PCU Live Broadcast)"));
    } else {
        int idx = m_comboBaud.FindStringExact(0, _T("115200"));
        if (idx != CB_ERR) m_comboBaud.SetCurSel(idx);
        AppendLog(_T("Mode changed to: VIRTUAL TARGET ECHO (Virtual ESP32 Protocol Simulator)"));
    }
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnConnect()
{
    if (m_comm.IsOpen()) {
        KillTimer(1);
        m_chkAutoSend.SetCheck(BST_UNCHECKED);

        if (m_hStopRxEvent) {
            SetEvent(m_hStopRxEvent);
            if (m_pRxThread) {
                WaitForSingleObject(m_pRxThread->m_hThread, 1000);
                m_pRxThread = nullptr;
            }
            CloseHandle(m_hStopRxEvent);
            m_hStopRxEvent = nullptr;
        }

        m_comm.Close();
        m_btnConnect.SetWindowText(_T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Disconnected"));
        AppendLog(_T("Closed serial connection."));
        return;
    }

    CString portName;
    m_comboPort.GetWindowText(portName);
    if (portName.IsEmpty()) {
        AfxMessageBox(_T("Please select a target COM port."));
        return;
    }

    CString strBaud;
    m_comboBaud.GetWindowText(strBaud);
    DWORD baud = (DWORD)_tstol(strBaud);
    if (baud == 0) baud = 9600;

    if (!m_comm.Open(portName, baud)) {
        AppendLog(m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(m_comm.GetLastErrorMsg(), MB_ICONERROR);
        return;
    }

    // Start Background RX Thread
    m_hStopRxEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_pRxThread = AfxBeginThread(RxWorkerThreadProc, this);

    CString modeStr = (m_nMode == 0) ? _T("Inverter TX") : _T("Virtual ESP32 Echo");
    CString statStr;
    statStr.Format(_T("Status: Connected to %s (%u baud) [%s]"), (LPCTSTR)portName, baud, (LPCTSTR)modeStr);
    m_staticStatus.SetWindowText(statStr);
    m_btnConnect.SetWindowText(_T("Disconnect"));

    AppendLog(CString(_T("Connected to ")) + portName + _T(" at ") + strBaud + _T(" baud (") + modeStr + _T(")"));

    if (m_chkAutoSend.GetCheck() == BST_CHECKED) {
        OnBnClickedChkAutoSend();
    }
}

void CSolarDisplaySerialSimulatorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSolarDisplaySerialSimulatorDlg::SyncAllValuesFromSliders()
{
    m_fMains = (float)m_sliderMains.GetPos() / 10.0f;
    m_fSolar = (float)m_sliderSolar.GetPos() / 10.0f;
    m_fBatt = (float)m_sliderBatt.GetPos() / 10.0f;
    m_fAcOut = (float)m_sliderAcOut.GetPos() / 10.0f;
    m_fLoad = (float)m_sliderLoad.GetPos();
    m_fChg = (float)m_sliderChg.GetPos() / 10.0f;
    m_fDisch = (float)m_sliderDisch.GetPos() / 10.0f;
    m_fDcBoost = (float)m_sliderDcBoost.GetPos() / 10.0f;
    m_fHeat = (float)m_sliderHeat.GetPos() / 10.0f;

    CString str;
    str.Format(_T("%.1f"), m_fMains); m_editMains.SetWindowText(str);
    str.Format(_T("%.1f"), m_fSolar); m_editSolar.SetWindowText(str);
    str.Format(_T("%.1f"), m_fBatt); m_editBatt.SetWindowText(str);
    str.Format(_T("%.1f"), m_fAcOut); m_editAcOut.SetWindowText(str);
    str.Format(_T("%.0f"), m_fLoad); m_editLoad.SetWindowText(str);
    str.Format(_T("%.1f"), m_fChg); m_editChg.SetWindowText(str);
    str.Format(_T("%.1f"), m_fDisch); m_editDisch.SetWindowText(str);
    str.Format(_T("%.1f"), m_fDcBoost); m_editDcBoost.SetWindowText(str);
    str.Format(_T("%.1f"), m_fHeat); m_editHeat.SetWindowText(str);
}

static int MapFaultComboIndexToCode(int index)
{
    switch (index) {
        case 1: return 1;   // SHORT_TRIP
        case 2: return 2;   // NOFEED_TRIP
        case 3: return 3;   // HEATOVER_TRIP
        case 4: return 4;   // OVERLOAD_TRIP
        case 5: return 5;   // DC_LO_TRIP
        case 6: return 6;   // HI_CURRENT_TRIP
        case 7: return 7;   // SOLAR_HIGH
        case 8: return 8;   // DC_HI_TRIP
        case 9: return 9;   // DC_FAIL_TRIP
        case 10: return 20; // OVERLOAD_WARN
        case 11: return 30; // LOWBATT_WARN
        case 12: return 40; // LOWBAT_TRIP
        default: return 0;  // FAULT_CLEARED
    }
}

CStringA CSolarDisplaySerialSimulatorDlg::GenerateTelemetryPacket()
{
    int onFlag = m_comboOnFlag.GetCurSel(); if (onFlag < 0) onFlag = 1;
    int solarState = m_comboSolarState.GetCurSel(); if (solarState < 0) solarState = 1;
    int chgState = m_comboChgState.GetCurSel(); if (chgState < 0) chgState = 2;
    int dcOk = (m_chkDcOk.GetCheck() == BST_CHECKED) ? 1 : 0;
    int shareMode = (m_chkShareMode.GetCheck() == BST_CHECKED) ? 1 : 0;
    int batGravity = (m_chkBatGravity.GetCheck() == BST_CHECKED) ? 1 : 0;

    int swState = (m_comboSwitchState.GetCurSel() == 1) ? 0 : 1;
    int dcBoostMode = (m_chkDcBoostMode.GetCheck() == BST_CHECKED) ? 1 : 0;
    int feedMode = m_comboFeedMode.GetCurSel(); if (feedMode < 0) feedMode = 0;
    int faultCode = MapFaultComboIndexToCode(m_comboFaultCode.GetCurSel());

    CStringA packet;
    packet.Format("$PCU,%.1f,%.1f,%.1f,%.1f,%.0f,%.1f,%.1f,%.1f,%.1f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
        m_fMains, m_fSolar, m_fBatt, m_fAcOut, m_fLoad,
        m_fChg, m_fDisch, m_fDcBoost, m_fHeat,
        onFlag, solarState, chgState, dcOk, shareMode, batGravity,
        swState, dcBoostMode, feedMode, faultCode);

    return packet;
}

void CSolarDisplaySerialSimulatorDlg::UpdatePacketPreview()
{
    CStringA pkt = GenerateTelemetryPacket();
    CString preview(pkt);
    preview.TrimRight(_T("\r\n"));
    m_staticPreview.SetWindowText(preview);

    int faultCode = MapFaultComboIndexToCode(m_comboFaultCode.GetCurSel());
    if (faultCode != 0) {
        CString strFault;
        strFault.Format(_T("ACTIVE FAULT CODE %d! Protection screen active on LCD. (Clear with 'Clear (0)')."), faultCode);
        m_staticCalcStatus.SetWindowText(strFault);
        return;
    }

    // Calculate display status mirroring ESP32 logic:
    CString strGridStat = (m_fMains < 15.0f) ? _T("OUTAGE") : (m_fMains < 185.0f) ? _T("UNDERVOLT") : (m_fMains > 265.0f) ? _T("OVERVOLT") : _T("GRID OK");
    CString strLoadStat = (m_fLoad > 100.0f) ? _T("OVERLOAD CRITICAL!") : (m_fLoad > 80.0f) ? _T("HIGH LOAD") : _T("NORMAL");
    CString strHeatStat = (m_fHeat > 85.0f) ? _T("OVERTEMP TRIP!") : (m_fHeat > 70.0f) ? _T("FAN 100%") : (m_fHeat > 50.0f) ? _T("FAN 50%") : _T("COOL");
    CString strBattStat = (m_fBatt > 30.5f) ? _T("OVERVOLT TRIP") : (m_fBatt < 21.0f) ? _T("LOW CUTOFF") : (m_fBatt < 23.5f) ? _T("LOW BATT") : _T("HEALTHY");

    int feedMode = m_comboFeedMode.GetCurSel(); if (feedMode < 0) feedMode = 0;
    int onFlag = m_comboOnFlag.GetCurSel(); if (onFlag < 0) onFlag = 1;

    CString strRouting;
    if (feedMode == 1) strRouting = _T("FEED: SOLAR PV");
    else if (feedMode == 2) strRouting = _T("FEED: BATTERY");
    else if (feedMode == 3) strRouting = (m_fMains < 15.0f) ? _T("FEED: OUTAGE OFF") : _T("FEED: MAINS GRID");
    else if (feedMode == 4) strRouting = _T("FEED: SMART SHARE");
    else {
        if (m_fLoad > 100.0f) strRouting = _T("ALARM: OVERLOAD");
        else if (onFlag == 1) {
            if (m_fDisch > 0.0f) strRouting = _T("FEED: BATTERY");
            else if (m_comboSolarState.GetCurSel() == 1 && m_fSolar > 20.0f) strRouting = _T("FEED: SOLAR PV");
            else strRouting = _T("FEED: INVERTER");
        } else {
            strRouting = (m_fMains < 15.0f) ? _T("FEED: OUTAGE OFF") : _T("FEED: MAINS GRID");
        }
    }

    int bPct = (int)(((m_fBatt - 22.0f) / (28.4f - 22.0f)) * 100.0f);
    if (bPct > 100) bPct = 100; if (bPct < 0) bPct = 0;

    CString calcStatus;
    calcStatus.Format(_T("STATUS -> Grid: %s (%.0fV) | Routing: %s | Load: %s (%.0f%%) | Batt: %s (%d%%) | Heat: %s (%.1fC)"),
        strGridStat, m_fMains, strRouting, strLoadStat, m_fLoad, strBattStat, bPct, strHeatStat, m_fHeat);
    m_staticCalcStatus.SetWindowText(calcStatus);
}

void CSolarDisplaySerialSimulatorDlg::SendCurrentTelemetry()
{
    CStringA pkt = GenerateTelemetryPacket();
    if (m_comm.IsOpen()) {
        m_comm.SendString(pkt);
        CString logText(pkt);
        logText.TrimRight(_T("\r\n"));
        AppendLog(logText, false, true, false);
    }
}

void CSolarDisplaySerialSimulatorDlg::SendPageCommand(int pageNum)
{
    CStringA pkt;
    pkt.Format("$PAGE,%d\r\n", pageNum);
    if (m_comm.IsOpen()) {
        m_comm.SendString(pkt);
        CString logText;
        logText.Format(_T("Switched ESP32 to Page %d ($PAGE,%d)"), pageNum, pageNum);
        AppendLog(logText, false, true, false);
    } else {
        AppendLog(_T("Cannot send: Serial port is not open."), true);
    }
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnTriggerFault()
{
    int fCode = MapFaultComboIndexToCode(m_comboFaultCode.GetCurSel());
    CStringA pkt;
    pkt.Format("$FAULT,%d\r\n", fCode);
    if (m_comm.IsOpen()) {
        m_comm.SendString(pkt);
        CString logText;
        logText.Format(_T("Injected Fault Code %d ($FAULT,%d)"), fCode, fCode);
        AppendLog(logText, false, true, false);
    } else {
        AppendLog(_T("Cannot send fault: Serial port is not open."), true);
    }
    UpdatePacketPreview();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnClearFault()
{
    m_comboFaultCode.SetCurSel(0);
    CStringA pkt = "$FAULT,0\r\n";
    if (m_comm.IsOpen()) {
        m_comm.SendString(pkt);
        AppendLog(_T("Sent Fault Cleared ($FAULT,0)"), false, true, false);
    } else {
        AppendLog(_T("Cannot send fault clear: Serial port is not open."), true);
    }
    UpdatePacketPreview();
}

void CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboFaultCode()
{
    UpdatePacketPreview();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendLimits()
{
    CString strBatFul, strBatWrn, strBatLo, strBatRst, strMainsLo, strMainsHi, strHiHeat, strSolMax, strSolMin, strDcMax;
    m_editBatFul.GetWindowText(strBatFul);
    m_editBatWrn.GetWindowText(strBatWrn);
    m_editBatLo.GetWindowText(strBatLo);
    m_editBatRst.GetWindowText(strBatRst);
    m_editMainsLo.GetWindowText(strMainsLo);
    m_editMainsHi.GetWindowText(strMainsHi);
    m_editHiHeat.GetWindowText(strHiHeat);
    m_editSolMax.GetWindowText(strSolMax);
    m_editSolMin.GetWindowText(strSolMin);
    m_editDcMax.GetWindowText(strDcMax);

    CStringA pkt;
    pkt.Format("$LIMITS,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r\n",
        (LPCSTR)CStringA(strBatFul), (LPCSTR)CStringA(strBatWrn), (LPCSTR)CStringA(strBatLo), (LPCSTR)CStringA(strBatRst),
        (LPCSTR)CStringA(strMainsLo), (LPCSTR)CStringA(strMainsHi), (LPCSTR)CStringA(strHiHeat),
        (LPCSTR)CStringA(strSolMax), (LPCSTR)CStringA(strSolMin), (LPCSTR)CStringA(strDcMax));

    if (m_comm.IsOpen()) {
        m_comm.SendString(pkt);
        CString logText(pkt);
        logText.TrimRight(_T("\r\n"));
        AppendLog(logText, false, true, false);
    } else {
        AppendLog(_T("Cannot send limits: Serial port is not open."), true);
    }
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnSendNow()
{
    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    if (m_comm.IsOpen()) {
        SendCurrentTelemetry();
    } else {
        AppendLog(_T("Serial port not open. Telemetry preview updated."));
    }
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedChkAutoSend()
{
    if (m_chkAutoSend.GetCheck() == BST_CHECKED) {
        UINT intervalMs = 1000;
        int sel = m_comboInterval.GetCurSel();
        if (sel == 1) intervalMs = 500;
        else if (sel == 2) intervalMs = 250;
        else if (sel == 3) intervalMs = 100;

        SetTimer(1, intervalMs, NULL);
        AppendLog(_T("Auto-broadcast started (Interval: ") + CString(sel == 1 ? _T("500ms") : sel == 2 ? _T("250ms") : sel == 3 ? _T("100ms") : _T("1000ms")) + _T(")"));
    } else {
        KillTimer(1);
        AppendLog(_T("Auto-broadcast stopped."));
    }
}

void CSolarDisplaySerialSimulatorDlg::OnCbnSelchangeComboInterval()
{
    if (m_chkAutoSend.GetCheck() == BST_CHECKED) {
        OnBnClickedChkAutoSend();
    }
}

void CSolarDisplaySerialSimulatorDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 1) {
        if (m_bDynamicSweep) {
            m_fSweepAngle += 0.08f;
            if (m_fSweepAngle > 6.283185f) m_fSweepAngle -= 6.283185f;

            m_fSolar = 75.0f + 25.0f * sinf(m_fSweepAngle);
            if (m_fSolar < 0.0f) m_fSolar = 0.0f;

            m_fLoad = 45.0f + 20.0f * cosf(m_fSweepAngle);
            if (m_fLoad < 0.0f) m_fLoad = 0.0f;

            m_fBatt = 25.0f + 2.5f * sinf(m_fSweepAngle * 0.5f);
            m_fHeat = 36.0f + 5.0f * sinf(m_fSweepAngle);

            m_sliderSolar.SetPos((int)(m_fSolar * 10.0f));
            m_sliderLoad.SetPos((int)m_fLoad);
            m_sliderBatt.SetPos((int)(m_fBatt * 10.0f));
            m_sliderHeat.SetPos((int)(m_fHeat * 10.0f));

            SyncAllValuesFromSliders();
            UpdatePacketPreview();
        }

        if (m_comm.IsOpen() && m_nMode == 0) {
            SendCurrentTelemetry();
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

// Scenario Presets
void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetSunny()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(2300);
    m_sliderSolar.SetPos(850);
    m_sliderBatt.SetPos(274);
    m_sliderAcOut.SetPos(2300);
    m_sliderLoad.SetPos(38);
    m_sliderChg.SetPos(220);
    m_sliderDisch.SetPos(0);
    m_sliderDcBoost.SetPos(3920);
    m_sliderHeat.SetPos(370);

    m_comboFeedMode.SetCurSel(1);    // 1 - FEED: SOLAR PV
    m_comboOnFlag.SetCurSel(1);      // Inverter Mode
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);  // Solar Active
    m_comboChgState.SetCurSel(2);    // Solar MPPT
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Sunny Harvest (Solar Active, MPPT Charging 22A, Load 38%, FEED: SOLAR PV)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetNight()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(2320);
    m_sliderSolar.SetPos(0);
    m_sliderBatt.SetPos(258);
    m_sliderAcOut.SetPos(2300);
    m_sliderLoad.SetPos(28);
    m_sliderChg.SetPos(125);
    m_sliderDisch.SetPos(0);
    m_sliderDcBoost.SetPos(3600);
    m_sliderHeat.SetPos(340);

    m_comboFeedMode.SetCurSel(3);    // 3 - FEED: MAINS GRID
    m_comboOnFlag.SetCurSel(0);      // Mains Bypass
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(0);  // Solar Off
    m_comboChgState.SetCurSel(1);    // AC Mains Charger
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Night Grid Charging (Solar 0V, Mains Bypass, AC Chg 12.5A, FEED: MAINS GRID)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetOutage()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(0);
    m_sliderSolar.SetPos(320);
    m_sliderBatt.SetPos(242);
    m_sliderAcOut.SetPos(2280);
    m_sliderLoad.SetPos(65);
    m_sliderChg.SetPos(0);
    m_sliderDisch.SetPos(245);
    m_sliderDcBoost.SetPos(3650);
    m_sliderHeat.SetPos(415);

    m_comboFeedMode.SetCurSel(2);    // 2 - FEED: BATTERY
    m_comboOnFlag.SetCurSel(1);      // Inverter Mode
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);  // Solar Weak
    m_comboChgState.SetCurSel(0);    // Charger Off
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Grid Outage (Mains 0V, Inverter Mode, Batt Discharging 24.5A, FEED: BATTERY)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetOverload()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(3000);   // 300.0 V Overvoltage Alarm!
    m_sliderSolar.SetPos(1200);   // 120.0 V High VOC
    m_sliderBatt.SetPos(320);     // 32.0 V Overvoltage Trip!
    m_sliderAcOut.SetPos(2500);   // 250.0 V
    m_sliderLoad.SetPos(120);     // 120 % Critical Overload Alarm!
    m_sliderChg.SetPos(600);      // 60.0 A
    m_sliderDisch.SetPos(600);    // 60.0 A
    m_sliderDcBoost.SetPos(4500); // 450.0 V
    m_sliderHeat.SetPos(950);     // 95.0 °C Critical Overtemp Alarm!

    m_comboFeedMode.SetCurSel(0);    // 0 - Auto Routing
    m_comboOnFlag.SetCurSel(1);
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);
    m_comboChgState.SetCurSel(2);
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_CHECKED);
    m_chkBatGravity.SetCheck(BST_CHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Critical Overload & Thermal Stress Test ($PCU,300V,120V,32V,250V,120%,95C)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetShare()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(2280);   // 228.0 V
    m_sliderSolar.SetPos(750);    // 75.0 V
    m_sliderBatt.SetPos(265);     // 26.5 V
    m_sliderAcOut.SetPos(2300);   // 230.0 V
    m_sliderLoad.SetPos(52);      // 52 %
    m_sliderChg.SetPos(140);      // 14.0 A
    m_sliderDisch.SetPos(60);     // 6.0 A
    m_sliderDcBoost.SetPos(3820); // 382.0 V
    m_sliderHeat.SetPos(385);     // 38.5 °C

    m_comboFeedMode.SetCurSel(4);    // 4 - FEED: SMART SHARE
    m_comboOnFlag.SetCurSel(1);      // Inverter Mode
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);  // Solar Active
    m_comboChgState.SetCurSel(3);    // Smart Share Charger
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_CHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Smart Share (Grid + Solar Co-generation, FEED: SMART SHARE)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetGridFailLow()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(1620);   // 162.0 V (Grid Brownout / Undervolt < 185V)
    m_sliderSolar.SetPos(680);    // 68.0 V
    m_sliderBatt.SetPos(252);     // 25.2 V
    m_sliderAcOut.SetPos(2300);   // 230.0 V
    m_sliderLoad.SetPos(45);      // 45 %
    m_sliderChg.SetPos(80);       // 8.0 A
    m_sliderDisch.SetPos(0);      // 0.0 A
    m_sliderDcBoost.SetPos(3750); // 375.0 V
    m_sliderHeat.SetPos(360);     // 36.0 °C

    m_comboFeedMode.SetCurSel(1);    // 1 - FEED: SOLAR PV
    m_comboOnFlag.SetCurSel(1);      // Inverter Mode
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);  // Solar Active
    m_comboChgState.SetCurSel(2);    // Solar MPPT
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Grid Undervolt / Brownout (Mains 162V < 185V -> Trigger UNDERVOLT Alarm)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetGridFailHigh()
{
    m_bDynamicSweep = false;
    m_sliderMains.SetPos(2820);   // 282.0 V (Grid Surge / Overvolt > 265V)
    m_sliderSolar.SetPos(400);    // 40.0 V
    m_sliderBatt.SetPos(246);     // 24.6 V
    m_sliderAcOut.SetPos(2300);   // 230.0 V
    m_sliderLoad.SetPos(48);      // 48 %
    m_sliderChg.SetPos(0);        // 0.0 A
    m_sliderDisch.SetPos(180);    // 18.0 A
    m_sliderDcBoost.SetPos(3700); // 370.0 V
    m_sliderHeat.SetPos(430);     // 43.0 °C

    m_comboFeedMode.SetCurSel(2);    // 2 - FEED: BATTERY
    m_comboOnFlag.SetCurSel(1);      // Inverter Mode
    m_comboSwitchState.SetCurSel(0); // 1 - Switch ON
    m_comboSolarState.SetCurSel(1);  // Solar Active
    m_comboChgState.SetCurSel(0);    // Charger Off
    m_chkDcBoostMode.SetCheck(BST_CHECKED);
    m_chkDcOk.SetCheck(BST_CHECKED);
    m_chkShareMode.SetCheck(BST_UNCHECKED);
    m_chkBatGravity.SetCheck(BST_UNCHECKED);

    SyncAllValuesFromSliders();
    UpdatePacketPreview();
    AppendLog(_T("Preset Loaded: Grid Overvolt / Surge (Mains 282V > 265V -> Trigger OVERVOLT Alarm)"));
    if (m_comm.IsOpen()) SendCurrentTelemetry();
}

void CSolarDisplaySerialSimulatorDlg::OnBnClickedPresetSweep()
{
    m_bDynamicSweep = !m_bDynamicSweep;
    if (m_bDynamicSweep) {
        AppendLog(_T("Dynamic Sweep Mode ACTIVATED: Live smooth wave variation enabled."));
    } else {
        AppendLog(_T("Dynamic Sweep Mode DEACTIVATED."));
    }
}

// Remote LCD Page Switching
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage0() { SendPageCommand(0); }
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage1() { SendPageCommand(1); }
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage2() { SendPageCommand(2); }
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage3() { SendPageCommand(3); }
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage4() { SendPageCommand(4); }
void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnPage5() { SendPageCommand(5); }

void CSolarDisplaySerialSimulatorDlg::OnBnClickedBtnClearLog()
{
    m_editLog.SetWindowText(_T(""));
}

void CSolarDisplaySerialSimulatorDlg::AppendLog(const CString& text, bool isError, bool isTx, bool isRx)
{
    HWND hWnd = m_editLog.GetSafeHwnd();
    if (!hWnd || !::IsWindow(hWnd)) return;

    SYSTEMTIME st;
    GetLocalTime(&st);

    CString prefix;
    if (isError) prefix = _T("[ERR] ");
    else if (isTx) prefix = _T("[TX ->] ");
    else if (isRx) prefix = _T("[<- RX] ");

    CString line;
    line.Format(_T("[%02d:%02d:%02d.%03d] %s%s\r\n"),
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        (LPCTSTR)prefix, (LPCTSTR)text);

    int len = (int)::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    if (len > 30000) {
        ::SendMessage(hWnd, EM_SETSEL, 0, 10000);
        ::SendMessage(hWnd, EM_REPLACESEL, FALSE, (LPARAM)_T(""));
        len = (int)::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    }

    ::SendMessage(hWnd, EM_SETSEL, len, len);
    ::SendMessage(hWnd, EM_REPLACESEL, FALSE, (LPARAM)(LPCTSTR)line);

    if (m_chkAutoScroll.GetCheck() == BST_CHECKED) {
        ::SendMessage(hWnd, WM_VSCROLL, SB_BOTTOM, 0);
    }
}

// Background Worker Thread to read incoming traffic & respond in Virtual Echo mode
UINT CSolarDisplaySerialSimulatorDlg::RxWorkerThreadProc(LPVOID pParam)
{
    CSolarDisplaySerialSimulatorDlg* pDlg = (CSolarDisplaySerialSimulatorDlg*)pParam;
    HANDLE hSerial = pDlg->m_comm.GetHandle();
    HANDLE hStop = pDlg->m_hStopRxEvent;

    uint8_t rxBuf[512];

    while (WaitForSingleObject(hStop, 10) == WAIT_TIMEOUT) {
        if (!pDlg->m_comm.IsOpen()) {
            Sleep(50);
            continue;
        }

        DWORD readBytes = 0;
        if (ReadFile(hSerial, rxBuf, sizeof(rxBuf), &readBytes, NULL) && readBytes > 0) {
            pDlg->ProcessRxBuffer(rxBuf, readBytes);
        }
    }

    return 0;
}

void CSolarDisplaySerialSimulatorDlg::ProcessRxBuffer(const uint8_t* buf, DWORD len)
{
    if (m_nMode == 0) {
        // Mode 0: Inverter TX mode - just display any incoming ASCII strings from ESP32
        CStringA strA((const char*)buf, (int)len);
        strA.TrimRight("\r\n");
        if (!strA.IsEmpty()) {
            AppendLog(CString(strA), false, false, true);
        }
        return;
    }

    // Mode 1: Virtual ESP32 Target Echo Mode (handles HRF protocol from SolarDisplayManager)
    static std::vector<uint8_t> frameAccum;
    frameAccum.insert(frameAccum.end(), buf, buf + len);

    while (frameAccum.size() >= sizeof(hrf_header_t) + 5) {
        // Search for sync bytes 0x53, 0x44 ('S', 'D')
        if (frameAccum[0] != HRF_SYNC_BYTE1 || frameAccum[1] != HRF_SYNC_BYTE2) {
            frameAccum.erase(frameAccum.begin());
            continue;
        }

        hrf_header_t* pHdr = (hrf_header_t*)frameAccum.data();
        size_t totalExpected = sizeof(hrf_header_t) + pHdr->length + 5; // header + payload + 4 crc + 1 trailer

        if (frameAccum.size() < totalExpected) {
            break; // wait for remaining bytes
        }

        if (frameAccum[totalExpected - 1] != HRF_FRAME_TRAILER) {
            // Bad frame trailer, discard sync byte and retry
            frameAccum.erase(frameAccum.begin());
            continue;
        }

        uint32_t calcCrc = pcu_calc_crc32(frameAccum.data(), sizeof(hrf_header_t) + pHdr->length);
        uint32_t rxCrc = *(uint32_t*)(frameAccum.data() + sizeof(hrf_header_t) + pHdr->length);

        if (calcCrc == rxCrc) {
            uint8_t* payload = frameAccum.data() + sizeof(hrf_header_t);
            uint8_t seq = pHdr->seq;
            uint8_t cmd = pHdr->cmd;

            switch (cmd) {
                case CMD_PING: {
                    AppendLog(_T("Received CMD_PING from PC manager!"), false, false, true);
                    hrf_ping_resp_t ping;
                    ping.device_state = m_virtualCfg.is_configured ? DEVICE_STATE_CONFIGURED : DEVICE_STATE_UNCONFIGURED;
                    ping.hw_model_id = 0x01;
                    ping.fw_version = PCU_CONFIG_VERSION;
                    ping.uptime_sec = 3600;
                    strcpy_s(ping.chip_model, "ESP32-D0WDQ6");

                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ping, sizeof(ping), seq);
                    AppendLog(_T("Replied RESP_ACK to CMD_PING (Device Configured, FW: v2.0)"), false, true, false);
                    break;
                }
                case CMD_READ_CONFIG: {
                    AppendLog(_T("Received CMD_READ_CONFIG from PC manager!"), false, false, true);
                    m_comm.SendFrame(RESP_CONFIG_DATA, (const uint8_t*)&m_virtualCfg, sizeof(pcu_config_t), seq);
                    CString logStr;
                    logStr.Format(_T("Replied RESP_CONFIG_DATA (Brand='%s', Model='%s', Theme=%u)"),
                        CString(m_virtualCfg.brand_title), CString(m_virtualCfg.model_name), m_virtualCfg.logo_theme);
                    AppendLog(logStr, false, true, false);
                    break;
                }
                case CMD_WRITE_CONFIG: {
                    AppendLog(_T("Received CMD_WRITE_CONFIG from PC manager!"), false, false, true);
                    if (pHdr->length == sizeof(pcu_config_t)) {
                        memcpy(&m_virtualCfg, payload, sizeof(pcu_config_t));
                        hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                        m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                        CString logStr;
                        logStr.Format(_T("Updated Virtual Config: Brand='%s', Model='%s', Theme=%u. Replied STATUS_OK."),
                            CString(m_virtualCfg.brand_title), CString(m_virtualCfg.model_name), m_virtualCfg.logo_theme);
                        AppendLog(logStr, false, true, false);
                    }
                    break;
                }
                case CMD_COMMIT_NVS: {
                    AppendLog(_T("Received CMD_COMMIT_NVS from PC manager!"), false, false, true);
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    AppendLog(_T("Simulated NVS commit & device reboot. Replied STATUS_OK."), false, true, false);
                    break;
                }
                case CMD_WRITE_LOGO_CHUNK: {
                    hrf_logo_chunk_t* chunk = (hrf_logo_chunk_t*)payload;
                    CString logStr;
                    logStr.Format(_T("Received CMD_WRITE_LOGO_CHUNK (Offset: %u, Length: %u)"), chunk->offset, chunk->length);
                    AppendLog(logStr, false, false, true);
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, chunk->offset };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    break;
                }
                case CMD_COMMIT_LOGO: {
                    AppendLog(_T("Received CMD_COMMIT_LOGO!"), false, false, true);
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    AppendLog(_T("Custom logo committed to virtual storage. Replied STATUS_OK."), false, true, false);
                    break;
                }
                case CMD_CLEAR_LOGO: {
                    AppendLog(_T("Received CMD_CLEAR_LOGO!"), false, false, true);
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    AppendLog(_T("Virtual logo cleared. Replied STATUS_OK."), false, true, false);
                    break;
                }
                case CMD_REBOOT: {
                    AppendLog(_T("Received CMD_REBOOT!"), false, false, true);
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    AppendLog(_T("Target simulated restart. Replied STATUS_OK."), false, true, false);
                    break;
                }
                case CMD_FACTORY_RESET: {
                    AppendLog(_T("Received CMD_FACTORY_RESET!"), false, false, true);
                    m_virtualCfg.is_configured = 0;
                    hrf_ack_payload_t ack = { cmd, STATUS_OK, 0 };
                    m_comm.SendFrame(RESP_ACK, (const uint8_t*)&ack, sizeof(ack), seq);
                    AppendLog(_T("Virtual memory erased. Device in Factory unconfigured state. Replied STATUS_OK."), false, true, false);
                    break;
                }
                default: {
                    CString logStr;
                    logStr.Format(_T("Unknown command: 0x%02X"), cmd);
                    AppendLog(logStr, true);
                    break;
                }
            }
        } else {
            AppendLog(_T("Received frame CRC mismatch!"), true);
        }

        frameAccum.erase(frameAccum.begin(), frameAccum.begin() + totalExpected);
    }
}
