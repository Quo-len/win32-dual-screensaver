#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
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
static const COLORREF COL_DUCK        = RGB(255, 220, 50);
static const COLORREF COL_BEAK        = RGB(255, 130, 0);
static const COLORREF COL_CRAB        = RGB(245, 80, 60);
static const COLORREF COL_CRAB_EYE    = RGB(255, 255, 255);
static const COLORREF COL_SHIP_SAIL   = RGB(255, 255, 255);
static const COLORREF COL_SHIP_WOOD   = RGB(230, 205, 45); // Asciiquarium yellow wood
static const COLORREF COL_SHIP_DARK   = RGB(180, 155, 35);
static const COLORREF COL_SHIP_CANNON = RGB(35, 35, 35);
static const COLORREF COL_WHALE_BODY  = RGB(90, 115, 245); // Ocean blue-indigo
static const COLORREF COL_WHALE_EYE   = RGB(255, 255, 255);
static const COLORREF COL_WHALE_SPOUT = RGB(85, 235, 255); // Cyan spout
static const COLORREF COL_CASTLE_WALL = RGB(225, 230, 240); // White/silver stone walls & battlements
static const COLORREF COL_CASTLE_ROOF = RGB(240, 195, 55);  // Sand/gold spire roof & arched doorway
static const COLORREF COL_CASTLE_FLAG = RGB(255, 70, 70);   // Red pennant flag

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

static const COLORREF JELLY_COLORS[] = {
    RGB(255, 160, 220), // Translucent pink
    RGB(170, 220, 255), // Cyan
    RGB(215, 180, 255), // Lavender
    RGB(160, 255, 230)  // Bioluminescent mint
};
static const int NUM_JELLY_COLORS = sizeof(JELLY_COLORS) / sizeof(JELLY_COLORS[0]);

struct Cell {
    char ch = ' ';
    COLORREF color = 0;
};

struct AquaFish {
    float x, y;
    float vx;
    int type;
    COLORREF color;
    int animFrame;
};

struct AquaBubble {
    float x, y;
    float speed;
    float swaySpeed;
    float swayPhase;
    int type;
};

struct AquaSeaweed {
    int x;
    int height;
    float phase;
    COLORREF color;
};

struct AquaJellyfish {
    float x, y;
    float baseX;
    float vy;
    float pulsePhase;
    COLORREF color;
    int state;        // 0 = swimming up, 1 = descending
    float topLimit;   // target row near surface before turning back down
    float swayOffset;
};

struct AquaCrab {
    float x;
    int y;
    float vx;
    int animFrame;
};

struct AquaSurfaceEntity {
    float x;
    float vx;
    bool active;
    int type; // 0 = Duck, 1 = Sailing Ship, 2 = Whale
    float animTimer;
};

struct AquaState {
    bool initialized = false;
    DWORD lastTick = 0;
    int widthInChars = 0;
    int heightInChars = 0;
    std::vector<AquaFish> fish;
    std::vector<AquaBubble> bubbles;
    std::vector<AquaSeaweed> seaweed;
    std::vector<AquaJellyfish> jelly;
    AquaCrab crab = { 10.0f, 0, 0.4f, 0 };
    AquaSurfaceEntity surface = { -15.0f, 0.25f, true, 0, 0.0f };
};

