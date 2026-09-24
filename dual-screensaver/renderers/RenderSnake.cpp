#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic 1980s Retro Arcade Snake Simulation
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio (e.g. 455x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;
static constexpr int CELL_SIZE     = 8; // 8x8 pixels per grid tile

// 32-bit ARGB Palette
static constexpr uint32_t COL_BLACK          = 0xFF040608;
static constexpr uint32_t COL_WHITE          = 0xFFFFFFFF;
static constexpr uint32_t COL_YELLOW         = 0xFFFFFF00;
static constexpr uint32_t COL_GOLD           = 0xFFFFD700;
static constexpr uint32_t COL_RED            = 0xFFFF2020;
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744;
static constexpr uint32_t COL_CYAN           = 0xFF00FFFF;
static constexpr uint32_t COL_ORANGE         = 0xFFFF9100;
static constexpr uint32_t COL_GREEN_LIME     = 0xFF00FF66;
static constexpr uint32_t COL_GREEN_EMERALD  = 0xFF00E676;
static constexpr uint32_t COL_GREEN_DARK     = 0xFF008833;
static constexpr uint32_t COL_GREEN_DEEP     = 0xFF005520;
static constexpr uint32_t COL_GREEN_HIGHLIGHT= 0xFF69F0AE;
static constexpr uint32_t COL_GRAY_LIGHT     = 0xFFB0B0C0;
static constexpr uint32_t COL_GRAY_DARK      = 0xFF353545;
static constexpr uint32_t COL_FLOOR_DOT      = 0xFF0E1A12;
static constexpr uint32_t COL_BORDER_OUTER   = 0xFF008833;
static constexpr uint32_t COL_BORDER_INNER   = 0xFF00FF66;

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
    case '!': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    case ':': { static const uint8_t g[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }; return g; }
    case '/': { static const uint8_t g[7] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 }; return g; }
    case '+': { static const uint8_t g[7] = { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }; return g; }
    case '.': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C }; return g; }
    case '<': { static const uint8_t g[7] = { 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 }; return g; }
    case '>': { static const uint8_t g[7] = { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 }; return g; }
    case '%': { static const uint8_t g[7] = { 0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13 }; return g; }
    default:  return GLYPH_BLANK;
    }
}

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================

struct Sparkle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    uint32_t color;
};

struct ScorePopup {
    float x, y;
    int score;
    int timer;
    uint32_t color;
};

struct SnakeState {
    bool initialized = false;
    uint64_t lastTick = 0;
    int virtualW = 455;
    int virtualH = BASE_ARCADE_H;
    std::vector<uint32_t> fb;

    // Grid Dimensions
    int cols = 0;
    int rows = 0;
    int totalCells = 0;
    int playLeft = 0;
    int playTop = 28;

    // Gameplay State
    int score = 0;
    int highScore = 5000;
    int applesEaten = 0;
    int wave = 1;
    bool isWon = false;
    uint64_t winStartTime = 0;
    int stuckFrames = 0;
    uint64_t timeAccumulator = 0;

    // Snake Body & AI
    std::vector<POINT> body;
    std::vector<int> cycle;
    std::vector<uint8_t> occupied;

    // Digestion Ripple pulses traveling down snake
    std::vector<int> foodPulses;

    // Food State
    POINT food = { 0, 0 };
    int foodType = 0;  // 0: Red Apple, 1: Golden Apple, 2: Pac-Man Cherry
    int foodScore = 100;
    int bonusTimer = 0;
    int tongueTimer = 0;

    // Particle Sparkles & Floating Score Popups
    std::vector<Sparkle> sparkles;
    std::vector<ScorePopup> popups;
};

// ============================================================================
// FRAMEBUFFER DRAWING UTILITIES
// ============================================================================

