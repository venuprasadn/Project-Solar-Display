// SolarDisplayManagerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "SolarDisplayManager.h"
#include "SolarDisplayManagerDlg.h"
#include "afxdialogex.h"

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
    DDX_Control(pDX, IDC_COMBO_PORT, m_comboPort);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_EDIT_APP_BIN, m_editAppBin);
    DDX_Control(pDX, IDC_COMBO_FLASH_BAUD, m_comboFlashBaud);
    DDX_Control(pDX, IDC_PROGRESS_FLASH, m_progressFlash);
    DDX_Control(pDX, IDC_CHK_AUTO_PROVISION, m_chkAutoProvision);

    DDX_Control(pDX, IDC_EDIT_BRAND, m_editBrand);
    DDX_Control(pDX, IDC_EDIT_MODEL, m_editModel);
    DDX_Control(pDX, IDC_EDIT_SERIAL, m_editSerial);
    DDX_Control(pDX, IDC_EDIT_HW_REV, m_editHwRev);
    DDX_Control(pDX, IDC_EDIT_VENDOR_CONTACT, m_editVendorContact);
    DDX_Control(pDX, IDC_EDIT_VENDOR_WEBSITE, m_editVendorWebsite);
    DDX_Control(pDX, IDC_COMBO_LOGO_THEME, m_comboLogoTheme);
    DDX_Control(pDX, IDC_EDIT_BOOT_SEC, m_editBootSec);
    DDX_Control(pDX, IDC_CHK_CAROUSEL, m_chkCarousel);
    DDX_Control(pDX, IDC_EDIT_CAROUSEL_SEC, m_editCarouselSec);
    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);
    DDX_Control(pDX, IDC_EDIT_LOGO_PATH, m_editLogoPath);

    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BEGIN_MESSAGE_MAP(CSolarDisplayManagerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_REFRESH_PORTS, &CSolarDisplayManagerDlg::OnBnClickedBtnRefreshPorts)
    ON_BN_CLICKED(IDC_BTN_CONNECT, &CSolarDisplayManagerDlg::OnBnClickedBtnConnect)
    ON_BN_CLICKED(IDC_BTN_BROWSE_APP, &CSolarDisplayManagerDlg::OnBnClickedBtnBrowseApp)
    ON_BN_CLICKED(IDC_BTN_FLASH, &CSolarDisplayManagerDlg::OnBnClickedBtnFlash)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnBrowseLogo)
    ON_BN_CLICKED(IDC_BTN_FLASH_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnFlashLogo)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnClearLogo)
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

    // 1. Initialize Flasher Settings
    m_comboFlashBaud.AddString(_T("921600"));
    m_comboFlashBaud.AddString(_T("460800"));
    m_comboFlashBaud.AddString(_T("115200"));
    m_comboFlashBaud.SetCurSel(1); // 460800 default

    m_editAppBin.SetWindowText(CEspFlasher::FindDefaultAppBinPath());
    m_chkAutoProvision.SetCheck(BST_CHECKED);
    m_progressFlash.SetRange(0, 100);
    m_progressFlash.SetPos(0);

    // 2. Initialize Configuration Dropdowns
    m_comboLogoTheme.AddString(_T("0 - Tactical Amber (Solar)"));
    m_comboLogoTheme.AddString(_T("1 - Cyber Cyan (Aerospace)"));
    m_comboLogoTheme.AddString(_T("2 - Emerald Defense (Military)"));
    m_comboLogoTheme.AddString(_T("3 - Crimson Alert (Industrial)"));
    m_comboLogoTheme.AddString(_T("4 - Titanium Cobalt (Marine)"));
    m_comboLogoTheme.AddString(_T("5 - Stealth Monochrome (Flight Deck)"));
    m_comboLogoTheme.SetCurSel(0);

    m_comboTelemBaud.AddString(_T("9600"));
    m_comboTelemBaud.AddString(_T("19200"));
    m_comboTelemBaud.AddString(_T("38400"));
    m_comboTelemBaud.AddString(_T("115200"));
    m_comboTelemBaud.SetCurSel(0); // 9600

    // 3. Load Factory Default Values to UI
    pcu_config_t defCfg;
    memset(&defCfg, 0, sizeof(defCfg));
    strcpy_s(defCfg.brand_title, "HYBRID PSU");
    strcpy_s(defCfg.model_name, "SOLAR INVERTER");
    strcpy_s(defCfg.serial_number, "HPSU-2026-X8849");
    strcpy_s(defCfg.hardware_version, "HW-V2.1");
    strcpy_s(defCfg.vendor_contact, "Toll Free: 1800-425-9999");
    strcpy_s(defCfg.vendor_website, "www.hybrid-psu.com");
    defCfg.production_date = 20261001;
    defCfg.logo_theme = 0;
    defCfg.boot_duration_sec = 3;
    defCfg.auto_carousel_enabled = 1;
    defCfg.carousel_interval_sec = 5;
    defCfg.telemetry_baudrate = 9600;
    LoadConfigToUI(defCfg);

    // 4. Enumerate Available Ports
    RefreshComPorts();

    AppendLog(_T("=== Hybrid PSU Display Manager Initialized ==="));
    AppendLog(_T("System ready. Standards: ISO 9001 / IEC 62109 / MIL-STD-810H"));

    return TRUE;
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

