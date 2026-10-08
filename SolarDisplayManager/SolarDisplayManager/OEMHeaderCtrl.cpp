#include "pch.h"
#include "OEMHeaderCtrl.h"
#include "OEMPayload.h"
#include <gdiplus.h>

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(COEMHeaderCtrl, CStatic)

COEMHeaderCtrl::COEMHeaderCtrl()
{
}

COEMHeaderCtrl::~COEMHeaderCtrl()
{
}

BEGIN_MESSAGE_MAP(COEMHeaderCtrl, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL COEMHeaderCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
    // Double buffering in OnPaint avoids flicker
    return TRUE;
}

void COEMHeaderCtrl::DrawGradientRoundedRect(
    Graphics& g,
    const RectF& rect,
    float radius,
    const Color& colTop,
    const Color& colBottom,
    const Color& colBorder
)
{
    GraphicsPath path;
    float d = radius * 2.0f;

    path.AddArc(rect.X, rect.Y, d, d, 180, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270, 90);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0, 90);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90, 90);
    path.CloseFigure();

    LinearGradientBrush brush(
        PointF(rect.X, rect.Y),
        PointF(rect.X, rect.Y + rect.Height),
        colTop,
        colBottom
    );

    g.FillPath(&brush, &path);

    Pen borderPen(colBorder, 1.2f);
    g.DrawPath(&borderPen, &path);
}

