// RenderNyanCat.cpp - Authentic ASCII Nyan Cat Screensaver
// Features:
// - 12 authentic 64x64 animation frames (cat, poptart, running legs, swishing tail, face, cheeks, stars)
// - Infinite undulating rainbow trail extending to the left edge of any widescreen monitor
// - Parallax drifting starfield across deep space navy background
// - "You have nyaned for X seconds!" timer
// - High-performance span-batched GDI rendering with Consolas font

#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "../assets/NyanCatFrames.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>

struct NyanStar {
    float x, y;
    float speed;
    int type;
    int phase;
};

struct NyanState {
    bool initialized = false;
    DWORD lastTick = 0;
    DWORD startTime = 0;
    int frameIndex = 0;
    HFONT font = NULL;
    int fontSize = 0;
    int widthInChars = 0;
    int heightInChars = 0;
    std::vector<NyanStar> stars;

    ~NyanState() {
        if (font) {
            DeleteObject(font);
            font = NULL;
        }
    }
};

// Space & Cat Palette
static const COLORREF COL_SPACE_BG    = RGB(0,   38,  80);   // Deep space navy (#002650)
static const COLORREF COL_STAR_WHITE  = RGB(255, 255, 255);
static const COLORREF COL_BORDER_BLCK = RGB(0,   0,   0);
static const COLORREF COL_CRUST_BG    = RGB(255, 205, 150);  // Golden pastry crust
static const COLORREF COL_CRUST_FG    = RGB(190, 130,  60);
static const COLORREF COL_FROST_BG    = RGB(255, 169, 255);  // Pink frosting
static const COLORREF COL_FROST_FG    = RGB(255, 225, 255);
static const COLORREF COL_SPRINKLE_BG = RGB(255, 60,  140);  // Vivid sprinkle
static const COLORREF COL_SPRINKLE_FG = RGB(255, 255, 255);

// Rainbow Bands (Red, Orange, Yellow, Green, Cyan, Purple)
static const COLORREF COL_RAIN_RED_BG = RGB(255, 25,  0);
static const COLORREF COL_RAIN_RED_FG = RGB(255, 150, 140);

static const COLORREF COL_RAIN_ORG_BG = RGB(255, 154, 0);
static const COLORREF COL_RAIN_ORG_FG = RGB(255, 215, 140);

static const COLORREF COL_RAIN_YEL_BG = RGB(255, 240, 0);
static const COLORREF COL_RAIN_YEL_FG = RGB(255, 255, 180);

static const COLORREF COL_RAIN_GRN_BG = RGB(40,  220, 0);
static const COLORREF COL_RAIN_GRN_FG = RGB(160, 255, 150);

static const COLORREF COL_RAIN_CYA_BG = RGB(0,   144, 255);
static const COLORREF COL_RAIN_CYA_FG = RGB(170, 230, 255);

static const COLORREF COL_RAIN_VIO_BG = RGB(104, 68,  255);
static const COLORREF COL_RAIN_VIO_FG = RGB(215, 190, 255);

// Cat Anatomy
static const COLORREF COL_CAT_GRAY_BG = RGB(153, 153, 153);
static const COLORREF COL_CAT_GRAY_FG = RGB(220, 220, 220);
static const COLORREF COL_CHEEK_BG    = RGB(255, 163, 152);
static const COLORREF COL_CHEEK_FG    = RGB(255, 235, 240);

struct CellStyle {
    char c1;
    char c2;
    COLORREF fg;
    COLORREF bg;
};

