#include "pch.h"
#include "SerialComm.h"
#include <tchar.h>

CSerialComm::CSerialComm()
    : m_hSerial(INVALID_HANDLE_VALUE)
    , m_seq(1)
{
}

CSerialComm::~CSerialComm()
{
    Close();
}

bool CSerialComm::Open(const CString& portName, DWORD baudRate)
{
    Close();

    CString formattedPort = portName;
    if (formattedPort.Find(_T("\\\\.\\")) != 0) {
        formattedPort = _T("\\\\.\\") + formattedPort;
    }

    m_hSerial = CreateFile(
        formattedPort,
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

    // Configure DCB
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

    // Explicitly release DTR and RTS lines so microcontroller runs freely
    EscapeCommFunction(m_hSerial, CLRDTR);
    EscapeCommFunction(m_hSerial, CLRRTS);

    // Setup Non-blocking Timeouts for instantaneous reading
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    SetCommTimeouts(m_hSerial, &timeouts);

    // Allow hardware to settle and clear any transition bytes
    Sleep(300);
    PurgeComm(m_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    return true;
}

void CSerialComm::Close()
{
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
}

bool CSerialComm::WriteExact(const uint8_t* buffer, DWORD bytesToWrite)
{
    DWORD written = 0;
    if (!WriteFile(m_hSerial, buffer, bytesToWrite, &written, NULL) || written != bytesToWrite) {
        m_lastError.Format(_T("Write failed: expected %lu, wrote %lu"), bytesToWrite, written);
        return false;
    }
    return true;
}

bool CSerialComm::ReadExact(uint8_t* buffer, DWORD bytesToRead, DWORD timeoutMs)
{
    DWORD totalRead = 0;
    DWORD start = GetTickCount();

    while (totalRead < bytesToRead) {
        DWORD bytesRead = 0;
        if (!ReadFile(m_hSerial, buffer + totalRead, bytesToRead - totalRead, &bytesRead, NULL)) {
            return false;
        }
        totalRead += bytesRead;

        if (totalRead >= bytesToRead) break;
        if (GetTickCount() - start > timeoutMs) {
            return false;
        }
        Sleep(5);
    }
    return true;
}

bool CSerialComm::SendFrame(uint8_t cmd, const uint8_t* payload, uint16_t length, uint8_t seq)
{
    if (!IsOpen()) return false;

    if (seq == 0) seq = m_seq++;

    hrf_header_t hdr;
    hdr.sync1 = HRF_SYNC_BYTE1;
    hdr.sync2 = HRF_SYNC_BYTE2;
    hdr.seq   = seq;
    hdr.cmd   = cmd;
    hdr.length = length;
    hdr.reserved = 0;

    std::vector<uint8_t> frame(sizeof(hrf_header_t) + length + 4 + 1);
    memcpy(frame.data(), &hdr, sizeof(hrf_header_t));
    if (length > 0 && payload != nullptr) {
        memcpy(frame.data() + sizeof(hrf_header_t), payload, length);
    }

    uint32_t crc = pcu_calc_crc32(frame.data(), sizeof(hrf_header_t) + length);
    size_t offset = sizeof(hrf_header_t) + length;
    memcpy(frame.data() + offset, &crc, 4);
    offset += 4;
    frame[offset] = HRF_FRAME_TRAILER;

    PurgeComm(m_hSerial, PURGE_RXCLEAR);
    return WriteExact(frame.data(), (DWORD)frame.size());
}

bool CSerialComm::ReceiveFrame(hrf_header_t* outHdr, uint8_t* outPayload, uint16_t maxPayloadLen, DWORD timeoutMs)
{
    if (!IsOpen()) return false;

    DWORD start = GetTickCount();
    int state = 0; // 0=Wait S, 1=Wait D, 2=Header, 3=Payload, 4=CRC, 5=Trailer
    hrf_header_t hdr;
    size_t payloadIdx = 0;
    uint32_t rxCrc = 0;
    size_t crcIdx = 0;
    std::vector<uint8_t> tempPayload;
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
                case 2: // Read remaining 6 bytes of header
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
                case 3: // Read payload
                    tempPayload[payloadIdx++] = b;
                    if (payloadIdx == hdr.length) {
                        state = 4;
                        crcIdx = 0;
                    }
                    break;
                case 4: // Read 4-byte CRC
                    ((uint8_t*)&rxCrc)[crcIdx++] = b;
                    if (crcIdx == 4) {
                        state = 5;
                    }
                    break;
                case 5: // Read Trailer
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

bool CSerialComm::Ping(hrf_ping_resp_t* outPing)
{
    for (int attempt = 0; attempt < 6; attempt++) {
        PurgeComm(m_hSerial, PURGE_RXCLEAR);
        if (SendFrame(CMD_PING)) {
            hrf_header_t hdr;
            hrf_ping_resp_t resp;
            if (ReceiveFrame(&hdr, (uint8_t*)&resp, sizeof(resp), 500)) {
                if (hdr.cmd == RESP_ACK) {
                    if (outPing) *outPing = resp;
                    return true;
                }
            }
        }
        Sleep(200);
    }
    return false;
}

bool CSerialComm::ReadConfig(pcu_config_t* outCfg)
{
    if (!SendFrame(CMD_READ_CONFIG)) return false;

    hrf_header_t hdr;
    pcu_config_t cfg;
    if (ReceiveFrame(&hdr, (uint8_t*)&cfg, sizeof(cfg), 1500)) {
        if (hdr.cmd == RESP_CONFIG_DATA && hdr.length == sizeof(pcu_config_t)) {
            if (outCfg) *outCfg = cfg;
            return true;
        }
    }
    return false;
}

bool CSerialComm::WriteConfig(const pcu_config_t* inCfg)
{
    if (!inCfg) return false;

    if (!SendFrame(CMD_WRITE_CONFIG, (const uint8_t*)inCfg, sizeof(pcu_config_t))) return false;

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 1500)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }
    return false;
}

bool CSerialComm::CommitNVS()
{
    if (!SendFrame(CMD_COMMIT_NVS)) return false;

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 2500)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }
    return false;
}

