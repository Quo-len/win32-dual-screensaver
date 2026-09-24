#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/SortSettings.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Retro Arcade Sorting Simulation & Algorithm Visualizer
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio (e.g. 455x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;

// 32-bit ARGB Retro Arcade Palette (Matching Tetris, Space Invaders, Snake)
static constexpr uint32_t COL_BLACK          = 0xFF050508;
static constexpr uint32_t COL_WHITE          = 0xFFFFFFFF;
static constexpr uint32_t COL_CYAN_NEON      = 0xFF00E5FF;
static constexpr uint32_t COL_CYAN_DARK      = 0xFF006064;
static constexpr uint32_t COL_BORDER_OUTER   = 0xFF1C2848;
static constexpr uint32_t COL_BORDER_INNER   = 0xFF00E5FF;
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744;
static constexpr uint32_t COL_CRIMSON_GLOW   = 0xFFFF5252;
static constexpr uint32_t COL_GOLD           = 0xFFFFD700;
static constexpr uint32_t COL_GREEN_LIME     = 0xFF00FF66;
static constexpr uint32_t COL_GREEN_EMERALD  = 0xFF00E676;
static constexpr uint32_t COL_GRAY_LIGHT     = 0xFFB0B0C0;
static constexpr uint32_t COL_GRID_LINE      = 0xFF0C1322;

// Tetris Signature 7-Color Palette for Beveled Blocks
static constexpr uint32_t TET_TIER_COLORS[7] = {
    0xFF1565C0, // 0: J-Piece Deep Blue
    0xFF8E24AA, // 1: T-Piece Purple
    0xFF00B0FF, // 2: I-Piece Electric Cyan
    0xFF00E676, // 3: S-Piece Neon Green
    0xFFFFD600, // 4: O-Piece Bright Yellow
    0xFFFF6D00, // 5: L-Piece Vivid Orange
    0xFFFF1744  // 6: Z-Piece Crimson Coral
};

// ============================================================================
// RETRO 5x7 ARCADE BITMAP FONT
// ============================================================================
static const uint8_t* GetGlyph(char c) {
    static const uint8_t GLYPH_BLANK[7] = { 0 };
    switch (c) {
    case '0': { static const uint8_t g[7] = { 0x1F, 0x11, 0x13, 0x15, 0x19, 0x11, 0x1F }; return g; }
    case '1': { static const uint8_t g[7] = { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
    case '2': { static const uint8_t g[7] = { 0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F }; return g; }
    case '3': { static const uint8_t g[7] = { 0x1F, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x1F }; return g; }
    case '4': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01 }; return g; }
    case '5': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F }; return g; }
    case '6': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F }; return g; }
    case '7': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }; return g; }
    case '8': { static const uint8_t g[7] = { 0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F }; return g; }
    case '9': { static const uint8_t g[7] = { 0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F }; return g; }
    case 'A': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
    case 'B': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }; return g; }
    case 'C': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F }; return g; }
    case 'D': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }; return g; }
    case 'E': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }; return g; }
    case 'F': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }; return g; }
    case 'G': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F }; return g; }
    case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
    case 'I': { static const uint8_t g[7] = { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
    case 'J': { static const uint8_t g[7] = { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E }; return g; }
    case 'K': { static const uint8_t g[7] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }; return g; }
    case 'L': { static const uint8_t g[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return g; }
    case 'M': { static const uint8_t g[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return g; }
    case 'N': { static const uint8_t g[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return g; }
    case 'O': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
    case 'P': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return g; }
    case 'Q': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }; return g; }
    case 'R': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }; return g; }
    case 'S': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }; return g; }
    case 'T': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case 'U': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
    case 'V': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }; return g; }
    case 'W': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 }; return g; }
    case 'X': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }; return g; }
    case 'Y': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case 'Z': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    case ':': { static const uint8_t g[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }; return g; }
    case '.': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C }; return g; }
    case '(': { static const uint8_t g[7] = { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 }; return g; }
    case ')': { static const uint8_t g[7] = { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 }; return g; }
    case '^': { static const uint8_t g[7] = { 0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00 }; return g; }
    case '|': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case '/': { static const uint8_t g[7] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 }; return g; }
    case '!': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }; return g; }
    default:  return GLYPH_BLANK;
    }
}

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================
struct SortSparkle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    uint32_t color;
};

