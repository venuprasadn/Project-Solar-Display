// SolarDisplayManagerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SolarDisplayManager.h"
#include "SolarDisplayManagerDlg.h"
#include "afxdialogex.h"
#include <algorithm>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSolarDisplayManagerDlg dialog

CSolarDisplayManagerDlg::CSolarDisplayManagerDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_SOLARDISPLAYMANAGER_DIALOG, pParent)
    , m_isFlashing(false)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSolarDisplayManagerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);

    // Top GDI+ Banner Control
    DDX_Control(pDX, IDC_STATIC_OEM_HEADER, m_oemHeader);

    // Common Header Controls
    DDX_Control(pDX, IDC_COMBO_PORT, m_comboPort);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_COMBO_FLASH_BAUD, m_comboFlashBaud);
    DDX_Control(pDX, IDC_PROGRESS_FLASH, m_progressFlash);
    DDX_Control(pDX, IDC_CHK_AUTO_PROVISION, m_chkAutoProvision);

    // Tab Control
    DDX_Control(pDX, IDC_TAB_MAIN, m_tabCtrl);

    // Common Action Buttons (Out of Tab)
    DDX_Control(pDX, IDC_BTN_READ_CONFIG, m_btnReadConfig);
    DDX_Control(pDX, IDC_BTN_WRITE_CONFIG, m_btnWriteConfig);
    DDX_Control(pDX, IDC_BTN_REBOOT, m_btnReboot);
    DDX_Control(pDX, IDC_BTN_FACTORY_RESET, m_btnFactoryReset);

    // Common Footer Controls
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BEGIN_MESSAGE_MAP(CSolarDisplayManagerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_REFRESH_PORTS, &CSolarDisplayManagerDlg::OnBnClickedBtnRefreshPorts)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CSolarDisplayManagerDlg::OnBnClickedBtnConnect)
    ON_BN_CLICKED(IDC_BTN_FLASH, &CSolarDisplayManagerDlg::OnBnClickedBtnFlash)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CSolarDisplayManagerDlg::OnTcnSelchangeTabMain)
    ON_BN_CLICKED(IDC_BTN_READ_CONFIG, &CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig)
    ON_BN_CLICKED(IDC_BTN_WRITE_CONFIG, &CSolarDisplayManagerDlg::OnBnClickedBtnWriteConfig)
    ON_BN_CLICKED(IDC_BTN_REBOOT, &CSolarDisplayManagerDlg::OnBnClickedBtnReboot)
    ON_BN_CLICKED(IDC_BTN_FACTORY_RESET, &CSolarDisplayManagerDlg::OnBnClickedBtnFactoryReset)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOG, &CSolarDisplayManagerDlg::OnBnClickedBtnClearLog)
    ON_MESSAGE(WM_USER + 100, &CSolarDisplayManagerDlg::OnFlashProgress)
    ON_MESSAGE(WM_USER + 101, &CSolarDisplayManagerDlg::OnFlashComplete)
END_MESSAGE_MAP()


// CSolarDisplayManagerDlg message handlers

