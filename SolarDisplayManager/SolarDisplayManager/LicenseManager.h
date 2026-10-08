#pragma once

#include <afx.h>
#include <afxstr.h>

/**
 * @brief Industrial License Manager for Solar Display Manager OEM Tool
 * 
 * Controls feature access to:
 * - Tab 1: Display Features (Brand, Model, Serial, Theme, Custom Logo)
 * - Tab 2: Mobile App & Cloud / IoT Features (Model Bits, Direct Wi-Fi, Cloud Diag)
 * 
 * Reads and verifies license.txt from application directory.
 * If license.txt does not exist, automatically generates a documented default template.
 */
class CLicenseManager
{
public:
    CLicenseManager();
    ~CLicenseManager();

    bool LoadLicense(const CString& forcedPath = _T(""));
    bool SaveDefaultTemplate(const CString& path);

    bool IsDisplayFeatureAllowed() const { return m_bFeatureDisplay && !m_bExpired; }
    bool IsAppFeatureAllowed() const { return m_bFeatureApp && !m_bExpired; }
    bool IsExpired() const { return m_bExpired; }

    CString GetLicenseeName() const { return m_strLicensee; }
    CString GetExpiryDate() const { return m_strExpiryDate; }
    CString GetLicenseFilePath() const { return m_strFilePath; }
    CString GetStatusSummary() const;

private:
    bool ParseLicenseFile(const CString& path);
    static CString Trim(const CString& str);
    static bool ParseBool(const CString& val);

    bool    m_bLoaded;
    bool    m_bFeatureDisplay;
    bool    m_bFeatureApp;
    bool    m_bExpired;
    CString m_strLicensee;
    CString m_strExpiryDate;
    CString m_strSignature;
    CString m_strFilePath;
};