bool CSerialComm::FactoryReset()
{
    if (!SendFrame(CMD_FACTORY_RESET)) return false;

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 2000)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }
    return false;
}

bool CSerialComm::Reboot()
{
    if (!SendFrame(CMD_REBOOT)) return false;

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 1000)) {
        return (hdr.cmd == RESP_ACK);
    }
    return true;
}

bool CSerialComm::WriteLogo(const uint8_t* logoData, size_t totalSize)
{
    if (!logoData || totalSize == 0) return false;

    const size_t CHUNK_SIZE = 256;
    size_t offset = 0;

    while (offset < totalSize) {
        size_t len = totalSize - offset;
        if (len > CHUNK_SIZE) len = CHUNK_SIZE;

        std::vector<uint8_t> payload(4 + len);
        hrf_logo_chunk_t* chunk = (hrf_logo_chunk_t*)payload.data();
        chunk->offset = (uint16_t)offset;
        chunk->length = (uint16_t)len;
        memcpy(chunk->data, logoData + offset, len);

        if (!SendFrame(CMD_WRITE_LOGO_CHUNK, payload.data(), (uint16_t)payload.size())) {
            m_lastError = _T("Failed to transmit logo chunk packet.");
            return false;
        }

        hrf_header_t hdr;
        hrf_ack_payload_t ack;
        if (!ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 1500) || hdr.cmd != RESP_ACK || ack.status_code != STATUS_OK) {
            m_lastError.Format(_T("Chunk at offset %lu rejected (status %u)"), (DWORD)offset, ack.status_code);
            return false;
        }

        offset += len;
    }

    // Commit Logo to NVS
    if (!SendFrame(CMD_COMMIT_LOGO)) {
        m_lastError = _T("Failed to send CMD_COMMIT_LOGO.");
        return false;
    }

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 2500)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }

    m_lastError = _T("Commit logo response timed out.");
    return false;
}

bool CSerialComm::ClearLogo()
{
    if (!SendFrame(CMD_CLEAR_LOGO)) return false;

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 2000)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }
    return false;
}

bool CSerialComm::ProvisionWifi(const char* ssid, const char* password)
{
    if (!ssid || strlen(ssid) == 0) return false;
    char payload[128];
    sprintf_s(payload, sizeof(payload), "%s,%s", ssid, password ? password : "");
    uint16_t len = (uint16_t)strlen(payload);

    if (!SendFrame(CMD_PROVISION_WIFI, (const uint8_t*)payload, len)) {
        m_lastError = _T("Failed to transmit CMD_PROVISION_WIFI frame.");
        return false;
    }

    hrf_header_t hdr;
    hrf_ack_payload_t ack;
    if (ReceiveFrame(&hdr, (uint8_t*)&ack, sizeof(ack), 3000)) {
        return (hdr.cmd == RESP_ACK && ack.status_code == STATUS_OK);
    }

    m_lastError = _T("Wi-Fi provisioning response timed out.");
    return false;
}

