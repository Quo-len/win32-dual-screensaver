#include "framework.h"
#include "Renderers.h"
#include "Settings.h"
#include <vector>

void RenderMaze(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int cellSize = max(10, g_TextSize);
    int padding = 40;
    int mCols = (width - padding * 2) / cellSize;
    int mRows = (height - padding * 2) / cellSize;

    if (mCols <= 2) mCols = 3;
    if (mRows <= 2) mRows = 3;

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
        data->lastMazeUpdate = GetTickCount();
        data->mazeWaitTimer = 0;
    }

    DWORD now = GetTickCount();
    int stepsPerFrame = max(1, (int)(data->mazeState == 1 ? g_MazeBuildSpeed : g_MazeSolveSpeed));

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

    HPEN hWallPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
    HPEN hOldPen = (HPEN)SelectObject(memDC, hWallPen);
    HBRUSH hPathBrush = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH hSolveBrush = CreateSolidBrush(RGB(50, 50, 150));
    HBRUSH hHeadBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH hStartBrush = CreateSolidBrush(RGB(255, 255, 0));
    HBRUSH hEndBrush = CreateSolidBrush(RGB(0, 255, 255));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));

    for (int cy = 0; cy < data->mazeRows; cy++) {
        for (int cx = 0; cx < data->mazeCols; cx++) {
            int i = cy * data->mazeCols + cx;
            int px = offsetX + cx * cellSize;
            int py = offsetY + cy * cellSize;

            if (data->mazeGrid[i].inPath) {
                RECT rc = { px + cellSize / 4, py + cellSize / 4, px + cellSize - cellSize / 4, py + cellSize - cellSize / 4 };
                FillRect(memDC, &rc, hPathBrush);
            }
            else if (data->mazeGrid[i].solveVisited) {
                RECT rc = { px + cellSize / 3, py + cellSize / 3, px + cellSize - cellSize / 3, py + cellSize - cellSize / 3 };
                FillRect(memDC, &rc, hSolveBrush);
            }

            if (data->mazeGrid[i].visited) {
                if (data->mazeGrid[i].wallTop) { MoveToEx(memDC, px, py, NULL); LineTo(memDC, px + cellSize, py); }
                if (data->mazeGrid[i].wallBottom) { MoveToEx(memDC, px, py + cellSize, NULL); LineTo(memDC, px + cellSize, py + cellSize); }
                if (data->mazeGrid[i].wallLeft) { MoveToEx(memDC, px, py, NULL); LineTo(memDC, px, py + cellSize); }
                if (data->mazeGrid[i].wallRight) { MoveToEx(memDC, px + cellSize, py, NULL); LineTo(memDC, px + cellSize, py + cellSize); }
            }
        }
    }

    if (data->mazeState > 0) {
        int sx = (data->mazeStart % data->mazeCols) * cellSize + offsetX;
        int sy = (data->mazeStart / data->mazeCols) * cellSize + offsetY;
        RECT sr = { sx + 2, sy + 2, sx + cellSize - 2, sy + cellSize - 2 };
        FillRect(memDC, &sr, hStartBrush);

        int ex = (data->mazeEnd % data->mazeCols) * cellSize + offsetX;
        int ey = (data->mazeEnd / data->mazeCols) * cellSize + offsetY;
        RECT er = { ex + 2, ey + 2, ex + cellSize - 2, ey + cellSize - 2 };
        FillRect(memDC, &er, hEndBrush);
    }

    if (data->mazeState == 1 && !data->mazeStack.empty()) {
        int current = data->mazeStack.back();
        int cx = current % data->mazeCols;
        int cy = current / data->mazeCols;
        RECT rc = { offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, offsetX + (cx + 1) * cellSize - 2, offsetY + (cy + 1) * cellSize - 2 };
        FillRect(memDC, &rc, hHeadBrush);
    }
    else if (data->mazeState == 2 && !data->solveStack.empty()) {
        int current = data->solveStack.back();
        int cx = current % data->mazeCols;
        int cy = current / data->mazeCols;
        RECT rc = { offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, offsetX + (cx + 1) * cellSize - 2, offsetY + (cy + 1) * cellSize - 2 };
        FillRect(memDC, &rc, hHeadBrush);
    }

    SelectObject(memDC, hOldPen);
    SelectObject(memDC, hOldBrush);
    DeleteObject(hWallPen);
    DeleteObject(hPathBrush);
    DeleteObject(hSolveBrush);
    DeleteObject(hHeadBrush);
    DeleteObject(hStartBrush);
    DeleteObject(hEndBrush);
}