static void InitAquarium(AquaState& s, int cols, int rows) {
    s.widthInChars = cols;
    s.heightInChars = rows;
    s.fish.clear();
    s.bubbles.clear();
    s.seaweed.clear();
    s.jelly.clear();

    // Spawn Seaweed across the seabed (spawns across full seabed, including in front of castle)
    int seaweedCount = (std::max)(6, cols / 10);
    for (int i = 0; i < seaweedCount; ++i) {
        AquaSeaweed sw;
        sw.x = rand() % (cols - 4) + 2;
        sw.height = rand() % (std::max)(4, rows / 3) + (rows / 5);
        sw.phase = (float)(rand() % 628) / 100.0f;
        sw.color = (rand() % 2 == 0) ? RGB(40, 190, 80) : RGB(25, 150, 65);
        s.seaweed.push_back(sw);
    }

    int waveRow = (rows < 22) ? 4 : 6;
    int minFishY = waveRow + 5;
    int maxFishY = rows - 5;
    if (maxFishY <= minFishY) maxFishY = minFishY + 1;
    int fishSpan = maxFishY - minFishY + 1;

    // Spawn Fish (well below the multi-tier surface waves)
    int fishCount = (std::max)(12, cols / 7);
    for (int i = 0; i < fishCount; ++i) {
        AquaFish f;
        f.x = (float)(rand() % cols);
        f.y = (float)(rand() % fishSpan + minFishY);
        bool goRight = (rand() % 2 == 0);
        float spd = 0.25f + (float)(rand() % 50) / 100.0f;
        f.vx = goRight ? spd : -spd;
        f.type = rand() % 5; // Types 0 to 4
        f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        f.animFrame = rand() % 10;
        s.fish.push_back(f);
    }

    // Spawn a couple of Jellyfish
    int jellyCount = (std::max)(2, cols / 40);
    for (int i = 0; i < jellyCount; ++i) {
        AquaJellyfish j;
        j.baseX = (float)(rand() % (cols - 14) + 3);
        j.x = j.baseX;
        if (i == 0) {
            j.y = (float)(rows / 2 + rand() % 5);
            j.state = 0; // Rising
        } else if (i == 1) {
            j.y = (float)(rows * 0.7f + rand() % 5);
            j.state = (rand() % 2 == 0) ? 0 : 1;
        } else {
            j.y = (float)(rows + 2 + rand() % 6); // Entering smoothly from below
            j.state = 0;
        }
        j.vy = -0.16f - (float)(rand() % 10) / 100.0f;
        j.pulsePhase = (float)(rand() % 628) / 100.0f;
        j.swayOffset = (float)(rand() % 628) / 100.0f;
        j.topLimit = (float)(rand() % (std::max)(1, (rows - waveRow) / 4) + waveRow + 5);
        j.color = JELLY_COLORS[rand() % NUM_JELLY_COLORS];
        s.jelly.push_back(j);
    }

    // Crab on seabed
    s.crab.x = (float)(cols / 2);
    s.crab.y = rows - 3;
    s.crab.vx = 0.25f;
    s.crab.animFrame = 0;

    // Surface vessel or creature (0=Duck, 1=1700s Ship, 2=Whale)
    s.surface.type = rand() % 3;
    bool goRight = (rand() % 2 == 0);
    float baseSpd = (s.surface.type == 1) ? 0.18f : (s.surface.type == 2 ? 0.22f : 0.25f);
    s.surface.vx = goRight ? baseSpd : -baseSpd;
    s.surface.x = goRight ? -15.0f : (float)(cols + 5);
    s.surface.active = true;
    s.surface.animTimer = 0.0f;

    s.lastTick = GetTickCount();
    s.initialized = true;
}

