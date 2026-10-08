#include "pch.h"
#include "EspFlasher.h"
#include "Resource.h"
#include "OEMPayload.h"
#include "pcu_protocol.h"
#include <string>
#include <vector>
#include <gdiplus.h>
#include <ole2.h>

CEspFlasher::CEspFlasher()
{
}

CEspFlasher::~CEspFlasher()
{
}

bool CEspFlasher::ExtractResourceToFile(UINT resId, const CString& outFilePath)
{
    HRSRC hRes = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hRes) {
        hRes = FindResource(NULL, MAKEINTRESOURCE(resId), RT_RCDATA);
    }
    if (!hRes) return false;

    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return false;

    DWORD size = SizeofResource(NULL, hRes);
    const void* pData = LockResource(hMem);
    if (!pData || size == 0) return false;

    CFile file;
    if (!file.Open(outFilePath, CFile::modeCreate | CFile::modeWrite)) {
        return false;
    }
    file.Write(pData, size);
    file.Close();
    return true;
}

bool CEspFlasher::ConvertPngToLogoBin(const uint8_t* pPngData, size_t pngLen, const CString& outBinPath)
{
    if (!pPngData || pngLen == 0) return false;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, pngLen);
    if (!hMem) return false;
    void* pMem = GlobalLock(hMem);
    if (pMem) {
        memcpy(pMem, pPngData, pngLen);
        GlobalUnlock(hMem);
    }

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK || !pStream) {
        return false;
    }

    Gdiplus::Bitmap bmp(pStream);
    pStream->Release();

    if (bmp.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    UINT origW = bmp.GetWidth();
    UINT origH = bmp.GetHeight();
    if (origW == 0 || origH == 0) return false;

    // Detect non-transparent content bounding box
    INT minX = (INT)origW, maxX = -1;
    INT minY = (INT)origH, maxY = -1;

    Gdiplus::BitmapData bmpData;
    Gdiplus::Rect fullRect(0, 0, origW, origH);
    if (bmp.LockBits(&fullRect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Gdiplus::Ok) {
        BYTE* pScan = (BYTE*)bmpData.Scan0;
        for (UINT y = 0; y < origH; y++) {
            DWORD* pRow = (DWORD*)(pScan + y * bmpData.Stride);
            for (UINT x = 0; x < origW; x++) {
                BYTE a = (BYTE)((pRow[x] >> 24) & 0xFF);
                if (a > 10) {
                    if ((INT)x < minX) minX = (INT)x;
                    if ((INT)x > maxX) maxX = (INT)x;
                    if ((INT)y < minY) minY = (INT)y;
                    if ((INT)y > maxY) maxY = (INT)y;
                }
            }
        }
        bmp.UnlockBits(&bmpData);
    }

    Gdiplus::RectF srcRect;
    if (maxX >= minX && maxY >= minY) {
        srcRect = Gdiplus::RectF((float)minX, (float)minY, (float)(maxX - minX + 1), (float)(maxY - minY + 1));
    } else {
        srcRect = Gdiplus::RectF(0.0f, 0.0f, (float)origW, (float)origH);
    }

    // 52x52 High-Fidelity ARGB8888 logo with full alpha transparency
    UINT targetW = 52;
    UINT targetH = 52;

    float aspect = srcRect.Width / srcRect.Height;
    float drawW = (float)targetW;
    float drawH = (float)targetH;
    if (aspect > 1.0f) {
        drawH = (float)targetW / aspect;
    } else {
        drawW = (float)targetH * aspect;
    }
    float drawX = ((float)targetW - drawW) / 2.0f;
    float drawY = ((float)targetH - drawH) / 2.0f;

    Gdiplus::Bitmap scaledBmp(targetW, targetH, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&scaledBmp);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        g.Clear(Gdiplus::Color(0, 0, 0, 0)); // 100% Transparent background (NO black box!)
        g.DrawImage(&bmp, Gdiplus::RectF(drawX, drawY, drawW, drawH), srcRect.X, srcRect.Y, srcRect.Width, srcRect.Height, Gdiplus::UnitPixel);
    }

    std::vector<uint8_t> pixelData;
    pixelData.reserve(targetW * targetH * 4);

    for (UINT y = 0; y < targetH; y++) {
        for (UINT x = 0; x < targetW; x++) {
            Gdiplus::Color pixel;
            scaledBmp.GetPixel(x, y, &pixel);

            uint8_t a = pixel.GetA();
            uint8_t r = pixel.GetR();
            uint8_t g = pixel.GetG();
            uint8_t b = pixel.GetB();

            // LVGL v9 ARGB8888 Memory Layout on Little-Endian ESP32: B, G, R, A
            pixelData.push_back(b);
            pixelData.push_back(g);
            pixelData.push_back(r);
            pixelData.push_back(a);
        }
    }

    pcu_logo_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = PCU_LOGO_MAGIC;
    hdr.width = (uint16_t)targetW;
    hdr.height = (uint16_t)targetH;
    hdr.cf = 2; // ARGB8888 (32-bit TrueColor with 8-bit Alpha Channel)
    hdr.data_size = (uint32_t)pixelData.size();
    hdr.crc32 = pcu_calc_crc32(pixelData.data(), pixelData.size());

    CFile outFile;
    if (!outFile.Open(outBinPath, CFile::modeCreate | CFile::modeWrite)) {
        return false;
    }
    outFile.Write(&hdr, sizeof(hdr));
    outFile.Write(pixelData.data(), (UINT)pixelData.size());
    outFile.Close();

    return true;
}

