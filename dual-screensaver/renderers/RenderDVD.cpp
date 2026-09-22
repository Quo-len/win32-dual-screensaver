#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "../settings/DvdSettings.h"
#include <string.h>
#include <cmath> // Added for fabsf, cosf, sinf

struct DvdState {
    float x = 0.0f;
    float y = 0.0f;
    float dx = 3.0f;
    float dy = 2.5f;
    int colorIndex = 0;
};

void RenderDVD(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<DvdState>(7);
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

    if (state.x == 0 && state.y == 0) {
        state.x = (float)(rand() % max(1, width - tw));
        state.y = (float)(rand() % max(1, height - visualHeight));
        
        // Ensure angle is not perfectly horizontal or vertical 
        int angleDegree = 0;
        do {
            angleDegree = rand() % 360;
        } while (angleDegree % 90 == 0);

        float angle = (float)angleDegree * 3.14159f / 180.0f;
        extern float g_DvdSpeed;
        state.dx = cosf(angle) * g_DvdSpeed;
        state.dy = sinf(angle) * g_DvdSpeed;
    }

    state.x += state.dx;
    state.y += state.dy;

    bool bounced = false;

    // Fixed: Replaced int abs() with float fabsf() to prevent precision truncation to 0
    if (state.x <= 0) {
        state.x = 0; state.dx = fabsf(state.dx); bounced = true;
    }
    else if (state.x + tw >= width) {
        state.x = (float)(width - tw); state.dx = -fabsf(state.dx); bounced = true;
    }

    if (state.y <= 0) {
        state.y = 0;
        state.dy = fabsf(state.dy);
        bounced = true;
    }
    else if (state.y + visualHeight >= height) {
        state.y = (float)(height - visualHeight);
        state.dy = -fabsf(state.dy);
        bounced = true;
    }

    if (bounced) {
        state.colorIndex = (state.colorIndex + 1) % 6;
    }

    COLORREF colors[6] = {
        RGB(255,50,50), RGB(50,255,50), RGB(100,100,255),
        RGB(255,255,50), RGB(255,50,255), RGB(50,255,255)
    };

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, colors[state.colorIndex]);

    TextOutA(memDC, (int)state.x, (int)state.y - tm.tmInternalLeading, logoText, (int)strlen(logoText));

    SelectObject(memDC, hOldFont);
    DeleteObject(hDvdFont);
}

REGISTER_SCREENSAVER(
    7,
    L"Bouncing DVD Logo",
    "dvd",
    { "dvd" },
    WRAP_LEGACY(RenderDVD),
    GetDvdSettings()
);