import os

key = 0x5A9E3C71

def encrypt_str(s, k):
    b = s.encode('utf-8')
    res = []
    for i, byte_val in enumerate(b):
        shift = (i % 4) * 8
        k_byte = (k >> shift) & 0xFF
        res.append(byte_val ^ k_byte ^ ((i * 37 + 13) & 0xFF))
    return res

strings = {
    'BRAND': 'MicroSanju',
    'MODEL': 'HYBRID MPPT PCU',
    'SERIAL': 'MS-2026-X8849',
    'HWREV': 'HW-V2.1',
    'CONTACT': 'Toll Free: 1800-425-9999',
    'WEBSITE': 'www.microsanju.in',
    'COMPANY': 'MicroSanju Solar Tech India Pvt Ltd'
}

png_path = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\res\logo_master.png'
with open(png_path, 'rb') as f:
    png_bytes = f.read()

lines = []
lines.append('#include "pch.h"')
lines.append('#include "OEMPayload.h"')
lines.append('#include <string>')
lines.append('#include <ole2.h>')

lines.append('')
lines.append('// Cryptographic dynamic obfuscation constants')
lines.append(f'static const uint32_t s_vaultKey = 0x{key:08X};')
lines.append('')

for name, val in strings.items():
    enc = encrypt_str(val, key)
    hex_str = ', '.join(f'0x{b:02X}' for b in enc)
    lines.append(f'static const uint8_t s_enc_{name}[] = {{ {hex_str} }};')
    lines.append(f'static const size_t s_len_{name} = {len(enc)};')
    lines.append('')

lines.append('static CString DecryptString(const uint8_t* encData, size_t len, uint32_t key)')
lines.append('{')
lines.append('    std::string s;')
lines.append('    s.reserve(len);')
lines.append('    for (size_t i = 0; i < len; i++) {')
lines.append('        uint8_t shift = (uint8_t)((i % 4) * 8);')
lines.append('        uint8_t k_byte = (uint8_t)((key >> shift) & 0xFF);')
lines.append('        uint8_t orig = (uint8_t)(encData[i] ^ k_byte ^ ((i * 37 + 13) & 0xFF));')
lines.append('        s.push_back((char)orig);')
lines.append('    }')
lines.append('    return CString(s.c_str());')
lines.append('}')

lines.append('')
lines.append('CString OEMPayload::GetBrandTitle()       { return DecryptString(s_enc_BRAND, s_len_BRAND, s_vaultKey); }')
lines.append('CString OEMPayload::GetModelName()        { return DecryptString(s_enc_MODEL, s_len_MODEL, s_vaultKey); }')
lines.append('CString OEMPayload::GetSerialPrefix()     { return DecryptString(s_enc_SERIAL, s_len_SERIAL, s_vaultKey); }')
lines.append('CString OEMPayload::GetHardwareVersion()  { return DecryptString(s_enc_HWREV, s_len_HWREV, s_vaultKey); }')
lines.append('CString OEMPayload::GetVendorContact()    { return DecryptString(s_enc_CONTACT, s_len_CONTACT, s_vaultKey); }')
lines.append('CString OEMPayload::GetVendorWebsite()    { return DecryptString(s_enc_WEBSITE, s_len_WEBSITE, s_vaultKey); }')
lines.append('CString OEMPayload::GetCompanyName()      { return DecryptString(s_enc_COMPANY, s_len_COMPANY, s_vaultKey); }')
lines.append('')
lines.append('CString OEMPayload::GetModeDescription()')
lines.append('{')
lines.append('#if ACTIVE_OEM_HW_MODE == OEM_HW_MODE_COMBO')
lines.append('    return _T("TFT LCD Display + AWS IoT Cloud [COMBO]");')
lines.append('#elif ACTIVE_OEM_HW_MODE == OEM_HW_MODE_TFT_ONLY')
lines.append('    return _T("TFT LCD Display [DEDICATED]");')
lines.append('#elif ACTIVE_OEM_HW_MODE == OEM_HW_MODE_CLOUD_ONLY')
lines.append('    return _T("AWS IoT Cloud / Mobile App [DEDICATED]");')
lines.append('#endif')
lines.append('}')
lines.append('')
lines.append('uint8_t OEMPayload::GetDefaultTheme()         { return 0; /* Amber Glow Solar */ }')
lines.append('uint8_t OEMPayload::GetDefaultBootSec()       { return 3; }')
lines.append('uint8_t OEMPayload::GetDefaultCarouselSec()   { return 5; }')
lines.append('uint8_t OEMPayload::GetTargetHardwareMode()   { return ACTIVE_OEM_HW_MODE; }')
lines.append('')

# Format PNG array into chunks of 16
png_lines = []
for i in range(0, len(png_bytes), 16):
    chunk = png_bytes[i:i+16]
    png_lines.append('    ' + ', '.join(f'0x{b:02X}' for b in chunk) + ',')

lines.append('static const uint8_t s_masterLogoPngData[] = {')
lines.extend(png_lines)
lines.append('};')
lines.append(f'static const size_t s_masterLogoPngSize = sizeof(s_masterLogoPngData);')
lines.append('')
lines.append('const uint8_t* OEMPayload::GetMasterLogoPng(size_t& outLen)')
lines.append('{')
lines.append('    outLen = s_masterLogoPngSize;')
lines.append('    return s_masterLogoPngData;')
lines.append('}')
lines.append('')
lines.append('Gdiplus::Bitmap* OEMPayload::GetMasterLogoBitmap()')
lines.append('{')
lines.append('    static Gdiplus::Bitmap* s_pCachedBitmap = nullptr;')
lines.append('    if (s_pCachedBitmap != nullptr) return s_pCachedBitmap;')
lines.append('')
lines.append('    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, s_masterLogoPngSize);')
lines.append('    if (!hMem) return nullptr;')
lines.append('    void* pMem = GlobalLock(hMem);')
lines.append('    if (pMem) {')
lines.append('        memcpy(pMem, s_masterLogoPngData, s_masterLogoPngSize);')
lines.append('        GlobalUnlock(hMem);')
lines.append('    }')
lines.append('    IStream* pStream = nullptr;')
lines.append('    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK && pStream != nullptr) {')
lines.append('        s_pCachedBitmap = Gdiplus::Bitmap::FromStream(pStream);')
lines.append('        pStream->Release();')
lines.append('    }')
lines.append('    return s_pCachedBitmap;')
lines.append('}')
lines.append('')
lines.append('std::vector<uint8_t> OEMPayload::DecryptPayload(const uint8_t* cipherData, size_t length, uint32_t key)')
lines.append('{')
lines.append('    std::vector<uint8_t> plain(length);')
lines.append('    for (size_t i = 0; i < length; i++) {')
lines.append('        uint8_t shift = (uint8_t)((i % 4) * 8);')
lines.append('        uint8_t k_byte = (uint8_t)((key >> shift) & 0xFF);')
lines.append('        plain[i] = (uint8_t)(cipherData[i] ^ k_byte ^ ((i * 37 + 13) & 0xFF));')
lines.append('    }')
lines.append('    return plain;')
lines.append('}')

out_cpp = r'D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\OEMPayload.cpp'
with open(out_cpp, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))

print('Generated OEMPayload.cpp successfully:', out_cpp)
