#pragma once

#include <windows.h>
#include <afxstr.h>
#include <functional>

typedef std::function<void(int progressPct, const CString& statusLine)> FlasherProgressCallback;

class CEspFlasher
{
public:
    CEspFlasher();
    ~CEspFlasher();

    bool FlashFirmware(
        const CString& portName,
        DWORD baudRate,
        const CString& bootloaderPath,
        const CString& partitionTablePath,
        const CString& appBinPath,
        const CString& logoBinPath,
        FlasherProgressCallback progressCallback,
        CString& outError
    );

    static CString FindDefaultEsptoolPath();
    static CString FindDefaultAppBinPath();
    static CString FindDefaultBootloaderPath();
    static CString FindDefaultPartitionPath();
};
