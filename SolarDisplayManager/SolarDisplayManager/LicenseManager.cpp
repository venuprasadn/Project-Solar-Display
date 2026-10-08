#include "pch.h"
#include "LicenseManager.h"
#include <fstream>
#include <sstream>

CLicenseManager::CLicenseManager()
    : m_bLoaded(false)
    , m_bFeatureDisplay(true)
    , m_bFeatureApp(true)
    , m_bExpired(false)
    , m_strLicensee(_T("Unregistered / Trial"))
    , m_strExpiryDate(_T("NEVER"))
    , m_strSignature(_T(""))
    , m_strFilePath(_T(""))
{
}

CLicenseManager::~CLicenseManager()
{
}

CString CLicenseManager::Trim(const CString& str)
{
    CString s = str;
    s.TrimLeft(_T(" \t\r\n"));
    s.TrimRight(_T(" \t\r\n"));
    return s;
}

bool CLicenseManager::ParseBool(const CString& val)
{
    CString upper = val;
    upper.MakeUpper();
    upper.Trim();
    if (upper == _T("1") || upper == _T("ENABLE") || upper == _T("ENABLED") ||
        upper == _T("TRUE") || upper == _T("YES") || upper == _T("ON") || upper == _T("Y"))
    {
        return true;
    }
    return false;
}

bool CLicenseManager::SaveDefaultTemplate(const CString& path)
{
    std::ofstream out((CT2A)path);
    if (!out.is_open()) return false;

    out << "# ==============================================================================\n";
    out << "# Solar Display Manager - OEM License Configuration File\n";
    out << "# \n";
    out << "# Controls feature access permissions for engineering & staging tabs.\n";
    out << "# Permitted boolean values: ENABLE, DISABLE, 1, 0, TRUE, FALSE, YES, NO\n";
    out << "# ==============================================================================\n\n";

    out << "# Authorized Licensee / Manufacturing Facility\n";
    out << "LICENSEE=Solar Tech OEM Facility\n\n";

    out << "# Display Customization Tab (Branding, Model, Serial, Theme, Custom Logo Flashing)\n";
    out << "FEATURE_DISPLAY=ENABLE\n\n";

    out << "# Mobile App & Cloud / IoT Tab (Model Feature Bits, Direct Wi-Fi Provisioning, Cloud Diag)\n";
    out << "FEATURE_APP=ENABLE\n\n";

    out << "# License Expiry Date (Format: YYYY-MM-DD or NEVER)\n";
    out << "EXPIRY_DATE=2030-12-31\n\n";

    out << "# Security Token (Reserved for Cryptographic Signature Verification in Staging)\n";
    out << "SIGNATURE_TOKEN=SDM-PRO-OEM-2026-X8849-VALID\n";

    out.close();
    return true;
}

bool CLicenseManager::ParseLicenseFile(const CString& path)
{
    std::ifstream in((CT2A)path);
    if (!in.is_open()) return false;

    // Reset defaults before parsing
    m_bFeatureDisplay = false;
    m_bFeatureApp = false;
    m_bExpired = false;
    m_strLicensee = _T("OEM Factory License");
    m_strExpiryDate = _T("NEVER");
    m_strSignature = _T("");

    std::string line;
    while (std::getline(in, line))
    {
        CString cLine = CA2T(line.c_str());
        cLine = Trim(cLine);

        // Skip blank lines and comments
        if (cLine.IsEmpty() || cLine[0] == _T('#') || cLine[0] == _T(';'))
            continue;

        int eqPos = cLine.Find(_T('='));
        if (eqPos <= 0) continue;

        CString key = Trim(cLine.Left(eqPos));
        CString val = Trim(cLine.Mid(eqPos + 1));
        key.MakeUpper();

        if (key == _T("LICENSEE") || key == _T("CUSTOMER") || key == _T("USER"))
        {
            m_strLicensee = val;
        }
        else if (key == _T("FEATURE_DISPLAY") || key == _T("DISPLAY_FEATURES") || key == _T("DISPLAY_TAB"))
        {
            m_bFeatureDisplay = ParseBool(val);
        }
        else if (key == _T("FEATURE_APP") || key == _T("APP_FEATURES") || key == _T("APP_TAB") || key == _T("IOT_FEATURES"))
        {
            m_bFeatureApp = ParseBool(val);
        }
        else if (key == _T("EXPIRY_DATE") || key == _T("EXPIRY") || key == _T("EXPIRES"))
        {
            m_strExpiryDate = val;
        }
        else if (key == _T("SIGNATURE_TOKEN") || key == _T("SIGNATURE") || key == _T("SIGN_KEY"))
        {
            m_strSignature = val;
        }
    }

    in.close();

    // Check expiration if specified in YYYY-MM-DD
    if (!m_strExpiryDate.IsEmpty() && m_strExpiryDate.CompareNoCase(_T("NEVER")) != 0)
    {
        int yr = 0, mo = 0, da = 0;
        if (_stscanf_s(m_strExpiryDate, _T("%d-%d-%d"), &yr, &mo, &da) == 3)
        {
            CTime expTime(yr, mo, da, 23, 59, 59);
            CTime now = CTime::GetCurrentTime();
            if (now > expTime)
            {
                m_bExpired = true;
            }
        }
    }

    m_bLoaded = true;
    m_strFilePath = path;
    return true;
}

bool CLicenseManager::LoadLicense(const CString& forcedPath)
{
    if (!forcedPath.IsEmpty())
    {
        if (ParseLicenseFile(forcedPath))
            return true;
    }

    // Determine directory of the running executable
    TCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);
    CString appDir = exePath;
    int slashPos = appDir.ReverseFind(_T('\\'));
    if (slashPos != -1)
    {
        appDir = appDir.Left(slashPos + 1);
    }
    else
    {
        appDir = _T(".\\");
    }

    CString primaryLicPath = appDir + _T("license.txt");
    if (GetFileAttributes(primaryLicPath) != INVALID_FILE_ATTRIBUTES)
    {
        return ParseLicenseFile(primaryLicPath);
    }

    CString altLicPath = appDir + _T("license.key");
    if (GetFileAttributes(altLicPath) != INVALID_FILE_ATTRIBUTES)
    {
        return ParseLicenseFile(altLicPath);
    }

    // Also check current working directory
    if (GetFileAttributes(_T("license.txt")) != INVALID_FILE_ATTRIBUTES)
    {
        return ParseLicenseFile(_T("license.txt"));
    }

    // If no license file exists anywhere, create default template in app directory
    SaveDefaultTemplate(primaryLicPath);
    return ParseLicenseFile(primaryLicPath);
}

CString CLicenseManager::GetStatusSummary() const
{
    CString summary;
    if (m_bExpired)
    {
        summary.Format(_T("License: %s | STATUS: EXPIRED (%s) - All Modules Locked"), 
            (LPCTSTR)m_strLicensee, (LPCTSTR)m_strExpiryDate);
        return summary;
    }

    CString dispStr = m_bFeatureDisplay ? _T("ACTIVE") : _T("LOCKED");
    CString appStr = m_bFeatureApp ? _T("ACTIVE") : _T("LOCKED");

    summary.Format(_T("License: %s | Display Tab: [%s] | Mobile App Tab: [%s] | Expires: %s"),
        (LPCTSTR)m_strLicensee, (LPCTSTR)dispStr, (LPCTSTR)appStr, (LPCTSTR)m_strExpiryDate);

    return summary;
}
