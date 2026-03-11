#include "framework.h"
#include "Renderers.h"
#include <string.h>

void RenderDVD(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    const char* logoText = "DVD";

    int dvdFontSize = max(50, height / 6);
    HFONT hDvdFont = CreateFontA(dvdFontSize, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_SWISS, "Impact");

    HFONT hOldFont = (HFONT)SelectObject(memDC, hDvdFont);

    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);

    SIZE textSize;
    GetTextExtentPoint32A(memDC, logoText, (int)strlen(logoText), &textSize);

    int tw = textSize.cx;

    int visualHeight = tm.tmAscent - tm.tmInternalLeading;

    if (data->logoX == 0 && data->logoY == 0) {
        data->logoX = (float)(rand() % max(1, width - tw));
        data->logoY = (float)(rand() % max(1, height - visualHeight));
        // Initialize direction with speed
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        extern float g_DvdSpeed;
        data->logoDX = cosf(angle) * g_DvdSpeed;
        data->logoDY = sinf(angle) * g_DvdSpeed;
    }

    data->logoX += data->logoDX;
    data->logoY += data->logoDY;

    bool bounced = false;

    if (data->logoX <= 0) {
        data->logoX = 0; data->logoDX = abs(data->logoDX); bounced = true;
    }
    else if (data->logoX + tw >= width) {
        data->logoX = (float)(width - tw); data->logoDX = -abs(data->logoDX); bounced = true;
    }

    if (data->logoY <= 0) {
        data->logoY = 0;
        data->logoDY = abs(data->logoDY);
        bounced = true;
    }
    else if (data->logoY + visualHeight >= height) {
        data->logoY = (float)(height - visualHeight);
        data->logoDY = -abs(data->logoDY);
        bounced = true;
    }

    if (bounced) {
        data->logoColorIndex = (data->logoColorIndex + 1) % 6;
    }

    COLORREF colors[6] = {
        RGB(255,50,50), RGB(50,255,50), RGB(100,100,255),
        RGB(255,255,50), RGB(255,50,255), RGB(50,255,255)
    };

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, colors[data->logoColorIndex]);

    TextOutA(memDC, (int)data->logoX, (int)data->logoY - tm.tmInternalLeading, logoText, (int)strlen(logoText));

    SelectObject(memDC, hOldFont);
    DeleteObject(hDvdFont);
}