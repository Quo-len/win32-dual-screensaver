#include "framework.h"
#include "Renderers.h"
#include "Settings.h"
#include <vector>

void RenderMaze(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

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

    if (data->mazeCols != mCols || data->mazeRows != mRows || data->mazeGrid.empty() || data->mazeState == 0) {
        data->mazeCols = mCols;
        data->mazeRows = mRows;

        data->mazeGrid.clear();
        data->mazeGrid.assign(mCols * mRows, { false, true, true, true, true, false, false });
        data->mazeState = 1;
        data->mazeStack.clear();
        data->solveStack.clear();

        data->mazeStart = rand() % (mCols * mRows);
        data->mazeEnd = rand() % (mCols * mRows);
        while (data->mazeEnd == data->mazeStart)
            data->mazeEnd = rand() % (mCols * mRows);

        data->mazeStack.push_back(data->mazeStart);
        data->mazeGrid[data->mazeStart].visited = true;
        data->lastMazeUpdate = GetTickCount64();
        data->mazeWaitTimer = 0;
    }

    DWORD now = GetTickCount64();
    int stepsPerFrame = max(1, (int)(data->mazeState == 1 ? g_MazeBuildSpeed : g_MazeSolveSpeed));

    // 2. Original Maze Generation Logic (Unchanged)
    if (now - data->lastMazeUpdate > 16) {
        data->lastMazeUpdate = now;

        if (data->mazeState == 1) {
            for (int step = 0; step < stepsPerFrame * 5 && !data->mazeStack.empty(); ++step) {
                int current = data->mazeStack.back();
                int cx = current % data->mazeCols;
                int cy = current / data->mazeCols;

                std::vector<int> neighbors, dirs;
                if (cy > 0 && !data->mazeGrid[current - data->mazeCols].visited) { neighbors.push_back(current - data->mazeCols); dirs.push_back(0); }
                if (cx < data->mazeCols - 1 && !data->mazeGrid[current + 1].visited) { neighbors.push_back(current + 1); dirs.push_back(1); }
                if (cy < data->mazeRows - 1 && !data->mazeGrid[current + data->mazeCols].visited) { neighbors.push_back(current + data->mazeCols); dirs.push_back(2); }
                if (cx > 0 && !data->mazeGrid[current - 1].visited) { neighbors.push_back(current - 1); dirs.push_back(3); }

                if (!neighbors.empty()) {
                    int r = rand() % (int)neighbors.size();
                    int next = neighbors[r];
                    int dir = dirs[r];

                    if (dir == 0) { data->mazeGrid[current].wallTop = false; data->mazeGrid[next].wallBottom = false; }
                    else if (dir == 1) { data->mazeGrid[current].wallRight = false; data->mazeGrid[next].wallLeft = false; }
                    else if (dir == 2) { data->mazeGrid[current].wallBottom = false; data->mazeGrid[next].wallTop = false; }
                    else if (dir == 3) { data->mazeGrid[current].wallLeft = false; data->mazeGrid[next].wallRight = false; }

                    data->mazeGrid[next].visited = true;
                    data->mazeStack.push_back(next);
                }
                else {
                    data->mazeStack.pop_back();
                }
            }
            if (data->mazeStack.empty()) {
                data->mazeState = 2;
                data->solveStack.push_back(data->mazeStart);
                data->mazeGrid[data->mazeStart].solveVisited = true;
                data->mazeGrid[data->mazeStart].inPath = true;
            }
        }
        else if (data->mazeState == 2) {
            for (int step = 0; step < stepsPerFrame && !data->solveStack.empty(); ++step) {
                int current = data->solveStack.back();

                if (current == data->mazeEnd) {
                    data->mazeState = 3;
                    data->mazeWaitTimer = 45;
                    break;
                }

                int cx = current % data->mazeCols;
                int cy = current / data->mazeCols;

                std::vector<int> nextDirs;
                if (!data->mazeGrid[current].wallTop && !data->mazeGrid[current - data->mazeCols].solveVisited) nextDirs.push_back(current - data->mazeCols);
                if (!data->mazeGrid[current].wallRight && !data->mazeGrid[current + 1].solveVisited) nextDirs.push_back(current + 1);
                if (!data->mazeGrid[current].wallBottom && !data->mazeGrid[current + data->mazeCols].solveVisited) nextDirs.push_back(current + data->mazeCols);
                if (!data->mazeGrid[current].wallLeft && !data->mazeGrid[current - 1].solveVisited) nextDirs.push_back(current - 1);

                if (!nextDirs.empty()) {
                    int next = nextDirs[0];
                    data->mazeGrid[next].solveVisited = true;
                    data->mazeGrid[next].inPath = true;
                    data->solveStack.push_back(next);
                }
                else {
                    data->mazeGrid[current].inPath = false;
                    data->solveStack.pop_back();
                }
            }
        }
        else if (data->mazeState == 3) {
            data->mazeWaitTimer--;
            if (data->mazeWaitTimer <= 0)
                data->mazeState = 0;
        }
    }

    int offsetX = (width - (data->mazeCols * cellSize)) / 2;
    int offsetY = (height - (data->mazeRows * cellSize)) / 2;

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
    for (int cy = 0; cy < data->mazeRows; cy++) {
        for (int cx = 0; cx < data->mazeCols; cx++) {
            int i = cy * data->mazeCols + cx;
            int px = offsetX + cx * cellSize;
            int py = offsetY + cy * cellSize;

            if (data->mazeGrid[i].inPath) {
                FillRectPx(px + cellSize / 4, py + cellSize / 4, cellSize - (cellSize / 4) * 2, cellSize - (cellSize / 4) * 2, COLOR_PATH);
            }
            else if (data->mazeGrid[i].solveVisited) {
                FillRectPx(px + cellSize / 3, py + cellSize / 3, cellSize - (cellSize / 3) * 2, cellSize - (cellSize / 3) * 2, COLOR_SOLVE);
            }

            if (data->mazeGrid[i].visited) {
                if (data->mazeGrid[i].wallTop) FillRectPx(px, py, cellSize, 2, COLOR_WALL);
                if (data->mazeGrid[i].wallBottom) FillRectPx(px, py + cellSize - 2, cellSize, 2, COLOR_WALL);
                if (data->mazeGrid[i].wallLeft) FillRectPx(px, py, 2, cellSize, COLOR_WALL);
                if (data->mazeGrid[i].wallRight) FillRectPx(px + cellSize - 2, py, 2, cellSize, COLOR_WALL);
            }
        }
    }

    if (data->mazeState > 0) {
        int sx = (data->mazeStart % data->mazeCols) * cellSize + offsetX;
        int sy = (data->mazeStart / data->mazeCols) * cellSize + offsetY;
        FillRectPx(sx + 2, sy + 2, cellSize - 4, cellSize - 4, COLOR_START);

        int ex = (data->mazeEnd % data->mazeCols) * cellSize + offsetX;
        int ey = (data->mazeEnd / data->mazeCols) * cellSize + offsetY;
        FillRectPx(ex + 2, ey + 2, cellSize - 4, cellSize - 4, COLOR_END);
    }

    if (data->mazeState == 1 && !data->mazeStack.empty()) {
        int current = data->mazeStack.back();
        int cx = current % data->mazeCols;
        int cy = current / data->mazeCols;
        FillRectPx(offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, cellSize - 4, cellSize - 4, COLOR_HEAD);
    }
    else if (data->mazeState == 2 && !data->solveStack.empty()) {
        int current = data->solveStack.back();
        int cx = current % data->mazeCols;
        int cy = current / data->mazeCols;
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