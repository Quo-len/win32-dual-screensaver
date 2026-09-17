#include "framework.h"
#include "Renderers.h"
#include <stdio.h>
#include <string.h>

void RenderClock(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int clockFontSize = max(30, height / 3);
    HFONT hClockFont = CreateFontA(clockFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FIXED_PITCH | FF_MODERN, "Consolas");

    HFONT hOldFont = (HFONT)SelectObject(memDC, hClockFont);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char newTime[16];
    sprintf_s(newTime, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

    if (data->currentStr[0] == '\0') {
        strcpy_s(data->currentStr, newTime);
        strcpy_s(data->targetStr, newTime);
        for (int i = 0; i < 8; i++) data->digitOffset[i] = 0.0f;
    }

    for (int i = 0; i < 8; i++) {
        if (data->targetStr[i] != newTime[i])
            data->targetStr[i] = newTime[i];
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
        if (data->currentStr[i] != data->targetStr[i]) {
            data->digitOffset[i] += 0.12f;
            if (data->digitOffset[i] >= 1.0f) {
                data->currentStr[i] = data->targetStr[i];
                data->digitOffset[i] = 0.0f;
            }
        }

        int x = startX + i * charW;
        int y = startY;

        HRGN hRgn = CreateRectRgn(x, y, x + charW, y + charH);
        SelectClipRgn(memDC, hRgn);

        if (data->currentStr[i] == data->targetStr[i]) {
            char str[2] = { data->currentStr[i], 0 };
            TextOutA(memDC, x, y, str, 1);
        }
        else {
            char strOld[2] = { data->currentStr[i], 0 };
            char targetOld[2] = { data->targetStr[i], 0 };
            int yOffset = (int)(data->digitOffset[i] * charH);
            TextOutA(memDC, x, y - yOffset, strOld, 1);
            TextOutA(memDC, x, y + charH - yOffset, targetOld, 1);
        }

        SelectClipRgn(memDC, NULL);
        DeleteObject(hRgn);
    }

    SelectObject(memDC, hOldFont);
    DeleteObject(hClockFont);
}