void RenderASCIIQuarium(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& s = data->GetCustomState<AquaState>(24);

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

    if (!s.initialized || s.widthInChars != cols || s.heightInChars != rows) {
        InitAquarium(s, cols, rows);
    }

    DWORD now = GetTickCount();
    float dt = (now - s.lastTick) / 1000.0f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.033f;
    s.lastTick = now;
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

    int waveRow = (rows < 22) ? 4 : 6;

    // 1. Water Surface & Waves (4-tier wave and underwater chop from Asciiquarium)
    static const char* WAVE_SEGMENTS[4] = {
        "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
        "^^^^ ^^^  ^^^   ^^^    ^^^^      ",
        "^^^^      ^^^^     ^^^    ^^     ",
        "^^      ^^^^      ^^^    ^^^^^^  "
    };
    static const int WAVE_SEG_LEN = 33;

    // Surface wave line
    float wavePhase = timeVal * 1.5f;
    for (int x = 0; x < cols; ++x) {
        float wave = sinf(wavePhase + x * 0.35f);
        COLORREF wCol = (wave > 0.4f) ? COL_WAVE_TOP : COL_WAVE;
        putChar(x, waveRow, '~', wCol);
    }

    // 3 sub-surface ripple / chop tiers
    int waveOffsets[3] = {
        (int)(timeVal * 1.8f),
        (int)(timeVal * 1.3f),
        (int)(timeVal * 0.9f)
    };
    for (int r = 0; r < 3; ++r) {
        int targetY = waveRow + 1 + r;
        if (targetY >= rows - 4) break;
        const char* seg = WAVE_SEGMENTS[r + 1];
        int off = waveOffsets[r];
        for (int x = 0; x < cols; ++x) {
            int idx = (x + off) % WAVE_SEG_LEN;
            if (idx < 0) idx += WAVE_SEG_LEN;
            char c = seg[idx];
            if (c != ' ') {
                putChar(x, targetY, c, COL_WAVE);
            }
        }
    }

    // Surface entity (Duck, 1700s Sailing Ship, or Surfacing Whale)
    if (s.surface.active) {
        s.surface.animTimer += dt;
        s.surface.x += s.surface.vx * (dt * 30.0f);
        int sx = (int)roundf(s.surface.x);
        bool goingRight = (s.surface.vx > 0.0f);

        switch (s.surface.type) {
            case 0: { // 1. Rubber Duck (bottom row on water surface waveRow)
                if (goingRight) {
                    // Clear background behind duck at waveRow
                    for (int i = 0; i <= 7; ++i) {
                        int dx = sx + i;
                        if (dx >= 0 && dx < cols) grid[waveRow * cols + dx].ch = ' ';
                    }

                    putStr(sx + 3, waveRow - 2, "__", COL_DUCK);
                    putStr(sx,     waveRow - 1, "___( ", COL_DUCK);
                    putChar(sx + 5, waveRow - 1, 'o', COL_CRAB_EYE);
                    putStr(sx + 6, waveRow - 1, ")>", COL_BEAK);
                    putStr(sx,     waveRow,     "\\ <_. )", COL_DUCK);
                } else {
                    // Clear background behind duck at waveRow
                    for (int i = 1; i <= 7; ++i) {
                        int dx = sx + i;
                        if (dx >= 0 && dx < cols) grid[waveRow * cols + dx].ch = ' ';
                    }

                    putStr(sx + 2, waveRow - 2, "__", COL_DUCK);
                    putChar(sx,    waveRow - 1, '<', COL_BEAK);
                    putStr(sx + 1, waveRow - 1, "(o )___", COL_DUCK);
                    putStr(sx + 1, waveRow,     "( ._> /", COL_DUCK);
                }
                break;
            }
            case 1: { // 2. 1700s Wooden Sailing Ship
                if (goingRight) {
                    // Mast tips
                    putChar(sx + 5,  waveRow - 5, '|', COL_SHIP_WOOD);
                    putChar(sx + 10, waveRow - 5, '|', COL_SHIP_WOOD);
                    putChar(sx + 15, waveRow - 5, '|', COL_SHIP_WOOD);

                    // Top sails
                    putStr(sx + 4,   waveRow - 4, ")_)  )_)  )_)", COL_SHIP_SAIL);

                    // Mid sails + rigging
                    putStr(sx + 3,   waveRow - 3, ")___))___))___)\\", COL_SHIP_SAIL);

                    // Lower sails + rigging
                    putStr(sx + 2,   waveRow - 2, ")____)____)_____)\\\\", COL_SHIP_SAIL);

                    // Deck line: wood deck with white rigging \\\ at sx+20..22 and wood at end
                    putStr(sx,       waveRow - 1, "_____|____|____|____", COL_SHIP_WOOD);
                    putStr(sx + 20,  waveRow - 1, "\\\\\\", COL_SHIP_SAIL);
                    putStr(sx + 23,  waveRow - 1, "__", COL_SHIP_WOOD);

                    // Clear water waves inside hull so ship is cleanly over waves
                    for (int i = 1; i < 20; ++i) {
                        int hx = sx + i;
                        if (hx >= 0 && hx < cols) grid[waveRow * cols + hx].ch = ' ';
                    }

                    // Hull at water level (straight water border forms the bottom)
                    putChar(sx,      waveRow, '\\', COL_SHIP_WOOD);
                    putChar(sx + 20, waveRow, '/',  COL_SHIP_WOOD);
                } else {
                    // Mast tips
                    putChar(sx + 9,  waveRow - 5, '|', COL_SHIP_WOOD);
                    putChar(sx + 14, waveRow - 5, '|', COL_SHIP_WOOD);
                    putChar(sx + 19, waveRow - 5, '|', COL_SHIP_WOOD);

                    // Top sails
                    putStr(sx + 8,   waveRow - 4, "(_(  (_(  (_(", COL_SHIP_SAIL);

                    // Mid sails + rigging
                    putStr(sx + 6,   waveRow - 3, "/(___((___((___(", COL_SHIP_SAIL);

                    // Lower sails + rigging
                    putStr(sx + 4,   waveRow - 2, "//(_____(____(____(", COL_SHIP_SAIL);

                    // Deck line: wood deck with white rigging /// at sx+2..4
                    putStr(sx,       waveRow - 1, "__", COL_SHIP_WOOD);
                    putStr(sx + 2,   waveRow - 1, "///", COL_SHIP_SAIL);
                    putStr(sx + 5,   waveRow - 1, "____|____|____|_____", COL_SHIP_WOOD);

                    // Clear water waves inside hull so ship is cleanly over waves
                    for (int i = 5; i < 24; ++i) {
                        int hx = sx + i;
                        if (hx >= 0 && hx < cols) grid[waveRow * cols + hx].ch = ' ';
                    }

                    // Hull at water level (straight water border forms the bottom)
                    putChar(sx + 4,  waveRow, '\\', COL_SHIP_WOOD);
                    putChar(sx + 24, waveRow, '/',  COL_SHIP_WOOD);
                }
                break;
            }
            case 2: { // 3. Surfacing Whale with Spout
                // Animated water spout (7 frames from ASCIIquarium)
                float spoutTimer = fmodf(s.surface.animTimer, 4.0f);
                if (spoutTimer >= 0.8f && spoutTimer < 2.9f) {
                    int frame = (int)((spoutTimer - 0.8f) / 0.3f);
                    int bx = goingRight ? (sx + 14) : (sx + 4);
                    switch (frame) {
                        case 0:
                            putChar(bx, waveRow - 3, ':', COL_WHALE_SPOUT);
                            break;
                        case 1:
                            putChar(bx, waveRow - 4, ':', COL_WHALE_SPOUT);
                            putChar(bx, waveRow - 3, ':', COL_WHALE_SPOUT);
                            break;
                        case 2:
                            putStr(bx - 1, waveRow - 5, ". .", COL_WHALE_SPOUT);
                            putStr(bx - 1, waveRow - 4, "-:-", COL_WHALE_SPOUT);
                            putChar(bx,     waveRow - 3, ':',   COL_WHALE_SPOUT);
                            break;
                        case 3:
                            putStr(bx - 1, waveRow - 5, ". .",   COL_WHALE_SPOUT);
                            putStr(bx - 2, waveRow - 4, ".-:-.", COL_WHALE_SPOUT);
                            putChar(bx,     waveRow - 3, ':',     COL_WHALE_SPOUT);
                            break;
                        case 4:
                            putStr(bx - 1, waveRow - 5, ". .",     COL_WHALE_SPOUT);
                            putStr(bx - 3, waveRow - 4, "'.-:-.`", COL_WHALE_SPOUT);
                            putStr(bx - 3, waveRow - 3, "'  :  '", COL_WHALE_SPOUT);
                            break;
                        case 5:
                            putStr(bx - 2, waveRow - 4, ".- -.",   COL_WHALE_SPOUT);
                            putStr(bx - 3, waveRow - 3, ";  :  ;", COL_WHALE_SPOUT);
                            break;
                        case 6:
                        default:
                            putStr(bx - 3, waveRow - 3, ";     ;", COL_WHALE_SPOUT);
                            break;
                    }
                }

                if (goingRight) {
                    // Whale swimming RIGHT: Head on RIGHT, tail flukes on LEFT
                    putStr(sx, waveRow - 2, "        .-----:", COL_WHALE_BODY);
                    putStr(sx, waveRow - 1, "      .'       `.", COL_WHALE_BODY);

                    // Clear only inside body texture at waveRow (sx+6..12 and sx+16)
                    // Water background between tail fluke (sx) and body slope (sx+5) remains visible at sx+1..4
                    for (int i = 6; i <= 12; ++i) {
                        int wx = sx + i;
                        if (wx >= 0 && wx < cols) grid[waveRow * cols + wx].ch = ' ';
                    }
                    if (sx + 16 >= 0 && sx + 16 < cols) grid[waveRow * cols + sx + 16].ch = ' ';

                    // Waterline row (waveRow): tail tip, slope, eye (o), head slope
                    putChar(sx,      waveRow, ',', COL_WHALE_BODY);
                    putChar(sx + 5,  waveRow, '/', COL_WHALE_BODY);
                    putChar(sx + 13, waveRow, '(', COL_WHALE_BODY);
                    putChar(sx + 14, waveRow, 'o', COL_WHALE_EYE);
                    putChar(sx + 15, waveRow, ')', COL_WHALE_BODY);
                    putChar(sx + 17, waveRow, '\\', COL_WHALE_BODY);

                    // Clear only inside belly texture at waveRow + 1 (sx+5..14)
                    for (int i = 5; i <= 14; ++i) {
                        int wx = sx + i;
                        if (wx >= 0 && wx < cols && waveRow + 1 < rows) {
                            grid[(waveRow + 1) * cols + wx].ch = ' ';
                        }
                    }

                    // Underwater row (waveRow + 1)
                    putStr(sx,      waveRow + 1, "\\`._/", COL_WHALE_BODY);
                    putStr(sx + 15, waveRow + 1, ",__)",   COL_WHALE_BODY);
                } else {
                    // Whale swimming LEFT: Head on LEFT, tail flukes on RIGHT
                    putStr(sx, waveRow - 2, "    :-----.", COL_WHALE_BODY);
                    putStr(sx, waveRow - 1, "  .'       `.", COL_WHALE_BODY);

                    // Clear only inside body texture at waveRow (sx+2 and sx+6..12)
                    // Water background between body slope (sx+13) and tail fluke (sx+18) remains visible at sx+14..17
                    if (sx + 2 >= 0 && sx + 2 < cols) grid[waveRow * cols + sx + 2].ch = ' ';
                    for (int i = 6; i <= 12; ++i) {
                        int wx = sx + i;
                        if (wx >= 0 && wx < cols) grid[waveRow * cols + wx].ch = ' ';
                    }

                    // Waterline row (waveRow): head slope, eye (o), back slope, tail tip
                    putChar(sx + 1,  waveRow, '/',  COL_WHALE_BODY);
                    putChar(sx + 3,  waveRow, '(',  COL_WHALE_BODY);
                    putChar(sx + 4,  waveRow, 'o',  COL_WHALE_EYE);
                    putChar(sx + 5,  waveRow, ')',  COL_WHALE_BODY);
                    putChar(sx + 13, waveRow, '\\', COL_WHALE_BODY);
                    putChar(sx + 18, waveRow, ',',  COL_WHALE_BODY);

                    // Clear only inside belly texture at waveRow + 1 (sx+4..13)
                    for (int i = 4; i <= 13; ++i) {
                        int wx = sx + i;
                        if (wx >= 0 && wx < cols && waveRow + 1 < rows) {
                            grid[(waveRow + 1) * cols + wx].ch = ' ';
                        }
                    }

                    // Underwater row (waveRow + 1)
                    putStr(sx,      waveRow + 1, "(__,",   COL_WHALE_BODY);
                    putStr(sx + 14, waveRow + 1, "\\_.'/", COL_WHALE_BODY);
                }
                break;
            }
        }

        if (goingRight && s.surface.x > cols + 35) {
            s.surface.active = false;
        } else if (!goingRight && s.surface.x < -35.0f) {
            s.surface.active = false;
        }
    } else if (rand() % 30 == 0) {
        s.surface.active = true;
        s.surface.type = (s.surface.type + 1 + (rand() % 2)) % 3;
        bool goRight = (rand() % 2 == 0);
        float baseSpd = (s.surface.type == 1) ? 0.18f : (s.surface.type == 2 ? 0.22f : 0.25f);
        s.surface.vx = goRight ? baseSpd : -baseSpd;
        s.surface.x = goRight ? -30.0f : (float)(cols + 30);
        s.surface.animTimer = 0.0f;
    }

    // 2. Seabed (Sand & Pebbles)
    int sandRow = rows - 2;
    for (int x = 0; x < cols; ++x) {
        char s1 = (x % 3 == 0) ? '~' : ((x % 5 == 0) ? '^' : '_');
        char s2 = (x % 4 == 0) ? 'w' : ((x % 7 == 0) ? 'm' : '~');
        putChar(x, sandRow, s1, COL_SAND1);
        putChar(x, sandRow + 1, s2, COL_SAND2);
    }

    // 2b. Sand Castle on Seabed (Classic Joan Stark Asciiquarium Castle)
    if (cols >= 36 && rows >= 18) {
        int castleW = 31;
        int castleH = 13;
        int castleX = cols - castleW - 4;
        if (castleX < 2) castleX = 2;
        int castleY = sandRow - castleH + 1;

        static const char* CASTLE_SHAPE[13] = {
            "               T~~             ",
            "               |               ",
            "              /^\\              ",
            "             /   \\             ",
            " _   _   _  /     \\  _   _   _ ",
            "[ ]_[ ]_[ ]/ _   _ \\[ ]_[ ]_[ ]",
            "|_=__-_ =_|_[ ]_[ ]_|_=-___-__|",
            " | _- =  | =_ = _    |= _=   | ",
            " |= -[]  |- = _ =    |_-=_[] | ",
            " | =_    |= - ___    | =_ =  | ",
            " |=  []- |-  /| |\\   |=_ =[] | ",
            " |- =_   | =| | | |  |- = -  | ",
            " |_______|__|_|_|_|__|_______| "
        };

        static const char* CASTLE_MASK[13] = {
            "                RR             ",
            "                w              ",
            "              yyy              ",
            "             y   y             ",
            "            y     y            ",
            "           y       y           ",
            "                               ",
            "                               ",
            "                               ",
            "              yyy              ",
            "             yy yy             ",
            "            y y y y            ",
            "            yyyyyyy            "
        };

        for (int r = 0; r < castleH; ++r) {
            int cy = castleY + r;
            if (cy <= waveRow + 3 || cy >= rows) continue;
            for (int c = 0; c < castleW; ++c) {
                char ch = CASTLE_SHAPE[r][c];
                if (ch != ' ') {
                    char m = CASTLE_MASK[r][c];
                    COLORREF col = COL_CASTLE_WALL;
                    if (m == 'R') col = COL_CASTLE_FLAG;
                    else if (m == 'y') col = COL_CASTLE_ROOF;
                    putChar(castleX + c, cy, ch, col);
                }
            }
        }
    }

    // 3. Seaweed Swaying
    for (const auto& sw : s.seaweed) {
        for (int h = 0; h < sw.height; ++h) {
            int curY = sandRow - 1 - h;
            if (curY <= waveRow) break;
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
    if (rand() % 15 == 0 && !s.seaweed.empty()) {
        int swIdx = rand() % s.seaweed.size();
        AquaBubble b;
        b.x = (float)s.seaweed[swIdx].x;
        b.y = (float)(sandRow - 2);
        b.speed = 0.35f + (float)(rand() % 30) / 100.0f;
        b.swaySpeed = 2.0f + (float)(rand() % 20) / 10.0f;
        b.swayPhase = (float)(rand() % 628) / 100.0f;
        int rType = rand() % 10;
        b.type = (rType < 5) ? 0 : (rType < 8 ? 1 : 2); // 0='.', 1='o', 2='O'
        s.bubbles.push_back(b);
    }

    // 5. Jellyfish
    for (auto& j : s.jelly) {
        float pulseSpeed = (j.state == 0) ? 2.3f : 1.6f;
        j.pulsePhase += dt * pulseSpeed;
        float pulse = sinf(j.pulsePhase);

        if (j.state == 0) {
            // Swimming UP:
            // Upward propulsion during contraction pulse, slower upward glide during expansion
            float thrust = (pulse > 0.1f) ? j.vy : (j.vy * 0.25f);
            j.y += thrust * (dt * 30.0f);

            // Reached apex below waves: transition to gentle descent
            if (j.y <= j.topLimit) {
                j.state = 1;
                j.topLimit = (float)(rand() % (std::max)(1, (rows - waveRow) / 4) + waveRow + 5);
            }
        } else {
            // Drifting / Swimming DOWN:
            // Sinks downwards with gentle pulses slowing the sink rate
            float sinkRate = (pulse > 0.2f) ? 0.04f : 0.15f;
            j.y += sinkRate * (dt * 30.0f);

            // Exited completely below screen: respawn smoothly from below
            if (j.y > rows + 6) {
                j.y = (float)(rows + 2 + rand() % 5);
                j.baseX = (float)(rand() % (cols - 14) + 3);
                j.state = 0;
                j.vy = -0.16f - (float)(rand() % 10) / 100.0f;
                j.swayOffset = (float)(rand() % 628) / 100.0f;
                j.topLimit = (float)(rand() % (std::max)(1, (rows - waveRow) / 4) + waveRow + 5);
                j.color = JELLY_COLORS[rand() % NUM_JELLY_COLORS];
            }
        }

        // Horizontal sway around baseX (no accumulation drift or truncation flickering)
        float sway = sinf(j.pulsePhase * 0.5f + j.swayOffset) * 1.5f;
        j.x = j.baseX + sway;

        int jx = (int)roundf(j.x);
        int jy = (int)roundf(j.y);

        if (pulse > 0.0f) {
            // Expanded bell (vertically symmetric, centered at jx + 4.5)
            putStr(jx + 2, jy,     ".-\"\"-.", j.color);
            putStr(jx + 1, jy + 1, "/      \\", j.color);
            putStr(jx,     jy + 2, "(`~~~~~~`)", j.color);
            putStr(jx + 1, jy + 3, "|| || ||", j.color);
            putStr(jx + 1, jy + 4, "|| || ||", j.color);
        } else {
            // Contracted bell (vertically symmetric, centered at jx + 4.5)
            putStr(jx + 3, jy,     "_.._", j.color);
            putStr(jx + 2, jy + 1, "/    \\", j.color);
            putStr(jx + 1, jy + 2, "(~~~~~~)", j.color);
            putStr(jx + 1, jy + 3, "// || \\\\", j.color);
            putStr(jx + 1, jy + 4, "// || \\\\", j.color);
        }
    }

    // 6. Crab on Seabed
    {
        auto& crab = s.crab;
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
    for (auto& f : s.fish) {
        f.x += f.vx * (dt * 30.0f);
        bool goingRight = (f.vx > 0.0f);

        // Turnaround / Wrap-around
        if (goingRight && f.x > cols + 15) {
            f.x = -15.0f;
            int minFishY = waveRow + 5;
            int maxFishY = rows - 5;
            if (maxFishY <= minFishY) maxFishY = minFishY + 1;
            f.y = (float)(rand() % (maxFishY - minFishY + 1) + minFishY);
            f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        } else if (!goingRight && f.x < -15.0f) {
            f.x = (float)(cols + 15);
            int minFishY = waveRow + 5;
            int maxFishY = rows - 5;
            if (maxFishY <= minFishY) maxFishY = minFishY + 1;
            f.y = (float)(rand() % (maxFishY - minFishY + 1) + minFishY);
            f.color = FISH_COLORS[rand() % NUM_FISH_COLORS];
        }

        // Random occasional mouth bubbles
        if (rand() % 120 == 0 && f.x > 2 && f.x < cols - 2) {
            AquaBubble b;
            b.x = goingRight ? f.x + 5.0f : f.x - 1.0f;
            b.y = f.y;
            b.speed = 0.4f + (float)(rand() % 25) / 100.0f;
            b.swaySpeed = 2.5f;
            b.swayPhase = (float)(rand() % 628) / 100.0f;
            b.type = (rand() % 2 == 0) ? 0 : 1;
            s.bubbles.push_back(b);
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

            case 1: // Guppy / Tetra
                if (goingRight) {
                    putStr(fx, fy, "><((('>", f.color); // tail left, body, head right
                } else {
                    putStr(fx, fy, "<')))><", f.color); // head left, body, tail right
                }
                break;

            case 2: // Classic Fish (5 rows, clearly directional)
                if (goingRight) {
                    putStr(fx + 4, fy - 1, "\\",       f.color);
                    putStr(fx,     fy,     "\\ /--\\", f.color);
                    putStr(fx,     fy + 1, ">=  (o>",  f.color);
                    putStr(fx,     fy + 2, "/ \\__/",  f.color);
                    putStr(fx + 4, fy + 3, "/",        f.color);
                    putChar(fx + 5, fy + 1, 'o', COL_CRAB_EYE);
                } else {
                    putStr(fx + 2, fy - 1, "/",        f.color);
                    putStr(fx + 1, fy,     "/--\\ /",  f.color);
                    putStr(fx,     fy + 1, "<o)  =<",  f.color);
                    putStr(fx + 1, fy + 2, "\\__/ \\", f.color);
                    putStr(fx + 2, fy + 3, "\\",        f.color);
                    putChar(fx + 1, fy + 1, 'o', COL_CRAB_EYE);
                }
                break;

            case 3: // Classic Tetra Fish (5 rows, clearly directional)
                if (goingRight) {
                    putStr(fx + 3, fy - 1, "\\",     f.color);
                    putStr(fx + 2, fy,     "/ \\",   f.color);
                    putStr(fx,     fy + 1, ">=_('>", f.color);
                    putStr(fx + 2, fy + 2, "\\_/",   f.color);
                    putStr(fx + 3, fy + 3, "/",      f.color);
                    putChar(fx + 4, fy + 1, '\'', COL_CRAB_EYE);
                } else {
                    putStr(fx + 2, fy - 1, "/",      f.color);
                    putStr(fx + 1, fy,     "/ \\",   f.color);
                    putStr(fx,     fy + 1, "<')_=<", f.color);
                    putStr(fx + 1, fy + 2, "\\_/",   f.color);
                    putStr(fx + 2, fy + 3, "\\",      f.color);
                    putChar(fx + 1, fy + 1, '\'', COL_CRAB_EYE);
                }
                break;

            case 4: // Shark (3 rows, clearly directional)
            default:
                if (goingRight) {
                    // Head > on RIGHT, tail == on LEFT, dorsal fin on top
                    putStr(fx + 4, fy,     "/\\",        f.color); // dorsal fin
                    putStr(fx,     fy + 1, "====( o)-->", f.color); // body: tail left, eye, snout right
                    putStr(fx + 4, fy + 2, "\\/",        f.color); // pectoral fin
                    putChar(fx + 6, fy + 1, 'o', COL_CRAB_EYE);
                } else {
                    // Head < on LEFT, tail == on RIGHT
                    putStr(fx + 3, fy,     "/\\",        f.color); // dorsal fin
                    putStr(fx,     fy + 1, "<--(o )===", f.color); // snout left, eye, tail right
                    putStr(fx + 3, fy + 2, "\\/",        f.color); // pectoral fin
                    putChar(fx + 4, fy + 1, 'o', COL_CRAB_EYE);
                }
                break;
        }
    }

    // 8. Bubbles Update & Rendering
    static const char BUBBLE_CHARS[] = { '.', 'o', 'O' };
    for (size_t i = 0; i < s.bubbles.size(); ) {
        auto& b = s.bubbles[i];
        b.y -= b.speed * (dt * 30.0f);
        b.swayPhase += dt * b.swaySpeed;
        int bx = (int)roundf(b.x + sinf(b.swayPhase) * 1.4f);
        int by = (int)b.y;

        if (by <= waveRow) {
            // Pop at water surface
            s.bubbles.erase(s.bubbles.begin() + i);
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

REGISTER_SCREENSAVER(
    24,
    L"ASCIIQuarium",
    "asciiquarium",
    { "asciiquarium", "aquarium", "fish" },
    WRAP_LEGACY(RenderASCIIQuarium),
    {}
);