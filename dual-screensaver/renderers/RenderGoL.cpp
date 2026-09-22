#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/GolSettings.h"

struct GolState {
    DWORD lastUpdate = 0;
};

void RenderGoL(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<GolState>(1);

    int targetCols = width / g_GolCellSize;
    int targetRows = height / g_GolCellSize;

    if (targetCols <= 0) targetCols = 1;
    if (targetRows <= 0) targetRows = 1;

    if (data->cols != targetCols || data->rows != targetRows || data->grid.empty()) {
        data->cols = targetCols;
        data->rows = targetRows;
        data->stride = targetCols + 2;

        int totalSize = data->stride * (targetRows + 2);
        data->grid.assign(totalSize, 0);
        data->nextGrid.assign(totalSize, 0);
        data->pixels.assign(targetCols * targetRows, 0);

        for (int y = 1; y <= data->rows; y++) {
            for (int x = 1; x <= data->cols; x++) {
                data->grid[y * data->stride + x] = ((rand() % 100) > 70) ? 1 : 0;
            }
        }
    }

    DWORD now = (DWORD)GetTickCount64();
    if (now - state.lastUpdate >= (DWORD)g_GolSpeed) {
        unsigned char* grid = data->grid.data();
        unsigned char* next = data->nextGrid.data();
        int stride = data->stride;
        int rows = data->rows;
        int cols = data->cols;

        for (int x = 1; x <= cols; x++) {
            grid[x] = grid[rows * stride + x];
            grid[(rows + 1) * stride + x] = grid[stride + x];
        }
        for (int y = 0; y <= rows + 1; y++) {
            grid[y * stride] = grid[y * stride + cols];
            grid[y * stride + cols + 1] = grid[y * stride + 1];
        }

        bool changed = false;
        int aliveCount = 0;

        for (int y = 1; y <= rows; y++) {
            int idx = y * stride + 1;
            for (int x = 1; x <= cols; x++, idx++) {
                int n = grid[idx - stride - 1] + grid[idx - stride] + grid[idx - stride + 1] +
                    grid[idx - 1] + grid[idx + 1] +
                    grid[idx + stride - 1] + grid[idx + stride] + grid[idx + stride + 1];

                unsigned char state = grid[idx];
                unsigned char nextState = (n == 3 || (n == 2 && state)) ? 1 : 0;

                next[idx] = nextState;

                if (nextState) aliveCount++;
                if (state != nextState) changed = true;
            }
        }

        if (!changed || aliveCount == 0) {
            for (int y = 1; y <= rows; y++) {
                for (int x = 1; x <= cols; x++) {
                    next[y * stride + x] = ((rand() % 100) > 70) ? 1 : 0;
                }
            }
        }

        data->grid.swap(data->nextGrid);
        state.lastUpdate = now;
    }

    uint32_t* px = data->pixels.data();
    unsigned char* grid = data->grid.data();
    int stride = data->stride;

    for (int y = 1; y <= data->rows; y++) {
        int rowOffset = y * stride;
        for (int x = 1; x <= data->cols; x++) {
            *px++ = grid[rowOffset + x] ? 0x0000FF00 : 0x00000000;
        }
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = data->cols;
    bmi.bmiHeader.biHeight = -data->rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height, 0, 0, data->cols, data->rows,
        data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

REGISTER_SCREENSAVER(
    1,
    L"Game of Life",
    "gol",
    { "gol", "life", "gameoflife" },
    WRAP_LEGACY(RenderGoL),
    GetGolSettings()
);