bool CSerialComm::ScanWifi(wifi_scan_result_payload_t* outScan)
{
    if (!outScan) return false;

    if (!SendFrame(CMD_SCAN_WIFI)) {
        m_lastError = _T("Failed to send CMD_SCAN_WIFI.");
        return false;
    }

    hrf_header_t hdr;
    if (ReceiveFrame(&hdr, (uint8_t*)outScan, sizeof(wifi_scan_result_payload_t), 6000)) {
        return (hdr.cmd == RESP_WIFI_SCAN_DATA);
    }

    m_lastError = _T("Wi-Fi scan response timed out.");
    return false;
}

bool CSerialComm::ReadIotStatus(hrf_iot_status_payload_t* outStatus)
{
    if (!outStatus) return false;

    if (!SendFrame(CMD_READ_IOT_STATUS)) {
        m_lastError = _T("Failed to send CMD_READ_IOT_STATUS.");
        return false;
    }

    hrf_header_t hdr;
    if (ReceiveFrame(&hdr, (uint8_t*)outStatus, sizeof(hrf_iot_status_payload_t), 2000)) {
        return (hdr.cmd == RESP_IOT_STATUS_DATA);
    }

    m_lastError = _T("Read IoT status response timed out.");
    return false;
}

#include <setupapi.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")

static bool ParseVidPid(const CString& hwId, uint16_t& outVid, uint16_t& outPid)
{
    outVid = 0;
    outPid = 0;
    CString upper = hwId;
    upper.MakeUpper();

    int vidIdx = upper.Find(_T("VID_"));
    if (vidIdx != -1 && vidIdx + 8 <= upper.GetLength()) {
        CString vidStr = upper.Mid(vidIdx + 4, 4);
        outVid = (uint16_t)_tcstoul((LPCTSTR)vidStr, NULL, 16);
    }

    int pidIdx = upper.Find(_T("PID_"));
    if (pidIdx == -1) pidIdx = upper.Find(_T("PID+"));
    if (pidIdx != -1 && pidIdx + 8 <= upper.GetLength()) {
        CString pidStr = upper.Mid(pidIdx + 4, 4);
        outPid = (uint16_t)_tcstoul((LPCTSTR)pidStr, NULL, 16);
    }

    return (outVid != 0);
}

