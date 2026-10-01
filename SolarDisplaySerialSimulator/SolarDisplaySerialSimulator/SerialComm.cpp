#include "pch.h"
#include "SerialComm.h"

CSerialComm::CSerialComm()
    : m_hSerial(INVALID_HANDLE_VALUE)
    , m_seq(1)
{
}

CSerialComm::~CSerialComm()
{
    Close();
}

std::vector<CString> CSerialComm::EnumeratePorts()
{
    std::vector<CString> ports;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD index = 0;
        TCHAR valueName[256];
        DWORD valNameLen = 256;
        BYTE data[256];
        DWORD dataLen = 256;
        DWORD type = 0;

        while (RegEnumValue(hKey, index++, valueName, &valNameLen, NULL, &type, data, &dataLen) == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                ports.push_back(CString((LPCTSTR)data));
            }
            valNameLen = 256;
            dataLen = 256;
        }
        RegCloseKey(hKey);
    }

    if (ports.empty()) {
        for (int i = 1; i <= 32; i++) {
            CString name;
            name.Format(_T("COM%d"), i);
            CString fullName = _T("\\\\.\\") + name;
            HANDLE h = CreateFile(fullName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                ports.push_back(name);
            }
        }
    }

    return ports;
}

bool CSerialComm::Open(const CString& portName, DWORD baudRate)
{
    Close();

    CString fullName = portName;
    if (fullName.Find(_T("\\\\.\\")) == -1) {
        fullName = _T("\\\\.\\") + portName;
    }

    m_hSerial = CreateFile(
        fullName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (m_hSerial == INVALID_HANDLE_VALUE) {
        m_lastError.Format(_T("Failed to open %s (Error %lu)"), (LPCTSTR)portName, GetLastError());
        return false;
    }

    DCB dcb;
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(m_hSerial, &dcb)) {
        m_lastError = _T("Failed to get serial port state.");
        Close();
        return false;
    }

    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;

    if (!SetCommState(m_hSerial, &dcb)) {
        m_lastError = _T("Failed to set serial port parameters.");
        Close();
        return false;
    }

    EscapeCommFunction(m_hSerial, CLRDTR);
    EscapeCommFunction(m_hSerial, CLRRTS);

    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(m_hSerial, &timeouts);

    PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    return true;
}

void CSerialComm::Close()
{
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
}

bool CSerialComm::WriteExact(const uint8_t* buffer, DWORD bytesToWrite)
{
    if (!IsOpen() || !buffer) return false;

    DWORD written = 0;
    DWORD total = 0;
    while (total < bytesToWrite) {
        if (!WriteFile(m_hSerial, buffer + total, bytesToWrite - total, &written, NULL)) {
            m_lastError.Format(_T("WriteFile failed (Error %lu)"), GetLastError());
            return false;
        }
        total += written;
    }
    return true;
}

bool CSerialComm::SendString(const CStringA& text)
{
    if (!IsOpen()) return false;
    return WriteExact((const uint8_t*)text.GetString(), text.GetLength());
}

bool CSerialComm::SendBytes(const uint8_t* buffer, DWORD count)
{
    if (!IsOpen() || !buffer || count == 0) return false;
    return WriteExact(buffer, count);
}

DWORD CSerialComm::ReadAvailable(uint8_t* buffer, DWORD maxLen)
{
    if (!IsOpen() || !buffer || maxLen == 0) return 0;
    DWORD readBytes = 0;
    if (ReadFile(m_hSerial, buffer, maxLen, &readBytes, NULL)) {
        return readBytes;
    }
    return 0;
}