void COEMHeaderCtrl::OnPaint()
{
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    if (rcClient.Width() <= 0 || rcClient.Height() <= 0) return;

    // Create memory DC for smooth double buffering
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap memBmp;
    memBmp.CreateCompatibleBitmap(&dc, rcClient.Width(), rcClient.Height());
    CBitmap* pOldBmp = memDC.SelectObject(&memBmp);

    {
        Graphics g(memDC.GetSafeHdc());
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        // Fill background with dialog default face color
        HBRUSH hSysBrush = ::GetSysColorBrush(COLOR_3DFACE);
        FillRect(memDC.GetSafeHdc(), &rcClient, hSysBrush);

        float cardW = (float)rcClient.Width() - 2.0f;
        float cardH = (float)rcClient.Height() - 2.0f;
        RectF cardRect(1.0f, 1.0f, cardW, cardH);

        // Draw Dark Industrial Rounded Gradient Card
        Color topGrad(255, 13, 20, 32);     // Slate-900 dark blue
        Color botGrad(255, 22, 33, 50);     // Slate-800 metallic tone
        Color borderCol(255, 51, 65, 85);   // Slate-700 subtle crisp border
        DrawGradientRoundedRect(g, cardRect, 6.0f, topGrad, botGrad, borderCol);

        // 1. Draw Master OEM Logo (Proportionally Scaled & Centered Vertically)
        Bitmap* pLogo = OEMPayload::GetMasterLogoBitmap();
        float logoSize = (cardH >= 80.0f) ? min(58.0f, cardH - 20.0f) : (cardH - 12.0f);
        if (logoSize < 32.0f) logoSize = 32.0f;
        float logoX = 14.0f;
        float logoY = (cardH - logoSize) / 2.0f + 1.0f;

        if (pLogo != nullptr && pLogo->GetLastStatus() == Ok) {
            g.DrawImage(pLogo, RectF(logoX, logoY, logoSize, logoSize));
        }

        // 2. Setup Typography & High-Contrast Industrial Color Brushes
        Gdiplus::FontFamily fontFamily(L"Segoe UI");
        Gdiplus::Font fontBrand(&fontFamily, 12.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::Font fontModel(&fontFamily, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::Font fontCompany(&fontFamily, 8.5f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::Font fontMetaLabel(&fontFamily, 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::Font fontMetaValue(&fontFamily, 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::Font fontContact(&fontFamily, 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::Font fontContactBold(&fontFamily, 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::Font fontBadge(&fontFamily, 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);

        Gdiplus::SolidBrush brushPureWhite(Gdiplus::Color(255, 255, 255, 255));
        Gdiplus::SolidBrush brushGold(Gdiplus::Color(255, 245, 158, 11));       // Amber-500 (#F59E0B)
        Gdiplus::SolidBrush brushLightSlate(Gdiplus::Color(255, 203, 213, 225)); // Slate-300 (#CBD5E1) - High Readability
        Gdiplus::SolidBrush brushMutedSlate(Gdiplus::Color(255, 148, 163, 184)); // Slate-400 (#94A3B8) - Clear Labels
        Gdiplus::SolidBrush brushSky(Gdiplus::Color(255, 56, 189, 248));        // Sky-400 (#38BDF8) - Cyan Accent
        Gdiplus::SolidBrush brushEmerald(Gdiplus::Color(255, 52, 211, 153));    // Emerald-400 (#34D399) - Status Dot

        StringFormat stringFormatTypo(StringFormat::GenericTypographic());
        stringFormatTypo.SetFormatFlags(StringFormatFlagsNoClip | StringFormatFlagsMeasureTrailingSpaces);

        float textStartX = logoX + logoSize + 16.0f;

        // Strings to render
        CStringW strBrandW = (LPCWSTR)CT2W(OEMPayload::GetBrandTitle());
        CStringW strModelW = (LPCWSTR)CT2W(OEMPayload::GetModelName());
        CStringW strCompanyW = (LPCWSTR)CT2W(OEMPayload::GetCompanyName());
        CString strHW = _T("HW: ") + OEMPayload::GetHardwareVersion();
        CString strSN = _T("SN: ") + OEMPayload::GetSerialPrefix();
        CString strContact = OEMPayload::GetVendorContact();
        CString strWeb = OEMPayload::GetVendorWebsite();

        // Calculate dynamic line heights and vertical centering
        float hLine1 = 20.0f;
        float hLine2 = 15.0f;
        float hLine3 = 15.0f;
        float hLine4 = 15.0f;
        float totalContentH = hLine1 + hLine2 + hLine3 + hLine4;

        float availableSpace = cardH - totalContentH;
        float rowSpacing = 4.0f;
        if (availableSpace > 28.0f) {
            rowSpacing = (availableSpace - 16.0f) / 3.0f;
            if (rowSpacing > 7.0f) rowSpacing = 7.0f;
        } else if (availableSpace < 12.0f) {
            rowSpacing = 2.5f;
        }

        float blockH = totalContentH + 3.0f * rowSpacing;
        float startY = max(5.0f, (cardH - blockH) / 2.0f + 1.0f);

        float y1 = startY;
        float y2 = y1 + hLine1 + rowSpacing;
        float y3 = y2 + hLine2 + rowSpacing;
        float y4 = y3 + hLine3 + rowSpacing;

        // Line 1: Brand Title + Separator + Model Highlight
        g.DrawString(strBrandW, -1, &fontBrand, Gdiplus::PointF(textStartX, y1), &stringFormatTypo, &brushPureWhite);
        RectF brandBounds;
        g.MeasureString(strBrandW, -1, &fontBrand, Gdiplus::PointF(textStartX, y1), &stringFormatTypo, &brandBounds);

        float modelStartX = textStartX + brandBounds.Width + 4.0f;
        CStringW strSepW = L"  |  ";
        g.DrawString(strSepW, -1, &fontModel, Gdiplus::PointF(modelStartX, y1 + 1.5f), &stringFormatTypo, &brushMutedSlate);
        RectF sepBounds;
        g.MeasureString(strSepW, -1, &fontModel, Gdiplus::PointF(modelStartX, y1 + 1.5f), &stringFormatTypo, &sepBounds);

        g.DrawString(strModelW, -1, &fontModel, Gdiplus::PointF(modelStartX + sepBounds.Width, y1 + 1.5f), &stringFormatTypo, &brushGold);

        // Line 2: Company Legal Entity / Subtitle (High-Contrast Light Slate)
        g.DrawString(strCompanyW, -1, &fontCompany, Gdiplus::PointF(textStartX, y2), &stringFormatTypo, &brushLightSlate);

        // Line 3: Hardware Revision | Serial Number
        CString strMetaRow1;
        strMetaRow1.Format(_T("HW: %s   |   SN: %s"), (LPCTSTR)OEMPayload::GetHardwareVersion(), (LPCTSTR)OEMPayload::GetSerialPrefix());
        CStringW strMetaRow1W = (LPCWSTR)CT2W(strMetaRow1);
        g.DrawString(strMetaRow1W, -1, &fontMetaValue, Gdiplus::PointF(textStartX, y3), &stringFormatTypo, &brushPureWhite);

        // Line 4: Support Contact & Web Portal (Vibrant Sky Blue Accent)
        CString strMetaRow2;
        strMetaRow2.Format(_T("Contact: %s   |   Web: %s"), (LPCTSTR)strContact, (LPCTSTR)strWeb);
        CStringW strMetaRow2W = (LPCWSTR)CT2W(strMetaRow2);
        g.DrawString(strMetaRow2W, -1, &fontContactBold, Gdiplus::PointF(textStartX, y4), &stringFormatTypo, &brushSky);

        // 3. Right-Aligned Operational Hardware Mode Badge (Generously Sized & Squeeze-Free)
        CStringW strModeDescW = (LPCWSTR)CT2W(OEMPayload::GetModeDescription());
        RectF modeBounds;
        g.MeasureString(strModeDescW, -1, &fontBadge, Gdiplus::PointF(0, 0), &stringFormatTypo, &modeBounds);

        float pillW = modeBounds.Width + 40.0f;
        float pillH = 24.0f;
        float pillX = cardW - pillW - 14.0f;
        float pillY = max(8.0f, y1 - 2.0f);

        RectF pillRect(pillX, pillY, pillW, pillH);
        DrawGradientRoundedRect(g, pillRect, 5.0f, Color(255, 10, 34, 56), Color(255, 6, 20, 36), Color(255, 14, 165, 233));

        // Green Active Indicator Dot with subtle outer glow
        Color dotGlow(90, 52, 211, 153);
        SolidBrush brushDotGlow(dotGlow);
        g.FillEllipse(&brushDotGlow, pillX + 8.0f, pillY + 6.0f, 11.0f, 11.0f);
        g.FillEllipse(&brushEmerald, pillX + 10.0f, pillY + 8.0f, 7.0f, 7.0f);

        // Mode Text (Cleanly positioned with ample right margin)
        g.DrawString(strModeDescW, -1, &fontBadge, Gdiplus::PointF(pillX + 26.0f, pillY + 5.0f), &stringFormatTypo, &brushSky);
    }

    // Blit backbuffer to screen
    dc.BitBlt(0, 0, rcClient.Width(), rcClient.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOldBmp);
}
