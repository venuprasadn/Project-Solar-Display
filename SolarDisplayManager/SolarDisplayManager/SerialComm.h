#pragma once

#include <windows.h>
#include <vector>
#include <afxstr.h>
#include "pcu_protocol.h"

class CSerialComm
{
public:
    CSerialComm();
    ~CSerialComm();

    bool Open(const CString& portName, DWORD baudRate = 115200);
    void Close();
    bool IsOpen() const { return m_hSerial != INVALID_HANDLE_VALUE; }

    // Industrial Frame Transmission
    bool SendFrame(uint8_t cmd, const uint8_t* payload = nullptr, uint16_t length = 0, uint8_t seq = 0);
    bool ReceiveFrame(hrf_header_t* outHdr, uint8_t* outPayload, uint16_t maxPayloadLen, DWORD timeoutMs = 1500);

    // High-Level Service Commands
    bool Ping(hrf_ping_resp_t* outPing);
    bool ReadConfig(pcu_config_t* outCfg);
    bool WriteConfig(const pcu_config_t* inCfg);
    bool CommitNVS();
    bool FactoryReset();
    bool Reboot();
    bool WriteLogo(const uint8_t* logoData, size_t totalSize);
    bool ClearLogo();

    // Port Enumeration
    static std::vector<CString> EnumeratePorts();

    CString GetLastErrorMsg() const { return m_lastError; }

private:
    HANDLE m_hSerial;
    uint8_t m_seq;
    CString m_lastError;

    bool WriteExact(const uint8_t* buffer, DWORD bytesToWrite);
    bool ReadExact(uint8_t* buffer, DWORD bytesToRead, DWORD timeoutMs);
};
