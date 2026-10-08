#pragma once

#include <windows.h>
#include <afxstr.h>
#include <functional>
#include <vector>

typedef std::function<void(int progressPct, const CString& statusLine)> FlasherProgressCallback;

class CEspFlasher
{
public:
    CEspFlasher();
    ~CEspFlasher();

    bool FlashMonolithicFirmware(
        const CString& portName,
        DWORD baudRate,
        FlasherProgressCallback progressCallback,
        CString& outError
    );

    static bool ExtractResourceToFile(UINT resId, const CString& outFilePath);
    static bool ConvertPngToLogoBin(const uint8_t* pPngData, size_t pngLen, const CString& outBinPath);
};
