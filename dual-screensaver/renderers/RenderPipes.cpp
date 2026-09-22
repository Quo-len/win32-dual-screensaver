#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <algorithm>

// Box-drawing chars: 0=empty 1=═ 2=║ 3=╔ 4=╗ 5=╚ 6=╝ 7=╬
static const wchar_t PIPE_CHARS[] = {
    L' ',
    L'\x2550', // ═
    L'\x2551', // ║
    L'\x2554', // ╔
    L'\x2557', // ╗
    L'\x255A', // ╚
    L'\x255D', // ╝
    L'\x256C', // ╬
};

// dirs: 0=R 1=D 2=L 3=U
// Returns the corner char type (1-6) that visually connects
// the tail (from where we came) to the new direction.
static unsigned char cornerType(int oldDir, int newDir) {
    // incoming tail direction = opposite of oldDir
    // ┌ (3): connects R+D  ┐ (4): connects L+D  └ (5): connects R+U  ┘ (6): connects L+U
    if ((oldDir == 0 && newDir == 1) || (oldDir == 3 && newDir == 2)) return 4; // ┐
    if ((oldDir == 0 && newDir == 3) || (oldDir == 1 && newDir == 2)) return 6; // ┘
    if ((oldDir == 2 && newDir == 1) || (oldDir == 3 && newDir == 0)) return 3; // ┌
    if ((oldDir == 2 && newDir == 3) || (oldDir == 1 && newDir == 0)) return 5; // └
    return 1; // fallback horizontal
}

static const COLORREF PALETTE[] = {
    RGB(255, 80, 80),  RGB(80, 255, 80),  RGB(80, 160, 255),
    RGB(255, 220, 0),  RGB(255, 80, 255), RGB(0,  230, 230),
    RGB(255, 160, 0),  RGB(160, 255, 80),
};
static const int PAL_SIZE = 8;

struct PipesState {
    std::vector<int> pipeHeads;
    std::vector<int> pipeDirs;
    std::vector<COLORREF> pipeColors;
};

static void spawnPipe(PipesState& state, int cols, int rows) {
    if ((int)state.pipeHeads.size() >= 7) return;
    state.pipeHeads.push_back((rand() % rows) * cols + (rand() % cols));
    state.pipeDirs.push_back(rand() % 4);
    state.pipeColors.push_back(PALETTE[rand() % PAL_SIZE]);
}

void RenderPipes(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<PipesState>(17);

    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);
    int cw = tm.tmAveCharWidth;
    int ch = tm.tmHeight;
    if (cw <= 0 || ch <= 0) return;

    int cols = width  / cw;
    int rows = height / ch;
    if (cols <= 0 || rows <= 0) return;

    // data->cols/rows hold cell-space dims; pixels[] repurposed as COLORREF per cell
    if (data->cols != cols || data->rows != rows || data->grid.empty()) {
        data->cols = cols;
        data->rows = rows;
        data->grid.assign(cols * rows, 0);         // char type
        data->pixels.assign(cols * rows, 0);       // color (as COLORREF)
        state.pipeHeads.clear();
        state.pipeDirs.clear();
        state.pipeColors.clear();
        int n = 2 + rand() % 3;
        for (int i = 0; i < n; i++) spawnPipe(state, cols, rows);
    }

    // Reset if too full (>= 70%)
    int filled = 0;
    for (auto c : data->grid) if (c) filled++;
    if (filled >= cols * rows * 70 / 100) {
        data->grid.assign(cols * rows, 0);
        data->pixels.assign(cols * rows, 0);
        state.pipeHeads.clear();
        state.pipeDirs.clear();
        state.pipeColors.clear();
        int n = 2 + rand() % 3;
        for (int i = 0; i < n; i++) spawnPipe(state, cols, rows);
    }

    // Advance each pipe head by 3 cells per frame
    int numPipes = (int)state.pipeHeads.size();
    for (int p = 0; p < numPipes; p++) {
        for (int step = 0; step < 3; step++) {
            int idx = state.pipeHeads[p];
            int x   = idx % cols;
            int y   = idx / cols;
            int dir = state.pipeDirs[p];

            bool isOccupied = (data->grid[idx] != 0);

            // Decide new direction
            int newDir = dir;
            // Only turn if the cell is empty (to avoid placing corners on top of existing pipes)
            if (!isOccupied && (rand() % 100 < 20)) { 
                int opts[2];
                if (dir == 0 || dir == 2) { opts[0] = 1; opts[1] = 3; }
                else                      { opts[0] = 0; opts[1] = 2; }
                newDir = opts[rand() % 2];
                // Overwrite current cell with the correct corner
                data->grid[idx] = cornerType(dir, newDir);
                data->pixels[idx] = (uint32_t)state.pipeColors[p];
            } else {
                // Keep straight. If already occupied by a different pipe, turn it into an intersection (7)
                if (isOccupied && data->grid[idx] != 7) {
                    data->grid[idx] = 7;
                    // We overwrite the color to the new pipe's color so it looks like it goes "over"
                    data->pixels[idx] = (uint32_t)state.pipeColors[p];
                } else if (!isOccupied) {
                    data->grid[idx]   = (dir == 0 || dir == 2) ? 1 : 2; // ═ or ║
                    data->pixels[idx] = (uint32_t)state.pipeColors[p];
                }
            }

            // Compute next position
            int nx = x, ny = y;
            if      (newDir == 0) nx++;
            else if (newDir == 1) ny++;
            else if (newDir == 2) nx--;
            else                  ny--;

            // If out of bounds, teleport pipe to new random start
            if (nx < 0 || nx >= cols || ny < 0 || ny >= rows) {
                state.pipeHeads[p] = (rand() % rows) * cols + (rand() % cols);
                state.pipeDirs[p]  = rand() % 4;
                state.pipeColors[p] = PALETTE[rand() % PAL_SIZE];
            } else {
                state.pipeHeads[p] = ny * cols + nx;
                state.pipeDirs[p]  = newDir;
            }
        }
    }

    // Occasionally spawn an extra pipe
    if (rand() % 180 == 0) spawnPipe(state, cols, rows);

    SetBkMode(memDC, TRANSPARENT);

    // Pass 1: Draw drop shadows for depth
    SetTextColor(memDC, RGB(30, 30, 30)); // Dark shadow color
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int idx = y * cols + x;
            unsigned char ct = data->grid[idx];
            if (ct == 0) continue;
            wchar_t wc = PIPE_CHARS[ct];
            TextOutW(memDC, x * cw + 2, y * ch + 2, &wc, 1);
        }
    }

    // Pass 2: Draw the actual pipes
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            int idx = y * cols + x;
            unsigned char ct = data->grid[idx];
            if (ct == 0) continue;
            wchar_t wc = PIPE_CHARS[ct];
            SetTextColor(memDC, (COLORREF)data->pixels[idx]);
            TextOutW(memDC, x * cw, y * ch, &wc, 1);
        }
    }
}

REGISTER_SCREENSAVER(17, L"Pipes", "pipes", { "pipes" }, WRAP_LEGACY(RenderPipes), {});