void CSolarDisplayManagerDlg::RefreshComPorts()
{
    m_comboPort.ResetContent();
    std::vector<CString> ports = CSerialComm::EnumeratePorts();

    for (const auto& p : ports) {
        m_comboPort.AddString(p);
    }

    if (m_comboPort.GetCount() > 0) {
        // Try to preselect COM3 if available
        int idx = m_comboPort.FindStringExact(0, _T("COM3"));
        if (idx == CB_ERR) idx = 0;
        m_comboPort.SetCurSel(idx);
    }
}

void CSolarDisplayManagerDlg::LoadConfigToUI(const pcu_config_t& cfg)
{
    m_editBrand.SetWindowText(CString(cfg.brand_title));
    m_editModel.SetWindowText(CString(cfg.model_name));
    m_editSerial.SetWindowText(CString(cfg.serial_number));
    m_editHwRev.SetWindowText(CString(cfg.hardware_version));
    m_editVendorContact.SetWindowText(CString(cfg.vendor_contact));
    m_editVendorWebsite.SetWindowText(CString(cfg.vendor_website));

    m_comboLogoTheme.SetCurSel(cfg.logo_theme % 6);

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
    cfg.logo_theme = (themeSel >= 0 && themeSel <= 5) ? (uint8_t)themeSel : 0;

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
    cfg.config_crc32 = pcu_calc_crc32((const uint8_t*)&cfg, offsetof(pcu_config_t, config_crc32));

    return true;
}