BOOL CSolarDisplayManagerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // Set Dynamic Window Title from Encrypted Vault
    CString wndTitle;
    wndTitle.Format(_T("%s - Solar PCU Staging Manager [%s]"),
        (LPCTSTR)OEMPayload::GetBrandTitle(),
        (LPCTSTR)OEMPayload::GetModelName()
    );
    SetWindowText(wndTitle);

    // 1. Initialize Flasher Settings
    m_comboFlashBaud.ResetContent();
    m_comboFlashBaud.AddString(_T("921600"));
    m_comboFlashBaud.AddString(_T("460800"));
    m_comboFlashBaud.AddString(_T("115200"));
    m_comboFlashBaud.SetCurSel(1); // 460800 default

    m_chkAutoProvision.SetCheck(BST_CHECKED);
    m_progressFlash.SetRange(0, 100);
    m_progressFlash.SetPos(0);


    // 2. Setup Tab Control & Child Dialogs based on Pre-Configured Hardware Mode
    uint8_t hwMode = OEMPayload::GetTargetHardwareMode();

    if (hwMode == OEM_HW_MODE_COMBO)
    {
        m_tabCtrl.InsertItem(0, _T("1. LCD Display Configuration"));
        m_tabCtrl.InsertItem(1, _T("2. Mobile App && AWS Cloud"));

        m_tabDisplay.Create(IDD_TAB_DISPLAY, this);
        m_tabDisplay.SetMainDlg(this);

        m_tabApp.Create(IDD_TAB_APP, this);
        m_tabApp.SetMainDlg(this);

        CRect rcTab;
        m_tabCtrl.GetWindowRect(&rcTab);
        ScreenToClient(&rcTab);
        m_tabCtrl.AdjustRect(FALSE, &rcTab);
        m_tabDisplay.MoveWindow(&rcTab);
        m_tabApp.MoveWindow(&rcTab);

        m_tabDisplay.ShowWindow(SW_SHOW);
        m_tabDisplay.BringWindowToTop();
        m_tabApp.ShowWindow(SW_HIDE);
        m_tabCtrl.SetCurSel(0);
    }
    else if (hwMode == OEM_HW_MODE_TFT_ONLY)
    {
        m_tabCtrl.InsertItem(0, _T("LCD Display Configuration (TFT Dedicated)"));

        m_tabDisplay.Create(IDD_TAB_DISPLAY, this);
        m_tabDisplay.SetMainDlg(this);

        CRect rcTab;
        m_tabCtrl.GetWindowRect(&rcTab);
        ScreenToClient(&rcTab);
        m_tabCtrl.AdjustRect(FALSE, &rcTab);
        m_tabDisplay.MoveWindow(&rcTab);

        m_tabDisplay.ShowWindow(SW_SHOW);
        m_tabDisplay.BringWindowToTop();
        m_tabCtrl.SetCurSel(0);
    }
    else if (hwMode == OEM_HW_MODE_CLOUD_ONLY)
    {
        m_tabCtrl.InsertItem(0, _T("Mobile App && AWS Cloud (IoT Dedicated)"));

        m_tabApp.Create(IDD_TAB_APP, this);
        m_tabApp.SetMainDlg(this);

        CRect rcTab;
        m_tabCtrl.GetWindowRect(&rcTab);
        ScreenToClient(&rcTab);
        m_tabCtrl.AdjustRect(FALSE, &rcTab);
        m_tabApp.MoveWindow(&rcTab);

        m_tabApp.ShowWindow(SW_SHOW);
        m_tabApp.BringWindowToTop();
        m_tabCtrl.SetCurSel(0);
    }

    // 3. Enumerate and Auto-Detect Available Ports
    RefreshComPorts();

    AppendLog(_T("=== Industrial OEM Staging Manager Initialized ==="));
    AppendLog(_T("Target Architecture: Dual-Core 32-bit Controller (16MB Flash, 8MB RAM)"));
    AppendLog(_T("Operational Mode: ") + OEMPayload::GetModeDescription());
    AppendLog(_T("Compliance Standards: ISO 9001 / IEC 62109 / MIL-STD-810H"));

    return TRUE;
}

void CSolarDisplayManagerDlg::OnTcnSelchangeTabMain(NMHDR *pNMHDR, LRESULT *pResult)
{
    uint8_t hwMode = OEMPayload::GetTargetHardwareMode();
    if (hwMode == OEM_HW_MODE_COMBO)
    {
        int sel = m_tabCtrl.GetCurSel();
        if (sel == 0)
        {
            m_tabDisplay.ShowWindow(SW_SHOW);
            m_tabDisplay.BringWindowToTop();
            m_tabApp.ShowWindow(SW_HIDE);
        }
        else
        {
            m_tabDisplay.ShowWindow(SW_HIDE);
            m_tabApp.ShowWindow(SW_SHOW);
            m_tabApp.BringWindowToTop();
        }
    }
    *pResult = 0;
}


void CSolarDisplayManagerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSolarDisplayManagerDlg::OnPaint()
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