std::vector<CSerialComm::PortInfo> CSerialComm::EnumerateDetailedPorts()
{
    std::vector<PortInfo> results;

    // 1. Enumerate via official Windows SetupAPI (Ports Class)
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        DWORD devIdx = 0;

        while (SetupDiEnumDeviceInfo(hDevInfo, devIdx++, &devInfoData))
        {
            // Query PortName from Device Registry
            HKEY hDevKey = SetupDiOpenDevRegKey(hDevInfo, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (hDevKey != INVALID_HANDLE_VALUE)
            {
                TCHAR portName[64] = { 0 };
                DWORD portSize = sizeof(portName);
                DWORD type = 0;

                if (RegQueryValueEx(hDevKey, _T("PortName"), NULL, &type, (LPBYTE)portName, &portSize) == ERROR_SUCCESS)
                {
                    CString strPort(portName);
                    if (!strPort.IsEmpty() && strPort.Left(3).CompareNoCase(_T("COM")) == 0)
                    {
                        PortInfo info;
                        info.portName = strPort;
                        info.vid = 0;
                        info.pid = 0;
                        info.isEspDevice = false;
                        info.rank = 99;

                        // Query Hardware ID
                        TCHAR hwIdBuf[512] = { 0 };
                        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)hwIdBuf, sizeof(hwIdBuf), NULL)) {
                            info.hardwareId = hwIdBuf;
                            ParseVidPid(info.hardwareId, info.vid, info.pid);
                        }

                        // Query Friendly Name
                        TCHAR friendlyBuf[512] = { 0 };
                        CString friendly;
                        if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)friendlyBuf, sizeof(friendlyBuf), NULL)) {
                            friendly = friendlyBuf;
                        }

                        // Strict VID & PID Hardware Classification
                        if (info.vid == 0x303A) // Espressif Systems
                        {
                            info.isEspDevice = true;
                            if (info.pid == 0x1001) {
                                info.rank = 1;
                                info.friendlyName.Format(_T("%s - ESP32-S3 Target (Native USB-JTAG/CDC)"), (LPCTSTR)strPort);
                            } else if (info.pid == 0x1002 || info.pid == 0x0002) {
                                info.rank = 1;
                                info.friendlyName.Format(_T("%s - ESP32 Controller (Native USB CDC)"), (LPCTSTR)strPort);
                            } else {
                                info.rank = 2;
                                info.friendlyName.Format(_T("%s - ESP32 Controller (Espressif USB)"), (LPCTSTR)strPort);
                            }
                        }
                        else if (info.vid == 0x1A86) // WCH (CH343 / CH9102 / CH340)
                        {
                            info.isEspDevice = true;
                            info.rank = 3;
                            if (info.pid == 0x55D4) {
                                info.friendlyName.Format(_T("%s - Controller Target (CH343 High-Speed)"), (LPCTSTR)strPort);
                            } else if (info.pid == 0xE523) {
                                info.friendlyName.Format(_T("%s - Controller Target (CH9102 Bridge)"), (LPCTSTR)strPort);
                            } else {
                                info.friendlyName.Format(_T("%s - Controller Target (CH340/CH341 Bridge)"), (LPCTSTR)strPort);
                            }
                        }
                        else if (info.vid == 0x10C4) // Silicon Labs CP210x
                        {
                            info.isEspDevice = true;
                            info.rank = 4;
                            if (info.pid == 0xEA60) {
                                info.friendlyName.Format(_T("%s - Controller Target (CP2102/CP2104 Bridge)"), (LPCTSTR)strPort);
                            } else if (info.pid == 0xEA70) {
                                info.friendlyName.Format(_T("%s - Controller Target (CP2105 Dual Bridge)"), (LPCTSTR)strPort);
                            } else {
                                info.friendlyName.Format(_T("%s - Controller Target (CP210x Bridge)"), (LPCTSTR)strPort);
                            }
                        }
                        else if (info.vid == 0x0403) // FTDI
                        {
                            info.isEspDevice = true;
                            info.rank = 5;
                            info.friendlyName.Format(_T("%s - Controller Target (FTDI Bridge)"), (LPCTSTR)strPort);
                        }
                        else
                        {
                            // Non-Target Device (Bluetooth, Motherboard serial, GPS, modem, etc.)
                            info.isEspDevice = false;
                            info.rank = 99;
                            if (!friendly.IsEmpty()) {
                                info.friendlyName.Format(_T("%s - %s"), (LPCTSTR)strPort, (LPCTSTR)friendly);
                            } else {
                                info.friendlyName.Format(_T("%s - Serial Device"), (LPCTSTR)strPort);
                            }
                        }

                        results.push_back(info);
                    }
                }
                RegCloseKey(hDevKey);
            }
        }
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    // 2. Fallback check from HARDWARE\DEVICEMAP\SERIALCOMM for any missed ports
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        TCHAR valueName[256];
        BYTE data[256];
        DWORD valSize = sizeof(valueName) / sizeof(TCHAR);
        DWORD dataSize = sizeof(data);
        DWORD type = 0;
        DWORD idx = 0;

        while (RegEnumValue(hKey, idx, valueName, &valSize, NULL, &type, data, &dataSize) == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                CString portStr = (LPCTSTR)data;
                bool alreadyFound = false;
                for (const auto& r : results) {
                    if (r.portName.CompareNoCase(portStr) == 0) {
                        alreadyFound = true;
                        break;
                    }
                }
                if (!alreadyFound) {
                    PortInfo info;
                    info.portName = portStr;
                    info.friendlyName.Format(_T("%s - Serial Port"), (LPCTSTR)portStr);
                    info.isEspDevice = false;
                    info.rank = 99;
                    info.vid = 0;
                    info.pid = 0;
                    results.push_back(info);
                }
            }
            valSize = sizeof(valueName) / sizeof(TCHAR);
            dataSize = sizeof(data);
            idx++;
        }
        RegCloseKey(hKey);
    }

    return results;
}

std::vector<CString> CSerialComm::EnumeratePorts()
{
    std::vector<CString> ports;
    std::vector<PortInfo> details = EnumerateDetailedPorts();
    for (const auto& info : details) {
        ports.push_back(info.portName);
    }
    return ports;
}

CString CSerialComm::AutoDetectEspPort()
{
    std::vector<PortInfo> ports = EnumerateDetailedPorts();

    // 1. Find the best candidate matching genuine ESP32 / USB-UART VIDs/PIDs
    int bestIdx = -1;
    int bestRank = 999;

    for (size_t i = 0; i < ports.size(); i++) {
        if (ports[i].isEspDevice && ports[i].rank < bestRank) {
            bestRank = ports[i].rank;
            bestIdx = (int)i;
        }
    }

    if (bestIdx >= 0) {
        return ports[bestIdx].portName;
    }

    // STRICT: Return empty if no VID/PID matched target device is found. Never connect to random COM port!
    return _T("");
}

