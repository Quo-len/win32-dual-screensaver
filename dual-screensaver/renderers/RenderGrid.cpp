#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"

struct GridState {
    std::vector<std::string> hexGrid;
    int hexCols = 0;
    int hexRows = 0;
    int activeRow = 0;
    int activeCol = 0;
    bool isRowActive = true;
    DWORD lastHexUpdate = 0;
    HFONT hHexFont = nullptr;
    int hexLastWidth = 0;

    ~GridState() {
        if (hHexFont) DeleteObject(hHexFont);
    }
};

void RenderGrid(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<GridState>(8);

    if (state.hexLastWidth != width || state.hHexFont == NULL) {
        if (state.hHexFont) DeleteObject(state.hHexFont);

        int fontHeight = (width / 45) * 2;
        if (fontHeight < 12) fontHeight = 12;

        state.hHexFont = CreateFontA(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, "Consolas");

        state.hexLastWidth = width;
    }

    SelectObject(memDC, state.hHexFont);
    SetBkMode(memDC, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cWidth = tm.tmAveCharWidth;
    int cHeight = tm.tmHeight;

    int cellSizeX = cWidth * 4;
    int cellSizeY = (int)(cHeight * 1.5);
    int padding = 0;

    int targetCols = (width - padding * 2) / cellSizeX;
    int targetRows = (height - padding * 2) / cellSizeY;

    if (targetCols <= 0) targetCols = 1;
    if (targetRows <= 0) targetRows = 1;

    if (state.hexCols != targetCols || state.hexRows != targetRows || state.hexGrid.empty()) {
        state.hexCols = targetCols;
        state.hexRows = targetRows;
        state.hexGrid.assign(targetCols * targetRows, "00");
        const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF", "1A", "C3", "00", "8F", "42" };
        for (int i = 0; i < targetCols * targetRows; i++) {
            state.hexGrid[i] = codes[rand() % 11];
        }
        state.activeRow = rand() % targetRows;
        state.activeCol = rand() % targetCols;
        state.isRowActive = true;
        state.lastHexUpdate = (DWORD)GetTickCount64();
    }

    DWORD now = (DWORD)GetTickCount64();
    if (now - state.lastHexUpdate > 800) {
        if (state.isRowActive) {
            state.activeCol = rand() % state.hexCols;
            state.isRowActive = false;
        }
        else {
            state.activeRow = rand() % state.hexRows;
            state.isRowActive = true;
        }
        state.lastHexUpdate = now;
    }

    int gridW = state.hexCols * cellSizeX;
    int gridH = state.hexRows * cellSizeY;
    int startX = (width - gridW) / 2;
    int startY = (height - gridH) / 2;

    HPEN outerPen = CreatePen(PS_SOLID, 2, RGB(0, 180, 255));
    HPEN crossPen = CreatePen(PS_SOLID, 1, RGB(30, 60, 90));
    HPEN highlightPen = CreatePen(PS_SOLID, 1, RGB(255, 60, 60));
    HPEN oldPen = (HPEN)SelectObject(memDC, outerPen);

    int br = 40;
    int pad = 15;
    MoveToEx(memDC, startX - pad, startY - pad + br, NULL); LineTo(memDC, startX - pad, startY - pad); LineTo(memDC, startX - pad + br, startY - pad);
    MoveToEx(memDC, startX + gridW + pad - br, startY - pad, NULL); LineTo(memDC, startX + gridW + pad, startY - pad); LineTo(memDC, startX + gridW + pad, startY - pad + br);
    MoveToEx(memDC, startX - pad, startY + gridH + pad - br, NULL); LineTo(memDC, startX - pad, startY + gridH + pad); LineTo(memDC, startX - pad + br, startY + gridH + pad);
    MoveToEx(memDC, startX + gridW + pad - br, startY + gridH + pad, NULL); LineTo(memDC, startX + gridW + pad, startY + gridH + pad); LineTo(memDC, startX + gridW + pad, startY + gridH + pad - br);

    SelectObject(memDC, crossPen);
    for (int r = 0; r <= state.hexRows; r++) {
        for (int c = 0; c <= state.hexCols; c++) {
            int cx = startX + c * cellSizeX;
            int cy = startY + r * cellSizeY;
            MoveToEx(memDC, cx - 6, cy, NULL); LineTo(memDC, cx + 7, cy);
            MoveToEx(memDC, cx, cy - 6, NULL); LineTo(memDC, cx, cy + 7);
        }
    }

    HBRUSH rowBg = CreateSolidBrush(RGB(15, 15, 30));
    HBRUSH colBg = CreateSolidBrush(RGB(30, 10, 15));
    HBRUSH actBg = CreateSolidBrush(RGB(180, 255, 255));

    if (state.isRowActive) {
        RECT r = { startX, startY + state.activeRow * cellSizeY, startX + gridW, startY + (state.activeRow + 1) * cellSizeY };
        FillRect(memDC, &r, rowBg);
        SelectObject(memDC, highlightPen);
        MoveToEx(memDC, startX, startY + state.activeRow * cellSizeY, NULL); LineTo(memDC, startX + gridW, startY + state.activeRow * cellSizeY);
        MoveToEx(memDC, startX, startY + (state.activeRow + 1) * cellSizeY, NULL); LineTo(memDC, startX + gridW, startY + (state.activeRow + 1) * cellSizeY);
    }
    else {
        RECT r = { startX + state.activeCol * cellSizeX, startY, startX + (state.activeCol + 1) * cellSizeX, startY + gridH };
        FillRect(memDC, &r, colBg);
        SelectObject(memDC, highlightPen);
        // FIXED: Replaced state.hexCols with state.activeCol to draw a straight vertical line
        MoveToEx(memDC, startX + state.activeCol * cellSizeX, startY, NULL); LineTo(memDC, startX + state.activeCol * cellSizeX, startY + gridH);
        MoveToEx(memDC, startX + (state.activeCol + 1) * cellSizeX, startY, NULL); LineTo(memDC, startX + (state.activeCol + 1) * cellSizeX, startY + gridH);
    }

    for (int r = 0; r < state.hexRows; r++) {
        for (int c = 0; c < state.hexCols; c++) {
            int idx = r * state.hexCols + c;
            int cellX = startX + c * cellSizeX;
            int cellY = startY + r * cellSizeY;

            COLORREF textColor = RGB(0, 110, 190);
            bool isActiveCell = (r == state.activeRow && c == state.activeCol);
            bool isInTrack = ((state.isRowActive && r == state.activeRow) || (!state.isRowActive && c == state.activeCol));

            if (isActiveCell) {
                RECT cellR = { cellX + 2, cellY + 2, cellX + cellSizeX - 2, cellY + cellSizeY - 2 };
                FillRect(memDC, &cellR, actBg);
                textColor = RGB(0, 0, 0);

                SelectObject(memDC, highlightPen);
                MoveToEx(memDC, cellX + 5, cellY + 5, NULL); LineTo(memDC, cellX + 15, cellY + 5);
                MoveToEx(memDC, cellX + 5, cellY + 5, NULL); LineTo(memDC, cellX + 5, cellY + 15);
            }
            else if (isInTrack) {
                textColor = RGB(255, 90, 90);
            }
            else {
                if (rand() % 1000 > 990) textColor = RGB(200, 255, 255);
            }

            if (rand() % 1000 > 980) {
                const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF", "4B", "00", "A1", "C3" };
                state.hexGrid[idx] = codes[rand() % 10];
            }

            SetTextColor(memDC, textColor);

            int tX = cellX + (cellSizeX - (cWidth * 2)) / 2;
            int tY = cellY + (cellSizeY - cHeight) / 2;
            TextOutA(memDC, tX, tY, state.hexGrid[idx].c_str(), 2);
        }
    }

    SelectObject(memDC, oldPen);
    DeleteObject(outerPen);
    DeleteObject(crossPen);
    DeleteObject(highlightPen);
    DeleteObject(rowBg);
    DeleteObject(colBg);
    DeleteObject(actBg);
}

REGISTER_SCREENSAVER(
    8,
    L"Grid",
    "grid",
    { "grid" },
    WRAP_LEGACY(RenderGrid),
    {}
);