HCURSOR CSolarDisplayManagerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CSolarDisplayManagerDlg::AppendLog(const CString& text, bool isError)
{
    HWND hWnd = m_editLog.GetSafeHwnd();
    if (!hWnd || !::IsWindow(hWnd)) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    CString line;
    line.Format(_T("[%02d:%02d:%02d] %s%s\r\n"),
        st.wHour, st.wMinute, st.wSecond,
        isError ? _T("ERROR: ") : _T(""),
        (LPCTSTR)text
    );

    int len = (int)::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    ::SendMessage(hWnd, EM_SETSEL, len, len);
    ::SendMessage(hWnd, EM_REPLACESEL, FALSE, (LPARAM)(LPCTSTR)line);
}

static CString GetTargetPortName(CComboBox& combo)
{
    CString str;
    int sel = combo.GetCurSel();
    if (sel != CB_ERR) {
        combo.GetLBText(sel, str);
    } else {
        combo.GetWindowText(str);
    }
    int space = str.Find(_T(' '));
    if (space > 0) {
        return str.Left(space);
    }
    return str;
}

void CSolarDisplayManagerDlg::RefreshComPorts()
{
    m_comboPort.ResetContent();
    std::vector<CSerialComm::PortInfo> ports = CSerialComm::EnumerateDetailedPorts();
    
    // Sort ports so genuine ESP devices appear at the top, ranked by Native USB first
    std::sort(ports.begin(), ports.end(), [](const CSerialComm::PortInfo& a, const CSerialComm::PortInfo& b) {
        if (a.isEspDevice != b.isEspDevice) return a.isEspDevice > b.isEspDevice;
        if (a.rank != b.rank) return a.rank < b.rank;
        return a.portName < b.portName;
    });

    for (size_t i = 0; i < ports.size(); i++) {
        m_comboPort.AddString(ports[i].friendlyName);
    }

    int selectedIdx = -1;
    bool isVerified = false;

    // Test handshake ping in rank order (ESP32-S3 Native USB first, then USB-UART bridges)
    if (!m_isFlashing) {
        for (size_t i = 0; i < ports.size(); i++) {
            if (ports[i].isEspDevice) {
                CSerialComm testComm;
                if (testComm.Open(ports[i].portName, 115200)) {
                    hrf_ping_resp_t ping;
                    if (testComm.Ping(&ping)) {
                        selectedIdx = (int)i;
                        isVerified = true;
                        testComm.Close();
                        break; // Highest-ranked responding controller selected!
                    }
                    testComm.Close();
                }
            }
        }
    }

    // If no port answered ping, fallback to highest-ranked ESP device (e.g. Native USB in bootloader mode)
    if (selectedIdx == -1) {
        for (size_t i = 0; i < ports.size(); i++) {
            if (ports[i].isEspDevice) {
                selectedIdx = (int)i;
                break;
            }
        }
    }

    if (m_comboPort.GetCount() > 0) {
        if (selectedIdx >= 0) {
            m_comboPort.SetCurSel(selectedIdx);
            CString selected;
            m_comboPort.GetLBText(selectedIdx, selected);
            if (isVerified) {
                m_staticStatus.SetWindowText(_T("Status: Target Verified on ") + ports[selectedIdx].portName);
                AppendLog(_T("Auto-detected & verified target hardware on ") + selected);
            } else {
                m_staticStatus.SetWindowText(_T("Status: Ready on ") + ports[selectedIdx].portName);
                AppendLog(_T("Auto-detected target controller (VID/PID matched): ") + selected);
            }
        } else {
            // No genuine ESP32 hardware detected among COM ports
            m_comboPort.SetCurSel(-1);
            m_staticStatus.SetWindowText(_T("Status: No ESP32 Detected"));
            AppendLog(_T("Auto-Detect: No compatible ESP32 / USB-UART hardware found (VID/PID filtered)."), true);
        }
    } else {
        m_staticStatus.SetWindowText(_T("Status: No COM Ports Available"));
    }
}