bool CSerialComm::SendFrame(uint8_t cmd, const uint8_t* payload, uint16_t length, uint8_t seq)
{
    if (!IsOpen()) {
        m_lastError = _T("Serial port is not open.");
        return false;
    }

    if (seq == 0) seq = m_seq++;

    hrf_header_t hdr;
    hdr.sync1 = HRF_SYNC_BYTE1;
    hdr.sync2 = HRF_SYNC_BYTE2;
    hdr.seq   = seq;
    hdr.cmd   = cmd;
    hdr.length = length;
    hdr.reserved = 0;

    std::vector<uint8_t> frame(sizeof(hrf_header_t) + length + 5);
    memcpy(frame.data(), &hdr, sizeof(hrf_header_t));

    if (payload && length > 0) {
        memcpy(frame.data() + sizeof(hrf_header_t), payload, length);
    }

    uint32_t crc = pcu_calc_crc32(frame.data(), sizeof(hrf_header_t) + length);
    uint32_t* crcPtr = (uint32_t*)(frame.data() + sizeof(hrf_header_t) + length);
    *crcPtr = crc;

    frame[sizeof(hrf_header_t) + length + 4] = HRF_FRAME_TRAILER;

    return WriteExact(frame.data(), (DWORD)frame.size());
}

bool CSerialComm::ReceiveFrame(hrf_header_t* outHdr, uint8_t* outPayload, uint16_t maxPayloadLen, DWORD timeoutMs)
{
    if (!IsOpen()) return false;

    DWORD start = GetTickCount();
    int state = 0;
    hrf_header_t hdr;
    std::vector<uint8_t> tempPayload;
    uint16_t payloadIdx = 0;
    uint32_t rxCrc = 0;
    int crcIdx = 0;
    uint8_t chunk[64];

    while (GetTickCount() - start < timeoutMs) {
        DWORD readBytes = 0;
        if (!ReadFile(m_hSerial, chunk, sizeof(chunk), &readBytes, NULL) || readBytes == 0) {
            Sleep(5);
            continue;
        }

        for (DWORD i = 0; i < readBytes; i++) {
            uint8_t b = chunk[i];
            switch (state) {
                case 0:
                    if (b == HRF_SYNC_BYTE1) state = 1;
                    break;
                case 1:
                    if (b == HRF_SYNC_BYTE2) {
                        hdr.sync1 = HRF_SYNC_BYTE1;
                        hdr.sync2 = HRF_SYNC_BYTE2;
                        payloadIdx = 0;
                        state = 2;
                    } else if (b == HRF_SYNC_BYTE1) {
                        state = 1;
                    } else {
                        state = 0;
                    }
                    break;
                case 2:
                    ((uint8_t*)&hdr)[2 + payloadIdx++] = b;
                    if (payloadIdx == sizeof(hrf_header_t) - 2) {
                        if (hdr.length > HRF_MAX_PAYLOAD_SIZE) {
                            state = 0;
                        } else if (hdr.length == 0) {
                            state = 4;
                            crcIdx = 0;
                        } else {
                            tempPayload.resize(hdr.length);
                            payloadIdx = 0;
                            state = 3;
                        }
                    }
                    break;
                case 3:
                    tempPayload[payloadIdx++] = b;
                    if (payloadIdx == hdr.length) {
                        state = 4;
                        crcIdx = 0;
                    }
                    break;
                case 4:
                    ((uint8_t*)&rxCrc)[crcIdx++] = b;
                    if (crcIdx == 4) {
                        state = 5;
                    }
                    break;
                case 5:
                    if (b == HRF_FRAME_TRAILER) {
                        std::vector<uint8_t> verifyBuf(sizeof(hrf_header_t) + hdr.length);
                        memcpy(verifyBuf.data(), &hdr, sizeof(hrf_header_t));
                        if (hdr.length > 0) {
                            memcpy(verifyBuf.data() + sizeof(hrf_header_t), tempPayload.data(), hdr.length);
                        }
                        uint32_t calcCrc = pcu_calc_crc32(verifyBuf.data(), verifyBuf.size());

                        if (calcCrc == rxCrc) {
                            if (outHdr) *outHdr = hdr;
                            if (outPayload && hdr.length > 0) {
                                uint16_t copyLen = min(hdr.length, maxPayloadLen);
                                memcpy(outPayload, tempPayload.data(), copyLen);
                            }
                            return true;
                        } else {
                            m_lastError = _T("CRC32 mismatch on received frame.");
                            return false;
                        }
                    }
                    state = 0;
                    break;
            }
        }
    }

    m_lastError = _T("Response timed out.");
    return false;
}
