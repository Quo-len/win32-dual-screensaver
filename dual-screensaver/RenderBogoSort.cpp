#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

void RenderBogoSort(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int numItems = 100;

    if (data->bogoArray.empty() || data->sortState == 0) {
        data->bogoArray.clear();
        for (int i = 1; i <= numItems; i++) data->bogoArray.push_back(i);

        for (int i = numItems - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(data->bogoArray[i], data->bogoArray[j]);
        }

        data->sortState = 1;
        data->bogoAttempts = 0;
        data->bogoComparisons = 0;
        data->sortSweepIdx = 0;
    }

    if (data->sortState == 1) {
        bool sorted = true;
        for (size_t i = 1; i < data->bogoArray.size(); i++) {
            data->bogoComparisons++;
            if (data->bogoArray[i - 1] > data->bogoArray[i]) {
                sorted = false;
                break;
            }
        }

        if (sorted && data->bogoAttempts > 0) {
            data->sortState = 2;
            data->sortSweepIdx = 0;
        }
        else {
            for (int i = numItems - 1; i > 0; i--) {
                int j = rand() % (i + 1);
                std::swap(data->bogoArray[i], data->bogoArray[j]);
            }
            data->bogoAttempts++;
        }
    }
    else if (data->sortState == 2) {
        data->sortSweepIdx += 1;
        if (data->sortSweepIdx >= numItems) {
            data->sortState = 3;
            data->sortWait = 0;
        }
    }
    else if (data->sortState == 3) {
        data->sortWait++;
        if (data->sortWait > 90) data->sortState = 0;
    }

    int barWidth = width / numItems;
    if (barWidth < 1) barWidth = 1;
    int maxBarHeight = height - 150;
    int startX = (width - (numItems * barWidth)) / 2;
    if (startX < 0) startX = 0;

    HBRUSH defaultBrush = CreateSolidBrush(RGB(200, 200, 200));
    HBRUSH greenBrush = CreateSolidBrush(RGB(50, 220, 50));

    for (int i = 0; i < numItems; i++) {
        HBRUSH brush = defaultBrush;

        if (data->sortState == 2 && i <= data->sortSweepIdx) brush = greenBrush;
        else if (data->sortState == 3) brush = greenBrush;

        int barH = (data->bogoArray[i] * maxBarHeight) / numItems;
        RECT r = { startX + i * barWidth, height - barH, startX + i * barWidth + barWidth - (barWidth > 2 ? 1 : 0), height  };
        FillRect(memDC, &r, brush);
    }

    DeleteObject(defaultBrush);
    DeleteObject(greenBrush);

    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(150, 150, 150));

    char txt[128];
    sprintf_s(txt, "Algorithm: BogoSort | Status: %s | Shuffles: %d | Comparisons: %lld",
        (data->sortState >= 2) ? "SORTED!" : "Shuffling...",
        data->bogoAttempts,
        data->bogoComparisons);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    TextOutA(memDC, (width - (int)strlen(txt) * tm.tmAveCharWidth) / 2, 30, txt, (int)strlen(txt));
}