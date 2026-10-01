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

    bool Open(const CString& portName, DWORD baudRate = 9600);
    void Close();
    bool IsOpen() const { return m_hSerial != INVALID_HANDLE_VALUE; }

    // Stream / String transmission for Inverter Telemetry
    bool SendString(const CStringA& text);
    bool SendBytes(const uint8_t* buffer, DWORD count);
    DWORD ReadAvailable(uint8_t* buffer, DWORD maxLen);

    // Industrial Binary Protocol (HRF v2.0)
    bool SendFrame(uint8_t cmd, const uint8_t* payload = nullptr, uint16_t length = 0, uint8_t seq = 0);
    bool ReceiveFrame(hrf_header_t* outHdr, uint8_t* outPayload, uint16_t maxPayloadLen, DWORD timeoutMs = 1500);

    // Port Enumeration
    static std::vector<CString> EnumeratePorts();

    CString GetLastErrorMsg() const { return m_lastError; }
    HANDLE GetHandle() const { return m_hSerial; }

private:
    HANDLE m_hSerial;
    uint8_t m_seq;
    CString m_lastError;

    bool WriteExact(const uint8_t* buffer, DWORD bytesToWrite);
};