void CSolarDisplayManagerDlg::SetUIEnabled(BOOL bEnable)
{
    GetDlgItem(IDC_BTN_FLASH)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(bEnable);

    m_btnReadConfig.EnableWindow(bEnable);
    m_btnWriteConfig.EnableWindow(bEnable);
    m_btnReboot.EnableWindow(bEnable);
    m_btnFactoryReset.EnableWindow(bEnable);

    uint8_t hwMode = OEMPayload::GetTargetHardwareMode();
    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_TFT_ONLY) {
        m_tabDisplay.SetUIEnabled(bEnable);
    }
    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_CLOUD_ONLY) {
        m_tabApp.SetUIEnabled(bEnable);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnRefreshPorts()
{
    RefreshComPorts();
    AppendLog(_T("Refreshed available COM ports."));
}

bool CSolarDisplayManagerDlg::EnsureConnected()
{
    if (m_comm.IsOpen()) {
        hrf_ping_resp_t ping;
        if (m_comm.Ping(&ping)) {
            return true;
        }
        m_comm.Close();
    }

    CString portName = GetTargetPortName(m_comboPort);
    if (portName.IsEmpty()) {
        AfxMessageBox(_T("Please select a target COM port."));
        return false;
    }

    AppendLog(_T("Opening ") + portName + _T(" at 115200 baud..."));
    if (!m_comm.Open(portName, 115200)) {
        AppendLog(m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(m_comm.GetLastErrorMsg(), MB_ICONERROR);
        return false;
    }

    AppendLog(_T("Synchronizing with controller service protocol..."));
    hrf_ping_resp_t ping;

    if (!m_comm.Ping(&ping)) {
        AppendLog(_T("No response from target controller on ") + portName + _T(". Check power and cable connection."), true);
        AfxMessageBox(_T("Could not communicate with target controller on ") + portName + _T(".\nPlease verify the board is powered and COM port is correct."), MB_ICONERROR);
        m_comm.Close();
        m_staticStatus.SetWindowText(_T("Status: Disconnected"));
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        return false;
    }

    CString devState = (ping.device_state == DEVICE_STATE_CONFIGURED) ? _T("CONFIGURED (RUNNING)") : _T("UNCONFIGURED (FACTORY)");
    CString statStr;
    statStr.Format(_T("Status: Connected to %s [%s] FW: v%u.%u"),
        (LPCTSTR)portName,
        (LPCTSTR)devState,
        ping.fw_version >> 8, ping.fw_version & 0xFF
    );
    m_staticStatus.SetWindowText(statStr);
    SetDlgItemText(IDC_BTN_CONNECT, _T("Disconnect"));
    AppendLog(CString(_T("Connected! Target State: ")) + devState);

    return true;
}

void CSolarDisplayManagerDlg::OnBnClickedBtnConnect()
{
    if (m_comm.IsOpen()) {
        m_comm.Close();
        m_staticStatus.SetWindowText(_T("Status: Disconnected"));
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        AppendLog(_T("Disconnected from COM port."));
        return;
    }

    if (EnsureConnected()) {
        AppendLog(_T("Connection ready. Navigate through tabs to configure features or click 'Read from Unit'."));
    }
}

// Structure to pass to background flash worker thread
struct FlashThreadParams {
    CSolarDisplayManagerDlg* pDlg;
    CString port;
    DWORD baud;
    bool autoProvision;
    pcu_config_t cfg;
};


static UINT FlashWorkerThread(LPVOID pParam)
{
    FlashThreadParams* params = (FlashThreadParams*)pParam;
    CSolarDisplayManagerDlg* pDlg = params->pDlg;

    CEspFlasher flasher;
    CString err;

    bool success = flasher.FlashMonolithicFirmware(
        params->port,
        params->baud,
        [pDlg](int pct, const CString& line) {
            if (pct >= 0) {
                pDlg->PostMessage(WM_USER + 100, (WPARAM)pct, 0);
            }
            pDlg->AppendLog(line);
        },
        err
    );

    if (success) {
        pDlg->AppendLog(_T("FLASHING COMPLETED SUCCESSFULLY!"));

        if (params->autoProvision) {
            pDlg->AppendLog(_T("Connecting to target controller in USB Service Mode for auto-provisioning..."));
            Sleep(1200);

            // Reconnect and provision NVS
            CSerialComm comm;
            if (comm.Open(params->port, 115200)) {
                Sleep(500);

                hrf_ping_resp_t ping;
                bool reachable = false;
                for (int attempt = 0; attempt < 10; attempt++) {
                    if (comm.Ping(&ping)) {
                        reachable = true;
                        break;
                    }
                    Sleep(300);
                }

                if (reachable) {
                    pDlg->AppendLog(_T("Target reached in USB Service Mode! Writing NVS configuration..."));

                    pcu_config_t cfg = params->cfg;

                    if (comm.WriteConfig(&cfg)) {
                        pDlg->AppendLog(_T("Configuration written. Committing to NVS flash..."));
                        if (comm.CommitNVS()) {
                            pDlg->AppendLog(_T("SUCCESS: NVS COMMITTED! Target running configured firmware."));
                        } else {
                            pDlg->AppendLog(_T("Commit to NVS failed!"), true);
                        }
                    } else {
                        pDlg->AppendLog(_T("WriteConfig failed!"), true);
                    }

                    pDlg->AppendLog(_T("SUCCESS: Auto-provisioning complete."));
                } else {
                    pDlg->AppendLog(_T("Could not ping device after flash (Target did not acknowledge service mode)."), true);
                }
                comm.Close();
            } else {
                pDlg->AppendLog(_T("Could not reopen port for auto-provisioning."), true);
            }
        }
    } else {
        pDlg->AppendLog(CString(_T("Flashing failed: ")) + err, true);
    }

    pDlg->PostMessage(WM_USER + 101, (WPARAM)success, 0);
    delete params;
    return 0;
}

void CSolarDisplayManagerDlg::OnBnClickedBtnFlash()
{
    CString port = GetTargetPortName(m_comboPort);
    if (port.IsEmpty()) {
        AfxMessageBox(_T("Please select a target COM port."));
        return;
    }

    if (m_comm.IsOpen()) {
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Flasher in progress..."));
    }

    CString strBaud;
    m_comboFlashBaud.GetWindowText(strBaud);
    DWORD baud = (DWORD)_tstol(strBaud);
    if (baud == 0) baud = 460800;

    m_progressFlash.SetPos(0);
    SetUIEnabled(FALSE);
    AppendLog(_T("=================================================="));
    AppendLog(CString(_T("Starting Monolithic Embedded Flash on ")) + port + _T(" at ") + strBaud + _T(" baud..."));

    FlashThreadParams* params = new FlashThreadParams();
    params->pDlg = this;
    params->port = port;
    params->baud = baud;
    params->autoProvision = (m_chkAutoProvision.GetCheck() == BST_CHECKED);

    // Collect display config and app feature flags based on hardware mode
    uint8_t hwMode = OEMPayload::GetTargetHardwareMode();

    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_TFT_ONLY) {
        m_tabDisplay.CollectConfigFromUI(params->cfg);
    }
    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_CLOUD_ONLY) {
        m_tabApp.CollectVendorDetails(params->cfg);
    }

    params->cfg.model_features = 0;
    if (hwMode == OEM_HW_MODE_COMBO) {
        params->cfg.model_features = MODEL_FEATURE_DISPLAY | MODEL_FEATURE_MOBILE_APP;
    } else if (hwMode == OEM_HW_MODE_TFT_ONLY) {
        params->cfg.model_features = MODEL_FEATURE_DISPLAY;
    } else if (hwMode == OEM_HW_MODE_CLOUD_ONLY) {
        params->cfg.model_features = MODEL_FEATURE_MOBILE_APP;
    }

    params->cfg.magic = PCU_CONFIG_MAGIC;
    params->cfg.version = PCU_CONFIG_VERSION;
    params->cfg.struct_size = sizeof(pcu_config_t);
    params->cfg.is_configured = 1;
    params->cfg.config_crc32 = pcu_calc_crc32((const uint8_t*)&params->cfg, offsetof(pcu_config_t, config_crc32));

    AfxBeginThread(FlashWorkerThread, params);
}


void CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig()
{
    if (!EnsureConnected()) return;

    pcu_config_t cfg;
    if (ReadConfig(cfg)) {
        uint8_t hwMode = OEMPayload::GetTargetHardwareMode();
        if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_TFT_ONLY) {
            m_tabDisplay.LoadConfigToUI(cfg);
        }
        if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_CLOUD_ONLY) {
            m_tabApp.UpdateFeaturesFromConfig(cfg.model_features);
            m_tabApp.LoadVendorDetails(cfg);
        }
        AppendLog(_T("SUCCESS: Loaded active unit configuration into UI."));
        AfxMessageBox(_T("Configuration read successfully from unit!"), MB_ICONINFORMATION);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnWriteConfig()
{
    if (!EnsureConnected()) return;

    pcu_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    uint8_t hwMode = OEMPayload::GetTargetHardwareMode();

    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_TFT_ONLY) {
        m_tabDisplay.CollectConfigFromUI(cfg);
    }
    if (hwMode == OEM_HW_MODE_COMBO || hwMode == OEM_HW_MODE_CLOUD_ONLY) {
        m_tabApp.CollectVendorDetails(cfg);
    }

    cfg.model_features = 0;
    if (hwMode == OEM_HW_MODE_COMBO) {
        cfg.model_features = MODEL_FEATURE_DISPLAY | MODEL_FEATURE_MOBILE_APP;
    } else if (hwMode == OEM_HW_MODE_TFT_ONLY) {
        cfg.model_features = MODEL_FEATURE_DISPLAY;
    } else if (hwMode == OEM_HW_MODE_CLOUD_ONLY) {
        cfg.model_features = MODEL_FEATURE_MOBILE_APP;
    }


    if (WriteConfig(cfg)) {
        AppendLog(_T("SUCCESS: Configuration committed to NVS flash!"));
        AfxMessageBox(_T("Configuration successfully committed to ESP32 NVS! Target is rebooting."), MB_ICONINFORMATION);
    }
}