void CSolarDisplayManagerDlg::SetUIEnabled(BOOL bEnable)
{
    GetDlgItem(IDC_BTN_FLASH)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_BROWSE_LOGO)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_FLASH_LOGO)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CLEAR_LOGO)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_READ_CONFIG)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_WRITE_CONFIG)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_REBOOT)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_FACTORY_RESET)->EnableWindow(bEnable);
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

    CString portName;
    m_comboPort.GetWindowText(portName);
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

    AppendLog(_T("Synchronizing with ESP32 service protocol..."));
    hrf_ping_resp_t ping;
    if (!m_comm.Ping(&ping)) {
        AppendLog(_T("No response from ESP32 on ") + portName + _T(". Check power and cable connection."), true);
        AfxMessageBox(_T("Could not communicate with ESP32 on ") + portName + _T(".\nPlease verify the board is powered and COM port is correct."), MB_ICONERROR);
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
    AppendLog(CString(_T("Connected! ESP32 State: ")) + devState);

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
        AppendLog(_T("Connection ready. Click 'Write Config' to apply changes or 'Read Active Config' to load settings."));
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnBrowseApp()
{
    CFileDialog dlg(TRUE, _T("bin"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("Binary Files (*.bin)|*.bin|All Files (*.*)|*.*||"), this);
    if (dlg.DoModal() == IDOK) {
        m_editAppBin.SetWindowText(dlg.GetPathName());
    }
}

// Structure to pass to background flash worker thread
struct FlashThreadParams {
    CSolarDisplayManagerDlg* pDlg;
    CString port;
    DWORD baud;
    CString bootloader;
    CString partitions;
    CString appBin;
    CString logoBin;
    bool autoProvision;
    std::vector<uint8_t> logoData;
    pcu_config_t cfg;
};

static UINT FlashWorkerThread(LPVOID pParam)
{
    FlashThreadParams* params = (FlashThreadParams*)pParam;
    CSolarDisplayManagerDlg* pDlg = params->pDlg;

    CEspFlasher flasher;
    CString err;

    bool success = flasher.FlashFirmware(
        params->port,
        params->baud,
        params->bootloader,
        params->partitions,
        params->appBin,
        params->logoBin,
        [pDlg](int pct, const CString& line) {
            pDlg->PostMessage(WM_USER + 100, (WPARAM)pct, 0);
            pDlg->AppendLog(line);
        },
        err
    );

    if (success) {
        pDlg->AppendLog(_T("FLASHING COMPLETED SUCCESSFULLY!"));

        if (params->autoProvision) {
            pDlg->AppendLog(_T("Connecting to ESP32 in USB Service Mode..."));
            Sleep(800);

            // Reconnect and provision
            CSerialComm comm;
            if (comm.Open(params->port, 115200)) {
                Sleep(400); // Allow USB-UART bridge to stabilize

                hrf_ping_resp_t ping;
                bool reachable = false;
                for (int attempt = 0; attempt < 8; attempt++) {
                    if (comm.Ping(&ping)) {
                        reachable = true;
                        break;
                    }
                    Sleep(300);
                }

                if (reachable) {
                    pDlg->AppendLog(_T("Target reached in USB Service Mode! Provisioning NVS configuration..."));

                    if (!params->logoData.empty()) {
                        pDlg->AppendLog(_T("Auto-provisioning custom logo to ESP32 NVS..."));
                        if (comm.WriteLogo(params->logoData.data(), params->logoData.size())) {
                            pDlg->AppendLog(_T("SUCCESS: Custom logo auto-provisioned to NVS!"));
                        } else {
                            pDlg->AppendLog(CString(_T("Auto-provision logo failed: ")) + comm.GetLastErrorMsg(), true);
                        }
                    }

                    pcu_config_t cfg = params->cfg;

                    if (comm.WriteConfig(&cfg)) {
                        pDlg->AppendLog(_T("Configuration written. Committing to NVS flash..."));
                        if (comm.CommitNVS()) {
                            pDlg->AppendLog(_T("SUCCESS: NVS COMMITTED! Target rebooting."));
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
    CString port;
    m_comboPort.GetWindowText(port);
    if (port.IsEmpty()) {
        AfxMessageBox(_T("Please select a target COM port."));
        return;
    }

    CString appBin;
    m_editAppBin.GetWindowText(appBin);
    if (appBin.IsEmpty() || GetFileAttributes(appBin) == INVALID_FILE_ATTRIBUTES) {
        AfxMessageBox(_T("Application binary file does not exist. Please check the path."));
        return;
    }

    // Close serial connection so esptool can have exclusive COM port access
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
    AppendLog(CString(_T("Starting Flash operation on ")) + port + _T(" at ") + strBaud + _T(" baud..."));

    FlashThreadParams* params = new FlashThreadParams();
    params->pDlg = this;
    params->port = port;
    params->baud = baud;
    params->bootloader = CEspFlasher::FindDefaultBootloaderPath();
    params->partitions = CEspFlasher::FindDefaultPartitionPath();
    params->appBin = appBin;
    params->autoProvision = (m_chkAutoProvision.GetCheck() == BST_CHECKED);
    CollectConfigFromUI(params->cfg);

    // Prepare logo data if specified
    CString logoPath;
    m_editLogoPath.GetWindowText(logoPath);
    if (!logoPath.IsEmpty() && GetFileAttributes(logoPath) != INVALID_FILE_ATTRIBUTES) {
        CString convErr;
        if (ConvertImageToLogoBin(logoPath, params->logoData, convErr)) {
            int slash = appBin.ReverseFind(_T('\\'));
            if (slash != -1) {
                CString binPath = appBin.Left(slash) + _T("\\logo_image.bin");
                CFile file;
                CFileException ex;
                if (file.Open(binPath, CFile::modeCreate | CFile::modeWrite, &ex)) {
                    file.Write(params->logoData.data(), (UINT)params->logoData.size());
                    file.Close();
                    params->logoBin = binPath;
                }
            }
        }
    } else {
        int slash = appBin.ReverseFind(_T('\\'));
        if (slash != -1) {
            CString binPath = appBin.Left(slash) + _T("\\logo_image.bin");
            CFile file;
            CFileException ex;
            if (file.Open(binPath, CFile::modeRead, &ex)) {
                UINT len = (UINT)file.GetLength();
                params->logoData.resize(len);
                file.Read(params->logoData.data(), len);
                file.Close();
                params->logoBin = binPath;
            }
        }
    }

    AfxBeginThread(FlashWorkerThread, params);
}

bool CSolarDisplayManagerDlg::ConvertImageToLogoBin(const CString& imagePath, std::vector<uint8_t>& outLogoData, CString& outError)
{
    const int TARGET_W = 48;
    const int TARGET_H = 48;
    const uint32_t pixelDataSize = TARGET_W * TARGET_H * 4; // LV_COLOR_FORMAT_ARGB8888 (9,216 bytes)

    outLogoData.clear();

    if (imagePath.IsEmpty()) {
        outError = _T("Image path is empty.");
        return false;
    }

    CStringW wPath(imagePath);
    Gdiplus::Bitmap* pSrcBmp = Gdiplus::Bitmap::FromFile(wPath.GetString());
    if (!pSrcBmp || pSrcBmp->GetLastStatus() != Gdiplus::Ok) {
        if (pSrcBmp) delete pSrcBmp;
        outError = _T("Failed to load image file. Supported formats: PNG, JPG, BMP, ICO.");
        return false;
    }

    int srcW = (int)pSrcBmp->GetWidth();
    int srcH = (int)pSrcBmp->GetHeight();
    if (srcW <= 0 || srcH <= 0) {
        delete pSrcBmp;
        outError = _T("Invalid image dimensions.");
        return false;
    }

    outLogoData.resize(sizeof(pcu_logo_header_t) + pixelDataSize, 0);

    pcu_logo_header_t* hdr = (pcu_logo_header_t*)outLogoData.data();
    hdr->magic = PCU_LOGO_MAGIC;
    hdr->width = TARGET_W;
    hdr->height = TARGET_H;
    hdr->cf = 2; // 2 = LV_COLOR_FORMAT_ARGB8888
    hdr->reserved = 0;
    hdr->data_size = pixelDataSize;

    {
        // Create 48x48 32-bit ARGB canvas with true transparent background
        Gdiplus::Bitmap dstBmp(TARGET_W, TARGET_H, PixelFormat32bppARGB);
        {
            Gdiplus::Graphics g(&dstBmp);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
            // Clear to 100% transparent (ARGB: 0, 0, 0, 0)
            g.Clear(Gdiplus::Color(0, 0, 0, 0));

            float scale = min((float)TARGET_W / srcW, (float)TARGET_H / srcH);
            int drawW = (int)(srcW * scale);
            int drawH = (int)(srcH * scale);
            if (drawW < 1) drawW = 1;
            if (drawH < 1) drawH = 1;
            int drawX = (TARGET_W - drawW) / 2;
            int drawY = (TARGET_H - drawH) / 2;

            g.DrawImage(pSrcBmp, drawX, drawY, drawW, drawH);
        }
        delete pSrcBmp;
        pSrcBmp = nullptr;

        // Lock bitmap bits in 32bpp ARGB format
        Gdiplus::BitmapData bmpData;
        Gdiplus::Rect rc(0, 0, TARGET_W, TARGET_H);
        if (dstBmp.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Gdiplus::Ok) {
            outLogoData.clear();
            outError = _T("Failed to lock destination bitmap memory.");
            return false;
        }

        uint8_t* pixelPtr = outLogoData.data() + sizeof(pcu_logo_header_t);

        for (int y = 0; y < TARGET_H; y++) {
            uint8_t* rowSrc = (uint8_t*)bmpData.Scan0 + (y * bmpData.Stride);
            for (int x = 0; x < TARGET_W; x++) {
                uint8_t b = rowSrc[x * 4 + 0];
                uint8_t g = rowSrc[x * 4 + 1];
                uint8_t r = rowSrc[x * 4 + 2];
                uint8_t a = rowSrc[x * 4 + 3];

                // LVGL ARGB8888 little-endian order: B, G, R, A (matches img_solar)
                *pixelPtr++ = b;
                *pixelPtr++ = g;
                *pixelPtr++ = r;
                *pixelPtr++ = a;
            }
        }
        dstBmp.UnlockBits(&bmpData);
    }

    // Auto-key out legacy solid backgrounds if the image was NOT transparent
    uint8_t* pix = outLogoData.data() + sizeof(pcu_logo_header_t);
    uint8_t c_b = pix[0];
    uint8_t c_g = pix[1];
    uint8_t c_r = pix[2];
    uint8_t c_a = pix[3];

    if (c_a >= 240) {
        uint32_t tr_idx = (TARGET_W - 1) * 4;
        uint32_t bl_idx = (TARGET_H - 1) * TARGET_W * 4;
        uint32_t br_idx = ((TARGET_H - 1) * TARGET_W + (TARGET_W - 1)) * 4;

        bool is_legacy_dark = (abs((int)c_r - 12) <= 8 && abs((int)c_g - 21) <= 8 && abs((int)c_b - 36) <= 8);
        bool corners_match = (abs((int)pix[tr_idx] - c_b) <= 6 && abs((int)pix[tr_idx + 1] - c_g) <= 6 && abs((int)pix[tr_idx + 2] - c_r) <= 6 &&
                              abs((int)pix[bl_idx] - c_b) <= 6 && abs((int)pix[bl_idx + 1] - c_g) <= 6 && abs((int)pix[bl_idx + 2] - c_r) <= 6 &&
                              abs((int)pix[br_idx] - c_b) <= 6 && abs((int)pix[br_idx + 1] - c_g) <= 6 && abs((int)pix[br_idx + 2] - c_r) <= 6);

        if (is_legacy_dark || corners_match) {
            for (int i = 0; i < TARGET_W * TARGET_H; i++) {
                uint8_t* p = pix + i * 4;
                int dr = abs((int)p[2] - (int)c_r);
                int dg = abs((int)p[1] - (int)c_g);
                int db = abs((int)p[0] - (int)c_b);
                if (dr <= 6 && dg <= 6 && db <= 6) {
                    p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0; // Transparent
                } else if (dr <= 16 && dg <= 16 && db <= 16) {
                    int max_d = max(dr, max(dg, db));
                    uint8_t alpha = (uint8_t)(((max_d - 6) * 255) / 10);
                    if (alpha < p[3]) p[3] = alpha;
                }
            }
        }
    }

    hdr->crc32 = pcu_calc_crc32(outLogoData.data() + sizeof(pcu_logo_header_t), pixelDataSize);
    return true;
}

void CSolarDisplayManagerDlg::OnBnClickedBtnBrowseLogo()
{
    CFileDialog dlg(TRUE, _T("png"), NULL,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        _T("Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.ico)|*.png;*.jpg;*.jpeg;*.bmp;*.ico|All Files (*.*)|*.*||"),
        this);

    if (dlg.DoModal() == IDOK) {
        CString filePath = dlg.GetPathName();
        m_editLogoPath.SetWindowText(filePath);

        std::vector<uint8_t> logoData;
        CString err;
        if (ConvertImageToLogoBin(filePath, logoData, err)) {
            CString appBinPath = CEspFlasher::FindDefaultAppBinPath();
            int slash = appBinPath.ReverseFind(_T('\\'));
            if (slash != -1) {
                CString binPath = appBinPath.Left(slash) + _T("\\logo_image.bin");
                CFile file;
                CFileException ex;
                if (file.Open(binPath, CFile::modeCreate | CFile::modeWrite, &ex)) {
                    file.Write(logoData.data(), (UINT)logoData.size());
                    file.Close();
                }
            }
            CString distBin = _T("D:\\Project_Solar_Display\\SolarDisplayManager_Distribution\\firmware\\logo_image.bin");
            CFile distFile;
            CFileException distEx;
            if (distFile.Open(distBin, CFile::modeCreate | CFile::modeWrite, &distEx)) {
                distFile.Write(logoData.data(), (UINT)logoData.size());
                distFile.Close();
            }
            AppendLog(CString(_T("Custom logo loaded & converted: 48x48 ARGB8888 (9,236 bytes). Ready to flash!")));
        } else {
            AppendLog(err, true);
            AfxMessageBox(err, MB_ICONWARNING);
        }
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnFlashLogo()
{
    CString logoPath;
    m_editLogoPath.GetWindowText(logoPath);
    if (logoPath.IsEmpty()) {
        AfxMessageBox(_T("Please browse and select a logo image first."));
        return;
    }

    std::vector<uint8_t> logoData;
    CString err;
    if (!ConvertImageToLogoBin(logoPath, logoData, err)) {
        AppendLog(err, true);
        AfxMessageBox(err, MB_ICONERROR);
        return;
    }

    CString appBinPath = CEspFlasher::FindDefaultAppBinPath();
    int slash = appBinPath.ReverseFind(_T('\\'));
    if (slash != -1) {
        CString binPath = appBinPath.Left(slash) + _T("\\logo_image.bin");
        CFile file;
        CFileException ex;
        if (file.Open(binPath, CFile::modeCreate | CFile::modeWrite, &ex)) {
            file.Write(logoData.data(), (UINT)logoData.size());
            file.Close();
        }
    }
    CString distBin = _T("D:\\Project_Solar_Display\\SolarDisplayManager_Distribution\\firmware\\logo_image.bin");
    CFile distFile;
    CFileException distEx;
    if (distFile.Open(distBin, CFile::modeCreate | CFile::modeWrite, &distEx)) {
        distFile.Write(logoData.data(), (UINT)logoData.size());
        distFile.Close();
    }

    if (!EnsureConnected()) return;

    AppendLog(_T("Flashing custom logo to ESP32 Flash & NVS (48x48 ARGB8888)..."));
    if (m_comm.WriteLogo(logoData.data(), logoData.size())) {
        AppendLog(_T("SUCCESS: Custom logo flashed & committed to NVS! Target is rebooting into Boot Screen with new logo!"));
        AfxMessageBox(_T("Custom logo successfully flashed!\nTarget is rebooting with your new custom logo on the Boot Screen & Vendor Screen."), MB_ICONINFORMATION);
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Disconnected (Target Rebooted)"));
    } else {
        AppendLog(_T("Flash logo failed: ") + m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(_T("Failed to write logo to target.\nMake sure unit is connected."), MB_ICONWARNING);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnClearLogo()
{
    m_editLogoPath.SetWindowText(_T(""));

    if (!EnsureConnected()) return;

    AppendLog(_T("Clearing custom logo from ESP32 Flash..."));
    if (m_comm.ClearLogo()) {
        AppendLog(_T("SUCCESS: Custom logo cleared! Restored default solar logo. Target rebooting."));
        AfxMessageBox(_T("Custom logo cleared.\nRestored factory default solar logo."), MB_ICONINFORMATION);
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Disconnected (Target Rebooted)"));
    } else {
        AppendLog(_T("Clear logo failed: ") + m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(_T("Failed to clear custom logo from unit."), MB_ICONWARNING);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig()
{
    if (!EnsureConnected()) return;

    AppendLog(_T("Reading active configuration from ESP32 NVS..."));
    pcu_config_t cfg;
    if (m_comm.ReadConfig(&cfg)) {
        LoadConfigToUI(cfg);
        CString themeStr;
        themeStr.Format(_T("Theme %u"), cfg.logo_theme % 6);
        AppendLog(CString(_T("Configuration loaded: Model=")) + CString(cfg.model_name) + _T(", Brand=") + CString(cfg.brand_title) + _T(", ") + themeStr);
        AfxMessageBox(_T("Configuration successfully read from ESP32!"), MB_ICONINFORMATION);
    } else {
        AppendLog(CString(_T("ReadConfig failed: ")) + m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(_T("Failed to read configuration from unit.\nMake sure unit is connected."), MB_ICONWARNING);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnWriteConfig()
{
    if (!EnsureConnected()) return;

    pcu_config_t cfg;
    CollectConfigFromUI(cfg);

    CString themeName;
    m_comboLogoTheme.GetWindowText(themeName);
    AppendLog(CString(_T("Writing configuration to target: Brand='")) + CString(cfg.brand_title) + _T("', Model='") + CString(cfg.model_name) + _T("', Theme='") + themeName + _T("'..."));

    if (m_comm.WriteConfig(&cfg)) {
        AppendLog(_T("Configuration staged successfully. Committing to NVS Flash..."));
        if (m_comm.CommitNVS()) {
            AppendLog(_T("SUCCESS: Configuration committed to NVS! Target is rebooting with new theme & settings."));
            AfxMessageBox(_T("Configuration successfully committed to NVS!\nTarget is rebooting with your new settings and theme."), MB_ICONINFORMATION);
            m_comm.Close();
            SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
            m_staticStatus.SetWindowText(_T("Status: Disconnected (Target Rebooted)"));
        } else {
            AppendLog(_T("Commit to NVS failed!"), true);
            AfxMessageBox(_T("Failed to commit configuration to NVS."), MB_ICONERROR);
        }
    } else {
        AppendLog(CString(_T("WriteConfig failed: ")) + m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(_T("Failed to write configuration to unit."), MB_ICONERROR);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnReboot()
{
    if (!EnsureConnected()) return;

    AppendLog(_T("Sending reboot command to target..."));
    if (m_comm.Reboot()) {
        AppendLog(_T("Reboot command acknowledged. Target resetting."));
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Target Rebooted"));
    } else {
        AppendLog(_T("Reboot command failed."), true);
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnFactoryReset()
{
    if (AfxMessageBox(_T("Are you sure you want to perform a FACTORY RESET?\nThis will erase the NVS configuration partition and the display will turn OFF until reprovisioned!"),
        MB_YESNO | MB_ICONWARNING) != IDYES) {
        return;
    }

    if (!EnsureConnected()) return;

    AppendLog(_T("Sending Factory Reset command to target..."));
    if (m_comm.FactoryReset()) {
        AppendLog(_T("Target NVS erased and reset. Device rebooted in unconfigured state (Display OFF)."));
        AfxMessageBox(_T("Factory Reset completed! Device is unconfigured."), MB_ICONINFORMATION);
        m_comm.Close();
        SetDlgItemText(IDC_BTN_CONNECT, _T("Connect"));
        m_staticStatus.SetWindowText(_T("Status: Target Unconfigured (Display OFF)"));
    } else {
        AppendLog(_T("Factory Reset command failed."), true);
        AfxMessageBox(_T("Failed to execute factory reset."), MB_ICONERROR);
    }
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
