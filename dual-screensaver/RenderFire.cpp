#include "framework.h"
#include "Renderers.h"
#include <string.h>

void RenderFire(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cWidth = tm.tmAveCharWidth;
    int cHeight = tm.tmHeight;

    int widthInChars = width / cWidth;
    int heightInChars = height / cHeight;
    int size = widthInChars * heightInChars;

    if (data->fireWidth != widthInChars || data->fireHeight != heightInChars) {
        data->fireWidth = widthInChars;
        data->fireHeight = heightInChars;
        data->fireGrid.assign(size + widthInChars + 1, 0);
    }

    for (int i = 0; i < data->fireWidth / 9; i++) {
        int r = rand() % data->fireWidth;
        data->fireGrid[r + data->fireWidth * (data->fireHeight - 1)] = 65;
    }

    for (int i = 0; i < size; i++) {
        data->fireGrid[i] = (data->fireGrid[i] + data->fireGrid[i + 1] +
            data->fireGrid[i + data->fireWidth] +
            data->fireGrid[i + data->fireWidth + 1]) / 4;
    }

    const char chars[] = " .:^*xsS#$";

    for (int i = 0; i < size - 1; i++) {
        int heat = data->fireGrid[i];

        COLORREF color;
        if (heat > 15) color = RGB(99, 143, 189);
        else if (heat > 9) color = RGB(255, 255, 85);
        else if (heat > 4) color = RGB(255, 85, 85);
        else color = RGB(128, 128, 128);

        int charIdx = (heat > 9) ? 9 : heat;
        char c = chars[charIdx];

        if (c != ' ') {
            SetTextColor(memDC, color);
            TextOutA(memDC, (i % data->fireWidth) * cWidth, (i / data->fireWidth) * cHeight, &c, 1);
        }
    }
}