void CSolarDisplayManagerDlg::OnBnClickedBtnReboot()
{
    RebootDevice();
}

void CSolarDisplayManagerDlg::OnBnClickedBtnFactoryReset()
{
    FactoryResetDevice();
}

bool CSolarDisplayManagerDlg::ReadConfig(pcu_config_t& cfg)
{
    if (!EnsureConnected()) return false;

    AppendLog(_T("Reading configuration from ESP32 NVS..."));
    if (m_comm.ReadConfig(&cfg)) {
        return true;
    }

    AppendLog(_T("ReadConfig failed: ") + m_comm.GetLastErrorMsg(), true);
    return false;
}

bool CSolarDisplayManagerDlg::WriteConfig(const pcu_config_t& cfg)
{
    if (!EnsureConnected()) return false;

    pcu_config_t toWrite = cfg;
    toWrite.magic = PCU_CONFIG_MAGIC;
    toWrite.version = PCU_CONFIG_VERSION;
    toWrite.struct_size = sizeof(pcu_config_t);
    toWrite.is_configured = 1;
    toWrite.config_crc32 = pcu_calc_crc32((const uint8_t*)&toWrite, offsetof(pcu_config_t, config_crc32));

    AppendLog(_T("Writing configuration to target NVS..."));
    if (m_comm.WriteConfig(&toWrite)) {
        if (m_comm.CommitNVS()) {
            AppendLog(_T("SUCCESS: Configuration committed to NVS! Target rebooting."));
            m_comm.Close();
            SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
            m_staticStatus.SetWindowText(_T("Status: Disconnected (Target Rebooted)"));
            return true;
        } else {
            AppendLog(_T("Commit to NVS failed!"), true);
        }
    } else {
        AppendLog(_T("WriteConfig failed: ") + m_comm.GetLastErrorMsg(), true);
    }
    return false;
}

