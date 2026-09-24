#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/MazeSettings.h"
#include <vector>

namespace MazeLegacy {

struct MazeCell {
    bool visited;
    bool wallTop, wallRight, wallBottom, wallLeft;
    bool solveVisited;
    bool inPath;
};

struct MazeStateLegacy {
    int cols = 0;
    int rows = 0;
    int state = 0;
    int start = 0;
    int end = 0;
    std::vector<MazeCell> grid;
    std::vector<int> stack;
    std::vector<int> solveStack;
    DWORD lastUpdate = 0;
    int waitTimer = 0;
};

void RenderMazeLegacy(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<MazeStateLegacy>(34);

    int cellSize = max(10, g_TextSize);
    int padding = 40;
    int mCols = (width - padding * 2) / cellSize;
    int mRows = (height - padding * 2) / cellSize;

    if (mCols <= 2) mCols = 3;
    if (mRows <= 2) mRows = 3;

    // 1. Manage Pixel Buffer Size
    if (data->pixels.size() != (size_t)(width * height)) {
        data->pixels.assign(width * height, 0x00000000);
    }
    else {
        // Fast-clear screen to black
        memset(data->pixels.data(), 0, width * height * sizeof(uint32_t));
    }

    if (state.cols != mCols || state.rows != mRows || state.grid.empty() || state.state == 0) {
        state.cols = mCols;
        state.rows = mRows;

        state.grid.clear();
        state.grid.assign(mCols * mRows, { false, true, true, true, true, false, false });
        state.state = 1;
        state.stack.clear();
        state.solveStack.clear();

        state.start = rand() % (mCols * mRows);
        state.end = rand() % (mCols * mRows);
        while (state.end == state.start)
            state.end = rand() % (mCols * mRows);

        state.stack.push_back(state.start);
        state.grid[state.start].visited = true;
        state.lastUpdate = (DWORD)GetTickCount64();
        state.waitTimer = 0;
    }

    DWORD now = (DWORD)GetTickCount64();
    int stepsPerFrame = max(1, (int)(state.state == 1 ? g_MazeBuildSpeed : g_MazeSolveSpeed));

    // 2. Original Maze Generation Logic (Unchanged)
    if (now - state.lastUpdate > 16) {
        state.lastUpdate = now;

        if (state.state == 1) {
            for (int step = 0; step < stepsPerFrame * 5 && !state.stack.empty(); ++step) {
                int current = state.stack.back();
                int cx = current % state.cols;
                int cy = current / state.cols;

                std::vector<int> neighbors, dirs;
                if (cy > 0 && !state.grid[current - state.cols].visited) { neighbors.push_back(current - state.cols); dirs.push_back(0); }
                if (cx < state.cols - 1 && !state.grid[current + 1].visited) { neighbors.push_back(current + 1); dirs.push_back(1); }
                if (cy < state.rows - 1 && !state.grid[current + state.cols].visited) { neighbors.push_back(current + state.cols); dirs.push_back(2); }
                if (cx > 0 && !state.grid[current - 1].visited) { neighbors.push_back(current - 1); dirs.push_back(3); }

                if (!neighbors.empty()) {
                    int r = rand() % (int)neighbors.size();
                    int next = neighbors[r];
                    int dir = dirs[r];

                    if (dir == 0) { state.grid[current].wallTop = false; state.grid[next].wallBottom = false; }
                    else if (dir == 1) { state.grid[current].wallRight = false; state.grid[next].wallLeft = false; }
                    else if (dir == 2) { state.grid[current].wallBottom = false; state.grid[next].wallTop = false; }
                    else if (dir == 3) { state.grid[current].wallLeft = false; state.grid[next].wallRight = false; }

                    state.grid[next].visited = true;
                    state.stack.push_back(next);
                }
                else {
                    state.stack.pop_back();
                }
            }
            if (state.stack.empty()) {
                state.state = 2;
                state.solveStack.push_back(state.start);
                state.grid[state.start].solveVisited = true;
                state.grid[state.start].inPath = true;
            }
        }
        else if (state.state == 2) {
            for (int step = 0; step < stepsPerFrame && !state.solveStack.empty(); ++step) {
                int current = state.solveStack.back();

                if (current == state.end) {
                    state.state = 3;
                    state.waitTimer = 45;
                    break;
                }

                int cx = current % state.cols;
                int cy = current / state.cols;

                std::vector<int> nextDirs;
                if (!state.grid[current].wallTop && !state.grid[current - state.cols].solveVisited) nextDirs.push_back(current - state.cols);
                if (!state.grid[current].wallRight && !state.grid[current + 1].solveVisited) nextDirs.push_back(current + 1);
                if (!state.grid[current].wallBottom && !state.grid[current + state.cols].solveVisited) nextDirs.push_back(current + state.cols);
                if (!state.grid[current].wallLeft && !state.grid[current - 1].solveVisited) nextDirs.push_back(current - 1);

                if (!nextDirs.empty()) {
                    int next = nextDirs[0];
                    state.grid[next].solveVisited = true;
                    state.grid[next].inPath = true;
                    state.solveStack.push_back(next);
                }
                else {
                    state.grid[current].inPath = false;
                    state.solveStack.pop_back();
                }
            }
        }
        else if (state.state == 3) {
            state.waitTimer--;
            if (state.waitTimer <= 0)
                state.state = 0;
        }
    }

    int offsetX = (width - (state.cols * cellSize)) / 2;
    int offsetY = (height - (state.rows * cellSize)) / 2;

    // 3. Fast Pixel Writing Helper
    uint32_t* pxArr = data->pixels.data();
    auto FillRectPx = [&](int rx, int ry, int rw, int rh, uint32_t color) {
        int startX = max(0, rx);
        int startY = max(0, ry);
        int endX = min(width, rx + rw);
        int endY = min(height, ry + rh);
        if (startX >= endX || startY >= endY) return;

        for (int y = startY; y < endY; ++y) {
            int rowOffset = y * width;
            for (int x = startX; x < endX; ++x) {
                pxArr[rowOffset + x] = color;
            }
        }
    };

    // Define colors directly as BGRA 32-bit integers
    const uint32_t COLOR_PATH = 0xFFFF0000; // Red
    const uint32_t COLOR_SOLVE = 0xFF963232; // Purple-ish
    const uint32_t COLOR_WALL = 0xFF00FF00; // Green
    const uint32_t COLOR_START = 0xFFFFFF00; // Yellow
    const uint32_t COLOR_END = 0xFF00FFFF; // Cyan
    const uint32_t COLOR_HEAD = 0xFFFFFFFF; // White

    // 4. Draw Maze entirely into RAM
    for (int cy = 0; cy < state.rows; cy++) {
        for (int cx = 0; cx < state.cols; cx++) {
            int i = cy * state.cols + cx;
            int px = offsetX + cx * cellSize;
            int py = offsetY + cy * cellSize;

            if (state.grid[i].inPath) {
                FillRectPx(px + cellSize / 4, py + cellSize / 4, cellSize - (cellSize / 4) * 2, cellSize - (cellSize / 4) * 2, COLOR_PATH);
            }
            else if (state.grid[i].solveVisited) {
                FillRectPx(px + cellSize / 3, py + cellSize / 3, cellSize - (cellSize / 3) * 2, cellSize - (cellSize / 3) * 2, COLOR_SOLVE);
            }

            if (state.grid[i].visited) {
                if (state.grid[i].wallTop) FillRectPx(px, py, cellSize, 2, COLOR_WALL);
                if (state.grid[i].wallBottom) FillRectPx(px, py + cellSize - 2, cellSize, 2, COLOR_WALL);
                if (state.grid[i].wallLeft) FillRectPx(px, py, 2, cellSize, COLOR_WALL);
                if (state.grid[i].wallRight) FillRectPx(px + cellSize - 2, py, 2, cellSize, COLOR_WALL);
            }
        }
    }

    if (state.state > 0) {
        int sx = (state.start % state.cols) * cellSize + offsetX;
        int sy = (state.start / state.cols) * cellSize + offsetY;
        FillRectPx(sx + 2, sy + 2, cellSize - 4, cellSize - 4, COLOR_START);

        int ex = (state.end % state.cols) * cellSize + offsetX;
        int ey = (state.end / state.cols) * cellSize + offsetY;
        FillRectPx(ex + 2, ey + 2, cellSize - 4, cellSize - 4, COLOR_END);
    }

    if (state.state == 1 && !state.stack.empty()) {
        int current = state.stack.back();
        int cx = current % state.cols;
        int cy = current / state.cols;
        FillRectPx(offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, cellSize - 4, cellSize - 4, COLOR_HEAD);
    }
    else if (state.state == 2 && !state.solveStack.empty()) {
        int current = state.solveStack.back();
        int cx = current % state.cols;
        int cy = current / state.cols;
        FillRectPx(offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, cellSize - 4, cellSize - 4, COLOR_HEAD);
    }

    // 5. Blast the final image to the screen in a single command
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height, 0, 0, width, height, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

} // namespace MazeLegacy

// Register as Screensaver ID 34
REGISTER_SCREENSAVER(
    34,
    L"Maze Generator (Legacy)",
    "maze-legacy",
    { "maze-legacy", "legacy-maze", "maze-old" },
    WRAP_LEGACY(MazeLegacy::RenderMazeLegacy),
    GetMazeSettings()
);
