#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "../settings/ClockSettings.h"
#include <stdio.h>
#include <string.h>

struct ClockState {
    float digitOffset[8] = { 0 };
    char currentStr[16] = { 0 };
    char targetStr[16] = { 0 };
};

void RenderClock(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<ClockState>(11);

    int clockFontSize = max(30, height / 3);
    HFONT hClockFont = CreateFontA(clockFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FIXED_PITCH | FF_MODERN, "Consolas");

    HFONT hOldFont = (HFONT)SelectObject(memDC, hClockFont);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char newTime[16];
    sprintf_s(newTime, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

    if (state.currentStr[0] == '\0') {
        strcpy_s(state.currentStr, newTime);
        strcpy_s(state.targetStr, newTime);
        for (int i = 0; i < 8; i++) state.digitOffset[i] = 0.0f;
    }

    for (int i = 0; i < 8; i++) {
        if (state.targetStr[i] != newTime[i])
            state.targetStr[i] = newTime[i];
    }

    SetTextColor(memDC, RGB(0, 255, 150));
    SetBkMode(memDC, TRANSPARENT);

    SIZE sz;
    GetTextExtentPoint32A(memDC, "0", 1, &sz);
    int charW = sz.cx;
    int charH = sz.cy;

    int totalW = 8 * charW;
    int startX = (width - totalW) / 2;
    int startY = (height - charH) / 2;

    for (int i = 0; i < 8; i++) {
        if (state.currentStr[i] != state.targetStr[i]) {
            state.digitOffset[i] += 0.12f;
            if (state.digitOffset[i] >= 1.0f) {
                state.currentStr[i] = state.targetStr[i];
                state.digitOffset[i] = 0.0f;
            }
        }

        int x = startX + i * charW;
        int y = startY;

        HRGN hRgn = CreateRectRgn(x, y, x + charW, y + charH);
        SelectClipRgn(memDC, hRgn);

        if (state.currentStr[i] == state.targetStr[i]) {
            char str[2] = { state.currentStr[i], 0 };
            TextOutA(memDC, x, y, str, 1);
        }
        else {
            char strOld[2] = { state.currentStr[i], 0 };
            char targetOld[2] = { state.targetStr[i], 0 };
            int yOffset = (int)(state.digitOffset[i] * charH);
            TextOutA(memDC, x, y - yOffset, strOld, 1);
            TextOutA(memDC, x, y + charH - yOffset, targetOld, 1);
        }

        SelectClipRgn(memDC, NULL);
        DeleteObject(hRgn);
    }

    SelectObject(memDC, hOldFont);
    DeleteObject(hClockFont);
}

REGISTER_SCREENSAVER(
    11,
    L"Odometer Clock",
    "clock",
    { "clock", "odometer" },
    WRAP_LEGACY(RenderClock),
    GetClockSettings()
);