bool CSolarDisplayManagerDlg::RebootDevice()
{
    if (!EnsureConnected()) return false;

    AppendLog(_T("Sending reboot command to target..."));
    if (m_comm.Reboot()) {
        AppendLog(_T("Reboot command acknowledged. Target resetting."));
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Target Rebooted"));
        return true;
    }

    AppendLog(_T("Reboot command failed."), true);
    return false;
}

bool CSolarDisplayManagerDlg::FactoryResetDevice()
{
    if (AfxMessageBox(_T("Are you sure you want to perform a FACTORY RESET?\nThis will erase the NVS configuration partition!"),
        MB_YESNO | MB_ICONWARNING) != IDYES) {
        return false;
    }

    if (!EnsureConnected()) return false;

    AppendLog(_T("Sending Factory Reset command to target..."));
    if (m_comm.FactoryReset()) {
        AppendLog(_T("Target NVS erased and reset. Device rebooted in unconfigured state."));
        AfxMessageBox(_T("Factory Reset completed! Device is unconfigured."), MB_ICONINFORMATION);
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Target Unconfigured (Display OFF)"));
        return true;
    }

    AppendLog(_T("Factory Reset command failed."), true);
    return false;
}

bool CSolarDisplayManagerDlg::ProvisionWifi(const CString& ssid, const CString& pass)
{
    if (!EnsureConnected()) return false;

    AppendLog(_T("Sending Wi-Fi provisioning credentials over USB..."));
    if (m_comm.ProvisionWifi(CT2A(ssid), CT2A(pass))) {
        AppendLog(_T("Wi-Fi credentials accepted by controller!"));
        return true;
    }

    AppendLog(_T("ProvisionWifi failed: ") + m_comm.GetLastErrorMsg(), true);
    return false;
}

bool CSolarDisplayManagerDlg::ScanWifi(std::vector<wifi_scan_ap_record_t>& outAps)
{
    if (!EnsureConnected()) return false;

    AppendLog(_T("Starting Wi-Fi scan over USB (may take 2-4 seconds)..."));
    wifi_scan_result_payload_t scanResult;
    memset(&scanResult, 0, sizeof(scanResult));

    if (m_comm.ScanWifi(&scanResult)) {
        outAps.clear();
        for (uint8_t i = 0; i < scanResult.count; i++) {
            outAps.push_back(scanResult.ap[i]);
        }
        AppendLog(_T("Wi-Fi scan completed successfully."));
        return true;
    }

    AppendLog(_T("ScanWifi failed: ") + m_comm.GetLastErrorMsg(), true);
    return false;
}

bool CSolarDisplayManagerDlg::QueryIotStatus(hrf_iot_status_payload_t& outStatus)
{
    if (!EnsureConnected()) return false;

    AppendLog(_T("Querying Cloud & IoT status from target..."));
    if (m_comm.ReadIotStatus(&outStatus)) {
        AppendLog(_T("Cloud & IoT status received."));
        return true;
    }

    AppendLog(_T("ReadIotStatus failed: ") + m_comm.GetLastErrorMsg(), true);
    return false;
}

void CSolarDisplayManagerDlg::OnBnClickedBtnClearLog()

{
    m_editLog.SetWindowText(_T(""));
}

LRESULT CSolarDisplayManagerDlg::OnFlashProgress(WPARAM wParam, LPARAM lParam)
{
    m_progressFlash.SetPos((int)wParam);
    return 0;
}

LRESULT CSolarDisplayManagerDlg::OnFlashComplete(WPARAM wParam, LPARAM lParam)
{
    BOOL success = (BOOL)wParam;
    SetUIEnabled(TRUE);
    RefreshComPorts();
    if (success) {
        m_progressFlash.SetPos(100);
        m_staticStatus.SetWindowText(_T("Status: Flashing completed successfully. Ready to connect!"));
        AppendLog(_T("Flashing finished. All controls re-enabled. Ready to Connect."));
    } else {
        m_staticStatus.SetWindowText(_T("Status: Flashing failed. Check log."));
        AppendLog(_T("Flashing ended with errors. All controls re-enabled."), true);
    }
    return 0;
}
