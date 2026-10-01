dlg_cpp_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\SolarDisplayManagerDlg.cpp'

with open(dlg_cpp_path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update DoDataExchange
old_ddx = '''    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);

    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);'''

new_ddx = '''    DDX_Control(pDX, IDC_COMBO_TELEM_BAUD, m_comboTelemBaud);
    DDX_Control(pDX, IDC_EDIT_LOGO_PATH, m_editLogoPath);

    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);'''

text = text.replace(old_ddx, new_ddx, 1)

# 2. Update MESSAGE_MAP
old_msg_map = '''    ON_BN_CLICKED(IDC_BTN_FLASH, &CSolarDisplayManagerDlg::OnBnClickedBtnFlash)
    ON_BN_CLICKED(IDC_BTN_READ_CONFIG, &CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig)'''

new_msg_map = '''    ON_BN_CLICKED(IDC_BTN_FLASH, &CSolarDisplayManagerDlg::OnBnClickedBtnFlash)
    ON_BN_CLICKED(IDC_BTN_BROWSE_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnBrowseLogo)
    ON_BN_CLICKED(IDC_BTN_FLASH_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnFlashLogo)
    ON_BN_CLICKED(IDC_BTN_CLEAR_LOGO, &CSolarDisplayManagerDlg::OnBnClickedBtnClearLogo)
    ON_BN_CLICKED(IDC_BTN_READ_CONFIG, &CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig)'''

text = text.replace(old_msg_map, new_msg_map, 1)

# 3. Update SetUIEnabled
old_ui_enable = '''    GetDlgItem(IDC_BTN_FLASH)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(bEnable);'''

new_ui_enable = '''    GetDlgItem(IDC_BTN_FLASH)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_BROWSE_LOGO)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_FLASH_LOGO)->EnableWindow(bEnable);
    GetDlgItem(IDC_BTN_CLEAR_LOGO)->EnableWindow(bEnable);'''

text = text.replace(old_ui_enable, new_ui_enable, 1)

# 4. Add ConvertImageToLogoBin, OnBnClickedBtnBrowseLogo, OnBnClickedBtnFlashLogo, OnBnClickedBtnClearLogo
logo_handlers = '''bool CSolarDisplayManagerDlg::ConvertImageToLogoBin(const CString& imagePath, std::vector<uint8_t>& outLogoData, CString& outError)
{
    CImage srcImg;
    HRESULT hr = srcImg.Load(imagePath);
    if (FAILED(hr)) {
        outError = _T("Failed to load image file. Supported formats: PNG, JPG, BMP, ICO.");
        return false;
    }

    const int TARGET_W = 48;
    const int TARGET_H = 48;

    CImage dstImg;
    if (!dstImg.Create(TARGET_W, TARGET_H, 24)) {
        outError = _T("Failed to create destination image canvas.");
        return false;
    }

    HDC hdc = dstImg.GetDC();
    HBRUSH hBrush = CreateSolidBrush(RGB(12, 21, 36));
    RECT rc = { 0, 0, TARGET_W, TARGET_H };
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    int srcW = srcImg.GetWidth();
    int srcH = srcImg.GetHeight();
    if (srcW <= 0 || srcH <= 0) {
        dstImg.ReleaseDC();
        outError = _T("Invalid image dimensions.");
        return false;
    }

    float scale = min((float)TARGET_W / srcW, (float)TARGET_H / srcH);
    int drawW = (int)(srcW * scale);
    int drawH = (int)(srcH * scale);
    int drawX = (TARGET_W - drawW) / 2;
    int drawY = (TARGET_H - drawH) / 2;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, NULL);
    srcImg.StretchBlt(hdc, drawX, drawY, drawW, drawH, 0, 0, srcW, srcH, SRCCOPY);
    dstImg.ReleaseDC();

    uint32_t pixelDataSize = TARGET_W * TARGET_H * 2;
    outLogoData.resize(sizeof(pcu_logo_header_t) + pixelDataSize);

    pcu_logo_header_t* hdr = (pcu_logo_header_t*)outLogoData.data();
    hdr->magic = PCU_LOGO_MAGIC;
    hdr->width = TARGET_W;
    hdr->height = TARGET_H;
    hdr->cf = 1;
    hdr->reserved = 0;
    hdr->data_size = pixelDataSize;

    uint8_t* pixelPtr = outLogoData.data() + sizeof(pcu_logo_header_t);

    for (int y = 0; y < TARGET_H; y++) {
        for (int x = 0; x < TARGET_W; x++) {
            COLORREF c = dstImg.GetPixel(x, y);
            uint8_t r = GetRValue(c);
            uint8_t g = GetGValue(c);
            uint8_t b = GetBValue(c);

            uint16_t r5 = (r >> 3) & 0x1F;
            uint16_t g6 = (g >> 2) & 0x3F;
            uint16_t b5 = (b >> 3) & 0x1F;
            uint16_t rgb565 = (uint16_t)((r5 << 11) | (g6 << 5) | b5);

            *pixelPtr++ = (uint8_t)(rgb565 & 0xFF);
            *pixelPtr++ = (uint8_t)((rgb565 >> 8) & 0xFF);
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
            int slash = appBinPath.ReverseFind(_T('\\\\'));
            if (slash != -1) {
                CString binPath = appBinPath.Left(slash) + _T("\\\\logo_image.bin");
                CFile file;
                if (file.Open(binPath, CFile::modeCreate | CFile::modeWrite)) {
                    file.Write(logoData.data(), (UINT)logoData.size());
                    file.Close();
                }
            }
            AppendLog(CString(_T("Custom logo loaded & converted: 48x48 RGB565 (4,628 bytes). Ready to flash!")));
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
    int slash = appBinPath.ReverseFind(_T('\\\\'));
    if (slash != -1) {
        CString binPath = appBinPath.Left(slash) + _T("\\\\logo_image.bin");
        CFile file;
        if (file.Open(binPath, CFile::modeCreate | CFile::modeWrite)) {
            file.Write(logoData.data(), (UINT)logoData.size());
            file.Close();
        }
    }

    bool openedLocally = false;
    if (!m_comm.IsOpen()) {
        CString portName;
        m_comboPort.GetWindowText(portName);
        if (portName.IsEmpty()) {
            AfxMessageBox(_T("Please select the target COM port."));
            return;
        }
        if (!m_comm.Open(portName, 115200)) {
            AppendLog(m_comm.GetLastErrorMsg(), true);
            AfxMessageBox(m_comm.GetLastErrorMsg());
            return;
        }
        openedLocally = true;
    }

    AppendLog(_T("Flashing custom logo to ESP32 Flash (48x48 RGB565)..."));
    if (m_comm.WriteLogo(logoData.data(), logoData.size())) {
        AppendLog(_T("SUCCESS: Custom logo flashed & committed to NVS! Target is rebooting into Boot Screen with new logo!"));
        AfxMessageBox(_T("Custom logo successfully flashed!\\nTarget is rebooting with your new custom logo on the Boot Screen & Vendor Screen."), MB_ICONINFORMATION);
    } else {
        AppendLog(_T("Flash logo failed: ") + m_comm.GetLastErrorMsg(), true);
        AfxMessageBox(_T("Failed to write logo to target.\\nMake sure unit is connected."), MB_ICONWARNING);
    }

    if (openedLocally) {
        m_comm.Close();
    }
}

void CSolarDisplayManagerDlg::OnBnClickedBtnClearLogo()
{
    m_editLogoPath.SetWindowText(_T(""));

    bool openedLocally = false;
    if (!m_comm.IsOpen()) {
        CString portName;
        m_comboPort.GetWindowText(portName);
        if (portName.IsEmpty()) {
            AfxMessageBox(_T("Please select the target COM port."));
            return;
        }
        if (!m_comm.Open(portName, 115200)) {
            AppendLog(m_comm.GetLastErrorMsg(), true);
            return;
        }
        openedLocally = true;
    }

    AppendLog(_T("Clearing custom logo from ESP32 Flash..."));
    if (m_comm.ClearLogo()) {
        AppendLog(_T("SUCCESS: Custom logo cleared! Restored default solar logo. Target rebooting."));
        AfxMessageBox(_T("Custom logo cleared.\\nRestored factory default solar logo."), MB_ICONINFORMATION);
    } else {
        AppendLog(_T("Clear logo failed: ") + m_comm.GetLastErrorMsg(), true);
    }

    if (openedLocally) {
        m_comm.Close();
    }
}
'''

# Insert before OnBnClickedBtnReadConfig
text = text.replace('void CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig()', logo_handlers + '\nvoid CSolarDisplayManagerDlg::OnBnClickedBtnReadConfig()', 1)

with open(dlg_cpp_path, 'w', encoding='utf-8') as f:
    f.write(text)

print('Successfully patched SolarDisplayManagerDlg.cpp with logo event handlers!')
