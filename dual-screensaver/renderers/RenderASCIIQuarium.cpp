#include "framework.h"
#include "Renderers.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Colors
static const COLORREF COL_WATER_BG = RGB(5, 12, 30);
static const COLORREF COL_WAVE     = RGB(70, 190, 255);
static const COLORREF COL_WAVE_TOP = RGB(190, 235, 255);
static const COLORREF COL_SAND1    = RGB(185, 155, 95);
static const COLORREF COL_SAND2    = RGB(140, 115, 65);
static const COLORREF COL_BUBBLE   = RGB(180, 235, 255);
static const COLORREF COL_DUCK     = RGB(255, 220, 50);
static const COLORREF COL_BEAK     = RGB(255, 130, 0);
static const COLORREF COL_CRAB     = RGB(245, 80, 60);
static const COLORREF COL_CRAB_EYE = RGB(255, 255, 255);

static const COLORREF FISH_COLORS[] = {
    RGB(255, 140, 30),  // Goldfish orange
    RGB(255, 70, 130),  // Neon pink
    RGB(40, 220, 240),  // Electric cyan
    RGB(255, 235, 50),  // Yellow tang
    RGB(160, 90, 255),  // Lavender
    RGB(70, 240, 120),  // Sea green
    RGB(255, 90, 60),   // Coral red
    RGB(220, 225, 235)  // Silver
};
static const int NUM_FISH_COLORS = sizeof(FISH_COLORS) / sizeof(FISH_COLORS[0]);

struct Cell {
    char ch = ' ';
    COLORREF color = 0;
};

static void InitAquarium(ScreenData* data, int cols, int rows) {
    data->aquaWidthInChars = cols;
    data->aquaHeightInChars = rows;
    data->aquaFish.clear();
    data->aquaBubbles.clear();
    data->aquaSeaweed.clear();
    data->aquaJelly.clear();

    // Spawn Seaweed across the seabed
    int seaweedCount = (std::max)(6, cols / 10);
    for (int i = 0; i < seaweedCount; ++i) {
        ScreenData::AquaSeaweed sw;
        sw.x = rand() % (cols - 4) + 2;
        sw.height = rand() % (std::max)(4, rows / 3) + (rows / 5);
        sw.phase = (float)(rand() % 628) / 100.0f;
        sw.color = (rand() % 2 == 0) ? RGB(40, 190, 80) : RGB(25, 150, 65);
        data->aquaSeaweed.push_back(sw);
    }

    // Spawn Fish
    int fishCount = (std::max)(12, cols / 7);
    for (int i = 0; i < fishCount; ++i) {
        ScreenData::AquaFish f;
        f.x = (float)(rand() % cols);
        f.y = (float)(rand() % (std::max)(5, rows - 9) + 4);
        bool goRight = (rand() % 2 == 0);
        float spd = 0.25f + (float)(rand() % 50) / 100.0f;
        f.vx = goRight ? spd : -spd;
        f.type = rand() % 5; // Types 0 to 4
        f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        f.animFrame = rand() % 10;
        data->aquaFish.push_back(f);
    }

    // Spawn a couple of Jellyfish
    int jellyCount = (std::max)(2, cols / 40);
    for (int i = 0; i < jellyCount; ++i) {
        ScreenData::AquaJellyfish j;
        j.x = (float)(rand() % (cols - 12) + 6);
        j.y = (float)(rand() % (rows - 15) + 6);
        j.vy = -0.15f - (float)(rand() % 15) / 100.0f;
        j.pulsePhase = (float)(rand() % 628) / 100.0f;
        j.color = (rand() % 2 == 0) ? RGB(255, 160, 220) : RGB(170, 220, 255);
        data->aquaJelly.push_back(j);
    }

    // Crab on seabed
    data->aquaCrab.x = (float)(cols / 2);
    data->aquaCrab.y = rows - 3;
    data->aquaCrab.vx = 0.25f;
    data->aquaCrab.animFrame = 0;

    // Duck on surface
    data->aquaDuck.x = -15.0f;
    data->aquaDuck.vx = 0.25f;
    data->aquaDuck.active = (rand() % 3 == 0);

    data->aquaLastTick = GetTickCount();
    data->aquaInitialized = true;
}

