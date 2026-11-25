#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "GlyphAtlas.h"
#include <string.h>

struct FireState {
    int width = 0;
    int height = 0;
    std::vector<int> grid;
    GlyphAtlas atlas;
    HFONT atlasFont = NULL;
    PixelCanvas canvas;
};

void RenderFire(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<FireState>(13);

    SelectObject(memDC, data->hFont);

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

    if (state.atlasFont != data->hFont) {
        state.atlas.Build(memDC, data->hFont, L' ', 95);
        state.atlasFont = data->hFont;
    }
    state.canvas.Begin(width, height);

    for (int i = 0; i < size - 1; i++) {
        int heat = state.grid[i];

        uint32_t color;
        if (heat > 15) color = 0x638FBD;
        else if (heat > 9) color = 0xFFFF55;
        else if (heat > 4) color = 0xFF5555;
        else color = 0x808080;

        int charIdx = (heat > 9) ? 9 : heat;
        char c = chars[charIdx];

        if (c != ' ') {
            state.atlas.Draw(state.canvas, (i % state.width) * cWidth, (i / state.width) * cHeight, c - ' ', color);
        }
    }

    state.canvas.End(memDC, rect.left, rect.top);
}

REGISTER_SCREENSAVER(
    13,
    L"ASCII Fire",
    "fire",
    { "fire", "flame" },
    WRAP_LEGACY(RenderFire),
    {}
);

