#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"

// add to settings ability to enter number of items and sorting speed, and maybe even specific algorithm to use (or exclude certain ones
struct SortState {
    std::vector<int> sortArray;
    int sortState = 0;
    int sortAlgo = 0;
    int sortI = 0, sortJ = 0, sortMin = 0;
    bool sortFlag = false;
    int sortSweepIdx = 0;
    int sortWait = 0;
    int sortRed1 = -1, sortRed2 = -1;
    char sortAlgoName[32] = { 0 };
    int sortSubState = 0;
    std::vector<int> sortStack;
    std::vector<int> sortOutput;
    int sortGap = 0;
    int sortCurrSize = 1;
    int sortLeftStart = 0;
    int sortExp = 1;
    long long sortComparisons = 0;
    long long sortSwaps = 0;
};

void RenderRandomSort(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<SortState>(15);
    int numItems = 100;

    if (state.sortArray.empty() || state.sortState == 0) {
        state.sortArray.clear();
        for (int i = 1; i <= numItems; i++) state.sortArray.push_back(i);
        for (int i = numItems - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(state.sortArray[i], state.sortArray[j]);
        }
        state.sortState = 1;
        state.sortAlgo = rand() % 9;
        state.sortI = 0; state.sortJ = 0; state.sortMin = 0; state.sortFlag = false;
        state.sortRed1 = -1; state.sortRed2 = -1;
        state.sortSubState = 0; state.sortStack.clear(); state.sortOutput.clear();
        state.sortComparisons = 0; state.sortSwaps = 0;

        if (state.sortAlgo == 0) strcpy_s(state.sortAlgoName, "Bubble Sort");
        else if (state.sortAlgo == 1) { strcpy_s(state.sortAlgoName, "Selection Sort"); state.sortJ = 1; }
        else if (state.sortAlgo == 2) { strcpy_s(state.sortAlgoName, "Insertion Sort"); state.sortI = 1; state.sortJ = 1; }
        else if (state.sortAlgo == 3) { strcpy_s(state.sortAlgoName, "Odd-Even Sort"); }
        else if (state.sortAlgo == 4) { strcpy_s(state.sortAlgoName, "Shell Sort"); state.sortGap = numItems / 2; state.sortI = state.sortGap; state.sortJ = state.sortGap; }
        else if (state.sortAlgo == 5) { strcpy_s(state.sortAlgoName, "Quick Sort"); state.sortStack.push_back(0); state.sortStack.push_back(numItems - 1); }
        else if (state.sortAlgo == 6) { strcpy_s(state.sortAlgoName, "Merge Sort (In-Place)"); state.sortCurrSize = 1; state.sortLeftStart = 0; }
        else if (state.sortAlgo == 7) { strcpy_s(state.sortAlgoName, "Heap Sort"); state.sortI = numItems / 2 - 1; }
        else if (state.sortAlgo == 8) { strcpy_s(state.sortAlgoName, "Radix Sort (LSD)"); state.sortExp = 1; state.sortMin = numItems; }
    }

    int stepsPerFrame = (state.sortAlgo == 2 || state.sortAlgo == 6) ? 1 : 4;

    if (state.sortState == 1) {
        for (int step = 0; step < stepsPerFrame && state.sortState == 1; step++) {
            state.sortRed1 = -1; state.sortRed2 = -1;

            if (state.sortAlgo == 0) {
                if (state.sortI < numItems - 1) {
                    if (state.sortJ < numItems - state.sortI - 1) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = state.sortJ + 1;
                        state.sortComparisons++;
                        if (state.sortArray[state.sortJ] > state.sortArray[state.sortJ + 1]) {
                            std::swap(state.sortArray[state.sortJ], state.sortArray[state.sortJ + 1]);
                            state.sortSwaps++;
                        }
                        state.sortJ++;
                    }
                    else { state.sortJ = 0; state.sortI++; }
                }
                else { state.sortState = 2; state.sortSweepIdx = 0; }
            }
            else if (state.sortAlgo == 1) {
                if (state.sortI < numItems - 1) {
                    if (state.sortJ < numItems) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = state.sortMin;
                        state.sortComparisons++;
                        if (state.sortArray[state.sortJ] < state.sortArray[state.sortMin]) state.sortMin = state.sortJ;
                        state.sortJ++;
                    }
                    else {
                        std::swap(state.sortArray[state.sortI], state.sortArray[state.sortMin]);
                        state.sortSwaps++;
                        state.sortI++; state.sortMin = state.sortI; state.sortJ = state.sortI + 1;
                    }
                }
                else { state.sortState = 2; state.sortSweepIdx = 0; }
            }
            else if (state.sortAlgo == 2) {
                if (state.sortI < numItems) {
                    state.sortComparisons++;
                    if (state.sortJ > 0 && state.sortArray[state.sortJ - 1] > state.sortArray[state.sortJ]) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = state.sortJ - 1;
                        std::swap(state.sortArray[state.sortJ], state.sortArray[state.sortJ - 1]);
                        state.sortSwaps++;
                        state.sortJ--;
                    }
                    else { state.sortI++; state.sortJ = state.sortI; }
                }
                else { state.sortState = 2; state.sortSweepIdx = 0; }
            }
            else if (state.sortAlgo == 3) {
                if (state.sortI == 0 || state.sortI == 1) {
                    if (state.sortJ < numItems - 1) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = state.sortJ + 1;
                        state.sortComparisons++;
                        if (state.sortArray[state.sortJ] > state.sortArray[state.sortJ + 1]) {
                            std::swap(state.sortArray[state.sortJ], state.sortArray[state.sortJ + 1]);
                            state.sortSwaps++;
                            state.sortFlag = true;
                        }
                        state.sortJ += 2;
                    }
                    else {
                        if (state.sortI == 0) { state.sortI = 1; state.sortJ = 1; }
                        else {
                            if (!state.sortFlag) { state.sortState = 2; state.sortSweepIdx = 0; }
                            else { state.sortI = 0; state.sortJ = 0; state.sortFlag = false; }
                        }
                    }
                }
            }
            else if (state.sortAlgo == 4) {
                if (state.sortGap == 0) { state.sortState = 2; state.sortSweepIdx = 0; break; }
                if (state.sortI < numItems) {
                    state.sortComparisons++;
                    if (state.sortJ >= state.sortGap && state.sortArray[state.sortJ - state.sortGap] > state.sortArray[state.sortJ]) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = state.sortJ - state.sortGap;
                        std::swap(state.sortArray[state.sortJ], state.sortArray[state.sortJ - state.sortGap]);
                        state.sortSwaps++;
                        state.sortJ -= state.sortGap;
                    }
                    else { state.sortI++; state.sortJ = state.sortI; }
                }
                else {
                    state.sortGap /= 2; state.sortI = state.sortGap; state.sortJ = state.sortGap;
                }
            }
            else if (state.sortAlgo == 5) {
                if (state.sortSubState == 0) {
                    if (state.sortStack.empty()) { state.sortState = 2; state.sortSweepIdx = 0; break; }
                    int h = state.sortStack.back(); state.sortStack.pop_back();
                    int l = state.sortStack.back(); state.sortStack.pop_back();
                    state.sortMin = state.sortArray[h];
                    state.sortI = l - 1; state.sortJ = l;
                    state.sortStack.push_back(l); state.sortStack.push_back(h);
                    state.sortSubState = 1;
                }
                else if (state.sortSubState == 1) {
                    int h = state.sortStack.back();
                    if (state.sortJ < h) {
                        state.sortRed1 = state.sortJ; state.sortRed2 = h;
                        state.sortComparisons++;
                        if (state.sortArray[state.sortJ] < state.sortMin) {
                            state.sortI++;
                            std::swap(state.sortArray[state.sortI], state.sortArray[state.sortJ]);
                            state.sortSwaps++;
                            state.sortRed1 = state.sortI;
                        }
                        state.sortJ++;
                    }
                    else { state.sortSubState = 2; }
                }
                else if (state.sortSubState == 2) {
                    int h = state.sortStack.back(); state.sortStack.pop_back();
                    int l = state.sortStack.back(); state.sortStack.pop_back();
                    state.sortI++;
                    std::swap(state.sortArray[state.sortI], state.sortArray[h]);
                    state.sortSwaps++;
                    state.sortRed1 = state.sortI; state.sortRed2 = h;
                    int p = state.sortI;
                    if (p - 1 > l) { state.sortStack.push_back(l); state.sortStack.push_back(p - 1); }
                    if (p + 1 < h) { state.sortStack.push_back(p + 1); state.sortStack.push_back(h); }
                    state.sortSubState = 0;
                }
            }
            else if (state.sortAlgo == 6) {
                if (state.sortCurrSize >= numItems) { state.sortState = 2; state.sortSweepIdx = 0; break; }
                if (state.sortSubState == 0) {
                    if (state.sortLeftStart < numItems - 1) {
                        int mid = state.sortLeftStart + state.sortCurrSize - 1;
                        if (mid >= numItems - 1) mid = numItems - 1;
                        state.sortI = state.sortLeftStart; state.sortJ = mid + 1; state.sortMin = mid;
                        state.sortSubState = 1;
                    }
                    else {
                        state.sortCurrSize *= 2; state.sortLeftStart = 0;
                    }
                }
                else if (state.sortSubState == 1) {
                    int rightEnd = state.sortLeftStart + 2 * state.sortCurrSize - 1;
                    if (rightEnd >= numItems - 1) rightEnd = numItems - 1;
                    if (state.sortI <= state.sortMin && state.sortJ <= rightEnd) {
                        state.sortRed1 = state.sortI; state.sortRed2 = state.sortJ;
                        state.sortComparisons++;
                        if (state.sortArray[state.sortI] <= state.sortArray[state.sortJ]) {
                            state.sortI++;
                        }
                        else {
                            int val = state.sortArray[state.sortJ];
                            for (int k = state.sortJ; k > state.sortI; k--) {
                                state.sortArray[k] = state.sortArray[k - 1];
                                state.sortSwaps++;
                            }
                            state.sortArray[state.sortI] = val;
                            state.sortSwaps++;
                            state.sortI++; state.sortMin++; state.sortJ++;
                        }
                    }
                    else {
                        state.sortLeftStart += 2 * state.sortCurrSize;
                        state.sortSubState = 0;
                    }
                }
            }
            else if (state.sortAlgo == 7) {
                if (state.sortSubState == 0) {
                    if (state.sortI >= 0) { state.sortJ = state.sortI; state.sortSubState = 1; }
                    else { state.sortI = numItems - 1; state.sortSubState = 2; }
                }
                else if (state.sortSubState == 1 || state.sortSubState == 3) {
                    int largest = state.sortJ, l = 2 * state.sortJ + 1, r = 2 * state.sortJ + 2;
                    int limit = (state.sortSubState == 1) ? numItems : state.sortI;

                    state.sortComparisons++;
                    if (l < limit && state.sortArray[l] > state.sortArray[largest]) largest = l;
                    state.sortComparisons++;
                    if (r < limit && state.sortArray[r] > state.sortArray[largest]) largest = r;

                    state.sortRed1 = state.sortJ; state.sortRed2 = largest;
                    if (largest != state.sortJ) {
                        std::swap(state.sortArray[state.sortJ], state.sortArray[largest]);
                        state.sortSwaps++;
                        state.sortJ = largest;
                    }
                    else {
                        state.sortI--;
                        state.sortSubState = (state.sortSubState == 1) ? 0 : 2;
                    }
                }
                else if (state.sortSubState == 2) {
                    if (state.sortI > 0) {
                        state.sortRed1 = 0; state.sortRed2 = state.sortI;
                        std::swap(state.sortArray[0], state.sortArray[state.sortI]);
                        state.sortSwaps++;
                        state.sortJ = 0; state.sortSubState = 3;
                    }
                    else { state.sortState = 2; state.sortSweepIdx = 0; }
                }
            }
            else if (state.sortAlgo == 8) {
                if (state.sortMin / state.sortExp <= 0) { state.sortState = 2; state.sortSweepIdx = 0; break; }
                if (state.sortSubState == 0) {
                    std::vector<int> count(10, 0);
                    for (int i = 0; i < numItems; i++) count[(state.sortArray[i] / state.sortExp) % 10]++;
                    for (int i = 1; i < 10; i++) count[i] += count[i - 1];
                    state.sortStack = count;
                    state.sortOutput.assign(numItems, 0);
                    state.sortI = numItems - 1;
                    state.sortSubState = 1;
                }
                else if (state.sortSubState == 1) {
                    if (state.sortI >= 0) {
                        int idx = (state.sortArray[state.sortI] / state.sortExp) % 10;
                        state.sortStack[idx]--;
                        state.sortOutput[state.sortStack[idx]] = state.sortArray[state.sortI];
                        state.sortSwaps++;
                        state.sortRed1 = state.sortI; state.sortI--;
                    }
                    else { state.sortI = 0; state.sortSubState = 2; }
                }
                else if (state.sortSubState == 2) {
                    if (state.sortI < numItems) {
                        state.sortArray[state.sortI] = state.sortOutput[state.sortI];
                        state.sortSwaps++;
                        state.sortRed1 = state.sortI; state.sortI++;
                    }
                    else { state.sortExp *= 10; state.sortSubState = 0; }
                }
            }
        }
    }
    else if (state.sortState == 2) {
        state.sortRed1 = -1; state.sortRed2 = -1;
        state.sortSweepIdx += 2;
        if (state.sortSweepIdx >= numItems) {
            state.sortState = 3;
            state.sortWait = 0;
        }
    }
    else if (state.sortState == 3) {
        state.sortWait++;
        if (state.sortWait > 60) state.sortState = 0;
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
        if (state.sortState == 1 && (i == state.sortRed1 || i == state.sortRed2)) brush = redBrush;
        else if (state.sortState == 2 && i <= state.sortSweepIdx) brush = greenBrush;
        else if (state.sortState == 3) brush = greenBrush;

        int barH = (state.sortArray[i] * maxBarHeight) / numItems;

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
        state.sortAlgoName,
        (state.sortState >= 2) ? "SORTED!" : "Sorting...",
        state.sortSwaps,
        state.sortComparisons);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    TextOutA(memDC, (width - (int)strlen(txt) * tm.tmAveCharWidth) / 2, 30, txt, (int)strlen(txt));
}

REGISTER_SCREENSAVER(
    15,
    L"Sorting Algorithms",
    "sort",
    { "sort", "sorting" },
    WRAP_LEGACY(RenderRandomSort),
    {}
);