#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/AntSettings.h"
#include <vector>
#include <cstdlib> // For rand()

struct AntState {
    int x, y;
    int dir;
    unsigned int color; // Track individual color for each ant
};

struct LangtonState {
    int antCols = 0;
    int antRows = 0;
    std::vector<AntState> ants;
    std::vector<unsigned char> antGrid;
};

void RenderLangton(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<LangtonState>(16);

    int cellSize = 2;
    int cols = width / cellSize;
    int rows = height / cellSize;

    if (cols <= 0 || rows <= 0) return;

    // Initialization block
    if (state.antCols != cols || state.antRows != rows) {
        state.antCols = cols;
        state.antRows = rows;
        state.antGrid.assign(cols * rows, 0);
        data->pixels.assign(cols * rows, 0xFF000000); // Black Background
        state.ants.clear();

        for (int i = 0; i < g_AntCount; i++) {
            // Random placement anywhere on the screen
            int rx = rand() % cols;
            int ry = rand() % rows;
            int rdir = rand() % 4;

            // Generate a random bright color for this specific ant
            unsigned char r = (rand() % 156) + 100;
            unsigned char g = (rand() % 156) + 100;
            unsigned char b = (rand() % 156) + 100;
            unsigned int antColor = 0xFF000000 | (r << 16) | (g << 8) | b;

            state.ants.push_back({ rx, ry, rdir, antColor });
        }
    }

    // Update block
    for (int step = 0; step < g_AntSpeed; step++) {
        for (auto& ant : state.ants) {
            // Wrap around screen edges
            if (ant.x < 0) ant.x += cols;
            if (ant.x >= cols) ant.x -= cols;
            if (ant.y < 0) ant.y += rows;
            if (ant.y >= rows) ant.y -= rows;

            int idx = ant.y * cols + ant.x;
            unsigned char cellState = state.antGrid[idx];

            if (cellState == 0) {
                ant.dir = (ant.dir + 1) % 4;
                state.antGrid[idx] = 1;
                data->pixels[idx] = ant.color; // Drop this specific ant's color
            }
            else {
                ant.dir = (ant.dir + 3) % 4;
                state.antGrid[idx] = 0;
                data->pixels[idx] = 0xFF000000;
            }

            // Move forward
            if (ant.dir == 0) ant.y--;
            else if (ant.dir == 1) ant.x++;
            else if (ant.dir == 2) ant.y++;
            else if (ant.dir == 3) ant.x--;
        }
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cols;
    bmi.bmiHeader.biHeight = -rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, cols * cellSize, rows * cellSize,
        0, 0, cols, rows, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

REGISTER_SCREENSAVER(
    16,
    L"Langton's Ant",
    "ant",
    { "ant", "langton" },
    WRAP_LEGACY(RenderLangton),
    GetAntSettings()
);