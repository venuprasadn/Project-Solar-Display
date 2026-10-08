#pragma once

#include <afxwin.h>
#include <gdiplus.h>

class COEMHeaderCtrl : public CStatic
{
    DECLARE_DYNAMIC(COEMHeaderCtrl)

public:
    COEMHeaderCtrl();
    virtual ~COEMHeaderCtrl();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    DECLARE_MESSAGE_MAP()

private:
    void DrawGradientRoundedRect(
        Gdiplus::Graphics& g,
        const Gdiplus::RectF& rect,
        float radius,
        const Gdiplus::Color& colTop,
        const Gdiplus::Color& colBottom,
        const Gdiplus::Color& colBorder
    );
};