static void DeleteTempDirectory(const CString& path)
{
    CFileFind finder;
    CString searchPath = path + _T("\\*.*");
    BOOL bWorking = finder.FindFile(searchPath);

    while (bWorking) {
        bWorking = finder.FindNextFile();
        if (finder.IsDots()) continue;
        if (!finder.IsDirectory()) {
            DeleteFile(finder.GetFilePath());
        }
    }
    finder.Close();
    RemoveDirectory(path);
}

bool CEspFlasher::FlashMonolithicFirmware(
    const CString& portName,
    DWORD baudRate,
    FlasherProgressCallback progressCallback,
    CString& outError
)
{
    // 1. Create unique secure temporary staging directory
    TCHAR tempBase[MAX_PATH];
    GetTempPath(MAX_PATH, tempBase);

    CString stagingDir;
    stagingDir.Format(_T("%sSolarOEM_Flasher_%lu"), tempBase, GetCurrentProcessId());
    CreateDirectory(stagingDir, NULL);

    CString exePath   = stagingDir + _T("\\esptool.exe");
    CString bootPath  = stagingDir + _T("\\bootloader.bin");
    CString partPath  = stagingDir + _T("\\partition-table.bin");
    CString appPath   = stagingDir + _T("\\firmware.bin");
    CString logoPath  = stagingDir + _T("\\logo_image.bin");

    if (progressCallback) progressCallback(5, _T("Extracting embedded standalone resources..."));

    // Extract resources
    if (!ExtractResourceToFile(IDR_BIN_ESPTOOL, exePath)) {
        outError = _T("Failed to extract embedded esptool.exe.");
        DeleteTempDirectory(stagingDir);
        return false;
    }

    if (!ExtractResourceToFile(IDR_BIN_BOOTLOADER, bootPath) ||
        !ExtractResourceToFile(IDR_BIN_PARTITIONS, partPath) ||
        !ExtractResourceToFile(IDR_BIN_FIRMWARE, appPath)) {
        outError = _T("Failed to extract embedded ESP32-S3 firmware binaries.");
        DeleteTempDirectory(stagingDir);
        return false;
    }

    // Convert and write Master OEM logo partition binary
    size_t pngLen = 0;
    const uint8_t* pPng = OEMPayload::GetMasterLogoPng(pngLen);
    if (!ConvertPngToLogoBin(pPng, pngLen, logoPath)) {
        outError = _T("Failed to generate OEM logo partition binary.");
        DeleteTempDirectory(stagingDir);
        return false;
    }

    if (progressCallback) progressCallback(15, _T("Connecting to ESP32-S3 ROM bootloader..."));

    // 2. Build esptool command line for ESP32-S3 (16MB Flash, 80MHz, Modern esptool v5.x syntax)
    CString cmdLine;
    cmdLine.Format(
        _T("\"%s\" --chip esp32s3 --port %s --baud %lu --before default-reset --after hard-reset write-flash -z ")
        _T("--flash-mode dio --flash-freq 80m --flash-size 16MB ")
        _T("0x0 \"%s\" 0x8000 \"%s\" 0x10000 \"%s\" 0x410000 \"%s\""),
        (LPCTSTR)exePath,
        (LPCTSTR)portName,
        baudRate,
        (LPCTSTR)bootPath,
        (LPCTSTR)partPath,
        (LPCTSTR)appPath,
        (LPCTSTR)logoPath
    );

    // 3. Setup anonymous pipes for stdout/stderr redirection
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        outError = _T("Failed to create anonymous pipe for esptool execution.");
        DeleteTempDirectory(stagingDir);
        return false;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    SecureZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    std::vector<TCHAR> cmdBuffer(cmdLine.GetLength() + 1);
    _tcscpy_s(cmdBuffer.data(), cmdBuffer.size(), (LPCTSTR)cmdLine);

    if (!CreateProcess(
        NULL,
        cmdBuffer.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        stagingDir,
        &si,
        &pi
    )) {
        CloseHandle(hWritePipe);
        CloseHandle(hReadPipe);
        outError.Format(_T("CreateProcess failed for esptool (Error %lu)"), GetLastError());
        DeleteTempDirectory(stagingDir);
        return false;
    }

    CloseHandle(hWritePipe);

    // 4. Stream process stdout / stderr
    char buffer[512];
    DWORD bytesRead = 0;
    std::string lineAccumulator;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        lineAccumulator += buffer;

        size_t pos = 0;
        while ((pos = lineAccumulator.find('\n')) != std::string::npos) {
            std::string singleLine = lineAccumulator.substr(0, pos);
            lineAccumulator.erase(0, pos + 1);

            while (!singleLine.empty() && (singleLine.back() == '\r' || singleLine.back() == ' ')) {
                singleLine.pop_back();
            }

            if (!singleLine.empty()) {
                CString lineMsg = CA2T(singleLine.c_str());

                int pct = -1;
                size_t pctPos = singleLine.find('%');
                if (pctPos != std::string::npos && pctPos >= 3) {
                    std::string numStr;
                    for (int i = (int)pctPos - 1; i >= 0 && (isdigit((unsigned char)singleLine[i]) || singleLine[i] == ' '); i--) {
                        if (isdigit((unsigned char)singleLine[i])) {
                            numStr = singleLine[i] + numStr;
                        }
                    }
                    if (!numStr.empty()) {
                        pct = atoi(numStr.c_str());
                    }
                }

                if (pct >= 0 && pct <= 100) {
                    int mappedProgress = 15 + (int)(pct * 0.80f);
                    if (progressCallback) progressCallback(mappedProgress, lineMsg);
                } else {
                    if (progressCallback) progressCallback(-1, lineMsg);
                }
            }
        }
    }

    CloseHandle(hReadPipe);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 5. Clean up temporary files
    DeleteTempDirectory(stagingDir);

    if (exitCode != 0) {
        outError.Format(_T("esptool returned error code %lu."), exitCode);
        return false;
    }

    if (progressCallback) progressCallback(100, _T("ESP32-S3 Flash Complete!"));
    return true;
}