static inline void PutPixel(SnakeState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static inline void DrawHLine(SnakeState& s, int x1, int x2, int y, uint32_t col) {
    if (y < 0 || y >= s.virtualH) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = (std::max)(0, x1);
    x2 = (std::min)(s.virtualW - 1, x2);
    for (int x = x1; x <= x2; ++x) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static inline void DrawVLine(SnakeState& s, int x, int y1, int y2, uint32_t col) {
    if (x < 0 || x >= s.virtualW) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = (std::max)(0, y1);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static void DrawChar(SnakeState& s, int px, int py, char c, uint32_t color) {
    const uint8_t* g = GetGlyph(c);
    for (int r = 0; r < 7; ++r) {
        uint8_t row = g[r];
        for (int col = 0; col < 5; ++col) {
            if ((row >> (4 - col)) & 1) {
                PutPixel(s, px + col, py + r, color);
            }
        }
    }
}

static void DrawText(SnakeState& s, int px, int py, const char* str, uint32_t color) {
    int x = px;
    while (*str) {
        DrawChar(s, x, py, *str, color);
        x += 6;
        str++;
    }
}

// ============================================================================
// PIXEL-ART SPRITES & TEXTURES
// ============================================================================

// 1. Food Sprites: 0 = Red Apple, 1 = Golden Apple, 2 = Namco Pac-Man Cherry
static void DrawFood(SnakeState& s, int px, int py, int type, int animFrame) {
    if (type == 0) {
        // Red Arcade Apple (8x8)
        PutPixel(s, px + 3, py + 0, 0xFF8D6E63); // Brown Stem
        PutPixel(s, px + 4, py + 0, 0xFF00FF66); // Leaf
        PutPixel(s, px + 5, py + 0, 0xFF69F0AE); // Leaf highlight
        PutPixel(s, px + 3, py + 1, 0xFF8D6E63);
        PutPixel(s, px + 4, py + 1, 0xFF00E676);

        // Apple shoulder
        for (int x = 1; x <= 6; ++x) {
            if (x != 3) PutPixel(s, px + x, py + 2, 0xFFFF2040);
        }
        PutPixel(s, px + 3, py + 2, 0xFFC2185B); // Stem indent shadow

        // Core body & specular glint
        PutPixel(s, px + 0, py + 3, 0xFFB71C1C);
        PutPixel(s, px + 1, py + 3, 0xFFFF5252);
        PutPixel(s, px + 2, py + 3, 0xFFFFFFFF); // White shine
        for (int x = 3; x <= 6; ++x) PutPixel(s, px + x, py + 3, 0xFFFF2040);
        PutPixel(s, px + 7, py + 3, 0xFFB71C1C);

        PutPixel(s, px + 0, py + 4, 0xFFB71C1C);
        PutPixel(s, px + 1, py + 4, 0xFFFFFFFF); // White shine
        for (int x = 2; x <= 6; ++x) PutPixel(s, px + x, py + 4, 0xFFFF2040);
        PutPixel(s, px + 7, py + 4, 0xFFB71C1C);

        PutPixel(s, px + 0, py + 5, 0xFFB71C1C);
        for (int x = 1; x <= 6; ++x) PutPixel(s, px + x, py + 5, 0xFFFF2040);
        PutPixel(s, px + 7, py + 5, 0xFFB71C1C);

        for (int x = 1; x <= 6; ++x) PutPixel(s, px + x, py + 6, 0xFFFF2040);

        PutPixel(s, px + 2, py + 7, 0xFFB71C1C);
        PutPixel(s, px + 3, py + 7, 0xFF880E4F);
        PutPixel(s, px + 4, py + 7, 0xFF880E4F);
        PutPixel(s, px + 5, py + 7, 0xFFB71C1C);
    } else if (type == 1) {
        // Golden Apple (8x8) with dynamic shimmer sparkle
        PutPixel(s, px + 3, py + 0, 0xFFFFD700);
        PutPixel(s, px + 4, py + 0, 0xFF00FF66);
        PutPixel(s, px + 5, py + 0, 0xFF69F0AE);
        PutPixel(s, px + 3, py + 1, 0xFFFFD700);
        PutPixel(s, px + 4, py + 1, 0xFF00E676);

        for (int x = 1; x <= 6; ++x) {
            if (x != 3) PutPixel(s, px + x, py + 2, 0xFFFFD700);
        }
        PutPixel(s, px + 3, py + 2, 0xFFFF8F00);

        PutPixel(s, px + 0, py + 3, 0xFFFF8F00);
        PutPixel(s, px + 1, py + 3, 0xFFFFF59D);
        PutPixel(s, px + 2, py + 3, 0xFFFFFFFF);
        for (int x = 3; x <= 6; ++x) PutPixel(s, px + x, py + 3, 0xFFFFD700);
        PutPixel(s, px + 7, py + 3, 0xFFFF8F00);

        PutPixel(s, px + 0, py + 4, 0xFFFF8F00);
        PutPixel(s, px + 1, py + 4, 0xFFFFFFFF);
        for (int x = 2; x <= 6; ++x) PutPixel(s, px + x, py + 4, 0xFFFFD700);
        PutPixel(s, px + 7, py + 4, 0xFFFF8F00);

        PutPixel(s, px + 0, py + 5, 0xFFFF8F00);
        for (int x = 1; x <= 6; ++x) PutPixel(s, px + x, py + 5, 0xFFFFD700);
        PutPixel(s, px + 7, py + 5, 0xFFFF8F00);

        for (int x = 1; x <= 6; ++x) PutPixel(s, px + x, py + 6, 0xFFFFD700);

        PutPixel(s, px + 2, py + 7, 0xFFFF8F00);
        PutPixel(s, px + 3, py + 7, 0xFFFF6F00);
        PutPixel(s, px + 4, py + 7, 0xFFFF6F00);
        PutPixel(s, px + 5, py + 7, 0xFFFF8F00);

        // Dynamic twinkle sparkle
        if ((animFrame / 6) % 2 == 0) {
            PutPixel(s, px + 6, py + 1, 0xFFFFFFFF);
            PutPixel(s, px + 7, py + 1, 0xFFFFF59D);
            PutPixel(s, px + 6, py + 0, 0xFFFFF59D);
        }
    } else {
        // Arcade Twin Cherries (Pac-Man Homage, 8x8)
        PutPixel(s, px + 3, py + 0, 0xFF00E676);
        PutPixel(s, px + 4, py + 0, 0xFF00E676);
        PutPixel(s, px + 2, py + 1, 0xFF00E676);
        PutPixel(s, px + 5, py + 1, 0xFF00E676);
        PutPixel(s, px + 1, py + 2, 0xFF00E676);
        PutPixel(s, px + 6, py + 2, 0xFF00E676);

        // Cherry Left
        PutPixel(s, px + 0, py + 3, 0xFFFF2040);
        PutPixel(s, px + 1, py + 3, 0xFFFF5252);
        PutPixel(s, px + 2, py + 3, 0xFFFF2040);
        PutPixel(s, px + 0, py + 4, 0xFFFF2040);
        PutPixel(s, px + 1, py + 4, 0xFFFFFFFF);
        PutPixel(s, px + 2, py + 4, 0xFFFF2040);
        PutPixel(s, px + 1, py + 5, 0xFFB71C1C);

        // Cherry Right
        PutPixel(s, px + 5, py + 3, 0xFFFF2040);
        PutPixel(s, px + 6, py + 3, 0xFFFF5252);
        PutPixel(s, px + 7, py + 3, 0xFFFF2040);
        PutPixel(s, px + 5, py + 4, 0xFFFF2040);
        PutPixel(s, px + 6, py + 4, 0xFFFFFFFF);
        PutPixel(s, px + 7, py + 4, 0xFFFF2040);
        PutPixel(s, px + 6, py + 5, 0xFFB71C1C);
    }
}

// 2. Snake Head: 8x8 with expressive tracking eyes and animated forked tongue
static void DrawSnakeHead(SnakeState& s, int px, int py, int dir, bool tongueOut) {
    auto MapCoord = [px, py, dir](int u, int v, int& outX, int& outY) {
        switch (dir) {
        case 0: // DIR_RIGHT: u is X (neck->snout), v is Y (top->bottom)
            outX = px + u;
            outY = py + v;
            break;
        case 1: // DIR_DOWN: u is Y (neck->snout), v is X
            outX = px + v;
            outY = py + u;
            break;
        case 2: // DIR_LEFT: u is -X, v is Y
            outX = px + (7 - u);
            outY = py + v;
            break;
        case 3: // DIR_UP: u is -Y, v is X
        default:
            outX = px + v;
            outY = py + (7 - u);
            break;
        }
    };

    auto SetCanonical = [&](int u, int v, uint32_t col) {
        int x, y;
        MapCoord(u, v, x, y);
        PutPixel(s, x, y, col);
    };

    for (int v = 0; v < 8; ++v) {
        for (int u = 0; u < 8; ++u) {
            if (v == 0 || v == 7) {
                if (u >= 0 && u <= 5) SetCanonical(u, v, COL_GREEN_DEEP);
            } else if (v == 1 || v == 6) {
                if (u <= 1) SetCanonical(u, v, COL_GREEN_DARK);
                else if (u >= 2 && u <= 6) SetCanonical(u, v, COL_GREEN_EMERALD);
                else if (u == 7) SetCanonical(u, v, COL_GREEN_DEEP);
            } else if (v == 2 || v == 5) {
                // Eye rows
                if (u <= 1) SetCanonical(u, v, COL_GREEN_EMERALD);
                else if (u == 2) SetCanonical(u, v, COL_GREEN_DARK);
                else if (u == 3) SetCanonical(u, v, COL_WHITE); // White Sclera
                else if (u == 4) SetCanonical(u, v, COL_WHITE);
                else if (u == 5) SetCanonical(u, v, 0xFF051018); // Pupil facing movement!
                else if (u == 6) SetCanonical(u, v, COL_GREEN_EMERALD);
                else if (u == 7) SetCanonical(u, v, COL_GREEN_DEEP);
            } else {
                // Center snout & crest (v == 3 || v == 4)
                if (u == 0) SetCanonical(u, v, COL_GREEN_EMERALD);
                else if (u >= 1 && u <= 4) SetCanonical(u, v, COL_GREEN_HIGHLIGHT);
                else if (u == 5) SetCanonical(u, v, COL_GREEN_EMERALD);
                else if (u == 6) SetCanonical(u, v, COL_GREEN_DEEP); // Nostril
                else if (u == 7) SetCanonical(u, v, COL_GREEN_DARK);
            }
        }
    }

    // Animated forked red tongue extending beyond snout!
    if (tongueOut) {
        SetCanonical(8, 3, COL_CRIMSON);
        SetCanonical(8, 4, COL_CRIMSON);
        SetCanonical(9, 3, COL_CRIMSON);
        SetCanonical(9, 4, COL_CRIMSON);
        SetCanonical(10, 2, COL_CRIMSON); // Fork tip top
        SetCanonical(10, 5, COL_CRIMSON); // Fork tip bottom
    }
}

// 3. Straight Body Segments with scale rib textures & digestion pulses
static void DrawSnakeStraight(SnakeState& s, int px, int py, bool isHoriz, bool isPulse, bool isGoldShimmer) {
    uint32_t colOutline = isGoldShimmer ? 0xFFFF8F00 : (isPulse ? 0xFFFFAB00 : COL_GREEN_DEEP);
    uint32_t colBase    = isGoldShimmer ? 0xFFFFD700 : (isPulse ? 0xFFFFEA00 : COL_GREEN_EMERALD);
    uint32_t colCore    = isGoldShimmer ? 0xFFFFFFFF : (isPulse ? 0xFFFFFFFF : COL_GREEN_HIGHLIGHT);
    uint32_t colSeam    = isGoldShimmer ? 0xFFFF8F00 : (isPulse ? 0xFFFFC400 : COL_GREEN_DARK);

    if (isHoriz) {
        for (int x = 0; x < 8; ++x) {
            PutPixel(s, px + x, py + 0, colOutline);
            PutPixel(s, px + x, py + 1, colBase);
            PutPixel(s, px + x, py + 2, colBase);
            PutPixel(s, px + x, py + 3, (x % 4 == 0) ? colSeam : colCore);
            PutPixel(s, px + x, py + 4, (x % 4 == 0) ? colSeam : colCore);
            PutPixel(s, px + x, py + 5, colBase);
            PutPixel(s, px + x, py + 6, colBase);
            PutPixel(s, px + x, py + 7, colOutline);
        }
    } else {
        for (int y = 0; y < 8; ++y) {
            PutPixel(s, px + 0, py + y, colOutline);
            PutPixel(s, px + 1, py + y, colBase);
            PutPixel(s, px + 2, py + y, colBase);
            PutPixel(s, px + 3, py + y, (y % 4 == 0) ? colSeam : colCore);
            PutPixel(s, px + 4, py + y, (y % 4 == 0) ? colSeam : colCore);
            PutPixel(s, px + 5, py + y, colBase);
            PutPixel(s, px + 6, py + y, colBase);
            PutPixel(s, px + 7, py + y, colOutline);
        }
    }
}

// 4. Smooth 90-degree Corner Elbow Joints (SE, SW, NE, NW)
static void DrawSnakeCorner(SnakeState& s, int px, int py, int cornerType, bool isPulse, bool isGoldShimmer) {
    uint32_t colOutline = isGoldShimmer ? 0xFFFF8F00 : (isPulse ? 0xFFFFAB00 : COL_GREEN_DEEP);
    uint32_t colBase    = isGoldShimmer ? 0xFFFFD700 : (isPulse ? 0xFFFFEA00 : COL_GREEN_EMERALD);
    uint32_t colCore    = isGoldShimmer ? 0xFFFFFFFF : (isPulse ? 0xFFFFFFFF : COL_GREEN_HIGHLIGHT);

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            float u = (float)x;
            float v = (float)y;
            switch (cornerType) {
            case 0: // NE (connects UP and RIGHT)
                v = 7.0f - v;
                break;
            case 1: // NW (connects UP and LEFT)
                u = 7.0f - u;
                v = 7.0f - v;
                break;
            case 2: // SE (connects DOWN and RIGHT)
                break;
            case 3: // SW (connects DOWN and LEFT)
                u = 7.0f - u;
                break;
            }

            float r = sqrtf(u * u + v * v);
            if (r > 7.5f || r < 0.6f) continue;

            if (r >= 6.4f || r <= 1.4f) {
                PutPixel(s, px + x, py + y, colOutline);
            } else if (r >= 3.0f && r <= 4.8f) {
                PutPixel(s, px + x, py + y, colCore);
            } else {
                PutPixel(s, px + x, py + y, colBase);
            }
        }
    }
}

// 5. Tapered Tail Tip in all 4 Directions
static void DrawSnakeTail(SnakeState& s, int px, int py, int dir, bool isGoldShimmer) {
    uint32_t colOutline = isGoldShimmer ? 0xFFFF8F00 : COL_GREEN_DEEP;
    uint32_t colBase    = isGoldShimmer ? 0xFFFFD700 : COL_GREEN_EMERALD;
    uint32_t colCore    = isGoldShimmer ? 0xFFFFFFFF : COL_GREEN_HIGHLIGHT;

    auto MapCoord = [px, py, dir](int u, int v, int& outX, int& outY) {
        switch (dir) {
        case 0: // DIR_RIGHT
            outX = px + u;
            outY = py + v;
            break;
        case 1: // DIR_DOWN
            outX = px + v;
            outY = py + u;
            break;
        case 2: // DIR_LEFT
            outX = px + (7 - u);
            outY = py + v;
            break;
        case 3: // DIR_UP
        default:
            outX = px + v;
            outY = py + (7 - u);
            break;
        }
    };

    auto SetCanonical = [&](int u, int v, uint32_t col) {
        int x, y;
        MapCoord(u, v, x, y);
        PutPixel(s, x, y, col);
    };

    // Canonical: u=0 is wide neck, u=7 is pointed tail tip
    for (int u = 0; u < 8; ++u) {
        int halfW = (7 - u) / 2 + 1;
        if (halfW > 3) halfW = 3;
        if (u == 7) halfW = 0;

        for (int v = 0; v < 8; ++v) {
            int d = abs(v * 2 + 1 - 8);
            if (d / 2 < halfW) {
                if (d / 2 == halfW - 1 || u == 6) {
                    SetCanonical(u, v, colOutline);
                } else if (d / 2 == 0) {
                    SetCanonical(u, v, colCore);
                } else {
                    SetCanonical(u, v, colBase);
                }
            }
        }
    }
}

// 6. Double-Lined Arcade Border (Pac-Man & Space Invaders styling)
static void DrawArcadeBorder(SnakeState& s) {
    int x1 = s.playLeft - 4;
    int y1 = s.playTop - 4;
    int x2 = s.playLeft + s.cols * 8 + 3;
    int y2 = s.playTop + s.rows * 8 + 3;

    // Outer double-line border
    DrawHLine(s, x1 + 2, x2 - 2, y1, COL_BORDER_OUTER);
    DrawHLine(s, x1 + 2, x2 - 2, y2, COL_BORDER_OUTER);
    DrawVLine(s, x1, y1 + 2, y2 - 2, COL_BORDER_OUTER);
    DrawVLine(s, x2, y1 + 2, y2 - 2, COL_BORDER_OUTER);

    // Chamfered outer corners
    PutPixel(s, x1 + 1, y1 + 1, COL_BORDER_OUTER);
    PutPixel(s, x2 - 1, y1 + 1, COL_BORDER_OUTER);
    PutPixel(s, x1 + 1, y2 - 1, COL_BORDER_OUTER);
    PutPixel(s, x2 - 1, y2 - 1, COL_BORDER_OUTER);

    // Inner bright neon border
    int ix1 = x1 + 2;
    int iy1 = y1 + 2;
    int ix2 = x2 - 2;
    int iy2 = y2 - 2;
    DrawHLine(s, ix1 + 1, ix2 - 1, iy1, COL_BORDER_INNER);
    DrawHLine(s, ix1 + 1, ix2 - 1, iy2, COL_BORDER_INNER);
    DrawVLine(s, ix1, iy1 + 1, iy2 - 1, COL_BORDER_INNER);
    DrawVLine(s, ix2, iy1 + 1, iy2 - 1, COL_BORDER_INNER);
}

// 7. Subtle Arcade CRT Floor Grid Texture
static void DrawFloorTexture(SnakeState& s) {
    for (int y = 0; y < s.rows; ++y) {
        for (int x = 0; x < s.cols; ++x) {
            int px = s.playLeft + x * 8;
            int py = s.playTop + y * 8;
            // 2x2 subtle dot at cell center for authentic arcade raster feel
            PutPixel(s, px + 3, py + 3, COL_FLOOR_DOT);
            PutPixel(s, px + 4, py + 3, COL_FLOOR_DOT);
            PutPixel(s, px + 3, py + 4, COL_FLOOR_DOT);
            PutPixel(s, px + 4, py + 4, COL_FLOOR_DOT);
        }
    }
}

// ============================================================================
// HAMILTONIAN CYCLE AI SIMULATION
// ============================================================================

static void GenerateHamiltonianCycle(int cols, int rows, std::vector<int>& outCycle) {
    int K = cols / 2;
    int M = rows / 2;
    int totalFine = cols * rows;
    outCycle.assign(totalFine, 0);

    std::vector<std::vector<int>> fineAdj(totalFine);
    fineAdj.reserve(totalFine);

    auto fineIdx = [cols](int x, int y) { return y * cols + x; };

    // 1. Initialize each 2x2 coarse block with a disjoint 4-cycle
    for (int cy = 0; cy < M; ++cy) {
        for (int cx = 0; cx < K; ++cx) {
            int tl = fineIdx(2 * cx,     2 * cy);
            int tr = fineIdx(2 * cx + 1, 2 * cy);
            int br = fineIdx(2 * cx + 1, 2 * cy + 1);
            int bl = fineIdx(2 * cx,     2 * cy + 1);

            fineAdj[tl] = { tr, bl };
            fineAdj[tr] = { tl, br };
            fineAdj[br] = { tr, bl };
            fineAdj[bl] = { br, tl };
        }
    }

    // 2. Randomized spanning tree on K x M coarse grid
    struct CoarseEdge {
        int cx1, cy1, cx2, cy2;
        bool isHoriz;
    };
    std::vector<CoarseEdge> treeEdges;
    treeEdges.reserve(K * M - 1);

    std::vector<bool> visited(K * M, false);
    std::vector<int> stack;
    stack.reserve(K * M);

    visited[0] = true;
    stack.push_back(0);

    while (!stack.empty()) {
        int curr = stack.back();
        int cx = curr % K;
        int cy = curr / K;

        struct Neighbor { int cx, cy, dir; };
        Neighbor nbrs[4];
        int nbrCount = 0;

        if (cx + 1 < K && !visited[cy * K + (cx + 1)]) nbrs[nbrCount++] = { cx + 1, cy, 0 };
        if (cy + 1 < M && !visited[(cy + 1) * K + cx]) nbrs[nbrCount++] = { cx, cy + 1, 1 };
        if (cx - 1 >= 0 && !visited[cy * K + (cx - 1)]) nbrs[nbrCount++] = { cx - 1, cy, 2 };
        if (cy - 1 >= 0 && !visited[(cy - 1) * K + cx]) nbrs[nbrCount++] = { cx, cy - 1, 3 };

        if (nbrCount > 0) {
            int pick = rand() % nbrCount;
            Neighbor chosen = nbrs[pick];
            visited[chosen.cy * K + chosen.cx] = true;
            stack.push_back(chosen.cy * K + chosen.cx);

            if (chosen.dir == 0) treeEdges.push_back({ cx, cy, chosen.cx, chosen.cy, true });
            else if (chosen.dir == 1) treeEdges.push_back({ cx, cy, chosen.cx, chosen.cy, false });
            else if (chosen.dir == 2) treeEdges.push_back({ chosen.cx, chosen.cy, cx, cy, true });
            else if (chosen.dir == 3) treeEdges.push_back({ chosen.cx, chosen.cy, cx, cy, false });
        } else {
            stack.pop_back();
        }
    }

    // 3. 2-opt surgery to merge all blocks into 1 Hamiltonian cycle
    auto removeEdge = [&fineAdj](int u, int v) {
        auto itU = std::find(fineAdj[u].begin(), fineAdj[u].end(), v);
        if (itU != fineAdj[u].end()) fineAdj[u].erase(itU);
        auto itV = std::find(fineAdj[v].begin(), fineAdj[v].end(), u);
        if (itV != fineAdj[v].end()) fineAdj[v].erase(itV);
    };

    auto addEdge = [&fineAdj](int u, int v) {
        fineAdj[u].push_back(v);
        fineAdj[v].push_back(u);
    };

    for (const auto& e : treeEdges) {
        if (e.isHoriz) {
            int tr1 = fineIdx(2 * e.cx1 + 1, 2 * e.cy1);
            int br1 = fineIdx(2 * e.cx1 + 1, 2 * e.cy1 + 1);
            int tl2 = fineIdx(2 * e.cx2,     2 * e.cy2);
            int bl2 = fineIdx(2 * e.cx2,     2 * e.cy2 + 1);

            removeEdge(tr1, br1);
            removeEdge(tl2, bl2);
            addEdge(tr1, tl2);
            addEdge(br1, bl2);
        } else {
            int bl1 = fineIdx(2 * e.cx1,     2 * e.cy1 + 1);
            int br1 = fineIdx(2 * e.cx1 + 1, 2 * e.cy1 + 1);
            int tl2 = fineIdx(2 * e.cx2,     2 * e.cy2);
            int tr2 = fineIdx(2 * e.cx2 + 1, 2 * e.cy2);

            removeEdge(bl1, br1);
            removeEdge(tl2, tr2);
            addEdge(bl1, tl2);
            addEdge(br1, tr2);
        }
    }

    // 4. Trace the cycle from (0, 0)
    int current = 0;
    int prev = -1;
    for (int step = 0; step < totalFine; ++step) {
        outCycle[current] = step;
        int nextNode = -1;
        if (fineAdj[current].size() >= 1 && fineAdj[current][0] != prev) {
            nextNode = fineAdj[current][0];
        } else if (fineAdj[current].size() >= 2) {
            nextNode = fineAdj[current][1];
        }
        prev = current;
        current = (nextNode != -1) ? nextNode : 0;
    }
}

// Spawning Food with Bonus Items (Golden Apple, Namco Cherries)
static void SpawnFood(SnakeState& s) {
    if ((int)s.body.size() >= s.totalCells) {
        s.isWon = true;
        s.winStartTime = GetTickCount64();
        return;
    }

    int start = rand() % s.totalCells;
    for (int i = 0; i < s.totalCells; ++i) {
        int idx = (start + i) % s.totalCells;
        if (!s.occupied[idx]) {
            s.food.x = idx % s.cols;
            s.food.y = idx / s.cols;

            // Every 5th apple eaten produces a bonus fruit!
            if (s.applesEaten > 0 && s.applesEaten % 5 == 0) {
                if ((s.applesEaten / 5) % 2 == 1) {
                    s.foodType = 1; // Golden Apple
                    s.foodScore = 300;
                } else {
                    s.foodType = 2; // Pac-Man Cherry
                    s.foodScore = 500;
                }
                s.bonusTimer = 400; // ~12 seconds
            } else {
                s.foodType = 0; // Red Apple
                s.foodScore = 100;
                s.bonusTimer = 0;
            }
            return;
        }
    }

    s.isWon = true;
    s.winStartTime = GetTickCount64();
}

static void ResetSnakeGame(SnakeState& s, int cols, int rows) {
    s.cols = cols;
    s.rows = rows;
    s.totalCells = cols * rows;
    s.applesEaten = 0;
    s.isWon = false;
    s.winStartTime = 0;
    s.lastTick = GetTickCount64();
    s.stuckFrames = 0;
    s.timeAccumulator = 0;
    s.foodPulses.clear();
    s.sparkles.clear();
    s.popups.clear();

    GenerateHamiltonianCycle(s.cols, s.rows, s.cycle);
    s.occupied.assign(s.totalCells, 0);

    // Initial snake body of length 4
    s.body.clear();
    POINT cellsByCycle[4];
    for (int y = 0; y < s.rows; ++y) {
        for (int x = 0; x < s.cols; ++x) {
            int c = s.cycle[y * s.cols + x];
            if (c >= 0 && c < 4) {
                cellsByCycle[c] = { x, y };
            }
        }
    }
    s.body.push_back(cellsByCycle[3]); // head
    s.body.push_back(cellsByCycle[2]);
    s.body.push_back(cellsByCycle[1]);
    s.body.push_back(cellsByCycle[0]); // tail

    for (const auto& p : s.body) {
        s.occupied[p.y * s.cols + p.x] = 1;
    }

    SpawnFood(s);
    s.initialized = true;
}

// AI Decision: Hamiltonian path with safe shortcuts
static POINT ChooseNextMove(const SnakeState& s) {
    const POINT& head = s.body.front();
    const POINT& tail = s.body.back();
    int N = s.totalCells;

    auto cycleDist = [N](int fromIdx, int toIdx) {
        return (toIdx - fromIdx + N) % N;
    };

    int headCycle = s.cycle[head.y * s.cols + head.x];
    int tailCycle = s.cycle[tail.y * s.cols + tail.x];
    int foodCycle = s.cycle[s.food.y * s.cols + s.food.x];

    int distToTail = cycleDist(headCycle, tailCycle);
    int distToFood = cycleDist(headCycle, foodCycle);

    static const int DX[4] = { 1, 0, -1, 0 };
    static const int DY[4] = { 0, 1, 0, -1 };

    struct Candidate {
        POINT pt;
        int cycle;
        int distH;
        int distF;
        int manhattan;
        bool isHamiltonianNext;
        bool isSafe;
    };

    Candidate valid[4];
    int validCount = 0;
    Candidate hamNext = { { head.x, head.y }, 0, 0, 0, 0, false, false };

    float fillRatio = (float)s.body.size() / (float)N;
    int safetyMargin = 3;
    if (fillRatio > 0.40f) safetyMargin = 6;
    if (fillRatio > 0.65f) safetyMargin = 12;
    if (fillRatio > 0.85f) safetyMargin = 25;

    for (int d = 0; d < 4; ++d) {
        int nx = head.x + DX[d];
        int ny = head.y + DY[d];

        if (nx < 0 || nx >= s.cols || ny < 0 || ny >= s.rows) continue;

        int nIdx = ny * s.cols + nx;
        // Moving into tail is legal because tail vacates
        if (s.occupied[nIdx] && !(nx == tail.x && ny == tail.y)) {
            continue;
        }

        int nCycle = s.cycle[nIdx];
        int distH = cycleDist(headCycle, nCycle);
        int distF = cycleDist(nCycle, foodCycle);
        int mDist = abs(nx - s.food.x) + abs(ny - s.food.y);

        bool isHam = (distH == 1);
        bool safe = false;

        if (isHam) {
            safe = true;
        } else if (fillRatio < 0.88f) {
            if (distH + safetyMargin < distToTail) {
                if (distH <= distToFood) {
                    safe = true;
                }
            }
        }

        Candidate cand = { { nx, ny }, nCycle, distH, distF, mDist, isHam, safe };
        if (isHam) hamNext = cand;
        if (safe) valid[validCount++] = cand;
    }

    if (validCount == 0) {
        if (hamNext.isHamiltonianNext && (!s.occupied[hamNext.pt.y * s.cols + hamNext.pt.x] ||
                                         (hamNext.pt.x == tail.x && hamNext.pt.y == tail.y))) {
            return hamNext.pt;
        }
        for (int d = 0; d < 4; ++d) {
            int nx = head.x + DX[d];
            int ny = head.y + DY[d];
            if (nx >= 0 && nx < s.cols && ny >= 0 && ny < s.rows) {
                int nIdx = ny * s.cols + nx;
                if (!s.occupied[nIdx] || (nx == tail.x && ny == tail.y)) {
                    return { nx, ny };
                }
            }
        }
        return head;
    }

    Candidate best = valid[0];
    for (int i = 1; i < validCount; ++i) {
        if (valid[i].distF < best.distF) {
            best = valid[i];
        } else if (valid[i].distF == best.distF && valid[i].manhattan < best.manhattan) {
            best = valid[i];
        }
    }

    return best.pt;
}

// ============================================================================
// SIMULATION UPDATE
// ============================================================================

static void UpdateSnake(SnakeState& s) {
    uint64_t now = GetTickCount64();

    if (s.isWon) {
        if (now - s.winStartTime > 2500) {
            s.wave++;
            ResetSnakeGame(s, s.cols, s.rows);
        }
        return;
    }

    // Step AI movement
    POINT nextHead = ChooseNextMove(s);
    if (nextHead.x != s.body.front().x || nextHead.y != s.body.front().y) {
        s.stuckFrames = 0;
        bool ateFood = (nextHead.x == s.food.x && nextHead.y == s.food.y);
        s.body.insert(s.body.begin(), nextHead);
        s.occupied[nextHead.y * s.cols + nextHead.x] = 1;

        if (ateFood) {
            s.score += s.foodScore;
            s.applesEaten++;
            if (s.score > s.highScore) s.highScore = s.score;

            // Trigger digestion pulse ripple
            s.foodPulses.push_back(0);

            // Burst of arcade star sparkles
            int fx = s.playLeft + s.food.x * 8 + 4;
            int fy = s.playTop + s.food.y * 8 + 4;
            for (int p = 0; p < 10; ++p) {
                float angle = (float)p * (6.2831853f / 10.0f);
                float speed = 0.8f + (float)(rand() % 100) / 100.0f * 1.6f;
                s.sparkles.push_back({
                    (float)fx, (float)fy,
                    cosf(angle) * speed, sinf(angle) * speed,
                    0, 18 + rand() % 8,
                    (p % 3 == 0) ? COL_GOLD : ((p % 3 == 1) ? COL_WHITE : COL_GREEN_LIME)
                });
            }

            // Floating score popup (+100, +300, +500)
            s.popups.push_back({
                (float)(fx - 8), (float)(fy - 4),
                s.foodScore, 0,
                (s.foodType == 1) ? COL_GOLD : ((s.foodType == 2) ? COL_RED : COL_CYAN)
            });

            SpawnFood(s);
        } else {
            POINT oldTail = s.body.back();
            s.occupied[oldTail.y * s.cols + oldTail.x] = 0;
            s.body.pop_back();
        }
    } else {
        s.stuckFrames++;
        if (s.stuckFrames > 15) {
            ResetSnakeGame(s, s.cols, s.rows);
        }
    }

    // Advance digestion pulses along snake
    for (size_t p = 0; p < s.foodPulses.size(); ) {
        s.foodPulses[p]++;
        if (s.foodPulses[p] >= (int)s.body.size()) {
            s.foodPulses.erase(s.foodPulses.begin() + p);
        } else {
            p++;
        }
    }

    // Bonus fruit expiration
    if (s.bonusTimer > 0) {
        s.bonusTimer--;
        if (s.bonusTimer == 0 && s.foodType != 0) {
            s.foodType = 0;
            s.foodScore = 100;
        }
    }
}

// ============================================================================
// COMPLETE ARCADE FRAME RENDERING
// ============================================================================

static void RenderArcadeFrame(SnakeState& s) {
    // 1. Clear Framebuffer to Arcade Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    uint64_t now = GetTickCount64();

    // 2. Playfield Arena Floor & Borders
    DrawFloorTexture(s);
    DrawArcadeBorder(s);

    // 3. Render Food
    if (!s.isWon) {
        int fx = s.playLeft + s.food.x * 8;
        int fy = s.playTop + s.food.y * 8;
        DrawFood(s, fx, fy, s.foodType, (int)(now / 50));
    }

    // 4. Render Snake Body Segments, Elbow Turns, Tail & Head
    size_t bodyLen = s.body.size();
    bool isGoldShimmer = s.isWon;

    for (size_t i = 0; i < bodyLen; ++i) {
        const POINT& curr = s.body[i];
        int px = s.playLeft + curr.x * 8;
        int py = s.playTop + curr.y * 8;

        // Check if digestion pulse is currently passing through this segment
        bool isPulse = false;
        for (int pIdx : s.foodPulses) {
            if (pIdx == (int)i || pIdx == (int)i + 1) {
                isPulse = true;
                break;
            }
        }

        // Shimmer wave if board completed
        bool segGold = isGoldShimmer && (((int)i + (int)(now / 60)) % 8 < 4);

        if (i == 0) {
            // Head
            int dir = 0; // DIR_RIGHT
            if (bodyLen >= 2) {
                int dx = curr.x - s.body[1].x;
                int dy = curr.y - s.body[1].y;
                if      (dx > 0) dir = 0;
                else if (dx < 0) dir = 2;
                else if (dy > 0) dir = 1;
                else if (dy < 0) dir = 3;
            }

            // Flick tongue periodically or rapidly when approaching food
            int distToFood = abs(curr.x - s.food.x) + abs(curr.y - s.food.y);
            bool tongueOut = (distToFood <= 3) ? ((now / 100) % 2 == 0) : ((now / 200) % 3 == 0);
            DrawSnakeHead(s, px, py, dir, tongueOut);

        } else if (i == bodyLen - 1) {
            // Tail
            const POINT& prev = s.body[i - 1];
            int dx = prev.x - curr.x;
            int dy = prev.y - curr.y;
            int dir = 0;
            // Point tip away from body segment ahead
            if      (dx > 0) dir = 2; // Neck on right -> tip points LEFT
            else if (dx < 0) dir = 0; // Neck on left  -> tip points RIGHT
            else if (dy > 0) dir = 3; // Neck on bottom-> tip points UP
            else if (dy < 0) dir = 1; // Neck on top   -> tip points DOWN

            DrawSnakeTail(s, px, py, dir, segGold);

        } else {
            // Body Segment
            const POINT& prev = s.body[i - 1];
            const POINT& next = s.body[i + 1];
            int dx1 = prev.x - curr.x;
            int dy1 = prev.y - curr.y;
            int dx2 = next.x - curr.x;
            int dy2 = next.y - curr.y;

            if (dy1 == 0 && dy2 == 0) {
                // Straight Horizontal
                DrawSnakeStraight(s, px, py, true, isPulse, segGold);
            } else if (dx1 == 0 && dx2 == 0) {
                // Straight Vertical
                DrawSnakeStraight(s, px, py, false, isPulse, segGold);
            } else {
                // 90-degree Corner Elbow Joints
                // 0: NE (UP & RIGHT), 1: NW (UP & LEFT), 2: SE (DOWN & RIGHT), 3: SW (DOWN & LEFT)
                int cornerType = 0;
                if ((dx1 == 1 && dy2 == -1) || (dx2 == 1 && dy1 == -1)) cornerType = 0; // NE
                else if ((dx1 == -1 && dy2 == -1) || (dx2 == -1 && dy1 == -1)) cornerType = 1; // NW
                else if ((dx1 == 1 && dy2 == 1) || (dx2 == 1 && dy1 == 1)) cornerType = 2; // SE
                else cornerType = 3; // SW

                DrawSnakeCorner(s, px, py, cornerType, isPulse, segGold);
            }
        }
    }

    // 5. Render Particle Sparkles
    for (const auto& sp : s.sparkles) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }

    // 6. Render Floating Score Popups (+100, +300, +500)
    for (const auto& pop : s.popups) {
        char buf[16];
        sprintf_s(buf, "+%d", pop.score);
        DrawText(s, (int)pop.x, (int)pop.y, buf, pop.color);
    }

    // 7. Top HUD
    DrawText(s, s.playLeft, 4, "SCORE", COL_WHITE);

    char scoreBuf[16];
    sprintf_s(scoreBuf, "%06d", s.score);
    DrawText(s, s.playLeft, 13, scoreBuf, COL_WHITE);

    int midX = s.virtualW / 2;
    DrawText(s, midX - 27, 4, "HIGH SCORE", COL_CYAN);
    char highBuf[16];
    sprintf_s(highBuf, "%06d", s.highScore);
    DrawText(s, midX - 18, 13, highBuf, COL_GOLD);

    DrawText(s, s.virtualW - s.playLeft - 36, 4, "LENGTH", COL_GRAY_LIGHT);
    char lenBuf[16];
    sprintf_s(lenBuf, "%d/%d", (int)s.body.size(), s.totalCells);
    int lenW = (int)strlen(lenBuf) * 6;
    DrawText(s, s.virtualW - s.playLeft - lenW, 13, lenBuf, COL_GREEN_LIME);

    // 8. Game State Banners
    if (s.isWon) {
        DrawText(s, midX - 36, 120, "STAGE CLEAR!", COL_GOLD);
    } else if (s.stuckFrames > 15) {
        DrawText(s, midX - 27, 120, "GAME OVER", COL_RED);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================

void RenderSnake(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<SnakeState>(27);

    // Adapt virtual resolution to widescreen aspect ratio (height locked to 256 for chunky retro pixel density)
    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    // Calculate symmetrical even grid dimensions
    int availCols = (vW - 16) / CELL_SIZE;
    int playCols = (availCols / 2) * 2;
    if (playCols < 16) playCols = 16;
    int playRows = 28; // 28 * 8 = 224 px

    int playLeft = (vW - playCols * CELL_SIZE) / 2;
    int playTop = 26;

    if (!s.initialized || s.virtualW != vW || s.cols != playCols || s.rows != playRows || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);
        s.playLeft = playLeft;
        s.playTop = playTop;
        ResetSnakeGame(s, playCols, playRows);
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200; // prevent surge
    s.lastTick = now;

    // Update Sparkles & Popups smoothly every frame
    for (size_t i = 0; i < s.sparkles.size(); ) {
        s.sparkles[i].x += s.sparkles[i].vx;
        s.sparkles[i].y += s.sparkles[i].vy;
        s.sparkles[i].life++;
        if (s.sparkles[i].life >= s.sparkles[i].maxLife) {
            s.sparkles.erase(s.sparkles.begin() + i);
        } else {
            ++i;
        }
    }
    for (size_t i = 0; i < s.popups.size(); ) {
        s.popups[i].y -= 0.35f;
        s.popups[i].timer++;
        if (s.popups[i].timer >= 32) {
            s.popups.erase(s.popups.begin() + i);
        } else {
            ++i;
        }
    }

    // Step AI snake at smooth 35ms intervals
    s.timeAccumulator += elapsed;
    const uint64_t stepInterval = 35;
    int stepsRun = 0;
    while (s.timeAccumulator >= stepInterval && stepsRun < 2) {
        s.timeAccumulator -= stepInterval;
        stepsRun++;
        UpdateSnake(s);
    }

    // Render virtual arcade framebuffer
    RenderArcadeFrame(s);

    // Pixel-Perfect Stretched Blit (Fills 100% of the display with zero black bars)
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

// Register as Screensaver ID 27
REGISTER_SCREENSAVER(
    27,
    L"Self-Playing Snake",
    "snake",
    { "snake", "ouroboros" },
    WRAP_LEGACY(RenderSnake),
    {}
);
