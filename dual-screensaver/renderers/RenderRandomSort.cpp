#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

// add to settings ability to enter number of items and sorting speed, and maybe even specific algorithm to use (or exclude certain ones
void RenderRandomSort(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int numItems = 100;

    if (data->sortArray.empty() || data->sortState == 0) {
        data->sortArray.clear();
        for (int i = 1; i <= numItems; i++) data->sortArray.push_back(i);
        for (int i = numItems - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(data->sortArray[i], data->sortArray[j]);
        }
        data->sortState = 1;
        data->sortAlgo = rand() % 9;
        data->sortI = 0; data->sortJ = 0; data->sortMin = 0; data->sortFlag = false;
        data->sortRed1 = -1; data->sortRed2 = -1;
        data->sortSubState = 0; data->sortStack.clear(); data->sortOutput.clear();
        data->sortComparisons = 0; data->sortSwaps = 0;

        if (data->sortAlgo == 0) strcpy_s(data->sortAlgoName, "Bubble Sort");
        else if (data->sortAlgo == 1) { strcpy_s(data->sortAlgoName, "Selection Sort"); data->sortJ = 1; }
        else if (data->sortAlgo == 2) { strcpy_s(data->sortAlgoName, "Insertion Sort"); data->sortI = 1; data->sortJ = 1; }
        else if (data->sortAlgo == 3) { strcpy_s(data->sortAlgoName, "Odd-Even Sort"); }
        else if (data->sortAlgo == 4) { strcpy_s(data->sortAlgoName, "Shell Sort"); data->sortGap = numItems / 2; data->sortI = data->sortGap; data->sortJ = data->sortGap; }
        else if (data->sortAlgo == 5) { strcpy_s(data->sortAlgoName, "Quick Sort"); data->sortStack.push_back(0); data->sortStack.push_back(numItems - 1); }
        else if (data->sortAlgo == 6) { strcpy_s(data->sortAlgoName, "Merge Sort (In-Place)"); data->sortCurrSize = 1; data->sortLeftStart = 0; }
        else if (data->sortAlgo == 7) { strcpy_s(data->sortAlgoName, "Heap Sort"); data->sortI = numItems / 2 - 1; }
        else if (data->sortAlgo == 8) { strcpy_s(data->sortAlgoName, "Radix Sort (LSD)"); data->sortExp = 1; data->sortMin = numItems; }
    }

    int stepsPerFrame = (data->sortAlgo == 2 || data->sortAlgo == 6) ? 1 : 4;

    if (data->sortState == 1) {
        for (int step = 0; step < stepsPerFrame && data->sortState == 1; step++) {
            data->sortRed1 = -1; data->sortRed2 = -1;

            if (data->sortAlgo == 0) {
                if (data->sortI < numItems - 1) {
                    if (data->sortJ < numItems - data->sortI - 1) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = data->sortJ + 1;
                        data->sortComparisons++;
                        if (data->sortArray[data->sortJ] > data->sortArray[data->sortJ + 1]) {
                            std::swap(data->sortArray[data->sortJ], data->sortArray[data->sortJ + 1]);
                            data->sortSwaps++;
                        }
                        data->sortJ++;
                    }
                    else { data->sortJ = 0; data->sortI++; }
                }
                else { data->sortState = 2; data->sortSweepIdx = 0; }
            }
            else if (data->sortAlgo == 1) {
                if (data->sortI < numItems - 1) {
                    if (data->sortJ < numItems) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = data->sortMin;
                        data->sortComparisons++;
                        if (data->sortArray[data->sortJ] < data->sortArray[data->sortMin]) data->sortMin = data->sortJ;
                        data->sortJ++;
                    }
                    else {
                        std::swap(data->sortArray[data->sortI], data->sortArray[data->sortMin]);
                        data->sortSwaps++;
                        data->sortI++; data->sortMin = data->sortI; data->sortJ = data->sortI + 1;
                    }
                }
                else { data->sortState = 2; data->sortSweepIdx = 0; }
            }
            else if (data->sortAlgo == 2) {
                if (data->sortI < numItems) {
                    data->sortComparisons++;
                    if (data->sortJ > 0 && data->sortArray[data->sortJ - 1] > data->sortArray[data->sortJ]) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = data->sortJ - 1;
                        std::swap(data->sortArray[data->sortJ], data->sortArray[data->sortJ - 1]);
                        data->sortSwaps++;
                        data->sortJ--;
                    }
                    else { data->sortI++; data->sortJ = data->sortI; }
                }
                else { data->sortState = 2; data->sortSweepIdx = 0; }
            }
            else if (data->sortAlgo == 3) {
                if (data->sortI == 0 || data->sortI == 1) {
                    if (data->sortJ < numItems - 1) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = data->sortJ + 1;
                        data->sortComparisons++;
                        if (data->sortArray[data->sortJ] > data->sortArray[data->sortJ + 1]) {
                            std::swap(data->sortArray[data->sortJ], data->sortArray[data->sortJ + 1]);
                            data->sortSwaps++;
                            data->sortFlag = true;
                        }
                        data->sortJ += 2;
                    }
                    else {
                        if (data->sortI == 0) { data->sortI = 1; data->sortJ = 1; }
                        else {
                            if (!data->sortFlag) { data->sortState = 2; data->sortSweepIdx = 0; }
                            else { data->sortI = 0; data->sortJ = 0; data->sortFlag = false; }
                        }
                    }
                }
            }
            else if (data->sortAlgo == 4) {
                if (data->sortGap == 0) { data->sortState = 2; data->sortSweepIdx = 0; break; }
                if (data->sortI < numItems) {
                    data->sortComparisons++;
                    if (data->sortJ >= data->sortGap && data->sortArray[data->sortJ - data->sortGap] > data->sortArray[data->sortJ]) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = data->sortJ - data->sortGap;
                        std::swap(data->sortArray[data->sortJ], data->sortArray[data->sortJ - data->sortGap]);
                        data->sortSwaps++;
                        data->sortJ -= data->sortGap;
                    }
                    else { data->sortI++; data->sortJ = data->sortI; }
                }
                else {
                    data->sortGap /= 2; data->sortI = data->sortGap; data->sortJ = data->sortGap;
                }
            }
            else if (data->sortAlgo == 5) {
                if (data->sortSubState == 0) {
                    if (data->sortStack.empty()) { data->sortState = 2; data->sortSweepIdx = 0; break; }
                    int h = data->sortStack.back(); data->sortStack.pop_back();
                    int l = data->sortStack.back(); data->sortStack.pop_back();
                    data->sortMin = data->sortArray[h];
                    data->sortI = l - 1; data->sortJ = l;
                    data->sortStack.push_back(l); data->sortStack.push_back(h);
                    data->sortSubState = 1;
                }
                else if (data->sortSubState == 1) {
                    int h = data->sortStack.back();
                    if (data->sortJ < h) {
                        data->sortRed1 = data->sortJ; data->sortRed2 = h;
                        data->sortComparisons++;
                        if (data->sortArray[data->sortJ] < data->sortMin) {
                            data->sortI++;
                            std::swap(data->sortArray[data->sortI], data->sortArray[data->sortJ]);
                            data->sortSwaps++;
                            data->sortRed1 = data->sortI;
                        }
                        data->sortJ++;
                    }
                    else { data->sortSubState = 2; }
                }
                else if (data->sortSubState == 2) {
                    int h = data->sortStack.back(); data->sortStack.pop_back();
                    int l = data->sortStack.back(); data->sortStack.pop_back();
                    data->sortI++;
                    std::swap(data->sortArray[data->sortI], data->sortArray[h]);
                    data->sortSwaps++;
                    data->sortRed1 = data->sortI; data->sortRed2 = h;
                    int p = data->sortI;
                    if (p - 1 > l) { data->sortStack.push_back(l); data->sortStack.push_back(p - 1); }
                    if (p + 1 < h) { data->sortStack.push_back(p + 1); data->sortStack.push_back(h); }
                    data->sortSubState = 0;
                }
            }
            else if (data->sortAlgo == 6) {
                if (data->sortCurrSize >= numItems) { data->sortState = 2; data->sortSweepIdx = 0; break; }
                if (data->sortSubState == 0) {
                    if (data->sortLeftStart < numItems - 1) {
                        int mid = data->sortLeftStart + data->sortCurrSize - 1;
                        if (mid >= numItems - 1) mid = numItems - 1;
                        data->sortI = data->sortLeftStart; data->sortJ = mid + 1; data->sortMin = mid;
                        data->sortSubState = 1;
                    }
                    else {
                        data->sortCurrSize *= 2; data->sortLeftStart = 0;
                    }
                }
                else if (data->sortSubState == 1) {
                    int rightEnd = data->sortLeftStart + 2 * data->sortCurrSize - 1;
                    if (rightEnd >= numItems - 1) rightEnd = numItems - 1;
                    if (data->sortI <= data->sortMin && data->sortJ <= rightEnd) {
                        data->sortRed1 = data->sortI; data->sortRed2 = data->sortJ;
                        data->sortComparisons++;
                        if (data->sortArray[data->sortI] <= data->sortArray[data->sortJ]) {
                            data->sortI++;
                        }
                        else {
                            int val = data->sortArray[data->sortJ];
                            for (int k = data->sortJ; k > data->sortI; k--) {
                                data->sortArray[k] = data->sortArray[k - 1];
                                data->sortSwaps++;
                            }
                            data->sortArray[data->sortI] = val;
                            data->sortSwaps++;
                            data->sortI++; data->sortMin++; data->sortJ++;
                        }
                    }
                    else {
                        data->sortLeftStart += 2 * data->sortCurrSize;
                        data->sortSubState = 0;
                    }
                }
            }
            else if (data->sortAlgo == 7) {
                if (data->sortSubState == 0) {
                    if (data->sortI >= 0) { data->sortJ = data->sortI; data->sortSubState = 1; }
                    else { data->sortI = numItems - 1; data->sortSubState = 2; }
                }
                else if (data->sortSubState == 1 || data->sortSubState == 3) {
                    int largest = data->sortJ, l = 2 * data->sortJ + 1, r = 2 * data->sortJ + 2;
                    int limit = (data->sortSubState == 1) ? numItems : data->sortI;

                    data->sortComparisons++;
                    if (l < limit && data->sortArray[l] > data->sortArray[largest]) largest = l;
                    data->sortComparisons++;
                    if (r < limit && data->sortArray[r] > data->sortArray[largest]) largest = r;

                    data->sortRed1 = data->sortJ; data->sortRed2 = largest;
                    if (largest != data->sortJ) {
                        std::swap(data->sortArray[data->sortJ], data->sortArray[largest]);
                        data->sortSwaps++;
                        data->sortJ = largest;
                    }
                    else {
                        data->sortI--;
                        data->sortSubState = (data->sortSubState == 1) ? 0 : 2;
                    }
                }
                else if (data->sortSubState == 2) {
                    if (data->sortI > 0) {
                        data->sortRed1 = 0; data->sortRed2 = data->sortI;
                        std::swap(data->sortArray[0], data->sortArray[data->sortI]);
                        data->sortSwaps++;
                        data->sortJ = 0; data->sortSubState = 3;
                    }
                    else { data->sortState = 2; data->sortSweepIdx = 0; }
                }
            }
            else if (data->sortAlgo == 8) {
                if (data->sortMin / data->sortExp <= 0) { data->sortState = 2; data->sortSweepIdx = 0; break; }
                if (data->sortSubState == 0) {
                    std::vector<int> count(10, 0);
                    for (int i = 0; i < numItems; i++) count[(data->sortArray[i] / data->sortExp) % 10]++;
                    for (int i = 1; i < 10; i++) count[i] += count[i - 1];
                    data->sortStack = count;
                    data->sortOutput.assign(numItems, 0);
                    data->sortI = numItems - 1;
                    data->sortSubState = 1;
                }
                else if (data->sortSubState == 1) {
                    if (data->sortI >= 0) {
                        int idx = (data->sortArray[data->sortI] / data->sortExp) % 10;
                        data->sortStack[idx]--;
                        data->sortOutput[data->sortStack[idx]] = data->sortArray[data->sortI];
                        data->sortSwaps++;
                        data->sortRed1 = data->sortI; data->sortI--;
                    }
                    else { data->sortI = 0; data->sortSubState = 2; }
                }
                else if (data->sortSubState == 2) {
                    if (data->sortI < numItems) {
                        data->sortArray[data->sortI] = data->sortOutput[data->sortI];
                        data->sortSwaps++;
                        data->sortRed1 = data->sortI; data->sortI++;
                    }
                    else { data->sortExp *= 10; data->sortSubState = 0; }
                }
            }
        }
    }
    else if (data->sortState == 2) {
        data->sortRed1 = -1; data->sortRed2 = -1;
        data->sortSweepIdx += 2;
        if (data->sortSweepIdx >= numItems) {
            data->sortState = 3;
            data->sortWait = 0;
        }
    }
    else if (data->sortState == 3) {
        data->sortWait++;
        if (data->sortWait > 60) data->sortState = 0;
    }

    int barWidth = width / numItems;
    if (barWidth < 1) barWidth = 1;
    int startX = (width - (numItems * barWidth)) / 2;
    if (startX < 0) startX = 0;

    double barWidthDouble = (double)width / numItems;
    int maxBarHeight = height - 150;

    HBRUSH defaultBrush = CreateSolidBrush(RGB(200, 200, 200));
    HBRUSH redBrush = CreateSolidBrush(RGB(255, 50, 50));
    HBRUSH greenBrush = CreateSolidBrush(RGB(50, 220, 50));

    for (int i = 0; i < numItems; i++) {
        HBRUSH brush = defaultBrush;
        if (data->sortState == 1 && (i == data->sortRed1 || i == data->sortRed2)) brush = redBrush;
        else if (data->sortState == 2 && i <= data->sortSweepIdx) brush = greenBrush;
        else if (data->sortState == 3) brush = greenBrush;

        int barH = (data->sortArray[i] * maxBarHeight) / numItems;

        int left = (int)(i * barWidthDouble);
        int right = (int)((i + 1) * barWidthDouble);

        if (right - left > 2) {
            right -= 1;
        }

        RECT r = { left, height - barH, right, height };
        FillRect(memDC, &r, brush);
    }

    DeleteObject(defaultBrush);
    DeleteObject(redBrush);
    DeleteObject(greenBrush);

    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(150, 150, 150));

    char txt[128];
    sprintf_s(txt, "Algorithm: %s | Status: %s | Swaps: %lld | Comparisons: %lld",
        data->sortAlgoName,
        (data->sortState >= 2) ? "SORTED!" : "Sorting...",
        data->sortSwaps,
        data->sortComparisons);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    TextOutA(memDC, (width - (int)strlen(txt) * tm.tmAveCharWidth) / 2, 30, txt, (int)strlen(txt));
}