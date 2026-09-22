#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <algorithm>

// States
static const unsigned char BB_OFF    = 0;
static const unsigned char BB_FIRE   = 1;
static const unsigned char BB_DYING  = 2;

// Pixel colours (BGRA in memory, but StretchDIBits treats as RGB)
static const uint32_t COL_OFF   = 0xFF000000;
static const uint32_t COL_FIRE  = 0xFFFFFFFF; // white
static const uint32_t COL_DYING = 0xFF2244AA; // blue

static const int BB_CELL = 3; // pixels per cell

void RenderBriansBrain(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    int cols = width  / BB_CELL;
    int rows = height / BB_CELL;
    if (cols <= 0 || rows <= 0) return;

    int sz = cols * rows;

    if (data->cols != cols || data->rows != rows || (int)data->grid.size() != sz) {
        data->cols = cols;
        data->rows = rows;
        data->grid.assign(sz, BB_OFF);
        data->nextGrid.assign(sz, BB_OFF);
        data->pixels.assign(sz, COL_OFF);
        // ~25% random seed
        for (int i = 0; i < sz; i++)
            data->grid[i] = (rand() % 4 == 0) ? BB_FIRE : BB_OFF;
    }

    // Tick
    int aliveCount = 0;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int idx = y * cols + x;
            unsigned char st = data->grid[idx];
            unsigned char ns;

            if (st == BB_FIRE) {
                ns = BB_DYING;
            } else if (st == BB_DYING) {
                ns = BB_OFF;
            } else {
                // Count firing (=1) neighbours — 8-connected
                int cnt = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    int ny = y + dy;
                    if (ny < 0 || ny >= rows) continue;
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        if (nx < 0 || nx >= cols) continue;
                        if (data->grid[ny * cols + nx] == BB_FIRE) cnt++;
                    }
                }
                ns = (cnt == 2) ? BB_FIRE : BB_OFF;
            }

            data->nextGrid[idx] = ns;
            data->pixels[idx]   = (ns == BB_FIRE)  ? COL_FIRE
                                 : (ns == BB_DYING) ? COL_DYING
                                 :                    COL_OFF;
            if (ns == BB_FIRE) aliveCount++;
        }
    }
    data->grid.swap(data->nextGrid);

    // Re-seed if activity dies out
    if (aliveCount < 8) {
        for (int i = 0; i < sz; i++)
            data->grid[i] = (rand() % 4 == 0) ? BB_FIRE : BB_OFF;
        data->pixels.assign(sz, COL_OFF);
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = cols;
    bmi.bmiHeader.biHeight      = -rows;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(memDC, 0, 0, cols * BB_CELL, rows * BB_CELL,
        0, 0, cols, rows, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

REGISTER_SCREENSAVER(18, L"Brian's Brain", "brain", { "brain", "brian" }, WRAP_LEGACY(RenderBriansBrain), {});
