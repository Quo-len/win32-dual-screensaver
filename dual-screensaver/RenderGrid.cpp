#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

void RenderGrid(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int padding = 40;
    int cellSize = g_TextSize * 2;
    if (cellSize < 10) cellSize = 10;

    int targetCols = (width - padding * 2) / cellSize;
    int targetRows = (height - padding * 2) / cellSize;

    if (targetCols <= 0) targetCols = 1;
    if (targetRows <= 0) targetRows = 1;

    if (data->hexCols != targetCols || data->hexRows != targetRows || data->hexGrid.empty()) {
        data->hexCols = targetCols;
        data->hexRows = targetRows;
        data->hexGrid.assign(targetCols * targetRows, "00");
        const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF" };
        for (int i = 0; i < targetCols * targetRows; i++) {
            data->hexGrid[i] = codes[rand() % 6];
        }
        data->activeRow = rand() % targetRows;
        data->activeCol = rand() % targetCols;
        data->isRowActive = true;
        data->lastHexUpdate = GetTickCount();
    }

    DWORD now = GetTickCount();
    if (now - data->lastHexUpdate > 1000) {
        if (data->isRowActive) {
            data->activeCol = rand() % data->hexCols;
            data->isRowActive = false;
        }
        else {
            data->activeRow = rand() % data->hexRows;
            data->isRowActive = true;
        }
        data->lastHexUpdate = now;
    }

    SelectObject(memDC, data->hFont);

    int startX = (width - (data->hexCols * cellSize)) / 2;
    int startY = (height - (data->hexRows * cellSize)) / 2;

    for (int r = 0; r < data->hexRows; r++) {
        for (int c = 0; c < data->hexCols; c++) {
            int idx = r * data->hexCols + c;
            COLORREF color = RGB(40, 200, 40);

            if (r == data->activeRow && c == data->activeCol) {
                color = RGB(0, 0, 0);
                SetBkMode(memDC, OPAQUE);
                SetBkColor(memDC, RGB(255, 0, 85));
            }
            else if ((data->isRowActive && r == data->activeRow) || (!data->isRowActive && c == data->activeCol)) {
                color = RGB(250, 250, 50);
                SetBkMode(memDC, TRANSPARENT);
            }
            else {
                SetBkMode(memDC, TRANSPARENT);
            }

            if (rand() % 1000 > 985) {
                const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF", "4B", "00" };
                data->hexGrid[idx] = codes[rand() % 8];
            }

            SetTextColor(memDC, color);
            TextOutA(memDC, startX + c * cellSize, startY + r * cellSize, data->hexGrid[idx].c_str(), 2);
        }
    }
}
