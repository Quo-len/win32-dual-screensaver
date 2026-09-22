#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <string.h>

struct FireState {
    int width = 0;
    int height = 0;
    std::vector<int> grid;
};

void RenderFire(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<FireState>(13);

    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cWidth = tm.tmAveCharWidth;
    int cHeight = tm.tmHeight;

    int widthInChars = width / cWidth;
    int heightInChars = height / cHeight;
    int size = widthInChars * heightInChars;

    if (state.width != widthInChars || state.height != heightInChars) {
        state.width = widthInChars;
        state.height = heightInChars;
        state.grid.assign(size + widthInChars + 1, 0);
    }

    for (int i = 0; i < state.width / 9; i++) {
        int r = rand() % state.width;
        state.grid[r + state.width * (state.height - 1)] = 65;
    }

    for (int i = 0; i < size; i++) {
        state.grid[i] = (state.grid[i] + state.grid[i + 1] +
            state.grid[i + state.width] +
            state.grid[i + state.width + 1]) / 4;
    }

    const char chars[] = " .:^*xsS#$";

    for (int i = 0; i < size - 1; i++) {
        int heat = state.grid[i];

        COLORREF color;
        if (heat > 15) color = RGB(99, 143, 189);
        else if (heat > 9) color = RGB(255, 255, 85);
        else if (heat > 4) color = RGB(255, 85, 85);
        else color = RGB(128, 128, 128);

        int charIdx = (heat > 9) ? 9 : heat;
        char c = chars[charIdx];

        if (c != ' ') {
            SetTextColor(memDC, color);
            TextOutA(memDC, (i % state.width) * cWidth, (i / state.width) * cHeight, &c, 1);
        }
    }
}

REGISTER_SCREENSAVER(
    13,
    L"ASCII Fire",
    "fire",
    { "fire", "flame" },
    WRAP_LEGACY(RenderFire),
    {}
);