// Fast lookup for ASCII glyphs and colors from Nyan Cat frame codes
static inline CellStyle GetNyanStyle(char code) {
    switch (code) {
    case '.': // Star sparkle or eye white
        return { '*', '*', COL_STAR_WHITE, COL_SPACE_BG };
    case '\'': // Black border outline
        return { '#', '#', COL_BORDER_BLCK, COL_BORDER_BLCK };
    case '@': // Tan poptart crust
        return { ':', ':', COL_CRUST_FG, COL_CRUST_BG };
    case '$': // Pink poptart frosting
        return { '~', '~', COL_FROST_FG, COL_FROST_BG };
    case '-': // Red sprinkles
        return { '<', '>', COL_SPRINKLE_FG, COL_SPRINKLE_BG };
    case '>': // Red rainbow
        return { '=', '=', COL_RAIN_RED_FG, COL_RAIN_RED_BG };
    case '&': // Orange rainbow
        return { '=', '=', COL_RAIN_ORG_FG, COL_RAIN_ORG_BG };
    case '+': // Yellow rainbow
        return { '=', '=', COL_RAIN_YEL_FG, COL_RAIN_YEL_BG };
    case '#': // Green rainbow
        return { '=', '=', COL_RAIN_GRN_FG, COL_RAIN_GRN_BG };
    case '=': // Light blue / cyan rainbow
        return { '=', '=', COL_RAIN_CYA_FG, COL_RAIN_CYA_BG };
    case ';': // Dark violet rainbow
        return { '=', '=', COL_RAIN_VIO_FG, COL_RAIN_VIO_BG };
    case '*': // Cat gray body, head, paws, tail
        return { ';', ';', COL_CAT_GRAY_FG, COL_CAT_GRAY_BG };
    case '%': // Pink cheeks
        return { '(', ')', COL_CHEEK_FG, COL_CHEEK_BG };
    case ',': // Space background
    default:
        return { ' ', ' ', COL_SPACE_BG, COL_SPACE_BG };
    }
}

// Initialize / reinitialize screensaver state
static void InitNyanCat(NyanState& s, int numCells, int rows) {
    s.initialized = true;
    s.startTime = GetTickCount();
    s.lastTick = s.startTime;
    s.frameIndex = 0;

    s.stars.clear();
    int starCount = (std::max)(50, (numCells * rows) / 45);
    s.stars.reserve(starCount);

    for (int i = 0; i < starCount; ++i) {
        NyanStar star;
        star.x = (float)(rand() % (std::max)(1, numCells + 20) - 10);
        star.y = (float)(rand() % rows);
        star.speed = 0.35f + (float)(rand() % 45) / 100.0f; // drifting warp speed
        star.type = rand() % 3; // 0 = single dot, 1 = diamond, 2 = cross
        star.phase = rand() % 60;
        s.stars.push_back(star);
    }
}