void RenderASCIIQuarium(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, OPAQUE);
    SetBkColor(memDC, COL_WATER_BG);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cW = tm.tmAveCharWidth;
    int cH = tm.tmHeight;
    if (cW <= 0) cW = 8;
    if (cH <= 0) cH = 16;

    int cols = width / cW;
    int rows = height / cH;
    if (cols < 20) cols = 20;
    if (rows < 12) rows = 12;

    if (!data->aquaInitialized || data->aquaWidthInChars != cols || data->aquaHeightInChars != rows) {
        InitAquarium(data, cols, rows);
    }

    DWORD now = GetTickCount();
    float dt = (now - data->aquaLastTick) / 1000.0f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.033f;
    data->aquaLastTick = now;
    float timeVal = now * 0.003f;

    // Clear background
    HBRUSH bgBrush = CreateSolidBrush(COL_WATER_BG);
    FillRect(memDC, &rect, bgBrush);
    DeleteObject(bgBrush);

    std::vector<Cell> grid(cols * rows);

    auto putChar = [&](int x, int y, char c, COLORREF col) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            if (c != ' ') {
                grid[y * cols + x].ch = c;
                grid[y * cols + x].color = col;
            }
        }
    };

    auto putStr = [&](int x, int y, const char* str, COLORREF col) {
        int len = (int)strlen(str);
        for (int i = 0; i < len; ++i) {
            putChar(x + i, y, str[i], col);
        }
    };

    // 1. Water Surface & Waves (Rows 1-2)
    for (int x = 0; x < cols; ++x) {
        float wave = sinf(timeVal * 1.5f + x * 0.28f);
        char wChar = (wave > 0.45f) ? '^' : (wave > -0.25f ? '~' : '_');
        COLORREF wCol = (wave > 0.65f) ? COL_WAVE_TOP : COL_WAVE;
        putChar(x, 2, wChar, wCol);
    }

    // Duck floating on surface
    if (data->aquaDuck.active) {
        data->aquaDuck.x += data->aquaDuck.vx * (dt * 30.0f);
        int dx = (int)data->aquaDuck.x;
        // Duck sprite facing right:
        //  Row 0:   __
        //  Row 1: <(o )___
        //  Row 2:  ( ._> /
        putStr(dx + 2, 0, "__", COL_DUCK);
        putChar(dx, 1, '<', COL_BEAK);
        putStr(dx + 1, 1, "(o )___", COL_DUCK);
        putStr(dx + 1, 2, "( ._> /", COL_DUCK);

        if (data->aquaDuck.x > cols + 5) {
            data->aquaDuck.active = false;
            data->aquaDuck.x = -15.0f;
        }
    } else if (rand() % 400 == 0) {
        data->aquaDuck.active = true;
        data->aquaDuck.x = -15.0f;
    }

    // 2. Seabed (Sand & Pebbles)
    int sandRow = rows - 2;
    for (int x = 0; x < cols; ++x) {
        char s1 = (x % 3 == 0) ? '~' : ((x % 5 == 0) ? '^' : '_');
        char s2 = (x % 4 == 0) ? 'w' : ((x % 7 == 0) ? 'm' : '~');
        putChar(x, sandRow, s1, COL_SAND1);
        putChar(x, sandRow + 1, s2, COL_SAND2);
    }

    // 3. Seaweed Swaying
    for (const auto& sw : data->aquaSeaweed) {
        for (int h = 0; h < sw.height; ++h) {
            int curY = sandRow - 1 - h;
            if (curY < 3) break;
            float sway = sinf(timeVal * 1.1f + sw.phase + h * 0.32f) * 1.8f;
            int curX = sw.x + (int)roundf(sway);
            char stem = (sway > 0.5f) ? '/' : (sway < -0.5f ? '\\' : '|');
            putChar(curX, curY, stem, sw.color);
            if (h % 3 == 1) {
                putChar(curX - 1, curY, '(', sw.color);
                putChar(curX + 1, curY, ')', sw.color);
            }
        }
    }

    // 4. Seaweed Bubble Spawner
    if (rand() % 15 == 0 && !data->aquaSeaweed.empty()) {
        int swIdx = rand() % data->aquaSeaweed.size();
        ScreenData::AquaBubble b;
        b.x = (float)data->aquaSeaweed[swIdx].x;
        b.y = (float)(sandRow - 2);
        b.speed = 0.35f + (float)(rand() % 30) / 100.0f;
        b.swaySpeed = 2.0f + (float)(rand() % 20) / 10.0f;
        b.swayPhase = (float)(rand() % 628) / 100.0f;
        int rType = rand() % 10;
        b.type = (rType < 5) ? 0 : (rType < 8 ? 1 : 2); // 0='.', 1='o', 2='O'
        data->aquaBubbles.push_back(b);
    }

    // 5. Jellyfish
    for (auto& j : data->aquaJelly) {
        j.pulsePhase += dt * 2.5f;
        float pulse = sinf(j.pulsePhase);
        j.y += (pulse > 0.2f ? j.vy : j.vy * 0.2f) * (dt * 40.0f);
        j.x += sinf(j.pulsePhase * 0.5f) * 0.15f;

        if (j.y < 3.0f) {
            j.y = (float)(rows - 10);
            j.x = (float)(rand() % (cols - 12) + 6);
        }

        int jx = (int)j.x;
        int jy = (int)j.y;
        if (pulse > 0.0f) {
            // Expanded bell
            putStr(jx + 1, jy,     ".-\"\"-.", j.color);
            putStr(jx,     jy + 1, "/      \\", j.color);
            putStr(jx,     jy + 2, "(`~~~~~~`)", j.color);
            putStr(jx + 1, jy + 3, "|| || ||", j.color);
            putStr(jx + 1, jy + 4, "|| || ||", j.color);
        } else {
            // Contracted bell
            putStr(jx + 2, jy,     "_.._", j.color);
            putStr(jx + 1, jy + 1, "/    \\", j.color);
            putStr(jx + 1, jy + 2, "(~~~~~~)", j.color);
            putStr(jx + 1, jy + 3, "// || \\\\", j.color);
            putStr(jx + 1, jy + 4, "// || \\\\", j.color);
        }
    }

    // 6. Crab on Seabed
    {
        auto& crab = data->aquaCrab;
        crab.x += crab.vx * (dt * 25.0f);
        if (crab.x > cols - 8) {
            crab.x = (float)(cols - 8);
            crab.vx = -fabsf(crab.vx);
        } else if (crab.x < 2.0f) {
            crab.x = 2.0f;
            crab.vx = fabsf(crab.vx);
        }
        crab.animFrame = ((int)(now / 200)) % 2;
        int cx = (int)crab.x;
        int cy = sandRow - 1;

        if (crab.animFrame == 0) {
            putStr(cx + 1, cy - 1, "(\\_/)", COL_CRAB);
            putStr(cx,     cy,     "(o.o)", COL_CRAB);
            putStr(cx,     cy + 1, "(> <)", COL_CRAB);
        } else {
            putStr(cx + 1, cy - 1, "(\\_/)", COL_CRAB);
            putStr(cx,     cy,     "(O.O)", COL_CRAB);
            putStr(cx,     cy + 1, "(< >)", COL_CRAB);
        }
    }

    // 7. Fish Simulation & Rendering
    for (auto& f : data->aquaFish) {
        f.x += f.vx * (dt * 30.0f);
        bool goingRight = (f.vx > 0.0f);

        // Turnaround / Wrap-around
        if (goingRight && f.x > cols + 15) {
            f.x = -15.0f;
            f.y = (float)(rand() % (std::max)(5, rows - 9) + 4);
            f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        } else if (!goingRight && f.x < -15.0f) {
            f.x = (float)(cols + 15);
            f.y = (float)(rand() % (std::max)(5, rows - 9) + 4);
            f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        }

        // Random occasional mouth bubbles
        if (rand() % 120 == 0 && f.x > 2 && f.x < cols - 2) {
            ScreenData::AquaBubble b;
            b.x = goingRight ? f.x + 5.0f : f.x - 1.0f;
            b.y = f.y;
            b.speed = 0.4f + (float)(rand() % 25) / 100.0f;
            b.swaySpeed = 2.5f;
            b.swayPhase = (float)(rand() % 628) / 100.0f;
            b.type = (rand() % 2 == 0) ? 0 : 1;
            data->aquaBubbles.push_back(b);
        }

        int fx = (int)f.x;
        int fy = (int)f.y;

        switch (f.type) {
            case 0: // Tiny darting fish: ><> or <><
                if (goingRight) {
                    putStr(fx, fy, "><>", f.color);
                } else {
                    putStr(fx, fy, "<><", f.color);
                }
                break;

            case 1: // Guppy / Tetra: >(')))>< or ><(((')<
                if (goingRight) {
                    putStr(fx, fy, ">(')))><", f.color);
                } else {
                    putStr(fx, fy, "><(((')<", f.color);
                }
                break;

            case 2: // Striped Angelfish (3 rows)
                if (goingRight) {
                    putStr(fx + 2, fy - 1, "\\", f.color);
                    putStr(fx + 1, fy,     "/--\\", f.color);
                    putStr(fx,     fy + 1, "< () >", f.color);
                    putStr(fx + 1, fy + 2, "\\--/", f.color);
                    putStr(fx + 2, fy + 3, "/", f.color);
                } else {
                    putStr(fx + 3, fy - 1, "/", f.color);
                    putStr(fx + 1, fy,     "/--\\", f.color);
                    putStr(fx,     fy + 1, "< () >", f.color);
                    putStr(fx + 1, fy + 2, "\\--/", f.color);
                    putStr(fx + 3, fy + 3, "\\", f.color);
                }
                break;

            case 3: // Fantail Goldfish (4 rows)
                if (goingRight) {
                    putStr(fx + 3, fy,     ",", f.color);
                    putStr(fx + 2, fy + 1, "/|", f.color);
                    putStr(fx + 1, fy + 2, "/_|/\\", f.color);
                    putStr(fx,     fy + 3, "<* )  >", f.color);
                    putStr(fx + 1, fy + 4, "\\ |\\/", f.color);
                    putStr(fx + 2, fy + 5, "\\|", f.color);
                    putStr(fx + 3, fy + 6, "`", f.color);
                } else {
                    putStr(fx + 2, fy,     ",", f.color);
                    putStr(fx + 2, fy + 1, "|\\", f.color);
                    putStr(fx + 1, fy + 2, "/\\|_\\", f.color);
                    putStr(fx,     fy + 3, "<  ( *>", f.color);
                    putStr(fx + 1, fy + 4, "\\/| /", f.color);
                    putStr(fx + 2, fy + 5, "|/", f.color);
                    putStr(fx + 2, fy + 6, "`", f.color);
                }
                break;

            case 4: // Big Shark / Whale (occasional deep cruiser)
            default:
                if (goingRight) {
                    putStr(fx + 7, fy,     "__", f.color);
                    putStr(fx,     fy + 1, "\\_____  / /", f.color);
                    putStr(fx + 1, fy + 2, "\\    \\/ /", f.color);
                    putStr(fx + 2, fy + 3, "\\  O   /", f.color);
                    putStr(fx + 3, fy + 4, ">     <", f.color);
                    putStr(fx + 2, fy + 5, "/       \\", f.color);
                    putStr(fx + 1, fy + 6, "/  _____  \\", f.color);
                } else {
                    putStr(fx + 2, fy,     "__", f.color);
                    putStr(fx + 1, fy + 1, "\\ \\  _____/", f.color);
                    putStr(fx + 1, fy + 2, "\\ \\/    /", f.color);
                    putStr(fx + 2, fy + 3, "\\   O  /", f.color);
                    putStr(fx + 3, fy + 4, ">     <", f.color);
                    putStr(fx + 2, fy + 5, "/       \\", f.color);
                    putStr(fx + 1, fy + 6, "/  _____  \\", f.color);
                }
                break;
        }
    }

    // 8. Bubbles Update & Rendering
    static const char BUBBLE_CHARS[] = { '.', 'o', 'O' };
    for (size_t i = 0; i < data->aquaBubbles.size(); ) {
        auto& b = data->aquaBubbles[i];
        b.y -= b.speed * (dt * 30.0f);
        b.swayPhase += dt * b.swaySpeed;
        int bx = (int)roundf(b.x + sinf(b.swayPhase) * 1.4f);
        int by = (int)b.y;

        if (by <= 2) {
            // Pop at water surface
            data->aquaBubbles.erase(data->aquaBubbles.begin() + i);
        } else {
            char bChar = BUBBLE_CHARS[b.type % 3];
            putChar(bx, by, bChar, COL_BUBBLE);
            ++i;
        }
    }

    // 9. Span-Batched GDI Rendering for Ultra-High Performance
    for (int y = 0; y < rows; ++y) {
        int x = 0;
        while (x < cols) {
            if (grid[y * cols + x].ch == ' ') {
                x++;
                continue;
            }

            int startX = x;
            COLORREF col = grid[y * cols + x].color;
            char span[512];
            int spanLen = 0;

            while (x < cols && grid[y * cols + x].ch != ' ' && grid[y * cols + x].color == col && spanLen < 500) {
                span[spanLen++] = grid[y * cols + x].ch;
                x++;
            }
            span[spanLen] = '\0';

            SetTextColor(memDC, col);
            TextOutA(memDC, startX * cW, y * cH, span, spanLen);
        }
    }
}
