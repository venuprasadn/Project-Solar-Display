#include "pch.h"
#include "EspFlasher.h"
#include <string>
#include <vector>

CEspFlasher::CEspFlasher()
{
}

CEspFlasher::~CEspFlasher()
{
}

static CString GetAppDir()
{
    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
        TCHAR* pLastSlash = _tcsrchr(szPath, _T('\\'));
        if (pLastSlash) {
            *pLastSlash = _T('\0');
            return CString(szPath);
        }
    }
    return _T(".");
}

CString CEspFlasher::FindDefaultEsptoolPath()
{
    CString appDir = GetAppDir();
    CString candidates[] = {
        appDir + _T("\\esptools\\esptool.exe"),
        appDir + _T("\\tools\\esptool.exe"),
        appDir + _T("\\esptool.exe"),
        _T("esptools\\esptool.exe"),
        _T("D:\\Project_Solar_Display\\SolarDisplayManager\\esptools\\esptool.exe"),
        _T("D:\\Project_Solar_Display\\SolarDisplayManager\\tools\\esptool-windows-amd64\\esptool.exe"),
        _T("tools\\esptool.exe"),
        _T("C:\\Espressif\\tools\\python\\v6.1\\venv\\Scripts\\esptool.exe")
    };
    for (const auto& path : candidates) {
        if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return _T("esptool.exe");
}

CString CEspFlasher::FindDefaultAppBinPath()
{
    CString appDir = GetAppDir();
    CString local = appDir + _T("\\firmware\\esp32_ili9341_lcd.bin");
    if (GetFileAttributes(local) != INVALID_FILE_ATTRIBUTES) return local;

    return _T("D:\\Project_Solar_Display\\Project-ESP32-LCD\\build\\esp32_ili9341_lcd.bin");
}

CString CEspFlasher::FindDefaultBootloaderPath()
{
    CString appDir = GetAppDir();
    CString local = appDir + _T("\\firmware\\bootloader.bin");
    if (GetFileAttributes(local) != INVALID_FILE_ATTRIBUTES) return local;

    return _T("D:\\Project_Solar_Display\\Project-ESP32-LCD\\build\\bootloader\\bootloader.bin");
}

CString CEspFlasher::FindDefaultPartitionPath()
{
    CString appDir = GetAppDir();
    CString local = appDir + _T("\\firmware\\partition-table.bin");
    if (GetFileAttributes(local) != INVALID_FILE_ATTRIBUTES) return local;

    return _T("D:\\Project_Solar_Display\\Project-ESP32-LCD\\build\\partition_table\\partition-table.bin");
}

bool CEspFlasher::FlashFirmware(
    const CString& portName,
    DWORD baudRate,
    const CString& bootloaderPath,
    const CString& partitionTablePath,
    const CString& appBinPath,
    const CString& logoBinPath,
    FlasherProgressCallback progressCallback,
    CString& outError
)
{
    CString esptoolExe = FindDefaultEsptoolPath();
    if (GetFileAttributes(esptoolExe) == INVALID_FILE_ATTRIBUTES) {
        outError.Format(_T("esptool.exe not found at %s"), (LPCTSTR)esptoolExe);
        return false;
    }

    if (GetFileAttributes(appBinPath) == INVALID_FILE_ATTRIBUTES) {
        outError.Format(_T("Application binary not found at %s"), (LPCTSTR)appBinPath);
        return false;
    }

    // Clean port name (e.g. COM3)
    CString cleanPort = portName;
    cleanPort.Replace(_T("\\\\.\\"), _T(""));

    // Find custom logo binary
    CString logoBin = logoBinPath;
    if (logoBin.IsEmpty() || GetFileAttributes(logoBin) == INVALID_FILE_ATTRIBUTES) {
        int slash = appBinPath.ReverseFind(_T('\\'));
        if (slash != -1) {
            CString candidate = appBinPath.Left(slash) + _T("\\logo_image.bin");
            if (GetFileAttributes(candidate) != INVALID_FILE_ATTRIBUTES) {
                logoBin = candidate;
            }
        }
    }
    if (logoBin.IsEmpty() || GetFileAttributes(logoBin) == INVALID_FILE_ATTRIBUTES) {
        CString appDir = GetAppDir();
        CString candidate = appDir + _T("\\firmware\\logo_image.bin");
        if (GetFileAttributes(candidate) != INVALID_FILE_ATTRIBUTES) {
            logoBin = candidate;
        }
    }
    if (logoBin.IsEmpty() || GetFileAttributes(logoBin) == INVALID_FILE_ATTRIBUTES) {
        CString candidate = _T("D:\\Project_Solar_Display\\SolarDisplayManager_Distribution\\firmware\\logo_image.bin");
        if (GetFileAttributes(candidate) != INVALID_FILE_ATTRIBUTES) {
            logoBin = candidate;
        }
    }

    bool hasLogo = (!logoBin.IsEmpty() && GetFileAttributes(logoBin) != INVALID_FILE_ATTRIBUTES);

    CString cmdLine;
    if (hasLogo) {
        if (progressCallback) {
            progressCallback(0, CString(_T("Custom logo binary detected: ")) + logoBin + _T(" (Flashing to 0x190000)"));
        }
        cmdLine.Format(
            _T("\"%s\" --chip esp32 -p %s -b %lu --before default-reset --after hard-reset write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m 0x1000 \"%s\" 0x8000 \"%s\" 0x10000 \"%s\" 0x190000 \"%s\""),
            (LPCTSTR)esptoolExe,
            (LPCTSTR)cleanPort,
            baudRate,
            (LPCTSTR)bootloaderPath,
            (LPCTSTR)partitionTablePath,
            (LPCTSTR)appBinPath,
            (LPCTSTR)logoBin
        );
    } else {
        cmdLine.Format(
            _T("\"%s\" --chip esp32 -p %s -b %lu --before default-reset --after hard-reset write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m 0x1000 \"%s\" 0x8000 \"%s\" 0x10000 \"%s\""),
            (LPCTSTR)esptoolExe,
            (LPCTSTR)cleanPort,
            baudRate,
            (LPCTSTR)bootloaderPath,
            (LPCTSTR)partitionTablePath,
            (LPCTSTR)appBinPath
        );
    }

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        outError = _T("Failed to create pipe for process redirection.");
        return false;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si;
    SecureZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    SecureZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    std::vector<TCHAR> cmdBuf(cmdLine.GetLength() + 1);
    _tcscpy_s(cmdBuf.data(), cmdBuf.size(), (LPCTSTR)cmdLine);

    if (!CreateProcess(NULL, cmdBuf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        outError.Format(_T("CreateProcess failed (%lu)"), GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe); // Close child end in parent

    char buffer[512];
    DWORD bytesRead = 0;
    std::string accumulated = "";

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        accumulated += buffer;

        // Process line by line
        size_t pos = 0;
        while ((pos = accumulated.find('\n')) != std::string::npos || (pos = accumulated.find('\r')) != std::string::npos) {
            std::string line = accumulated.substr(0, pos);
            accumulated.erase(0, pos + 1);

            if (line.empty()) continue;

            CString lineStr(line.c_str());
            lineStr.Trim();

            // Check for progress percentage (e.g. "( 45 %)" or "(45 %)")
            int pct = -1;
            size_t pOpen = line.find('(');
            size_t pPct = line.find('%');
            if (pOpen != std::string::npos && pPct != std::string::npos && pPct > pOpen) {
                std::string numStr = line.substr(pOpen + 1, pPct - pOpen - 1);
                pct = atoi(numStr.c_str());
            }

            if (progressCallback) {
                progressCallback(pct, lineStr);
            }
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    if (exitCode != 0) {
        outError.Format(_T("esptool failed with exit code %lu"), exitCode);
        return false;
    }

    if (progressCallback) {
        progressCallback(100, _T("Flashing completed and verified successfully!"));
    }

    return true;
}