struct SortState {
    bool initialized = false;
    uint64_t lastTick = 0;

    int virtualW = 455;
    int virtualH = BASE_ARCADE_H;
    std::vector<uint32_t> fb;

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

    // Arcade FX
    std::vector<SortSparkle> sparkles;
};

// ============================================================================
// FRAMEBUFFER DRAWING UTILITIES
// ============================================================================
static inline void PutPixel(SortState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static inline void DrawHLine(SortState& s, int x1, int x2, int y, uint32_t col) {
    if (y < 0 || y >= s.virtualH) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = (std::max)(0, x1);
    x2 = (std::min)(s.virtualW - 1, x2);
    for (int x = x1; x <= x2; ++x) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static inline void DrawVLine(SortState& s, int x, int y1, int y2, uint32_t col) {
    if (x < 0 || x >= s.virtualW) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = (std::max)(0, y1);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static void DrawText(SortState& s, int x, int y, const char* str, uint32_t color, int scale = 1) {
    int curX = x;
    while (*str) {
        if (*str != ' ') {
            const uint8_t* glyph = GetGlyph(*str);
            for (int r = 0; r < 7; ++r) {
                uint8_t rowBits = glyph[r];
                for (int c = 0; c < 5; ++c) {
                    if ((rowBits >> (4 - c)) & 1) {
                        for (int dy = 0; dy < scale; ++dy) {
                            for (int dx = 0; dx < scale; ++dx) {
                                PutPixel(s, curX + c * scale + dx, y + r * scale + dy, color);
                            }
                        }
                    }
                }
            }
        }
        curX += 6 * scale;
        str++;
    }
}

// Double-Line Neon Arcade Borders (Matching Tetris, Space Invaders, Snake, Pong)
static void DrawArcadeBorder(SortState& s) {
    int x1 = 10;
    int y1 = 26;
    int x2 = s.virtualW - 11;
    int y2 = 250;

    // Outer double-line border (deep metallic blue)
    DrawHLine(s, x1 + 2, x2 - 2, y1, COL_BORDER_OUTER);
    DrawHLine(s, x1 + 2, x2 - 2, y2, COL_BORDER_OUTER);
    DrawVLine(s, x1, y1 + 2, y2 - 2, COL_BORDER_OUTER);
    DrawVLine(s, x2, y1 + 2, y2 - 2, COL_BORDER_OUTER);
    PutPixel(s, x1 + 1, y1 + 1, COL_BORDER_OUTER);
    PutPixel(s, x2 - 1, y1 + 1, COL_BORDER_OUTER);
    PutPixel(s, x1 + 1, y2 - 1, COL_BORDER_OUTER);
    PutPixel(s, x2 - 1, y2 - 1, COL_BORDER_OUTER);

    // Inner bright neon border (electric cyan)
    int ix1 = x1 + 2;
    int iy1 = y1 + 2;
    int ix2 = x2 - 2;
    int iy2 = y2 - 2;
    DrawHLine(s, ix1 + 1, ix2 - 1, iy1, COL_BORDER_INNER);
    DrawHLine(s, ix1 + 1, ix2 - 1, iy2, COL_BORDER_INNER);
    DrawVLine(s, ix1, iy1 + 1, iy2 - 1, COL_BORDER_INNER);
    DrawVLine(s, ix2, iy1 + 1, iy2 - 1, COL_BORDER_INNER);


}

// Background Cyber Equalizer Grid
static void DrawCyberGrid(SortState& s) {
    int x1 = 14;
    int x2 = s.virtualW - 15;
    for (int y = 50; y <= 240; y += 25) {
        for (int x = x1; x <= x2; x += 6) {
            PutPixel(s, x, y, COL_GRID_LINE);
        }
    }
}

// Color mapper for Tetris 7-Tier Palette
static uint32_t GetTetrisTierColor(int val, int maxVal) {
    float t = (float)(val - 1) / (float)(std::max)(1, maxVal - 1);
    int tier = (int)(t * 7.0f);
    if (tier < 0) tier = 0;
    if (tier > 6) tier = 6;
    return TET_TIER_COLORS[tier];
}

static const char* GetAlgoComplexity(int algo) {
    switch (algo) {
    case 0: return "O(N^2) - BUBBLE PASS";
    case 1: return "O(N^2) - MIN SCAN";
    case 2: return "O(N^2) - SHIFT & INSERT";
    case 3: return "O(N^2) - PARALLEL BRICK";
    case 4: return "O(N LOG^2 N) - GAP INTERVALS";
    case 5: return "O(N LOG N) - PIVOT RECURSION";
    case 6: return "O(N LOG^2 N) - IN-PLACE MERGE";
    case 7: return "O(N LOG N) - MAX-HEAPIFY";
    case 8: return "O(N * K) - DIGIT BUCKETS";
    default: return "O(N LOG N)";
    }
}

// ============================================================================
// SOLID BEVELED 3D BLOCK BAR RENDERER (Tetris Mino Block Aesthetics)
// Draws each bar as ONE solid, continuous beveled block with tight hairline gaps
// ============================================================================
static void DrawSolidBlockBar(SortState& s, int x1, int x2, int yTop, int yBottom, uint32_t baseCol) {
    int w = x2 - x1 + 1;
    int h = yBottom - yTop + 1;
    if (w <= 0 || h <= 0) return;

    uint32_t r = (baseCol >> 16) & 0xFF;
    uint32_t g = (baseCol >> 8) & 0xFF;
    uint32_t b = baseCol & 0xFF;

    // Highlight (top & left) - 1.55x brightness
    uint32_t hiR = (std::min)(255, (int)(r * 1.55f + 35));
    uint32_t hiG = (std::min)(255, (int)(g * 1.55f + 35));
    uint32_t hiB = (std::min)(255, (int)(b * 1.55f + 35));
    uint32_t hiCol = 0xFF000000 | (hiR << 16) | (hiG << 8) | hiB;

    // Shadow (bottom & right) - 0.42x brightness
    uint32_t shR = (uint32_t)(r * 0.42f);
    uint32_t shG = (uint32_t)(g * 0.42f);
    uint32_t shB = (uint32_t)(b * 0.42f);
    uint32_t shCol = 0xFF000000 | (shR << 16) | (shG << 8) | shB;

    for (int y = yTop; y <= yBottom; ++y) {
        for (int x = x1; x <= x2; ++x) {
            uint32_t col = baseCol;
            if (y == yTop || (h >= 6 && y == yTop + 1)) {
                col = (y == yTop && w >= 3) ? COL_WHITE : hiCol; // Bright highlighted top cap
            } else if (x == x1) {
                col = hiCol; // Left beveled highlight
            } else if (x == x2) {
                col = shCol; // Right beveled shadow
            } else if (y == yBottom) {
                col = shCol; // Bottom beveled shadow
            }
            PutPixel(s, x, y, col);
        }
    }

    // Glossy corner highlight on top-left
    if (w >= 4 && h >= 4) {
        PutPixel(s, x1 + 1, yTop + 1, COL_WHITE);
    }
}

// ============================================================================
// SORTING SIMULATION STEP ENGINE
// ============================================================================
static void StepSortingSimulation(SortState& state) {
    int numItems = g_SortItemCount;
    if (numItems < 10) numItems = 10;
    if (numItems > 200) numItems = 200;

    if (state.sortArray.size() != (size_t)numItems || state.sortState == 0) {
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
        state.sparkles.clear();

        if (state.sortAlgo == 0) strcpy_s(state.sortAlgoName, "BUBBLE SORT");
        else if (state.sortAlgo == 1) { strcpy_s(state.sortAlgoName, "SELECTION SORT"); state.sortJ = 1; }
        else if (state.sortAlgo == 2) { strcpy_s(state.sortAlgoName, "INSERTION SORT"); state.sortI = 1; state.sortJ = 1; }
        else if (state.sortAlgo == 3) { strcpy_s(state.sortAlgoName, "ODD-EVEN SORT"); }
        else if (state.sortAlgo == 4) { strcpy_s(state.sortAlgoName, "SHELL SORT"); state.sortGap = numItems / 2; state.sortI = state.sortGap; state.sortJ = state.sortGap; }
        else if (state.sortAlgo == 5) { strcpy_s(state.sortAlgoName, "QUICK SORT"); state.sortStack.push_back(0); state.sortStack.push_back(numItems - 1); }
        else if (state.sortAlgo == 6) { strcpy_s(state.sortAlgoName, "MERGE SORT (IN-PLACE)"); state.sortCurrSize = 1; state.sortLeftStart = 0; }
        else if (state.sortAlgo == 7) { strcpy_s(state.sortAlgoName, "HEAP SORT"); state.sortI = numItems / 2 - 1; }
        else if (state.sortAlgo == 8) { strcpy_s(state.sortAlgoName, "RADIX SORT (LSD)"); state.sortExp = 1; state.sortMin = numItems; }
    }

    // Dynamic steps per frame according to item count
    int stepsPerFrame = (state.sortAlgo == 2 || state.sortAlgo == 6) ?
        (std::max)(1, numItems / 40) :
        (std::max)(2, numItems / 18);

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
        state.sortSweepIdx += (std::max)(1, numItems / 35);
        if (state.sortSweepIdx >= numItems) {
            state.sortState = 3;
            state.sortWait = 0;

            // Completion starburst explosion
            int midX = state.virtualW / 2;
            for (int p = 0; p < 24; ++p) {
                float ang = (float)p * (6.2831853f / 24.0f);
                state.sparkles.push_back({ (float)midX, 130.0f, cosf(ang) * 2.5f, sinf(ang) * 2.5f, 0, 24, COL_GREEN_LIME });
            }
        }
    }
    else if (state.sortState == 3) {
        state.sortWait++;
        if (state.sortWait > 75) state.sortState = 0;
    }

    // Sparkles update
    for (size_t i = 0; i < state.sparkles.size(); ) {
        state.sparkles[i].x += state.sparkles[i].vx;
        state.sparkles[i].y += state.sparkles[i].vy;
        state.sparkles[i].life++;
        if (state.sparkles[i].life >= state.sparkles[i].maxLife) {
            state.sparkles.erase(state.sparkles.begin() + i);
        } else {
            ++i;
        }
    }
}

// ============================================================================
// COMPLETE ARCADE FRAME RENDERING
// ============================================================================
static void RenderArcadeFrame(SortState& s) {
    // 1. Clear Framebuffer to Obsidian Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 2. Playfield Arena Double-Line Neon Borders & Background Grid
    DrawArcadeBorder(s);
    DrawCyberGrid(s);

    int numItems = (int)s.sortArray.size();
    if (numItems <= 0) numItems = g_SortItemCount;

    int courtLeft = 14;
    int courtRight = s.virtualW - 15;
    int courtWidth = courtRight - courtLeft + 1;
    int baselineY = 247; // Resting directly on top of the inner bottom border (y=248)
    int maxBarHeight = 210; // From y=38 down to baseline y=247

    // 3. Render Each Bar as ONE Solid Beveled Block with Tight Hairline Gaps
    for (int i = 0; i < numItems; i++) {
        int val = s.sortArray[i];

        // 1-pixel tight hairline gap between bars spanning across court
        int slotStart = courtLeft + (int)((float)i * (float)courtWidth / (float)numItems);
        int slotEnd = courtLeft + (int)((float)(i + 1) * (float)courtWidth / (float)numItems);
        int x1 = slotStart;
        int x2 = (slotEnd - slotStart > 3) ? (slotEnd - 2) : (slotEnd - 1);
        if (x2 < x1) x2 = x1;

        int barH = (val * maxBarHeight) / numItems;
        if (barH < 3) barH = 3;
        int yTop = baselineY - barH + 1;

        uint32_t barBaseCol = GetTetrisTierColor(val, numItems);
        bool isComparing = (s.sortState == 1 && (i == s.sortRed1 || i == s.sortRed2));
        bool isSwept = (s.sortState == 2 && i <= s.sortSweepIdx);
        bool isComplete = (s.sortState == 3);

        if (isComparing) {
            barBaseCol = COL_CRIMSON;
        } else if (isSwept || isComplete) {
            barBaseCol = COL_GREEN_EMERALD;
            if (s.sortState == 2 && i == s.sortSweepIdx) {
                barBaseCol = COL_WHITE; // Leading sweep beam
            }
        }

        // Draw the single solid beveled block
        DrawSolidBlockBar(s, x1, x2, yTop, baselineY, barBaseCol);

        // Active Comparison Needle Cursor
        if (isComparing && yTop > 34) {
            int midX = (x1 + x2) / 2;
            DrawVLine(s, midX, yTop - 5, yTop - 2, COL_CRIMSON_GLOW);
            PutPixel(s, midX, yTop - 6, COL_WHITE);

            // Spurt comparison spark
            if (s.sparkles.size() < 16 && (rand() % 4 == 0)) {
                float vx = ((rand() % 100) - 50) * 0.02f;
                float vy = -1.2f - ((rand() % 100) * 0.01f);
                s.sparkles.push_back({ (float)midX, (float)(yTop - 3), vx, vy, 0, 10, COL_CRIMSON_GLOW });
            }
        }
    }

    // 4. Sparkle Particles
    for (const auto& sp : s.sparkles) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }

    // 5. Retro Arcade Top HUD (Matching Tetris, Space Invaders, Snake)
    int playLeft = 14;

    // Top Left: Algorithm Name & Complexity
    DrawText(s, playLeft + 2, 4, s.sortAlgoName, COL_CYAN_NEON);
    DrawText(s, playLeft + 2, 13, GetAlgoComplexity(s.sortAlgo), COL_GRAY_LIGHT);

    // Top Center: Operation Counters
    int midX = s.virtualW / 2;
    char swapBuf[32];
    sprintf_s(swapBuf, "SWAPS: %lld", s.sortSwaps);
    int sw = (int)strlen(swapBuf) * 6;
    DrawText(s, midX - sw / 2, 4, swapBuf, COL_GOLD);

    char compBuf[32];
    sprintf_s(compBuf, "COMPS: %lld", s.sortComparisons);
    int cw = (int)strlen(compBuf) * 6;
    DrawText(s, midX - cw / 2, 13, compBuf, COL_WHITE);

    // Top Right: Array Size & Current Status
    char arrBuf[32];
    sprintf_s(arrBuf, "ARRAY: %d ITEMS", numItems);
    DrawText(s, s.virtualW - playLeft - 94, 4, arrBuf, COL_GREEN_LIME);
    if (s.sortState == 1) {
        DrawText(s, s.virtualW - playLeft - 94, 13, "STATUS: SORTING", COL_WHITE);
    } else if (s.sortState == 2) {
        DrawText(s, s.virtualW - playLeft - 94, 13, "STATUS: VERIFY", COL_GOLD);
    } else {
        DrawText(s, s.virtualW - playLeft - 94, 13, "STATUS: COMPLETE", COL_GREEN_EMERALD);
    }

    // (Bottom text is completely removed per user request for clean arena consistency)
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderRandomSort(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<SortState>(15);

    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    if (!s.initialized || s.virtualW != vW || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);
        s.sortState = 0;
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200;
    s.lastTick = now;

    int subSteps = (int)(elapsed / 16);
    if (subSteps < 1) subSteps = 1;
    if (subSteps > 4) subSteps = 4;

    for (int i = 0; i < subSteps; ++i) {
        StepSortingSimulation(s);
    }

    // Render virtual arcade framebuffer
    RenderArcadeFrame(s);

    // Fullscreen Pixel-Perfect Stretched Blit (Fills 100% of the screen, zero black bars)
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = s.virtualW;
    bmi.bmiHeader.biHeight = -s.virtualH; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(memDC, COLORONCOLOR);
    StretchDIBits(
        memDC,
        0, 0, width, height,
        0, 0, s.virtualW, s.virtualH,
        s.fb.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// Register as Screensaver ID 15
REGISTER_SCREENSAVER(
    15,
    L"Sorting Algorithms",
    "sort",
    { "sort", "sorting" },
    WRAP_LEGACY(RenderRandomSort),
    GetSortSettings()
);