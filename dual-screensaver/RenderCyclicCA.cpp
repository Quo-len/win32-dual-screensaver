#include "framework.h"
#include "Renderers.h"
#include <algorithm>

static const int CYCLIC_K    = 16; // number of states
static const int CYCLIC_CELL = 3;  // px per cell

// HSV→RGB, hue in [0,6)
static uint32_t cyclicColor(int state) {
    float h = (float)state / CYCLIC_K * 6.0f;
    int   i = (int)h;
    float f = h - (float)i;
    // full saturation, value=1, dimmed slightly
    float v = 0.95f, s = 1.0f;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
    return 0xFF000000 | ((uint32_t)(r * 255) << 16) | ((uint32_t)(g * 255) << 8) | (uint32_t)(b * 255);
}

void RenderCyclicCA(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    int cols = width  / CYCLIC_CELL;
    int rows = height / CYCLIC_CELL;
    if (cols <= 0 || rows <= 0) return;

    int sz = cols * rows;

    if (data->antCols != cols || data->antRows != rows) {
        data->antCols  = cols;
        data->antRows  = rows;
        data->antGrid.assign(sz, 0);
        data->cyclicNext.assign(sz, 0);
        data->pixels.assign(sz, 0xFF000000);
        for (auto& c : data->antGrid) c = (unsigned char)(rand() % CYCLIC_K);
    }

    // Precompute colour table once
    static uint32_t colorTbl[CYCLIC_K];
    static bool     colorInit = false;
    if (!colorInit) {
        for (int i = 0; i < CYCLIC_K; i++) colorTbl[i] = cyclicColor(i);
        colorInit = true;
    }

    // One CA tick — a cell advances only when ≥1 neighbour holds (state+1)%K
    auto& cur  = data->antGrid;
    auto& next = data->cyclicNext;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int idx    = y * cols + x;
            int st     = cur[idx];
            int want   = (st + 1) % CYCLIC_K;
            bool adv   = false;

            if (x > 0      && cur[idx - 1]    == want) adv = true;
            if (x < cols-1 && cur[idx + 1]    == want) adv = true;
            if (y > 0      && cur[idx - cols]  == want) adv = true;
            if (y < rows-1 && cur[idx + cols]  == want) adv = true;

            unsigned char ns = adv ? (unsigned char)want : (unsigned char)st;
            next[idx]          = ns;
            data->pixels[idx]  = colorTbl[ns];
        }
    }
    std::swap(cur, next);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = cols;
    bmi.bmiHeader.biHeight      = -rows;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(memDC, 0, 0, cols * CYCLIC_CELL, rows * CYCLIC_CELL,
        0, 0, cols, rows, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}