void RenderNyanCat(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& s = data->GetCustomState<NyanState>(26);

    // 1. Font Selection
    int targetFontH;
    if (data->isPreview) {
        targetFontH = (std::max)(5, height / 24);
    } else {
        targetFontH = (height >= 1400) ? 20 : 16;
    }

    if (s.font == NULL || s.fontSize != targetFontH) {
        if (s.font) {
            DeleteObject(s.font);
            s.font = NULL;
        }
        s.fontSize = targetFontH;
        s.font = CreateFontA(
            targetFontH, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    }

    HFONT hOldFont = (HFONT)SelectObject(memDC, s.font);
    SetBkMode(memDC, OPAQUE);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cW = tm.tmAveCharWidth;
    int cH = tm.tmHeight;
    if (cW <= 0) cW = 8;
    if (cH <= 0) cH = 16;

    int cols = width / cW;
    int rows = height / cH;
    if (cols < 20) cols = 20;
    if (rows < 10) rows = 10;

    int numCells = cols / 2; // Each Nyan Cat cell is 2 characters wide for square 1:1 aspect ratio

    // 2. Initialize or reinitialize on resize
    if (!s.initialized || s.widthInChars != cols || s.heightInChars != rows) {
        s.widthInChars = cols;
        s.heightInChars = rows;
        InitNyanCat(s, numCells, rows);
    }

    DWORD now = GetTickCount();

    // 3. Advance Animation Frame (~11.5 FPS, authentic Nyan Cat timing)
    if (now - s.lastTick >= 85) {
        int framesToAdvance = (int)((now - s.lastTick) / 85);
        s.frameIndex = (s.frameIndex + framesToAdvance) % 12;
        s.lastTick = now;
    }

    int frameIdx = s.frameIndex;

    // 4. Update Starfield Positions
    for (auto& star : s.stars) {
        star.x -= star.speed;
        star.phase = (star.phase + 1) % 60;
        if (star.x < -15.0f) {
            star.x = (float)(numCells + (rand() % 10));
            star.y = (float)(rand() % rows);
            star.speed = 0.35f + (float)(rand() % 45) / 100.0f;
            star.type = rand() % 3;
        }
    }

    // 5. Screen Grid Allocation
    struct ScreenCell {
        char c1 = ' ';
        char c2 = ' ';
        COLORREF fg = COL_SPACE_BG;
        COLORREF bg = COL_SPACE_BG;
        bool isOpaque = false;
    };

    std::vector<ScreenCell> grid((size_t)(numCells * rows));

    // 6. Draw Drifting Stars into Background
    for (const auto& star : s.stars) {
        int sx = (int)roundf(star.x);
        int sy = (int)roundf(star.y);
        int twinkle = (star.phase / 6) % 4;

        auto putStarCell = [&](int cx, int cy, char ch1, char ch2, COLORREF col) {
            if (cx >= 0 && cx < numCells && cy >= 0 && cy < rows) {
                ScreenCell& cell = grid[cy * numCells + cx];
                if (!cell.isOpaque) {
                    cell.c1 = ch1;
                    cell.c2 = ch2;
                    cell.fg = col;
                    cell.bg = COL_SPACE_BG;
                }
            }
        };

        if (star.type == 0) { // Single twinkle star
            char ch = (twinkle == 0) ? '*' : ((twinkle == 1) ? '+' : '.');
            putStarCell(sx, sy, ch, ' ', COL_STAR_WHITE);
        } else if (star.type == 1) { // Cross star
            if (twinkle == 0) {
                putStarCell(sx, sy, '*', '*', COL_STAR_WHITE);
            } else if (twinkle == 1 || twinkle == 3) {
                putStarCell(sx, sy - 1, ' ', '|', COL_STAR_WHITE);
                putStarCell(sx - 1, sy, '-', '+', COL_STAR_WHITE);
                putStarCell(sx + 1, sy, '-', ' ', COL_STAR_WHITE);
                putStarCell(sx, sy + 1, ' ', '|', COL_STAR_WHITE);
            } else {
                putStarCell(sx, sy, '+', ' ', COL_STAR_WHITE);
            }
        } else { // Diamond star
            if (twinkle == 0) {
                putStarCell(sx, sy, '.', '.', COL_STAR_WHITE);
            } else if (twinkle == 1 || twinkle == 3) {
                putStarCell(sx, sy - 1, '.', ' ', COL_STAR_WHITE);
                putStarCell(sx - 1, sy, '.', ' ', COL_STAR_WHITE);
                putStarCell(sx + 1, sy, '.', ' ', COL_STAR_WHITE);
                putStarCell(sx, sy + 1, '.', ' ', COL_STAR_WHITE);
            } else {
                putStarCell(sx, sy, '*', ' ', COL_STAR_WHITE);
            }
        }
    }

    // 7. Calculate Viewport & Cat Centering
    // Frame is 64x64. Cat occupies rows 23..41 and cols 0..47.
    // Shift slightly to the right so rainbow trail extends gloriously across the screen.
    int shiftRight = (numCells > 64) ? (numCells - 64) / 4 : 0;
    int min_col = (64 - numCells) / 2 - shiftRight;
    int max_col = min_col + numCells;
    int min_row = (64 - rows) / 2;
    int max_row = min_row + rows;

    // 8. Render Nyan Cat Frame & Infinite Undulating Rainbow Tail
    static const char rainbowStr[] = ",,>>&&&+++###==;;;,,";

    for (int y = min_row; y < max_row; ++y) {
        int screenY = y - min_row;
        if (screenY < 0 || screenY >= rows) continue;

        for (int x = min_col; x < max_col; ++x) {
            int screenX = x - min_col;
            if (screenX < 0 || screenX >= numCells) continue;

            char colorCode = ',';

            if (y > 23 && y < 43 && x < 0) {
                // Authentic Nyan Cat square-wave undulating rainbow tail
                int mod_x = ((-x + 2) % 16 + 16) % 16 / 8;
                if ((frameIdx / 2) % 2) {
                    mod_x = 1 - mod_x;
                }
                int rIdx = mod_x + (y - 23);
                if (rIdx >= 0 && rIdx < 20) {
                    colorCode = rainbowStr[rIdx];
                }
            } else if (x >= 0 && x < 64 && y >= 0 && y < 64) {
                colorCode = NYAN_FRAMES[frameIdx][y][x];
            }

            if (colorCode != ',') {
                CellStyle cs = GetNyanStyle(colorCode);

                // Special handling for cat eyes: in row 33, '.' is the white eye shine inside black border
                if (colorCode == '.' && y == 33 && x >= 30 && x <= 45) {
                    cs.bg = COL_BORDER_BLCK;
                    cs.fg = COL_STAR_WHITE;
                    cs.c1 = 'o';
                    cs.c2 = 'o';
                }

                ScreenCell& cell = grid[screenY * numCells + screenX];
                cell.c1 = cs.c1;
                cell.c2 = cs.c2;
                cell.fg = cs.fg;
                cell.bg = cs.bg;
                cell.isOpaque = true;
            }
        }
    }

    // 9. Authentic Bottom Counter: "You have nyaned for X seconds!"
    if (!data->isPreview) {
        DWORD nyanSeconds = (now - s.startTime) / 1000;
        char counterBuf[64];
        int counterLen = sprintf_s(counterBuf, "You have nyaned for %lu seconds!", (unsigned long)nyanSeconds);

        int counterRow = (std::min)(rows - 2, (max_row - min_row) - 1);
        if (counterRow < rows && counterRow >= 0) {
            int startCharCol = (cols - counterLen) / 2;
            if (startCharCol < 0) startCharCol = 0;

            // Map counter characters into cell grid (accounting for 2 chars per cell)
            for (int i = 0; i < counterLen; ++i) {
                int chCol = startCharCol + i;
                int cCell = chCol / 2;
                int cSub  = chCol % 2;
                if (cCell < numCells) {
                    ScreenCell& cell = grid[counterRow * numCells + cCell];
                    if (!cell.isOpaque) {
                        cell.bg = COL_SPACE_BG;
                        cell.fg = COL_STAR_WHITE;
                    }
                    if (cSub == 0) cell.c1 = counterBuf[i];
                    else cell.c2 = counterBuf[i];
                }
            }
        }
    }

    // 10. Ultra-High Performance Span-Batched GDI Rendering
    for (int y = 0; y < rows; ++y) {
        int x = 0;
        while (x < numCells) {
            int startX = x;
            COLORREF curFg = grid[y * numCells + x].fg;
            COLORREF curBg = grid[y * numCells + x].bg;

            char span[512];
            int spanLen = 0;

            while (x < numCells &&
                   grid[y * numCells + x].fg == curFg &&
                   grid[y * numCells + x].bg == curBg &&
                   spanLen < 500) {
                span[spanLen++] = grid[y * numCells + x].c1;
                span[spanLen++] = grid[y * numCells + x].c2;
                x++;
            }
            span[spanLen] = '\0';

            SetTextColor(memDC, curFg);
            SetBkColor(memDC, curBg);
            TextOutA(memDC, startX * (2 * cW), y * cH, span, spanLen);
        }
    }

    SelectObject(memDC, hOldFont);
}

REGISTER_SCREENSAVER(
    26,
    L"Nyan Cat (ASCII)",
    "nyancat",
    { "nyancat", "nyan", "cat" },
    WRAP_LEGACY(RenderNyanCat),
    {}
);