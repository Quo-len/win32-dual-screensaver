#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/BreakoutSettings.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic 1986 Retro Arcade Arkanoid / Breakout Neon Simulation
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio (e.g. 455x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;

// 32-bit ARGB Retro Arcade Palette
static constexpr uint32_t COL_BLACK          = 0xFF050508;
static constexpr uint32_t COL_WHITE          = 0xFFFFFFFF;
static constexpr uint32_t COL_GOLD           = 0xFFFFD700;
static constexpr uint32_t COL_YELLOW         = 0xFFFFD600;
static constexpr uint32_t COL_ORANGE         = 0xFFFF6D00;
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744;
static constexpr uint32_t COL_CRIMSON_GLOW   = 0xFFFF5252;
static constexpr uint32_t COL_CYAN_NEON      = 0xFF00E5FF;
static constexpr uint32_t COL_CYAN_CORE      = 0xFF80D8FF;
static constexpr uint32_t COL_BLUE_ELECTRIC  = 0xFF00B0FF;
static constexpr uint32_t COL_BLUE_DARK      = 0xFF0D47A1;
static constexpr uint32_t COL_MAGENTA        = 0xFFD500F9;
static constexpr uint32_t COL_GREEN_LIME     = 0xFF00FF66;
static constexpr uint32_t COL_GREEN_EMERALD  = 0xFF00E676;
static constexpr uint32_t COL_GRAY_LIGHT     = 0xFFB0BEC5;
static constexpr uint32_t COL_GRAY_MID       = 0xFF546E7A;
static constexpr uint32_t COL_GRAY_DARK      = 0xFF263238;
static constexpr uint32_t COL_BORDER_OUTER   = 0xFF152040;
static constexpr uint32_t COL_BORDER_INNER   = 0xFF00E5FF;
static constexpr uint32_t COL_GRID_LINE      = 0xFF0D1526;
static constexpr uint32_t COL_SILVER         = 0xFFCFD8DC;

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
    case ':': { static const uint8_t g[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    case '!': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }; return g; }
    case '+': { static const uint8_t g[7] = { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }; return g; }
    default:  return GLYPH_BLANK;
    }
}

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================
enum PowerUpType {
    POW_NONE = 0,
    POW_LASER,      // 'L' - Twin Laser Blasters
    POW_EXPAND,     // 'E' - Expanded Paddle
    POW_MULTI,      // 'M' - Tri-Ball Split
    POW_SLOW,       // 'S' - Slow Motion Energy
    POW_POINTS      // 'P' - 1000 Bonus Points
};

struct BreakoutSpark {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    uint32_t color;
};

struct FloatingScore {
    float x, y;
    int score;
    int life;
    uint32_t color;
};

struct Ball {
    float x, y;
    float vx, vy;
    bool active;
    bool attached; // Glued to paddle at start of round / life
};

struct Brick {
    int row, col;
    int type;      // 0: dead, 1..6: normal colors, 7: silver (2-hit), 8: gold (unbreakable)
    int hp;
    uint32_t color;
    bool active;
};

struct PowerUp {
    float x, y;
    float vy;
    PowerUpType type;
    bool active;
};

struct Laser {
    float x, y;
    float vy;
    bool active;
};

struct ArkanoidState {
    int virtualW = 0;
    int virtualH = 0;
    std::vector<uint32_t> fb;

    // Game stats
    int score = 0;
    int highScore = 50000;
    int lives = 3;
    int round = 1;
    int state = 0; // 0: init, 1: playing, 2: round clear pause, 3: game over pause

    // Paddle
    float paddleX = 0.0f;
    float paddleY = 236.0f;
    float paddleW = 32.0f;
    float paddleTargetW = 32.0f;
    float paddleH = 6.0f;
    float paddleVx = 0.0f;

    // Power-up timers
    PowerUpType currentPower = POW_NONE;
    int powerTimer = 0;
    int laserCooldown = 0;

    // Entities
    std::vector<Ball> balls;
    std::vector<Brick> bricks;
    std::vector<PowerUp> powerups;
    std::vector<Laser> lasers;
    std::vector<BreakoutSpark> sparks;
    std::vector<FloatingScore> floatingScores;

    int brickCols = 14;
    int brickRows = 8;
    int brickStartX = 14;
    int brickW = 0;
    int brickH = 8;
    int remainingBricks = 0;

    int pauseTimer = 0;
    uint64_t lastTick = 0;
    bool initialized = false;
};

// ============================================================================
// FAST LOW-LEVEL DRAWING PRIMITIVES
// ============================================================================
static inline void PutPixel(ArkanoidState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void DrawHLine(ArkanoidState& s, int x1, int x2, int y, uint32_t color) {
    if (y < 0 || y >= s.virtualH) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = (std::max)(0, x1);
    x2 = (std::min)(s.virtualW - 1, x2);
    uint32_t* row = &s.fb[y * s.virtualW];
    for (int x = x1; x <= x2; ++x) row[x] = color;
}

static void DrawVLine(ArkanoidState& s, int x, int y1, int y2, uint32_t color) {
    if (x < 0 || x >= s.virtualW) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = (std::max)(0, y1);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) s.fb[y * s.virtualW + x] = color;
}

static void FillRect(ArkanoidState& s, int x1, int y1, int x2, int y2, uint32_t color) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    x1 = (std::max)(0, x1);
    y1 = (std::max)(0, y1);
    x2 = (std::min)(s.virtualW - 1, x2);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        uint32_t* row = &s.fb[y * s.virtualW];
        for (int x = x1; x <= x2; ++x) row[x] = color;
    }
}

static void DrawGlyphChar(ArkanoidState& s, int px, int py, char c, uint32_t color) {
    const uint8_t* rows = GetGlyph(c);
    for (int r = 0; r < 7; ++r) {
        uint8_t bits = rows[r];
        for (int col = 0; col < 5; ++col) {
            if (bits & (0x10 >> col)) PutPixel(s, px + col, py + r, color);
        }
    }
}

static void DrawText(ArkanoidState& s, int px, int py, const char* str, uint32_t color) {
    while (*str) {
        DrawGlyphChar(s, px, py, *str, color);
        px += 6;
        str++;
    }
}

// 3D Beveled Neon Brick (Authentic Arcade Feel)
static void DrawBeveledBrick(ArkanoidState& s, int x1, int y1, int x2, int y2, uint32_t baseCol, bool hitFlash) {
    if (x2 < x1 || y2 < y1) return;
    if (hitFlash) {
        FillRect(s, x1, y1, x2, y2, COL_WHITE);
        return;
    }

    uint32_t r = (baseCol >> 16) & 0xFF;
    uint32_t g = (baseCol >> 8) & 0xFF;
    uint32_t b = baseCol & 0xFF;

    // Highlight top/left
    uint32_t hiR = (std::min)(255, (int)(r * 1.55f + 40));
    uint32_t hiG = (std::min)(255, (int)(g * 1.55f + 40));
    uint32_t hiB = (std::min)(255, (int)(b * 1.55f + 40));
    uint32_t hiCol = 0xFF000000 | (hiR << 16) | (hiG << 8) | hiB;

    // Shadow bottom/right
    uint32_t shR = (uint32_t)(r * 0.40f);
    uint32_t shG = (uint32_t)(g * 0.40f);
    uint32_t shB = (uint32_t)(b * 0.40f);
    uint32_t shCol = 0xFF000000 | (shR << 16) | (shG << 8) | shB;

    FillRect(s, x1, y1, x2, y2, baseCol);
    DrawHLine(s, x1, x2, y1, hiCol);
    DrawVLine(s, x1, y1, y2, hiCol);
    DrawHLine(s, x1, x2, y2, shCol);
    DrawVLine(s, x2, y1, y2, shCol);

    // Glossy pip
    if (x2 - x1 >= 4 && y2 - y1 >= 4) {
        PutPixel(s, x1 + 1, y1 + 1, COL_WHITE);
    }
}

// Retro Neon Double Border
static void DrawArcadeBorder(ArkanoidState& s) {
    int x1 = 10;
    int y1 = 26;
    int x2 = s.virtualW - 11;
    int y2 = 250;

    // Outer deep blue border
    DrawHLine(s, x1 + 2, x2 - 2, y1, COL_BORDER_OUTER);
    DrawHLine(s, x1 + 2, x2 - 2, y2, COL_BORDER_OUTER);
    DrawVLine(s, x1, y1 + 2, y2 - 2, COL_BORDER_OUTER);
    DrawVLine(s, x2, y1 + 2, y2 - 2, COL_BORDER_OUTER);

    // Inner bright electric cyan border
    int ix1 = x1 + 2;
    int iy1 = y1 + 2;
    int ix2 = x2 - 2;
    int iy2 = y2 - 2;
    DrawHLine(s, ix1 + 1, ix2 - 1, iy1, COL_BORDER_INNER);
    DrawVLine(s, ix1, iy1 + 1, iy2, COL_BORDER_INNER);
    DrawVLine(s, ix2, iy1 + 1, iy2, COL_BORDER_INNER);

    // Open bottom hazard zone (subtle red warning dashes)
    for (int x = ix1 + 4; x <= ix2 - 4; x += 10) {
        DrawHLine(s, x, x + 4, iy2, COL_CRIMSON);
    }

    // Corner targeting ticks
    PutPixel(s, ix1 + 1, iy1 + 1, COL_WHITE);
    PutPixel(s, ix2 - 1, iy1 + 1, COL_WHITE);
}

// Background Cyber Equalizer Grid
static void DrawCyberGrid(ArkanoidState& s) {
    int x1 = 14;
    int x2 = s.virtualW - 15;
    for (int y = 45; y <= 240; y += 22) {
        for (int x = x1; x <= x2; x += 8) {
            PutPixel(s, x, y, COL_GRID_LINE);
        }
    }
}

// ============================================================================
// STAGE LAYOUT GENERATOR
// ============================================================================
static void SetupStage(ArkanoidState& s, int stageNum) {
    s.bricks.clear();
    s.powerups.clear();
    s.lasers.clear();

    const int innerPlayLeft = 13;
    const int innerPlayRight = s.virtualW - 14;
    const int courtWidth = innerPlayRight - innerPlayLeft + 1; // Exactly s.virtualW - 26 (guaranteed even)

    s.brickCols = 14;
    s.brickRows = 7;
    s.brickW = courtWidth / s.brickCols;
    s.brickH = 8;

    // Each brick is drawn from bx1 to bx2 = bx1 + s.brickW - 3 (2px gap between adjacent bricks)
    // Entire row spans from leftmost pixel s.brickStartX to rightmost pixel s.brickStartX + s.brickCols * s.brickW - 3.
    // Total visual span = s.brickCols * s.brickW - 2 (always even)
    int visualSpan = s.brickCols * s.brickW - 2;
    int totalMargin = courtWidth - visualSpan; // Even - Even = Even
    int leftMargin = totalMargin / 2;
    s.brickStartX = innerPlayLeft + leftMargin;
    s.remainingBricks = 0;

    static const uint32_t ROW_COLORS[7] = {
        COL_SILVER,         // 2-hit Silver
        COL_CRIMSON,        // Crimson
        COL_ORANGE,         // Orange
        COL_YELLOW,         // Yellow
        COL_GREEN_LIME,     // Lime Green
        COL_CYAN_NEON,      // Cyan
        COL_MAGENTA         // Magenta
    };

    int startY = 40;

    for (int r = 0; r < s.brickRows; ++r) {
        for (int c = 0; c < s.brickCols; ++c) {
            bool present = true;
            int type = r; // Row-based color
            int hp = 1;

            if (stageNum % 3 == 1) {
                // Classic Rainbow Wall
                present = true;
                if (r == 0) { type = 7; hp = 2; } // Silver top row
            } else if (stageNum % 3 == 2) {
                // Pyramid / Fortress Shape (Bilateral symmetric across center line)
                int distFromCenter = (c < s.brickCols / 2) ? (s.brickCols / 2 - 1 - c) : (c - s.brickCols / 2);
                present = (distFromCenter <= r + 1);
                if (r == 0 || distFromCenter == 0) { type = 7; hp = 2; }
            } else {
                // Space Invader Alien Silhouette in Bricks (Bilateral symmetric across center line)
                static const uint16_t ALIEN_MASK[7] = {
                    0x0738, // ..###..###..
                    0x0FFC, // .#########.
                    0x1FFE, // ############
                    0x1DEE, // ##.####.####
                    0x1FFE, // ############
                    0x05C8, // ..#.##.#....
                    0x0A34  // .#.##..#.#..
                };
                present = (ALIEN_MASK[r] & (1 << (s.brickCols - 1 - c))) != 0;
                if (r == 1 || r == 4) { type = 7; hp = 2; }
            }

            if (present) {
                uint32_t col = (type == 7) ? COL_SILVER : ROW_COLORS[type % 7];
                s.bricks.push_back({ r, c, type, hp, col, true });
                s.remainingBricks++;
            }
        }
    }

    // Reset paddle and launch ball
    s.paddleTargetW = 34.0f;
    s.paddleW = s.paddleTargetW;
    s.paddleX = (s.virtualW - s.paddleW) / 2.0f;
    s.paddleY = 236.0f;
    s.paddleVx = 0.0f;

    s.currentPower = POW_NONE;
    s.powerTimer = 0;

    s.balls.clear();
    float ballSpd = g_BreakoutBallSpeed;
    s.balls.push_back({ s.paddleX + s.paddleW / 2.0f, s.paddleY - 4.0f, ballSpd * 0.7f, -ballSpd, true, false });
}

// Reset Game
static void ResetGame(ArkanoidState& s) {
    s.score = 0;
    s.lives = 3;
    s.round = 1;
    s.state = 1; // Playing
    SetupStage(s, s.round);
}

// ============================================================================
// SIMULATION & AUTOPLAY AI ENGINE
// ============================================================================
static void StepBreakoutSimulation(ArkanoidState& s) {
    // 1. Update Particles
    for (size_t i = 0; i < s.sparks.size(); ) {
        s.sparks[i].x += s.sparks[i].vx;
        s.sparks[i].y += s.sparks[i].vy;
        s.sparks[i].vy += 0.06f; // Gravity
        s.sparks[i].life++;
        if (s.sparks[i].life >= s.sparks[i].maxLife) {
            s.sparks.erase(s.sparks.begin() + i);
        } else {
            ++i;
        }
    }

    // 2. Update Floating Scores
    for (size_t i = 0; i < s.floatingScores.size(); ) {
        s.floatingScores[i].y -= 0.4f;
        s.floatingScores[i].life--;
        if (s.floatingScores[i].life <= 0) {
            s.floatingScores.erase(s.floatingScores.begin() + i);
        } else {
            ++i;
        }
    }

    if (s.state == 0) {
        ResetGame(s);
        return;
    }

    // Pause Handlers (Round Clear or Game Over)
    if (s.state == 2) {
        s.pauseTimer--;
        if (s.pauseTimer <= 0) {
            s.round++;
            SetupStage(s, s.round);
            s.state = 1;
        }
        return;
    } else if (s.state == 3) {
        s.pauseTimer--;
        if (s.pauseTimer <= 0) {
            ResetGame(s);
        }
        return;
    }

    // Power-up Timer Countdown
    if (s.powerTimer > 0) {
        s.powerTimer--;
        if (s.powerTimer <= 0) {
            s.currentPower = POW_NONE;
            s.paddleTargetW = 34.0f;
        }
    }

    // Smooth Paddle Resize (Expand Power-up)
    if (s.paddleW < s.paddleTargetW) s.paddleW += 1.0f;
    else if (s.paddleW > s.paddleTargetW) s.paddleW -= 1.0f;

    const int courtLeft = 13;
    const int courtRight = s.virtualW - 14;
    const int courtWidth = courtRight - courtLeft + 1;

    // 3. AUTOPLAY AI: Track lowest falling ball & predict landing
    float targetX = s.paddleX + s.paddleW / 2.0f;
    float lowestY = -1.0f;
    Ball* targetBall = nullptr;

    for (auto& b : s.balls) {
        if (!b.active) continue;
        if (b.y > lowestY) {
            lowestY = b.y;
            targetBall = &b;
        }
    }

    // Horizontal center of mass of remaining bricks (for active directional aiming)
    float brickSumX = 0.0f;
    int brickCount = 0;
    for (const auto& brk : s.bricks) {
        if (brk.active) {
            brickSumX += (float)(s.brickStartX + brk.col * s.brickW + s.brickW / 2);
            brickCount++;
        }
    }
    float targetBrickX = (brickCount > 0) ? (brickSumX / brickCount) : ((courtLeft + courtRight) * 0.5f);

    if (targetBall) {
        // Predict trajectory when ball is coming down
        if (targetBall->vy > 0) {
            float distY = s.paddleY - targetBall->y;
            float predX = targetBall->x + (targetBall->vx / targetBall->vy) * distY;

            // Bounce simulation across side walls
            int bounces = 0;
            while (predX < courtLeft || predX > courtRight) {
                if (predX < courtLeft) predX = courtLeft + (courtLeft - predX);
                if (predX > courtRight) predX = courtRight - (predX - courtRight);
                bounces++;
                if (bounces > 8) break;
            }

            // Directional aim offset: aim ball toward remaining bricks instead of hitting dead-center!
            // When targetBrickX is right of predX, shift paddle left so ball strikes right wing (deflecting right).
            float aimBias = 0.0f;
            if (targetBrickX > predX + 8.0f) {
                aimBias = -s.paddleW * 0.25f; // Paddle left -> ball strikes right -> deflects right
            } else if (targetBrickX < predX - 8.0f) {
                aimBias = s.paddleW * 0.25f;  // Paddle right -> ball strikes left -> deflects left
            } else {
                // Subtle alternating bias to permanently prevent any straight up-and-down vertical loop
                aimBias = (targetBall->vx >= 0.0f) ? (-s.paddleW * 0.18f) : (s.paddleW * 0.18f);
            }

            // Humanized imperfection & reaction pressure (allows bot to occasionally make mistakes and lose):
            float humanError = 0.0f;
            if (targetBall->y < 100.0f) {
                // Slower early anticipation while ball is high in brick field
                humanError = (bounces > 0) ? ((targetBall->vx > 0) ? 10.0f : -10.0f) : 0.0f;
            } else if (targetBall->y > 200.0f && std::abs(targetBall->vx) > 3.0f && bounces >= 2) {
                // Occasional edge-miss on difficult sharp high-speed bank shots (~3% chance)
                if ((s.score + s.round * 7) % 31 == 0) {
                    humanError = (targetBall->vx > 0) ? -16.0f : 16.0f;
                }
            }

            targetX = predX + aimBias + humanError;
        } else {
            // Ball going up: Track falling power-up if safe, or return towards center/brick cluster
            float lowestPowY = -1.0f;
            PowerUp* targetPow = nullptr;
            for (auto& p : s.powerups) {
                if (p.active && p.y > lowestPowY && p.y < s.paddleY) {
                    lowestPowY = p.y;
                    targetPow = &p;
                }
            }
            if (targetPow) {
                targetX = targetPow->x;
            } else {
                targetX = targetBrickX * 0.6f + ((courtLeft + courtRight) / 2.0f) * 0.4f;
            }
        }
    }

    // Steer paddle with realistic acceleration
    float paddleSpeed = g_BreakoutPaddleSpeed;
    float paddleCenter = s.paddleX + s.paddleW / 2.0f;
    float diff = targetX - paddleCenter;

    float prevPaddleX = s.paddleX;
    if (std::abs(diff) > 1.5f) {
        float step = (diff > 0) ? paddleSpeed : -paddleSpeed;
        if (std::abs(step) > std::abs(diff)) step = diff;
        s.paddleX += step;
    }

    // Clamp paddle to court boundaries
    if (s.paddleX < courtLeft) s.paddleX = (float)courtLeft;
    if (s.paddleX + s.paddleW > courtRight) s.paddleX = (float)(courtRight - s.paddleW);

    s.paddleVx = s.paddleX - prevPaddleX;

    // 4. AUTOPLAY LASER FIRING
    if (s.currentPower == POW_LASER) {
        s.laserCooldown++;
        if (s.laserCooldown >= 14) {
            s.laserCooldown = 0;
            s.lasers.push_back({ s.paddleX + 3.0f, s.paddleY - 2.0f, -6.0f, true });
            s.lasers.push_back({ s.paddleX + s.paddleW - 4.0f, s.paddleY - 2.0f, -6.0f, true });
        }
    }

    // 5. Update Lasers
    for (auto& l : s.lasers) {
        if (!l.active) continue;
        l.y += l.vy;
        if (l.y < 30) { l.active = false; continue; }

        // Laser vs Brick Collision
        for (auto& brk : s.bricks) {
            if (!brk.active) continue;
            int bx1 = s.brickStartX + brk.col * s.brickW;
            int by1 = 40 + brk.row * s.brickH;
            int bx2 = bx1 + s.brickW - 3;
            int by2 = by1 + s.brickH - 2;

            if (l.x >= bx1 && l.x <= bx2 && l.y >= by1 && l.y <= by2) {
                l.active = false;
                brk.hp--;
                if (brk.hp <= 0) {
                    brk.active = false;
                    s.remainingBricks--;
                    s.score += (brk.type == 7) ? 200 : 100;
                    if (s.score > s.highScore) s.highScore = s.score;

                    // Spark explosion
                    for (int k = 0; k < 8; ++k) {
                        float spdVx = ((rand() % 100) - 50) * 0.05f;
                        float spdVy = ((rand() % 100) - 50) * 0.05f;
                        s.sparks.push_back({ l.x, l.y, spdVx, spdVy, 0, 16, brk.color });
                    }
                }
                break;
            }
        }
    }

    // 6. Update Power-ups
    for (auto& p : s.powerups) {
        if (!p.active) continue;
        p.y += p.vy;

        // Catch power-up with paddle
        if (p.y >= s.paddleY - 2.0f && p.y <= s.paddleY + s.paddleH) {
            if (p.x >= s.paddleX - 4.0f && p.x <= s.paddleX + s.paddleW + 4.0f) {
                p.active = false;
                s.currentPower = p.type;
                s.powerTimer = 500; // ~15 seconds

                if (p.type == POW_EXPAND) {
                    s.paddleTargetW = 50.0f;
                } else if (p.type == POW_MULTI) {
                    // Split current active balls
                    size_t currBallCount = s.balls.size();
                    for (size_t b = 0; b < currBallCount && s.balls.size() < 6; ++b) {
                        if (s.balls[b].active) {
                            float bSpeed = g_BreakoutBallSpeed;
                            s.balls.push_back({ s.balls[b].x, s.balls[b].y, -bSpeed * 0.7f, s.balls[b].vy, true, false });
                            s.balls.push_back({ s.balls[b].x, s.balls[b].y, bSpeed * 0.7f, s.balls[b].vy, true, false });
                        }
                    }
                } else if (p.type == POW_SLOW) {
                    for (auto& b : s.balls) {
                        b.vx *= 0.7f;
                        b.vy *= 0.7f;
                    }
                } else if (p.type == POW_POINTS) {
                    s.score += 1000;
                    if (s.score > s.highScore) s.highScore = s.score;
                    s.floatingScores.push_back({ s.paddleX + s.paddleW / 2.0f, s.paddleY - 8.0f, 1000, 40, COL_GOLD });
                }

                // Power-up caught sparkles
                for (int k = 0; k < 12; ++k) {
                    float spdVx = ((rand() % 100) - 50) * 0.04f;
                    float spdVy = -1.0f - (rand() % 100) * 0.02f;
                    s.sparks.push_back({ p.x, p.y, spdVx, spdVy, 0, 18, COL_CYAN_NEON });
                }
            }
        }

        if (p.y > 250) p.active = false;
    }

    // 7. Update Balls & Physical Collision
    int activeBalls = 0;
    float maxBallSpd = g_BreakoutBallSpeed * 1.5f;

    for (auto& b : s.balls) {
        if (!b.active) continue;
        activeBalls++;

        // Add comet trail sparkle
        if (rand() % 2 == 0) {
            s.sparks.push_back({ b.x, b.y, -b.vx * 0.2f, -b.vy * 0.2f, 0, 8, COL_CYAN_CORE });
        }

        b.x += b.vx;
        b.y += b.vy;

        // Bounce against left & right inner borders
        if (b.x - 2.5f <= courtLeft) {
            b.x = courtLeft + 2.5f;
            b.vx = std::abs(b.vx);
            for (int k = 0; k < 4; ++k) s.sparks.push_back({ b.x, b.y, 0.5f, ((rand() % 100) - 50) * 0.03f, 0, 8, COL_WHITE });
        } else if (b.x + 2.5f >= courtRight) {
            b.x = courtRight - 2.5f;
            b.vx = -std::abs(b.vx);
            for (int k = 0; k < 4; ++k) s.sparks.push_back({ b.x, b.y, -0.5f, ((rand() % 100) - 50) * 0.03f, 0, 8, COL_WHITE });
        }

        // Bounce against top inner border
        if (b.y - 2.5f <= 28) {
            b.y = 28 + 2.5f;
            b.vy = std::abs(b.vy);
            if (std::abs(b.vx) < 0.50f) {
                b.vx = (rand() % 2 == 0) ? 0.65f : -0.65f;
            }
            for (int k = 0; k < 4; ++k) s.sparks.push_back({ b.x, b.y, ((rand() % 100) - 50) * 0.03f, 0.5f, 0, 8, COL_WHITE });
        }

        // Paddle Collision & Angular Deflection
        if (b.vy > 0 && b.y + 2.5f >= s.paddleY && b.y - 2.5f <= s.paddleY + s.paddleH) {
            if (b.x >= s.paddleX - 2.5f && b.x <= s.paddleX + s.paddleW + 2.5f) {
                // Calculate impact offset (-1.0 to 1.0)
                float hitOffset = (b.x - (s.paddleX + s.paddleW / 2.0f)) / (s.paddleW / 2.0f);
                hitOffset = (std::max)(-0.92f, (std::min)(0.92f, hitOffset));

                float currentSpeed = std::sqrt(b.vx * b.vx + b.vy * b.vy);
                currentSpeed += 0.05f; // Gradual arcade speed increase
                if (currentSpeed > maxBallSpd) currentSpeed = maxBallSpd;

                // Paddle motion momentum transfer (spin)
                float spin = (s.paddleVx / g_BreakoutPaddleSpeed) * 0.22f;

                // Angle from -65° to +65°
                float angle = hitOffset * 1.15f + spin;

                // Anti-vertical guard: prevent straight 180° / 90° up-down bouncing loop!
                if (std::abs(angle) < 0.22f) {
                    float nudge = 0.22f + ((rand() % 8) * 0.01f);
                    angle = (angle >= 0.0f) ? nudge : -nudge;
                }

                b.vx = currentSpeed * std::sin(angle);
                b.vy = -currentSpeed * std::cos(angle);

                // Ensure non-zero horizontal velocity
                if (std::abs(b.vx) < 0.65f) {
                    b.vx = (b.vx >= 0.0f) ? 0.65f : -0.65f;
                    float spd = std::sqrt(b.vx * b.vx + b.vy * b.vy);
                    if (spd > 0.001f) {
                        b.vx = (b.vx / spd) * currentSpeed;
                        b.vy = (b.vy / spd) * currentSpeed;
                    }
                }

                b.y = s.paddleY - 2.5f;

                // Impact sparks
                for (int k = 0; k < 6; ++k) {
                    float spdVx = ((rand() % 100) - 50) * 0.03f;
                    float spdVy = -0.8f - (rand() % 100) * 0.02f;
                    s.sparks.push_back({ b.x, s.paddleY, spdVx, spdVy, 0, 10, COL_GOLD });
                }
            }
        }

        // Brick Collisions
        for (auto& brk : s.bricks) {
            if (!brk.active) continue;

            int bx1 = s.brickStartX + brk.col * s.brickW;
            int by1 = 40 + brk.row * s.brickH;
            int bx2 = bx1 + s.brickW - 3;
            int by2 = by1 + s.brickH - 2;

            // AABB vs Ball circle check
            if (b.x + 2.5f >= bx1 && b.x - 2.5f <= bx2 && b.y + 2.5f >= by1 && b.y - 2.5f <= by2) {
                // Determine collision normal
                float overlapLeft   = (b.x + 2.5f) - bx1;
                float overlapRight  = bx2 - (b.x - 2.5f);
                float overlapTop    = (b.y + 2.5f) - by1;
                float overlapBottom = by2 - (b.y - 2.5f);

                float minOverlapX = (std::min)(overlapLeft, overlapRight);
                float minOverlapY = (std::min)(overlapTop, overlapBottom);

                if (minOverlapX < minOverlapY) {
                    b.vx = (overlapLeft < overlapRight) ? -std::abs(b.vx) : std::abs(b.vx);
                } else {
                    b.vy = (overlapTop < overlapBottom) ? -std::abs(b.vy) : std::abs(b.vy);
                    // Prevent vertical lock when hitting brick top/bottom
                    if (std::abs(b.vx) < 0.50f) {
                        b.vx = (rand() % 2 == 0) ? 0.65f : -0.65f;
                    }
                }

                brk.hp--;
                if (brk.hp <= 0) {
                    brk.active = false;
                    s.remainingBricks--;
                    int pts = (brk.type == 7) ? 200 : (100 + brk.row * 10);
                    s.score += pts;
                    if (s.score > s.highScore) s.highScore = s.score;

                    // Brick shatter particle debris
                    for (int k = 0; k < 10; ++k) {
                        float spdVx = ((rand() % 100) - 50) * 0.06f;
                        float spdVy = ((rand() % 100) - 50) * 0.06f;
                        s.sparks.push_back({ b.x, b.y, spdVx, spdVy, 0, 20, brk.color });
                    }

                    // Power-up Drop Chance (18%)
                    if ((rand() % 100) < 18 && s.powerups.size() < 4) {
                        PowerUpType powTypes[5] = { POW_LASER, POW_EXPAND, POW_MULTI, POW_SLOW, POW_POINTS };
                        PowerUpType pType = powTypes[rand() % 5];
                        s.powerups.push_back({ (float)(bx1 + (bx2 - bx1) / 2), (float)by2, 1.2f, pType, true });
                    }
                } else {
                    // Hit flash on silver multi-hit brick
                    for (int k = 0; k < 6; ++k) {
                        float spdVx = ((rand() % 100) - 50) * 0.04f;
                        float spdVy = ((rand() % 100) - 50) * 0.04f;
                        s.sparks.push_back({ b.x, b.y, spdVx, spdVy, 0, 10, COL_WHITE });
                    }
                }
                break;
            }
        }

        // Ball fell past bottom
        if (b.y > 255) {
            b.active = false;
        }
    }

    // 8. Check Ball Loss
    if (activeBalls == 0) {
        s.lives--;
        s.currentPower = POW_NONE;
        s.paddleTargetW = 34.0f;
        s.powerTimer = 0;

        if (s.lives > 0) {
            // Respawn fresh ball
            float ballSpd = g_BreakoutBallSpeed;
            s.balls.clear();
            s.balls.push_back({ s.paddleX + s.paddleW / 2.0f, s.paddleY - 4.0f, ballSpd * 0.7f, -ballSpd, true, false });
        } else {
            // Game Over
            s.state = 3;
            s.pauseTimer = 90; // 3 seconds pause
        }
    }

    // 9. Check Stage Clear
    if (s.remainingBricks <= 0) {
        s.state = 2;
        s.pauseTimer = 75; // 2.5 seconds pause
        s.score += 2000; // Stage clear bonus
        if (s.score > s.highScore) s.highScore = s.score;

        // Celebratory fireworks cascade
        for (int k = 0; k < 40; ++k) {
            float fx = (float)(courtLeft + rand() % courtWidth);
            float fy = (float)(40 + rand() % 80);
            float angle = (float)(rand() % 628) / 100.0f;
            float spd = 0.5f + (rand() % 150) * 0.01f;
            uint32_t c = (rand() % 2 == 0) ? COL_GOLD : COL_GREEN_EMERALD;
            s.sparks.push_back({ fx, fy, std::cos(angle) * spd, std::sin(angle) * spd, 0, 30, c });
        }
    }
}

// ============================================================================
// COMPLETE RETRO ARCADE FRAME RENDERING
// ============================================================================
static void RenderArcadeFrame(ArkanoidState& s) {
    // 1. Clear Framebuffer to Obsidian Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 2. Playfield Arena Double-Line Neon Borders & Cyber Grid
    DrawArcadeBorder(s);
    DrawCyberGrid(s);

    // 3. Render Bricks
    for (const auto& brk : s.bricks) {
        if (!brk.active) continue;
        int bx1 = s.brickStartX + brk.col * s.brickW;
        int by1 = 40 + brk.row * s.brickH;
        int bx2 = bx1 + s.brickW - 3;
        int by2 = by1 + s.brickH - 2;

        DrawBeveledBrick(s, bx1, by1, bx2, by2, brk.color, false);
    }

    // 4. Render Power-up Pills
    for (const auto& p : s.powerups) {
        if (!p.active) continue;
        int px1 = (int)p.x - 5;
        int py1 = (int)p.y - 3;
        int px2 = (int)p.x + 5;
        int py2 = (int)p.y + 3;

        uint32_t pCol = COL_GOLD;
        char pChar = 'P';
        switch (p.type) {
        case POW_LASER:  pCol = COL_CRIMSON;       pChar = 'L'; break;
        case POW_EXPAND: pCol = COL_BLUE_ELECTRIC; pChar = 'E'; break;
        case POW_MULTI:  pCol = COL_GREEN_EMERALD; pChar = 'M'; break;
        case POW_SLOW:   pCol = COL_CYAN_CORE;     pChar = 'S'; break;
        case POW_POINTS: pCol = COL_GOLD;          pChar = 'P'; break;
        default: break;
        }

        FillRect(s, px1, py1, px2, py2, pCol);
        DrawHLine(s, px1 + 1, px2 - 1, py1, COL_WHITE);
        DrawGlyphChar(s, px1 + 3, py1, pChar, COL_WHITE);
    }

    // 5. Render Lasers
    for (const auto& l : s.lasers) {
        if (!l.active) continue;
        int lx = (int)l.x;
        int ly = (int)l.y;
        DrawVLine(s, lx, ly - 4, ly, COL_CRIMSON);
        PutPixel(s, lx, ly - 5, COL_WHITE);
    }

    // 6. Render Vaus (Paddle)
    int px1 = (int)s.paddleX;
    int py1 = (int)s.paddleY;
    int px2 = (int)(s.paddleX + s.paddleW - 1.0f);
    int py2 = py1 + (int)s.paddleH - 1;

    // Metallic Capsule Paddle Ends
    FillRect(s, px1, py1, px2, py2, COL_GRAY_MID);
    DrawHLine(s, px1 + 2, px2 - 2, py1, COL_WHITE); // Chrome top highlight
    DrawHLine(s, px1 + 2, px2 - 2, py2, COL_GRAY_DARK); // Shadow bottom

    // Glowing Neon Cyan Center Energy Core
    int coreMargin = (int)(s.paddleW * 0.25f);
    uint32_t coreCol = (s.currentPower == POW_LASER) ? COL_CRIMSON : COL_CYAN_NEON;
    FillRect(s, px1 + coreMargin, py1 + 2, px2 - coreMargin, py2 - 1, coreCol);

    // Twin Laser Blaster Pods
    if (s.currentPower == POW_LASER) {
        FillRect(s, px1, py1 - 2, px1 + 2, py1, COL_CRIMSON);
        FillRect(s, px2 - 2, py1 - 2, px2, py1, COL_CRIMSON);
    }

    // 7. Render Energy Balls
    for (const auto& b : s.balls) {
        if (!b.active) continue;
        int bx = (int)b.x;
        int by = (int)b.y;

        // 5x5 Circular Energy Sphere
        FillRect(s, bx - 1, by - 2, bx + 1, by + 2, COL_CYAN_CORE);
        FillRect(s, bx - 2, by - 1, bx + 2, by + 1, COL_CYAN_CORE);
        FillRect(s, bx - 1, by - 1, bx + 1, by + 1, COL_WHITE);
    }

    // 8. Sparkle Particles
    for (const auto& sp : s.sparks) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }

    // 9. Floating Score Popups
    for (const auto& fs : s.floatingScores) {
        char sBuf[16];
        sprintf_s(sBuf, "+%d", fs.score);
        DrawText(s, (int)fs.x - 10, (int)fs.y, sBuf, fs.color);
    }

    // 10. Retro Arcade Top HUD
    int playLeft = 14;

    // Top Left: Game Title & Round
    DrawText(s, playLeft + 2, 4, "ARKANOID NEON", COL_CYAN_NEON);
    char roundBuf[32];
    sprintf_s(roundBuf, "ROUND: %02d", s.round);
    DrawText(s, playLeft + 2, 13, roundBuf, COL_GRAY_LIGHT);

    // Top Center: Live Score & High Score
    int midX = s.virtualW / 2;
    char scoreBuf[32];
    sprintf_s(scoreBuf, "SCORE: %06d", s.score);
    int sw = (int)strlen(scoreBuf) * 6;
    DrawText(s, midX - sw / 2, 4, scoreBuf, COL_GOLD);

    char highBuf[32];
    sprintf_s(highBuf, "HIGH: %06d", s.highScore);
    int hw = (int)strlen(highBuf) * 6;
    DrawText(s, midX - hw / 2, 13, highBuf, COL_WHITE);

    // Top Right: Power-up Status & Lives
    char pwrBuf[32];
    if (s.currentPower == POW_LASER) sprintf_s(pwrBuf, "POW: LASER");
    else if (s.currentPower == POW_EXPAND) sprintf_s(pwrBuf, "POW: EXPAND");
    else if (s.currentPower == POW_MULTI) sprintf_s(pwrBuf, "POW: MULTI");
    else if (s.currentPower == POW_SLOW) sprintf_s(pwrBuf, "POW: SLOW");
    else sprintf_s(pwrBuf, "STATUS: PLAY");
    DrawText(s, s.virtualW - playLeft - 84, 4, pwrBuf, (s.currentPower != POW_NONE) ? COL_GREEN_LIME : COL_GRAY_LIGHT);

    char livesBuf[32];
    sprintf_s(livesBuf, "LIVES: %d", s.lives);
    DrawText(s, s.virtualW - playLeft - 84, 13, livesBuf, (s.lives > 1) ? COL_GREEN_EMERALD : COL_CRIMSON);

    // Center Banners (Clear / Game Over)
    if (s.state == 2) {
        const char* clrTxt = "ROUND CLEAR!";
        int tw = (int)strlen(clrTxt) * 6;
        DrawText(s, midX - tw / 2, 130, clrTxt, COL_GREEN_LIME);
    } else if (s.state == 3) {
        const char* goTxt = "GAME OVER";
        int tw = (int)strlen(goTxt) * 6;
        DrawText(s, midX - tw / 2, 130, goTxt, COL_CRIMSON);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderArkanoid(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<ArkanoidState>(35);

    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    vW = (vW + 1) & ~1; // Force strictly even virtual width for exact bilateral symmetry
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    if (!s.initialized || s.virtualW != vW || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);
        s.state = 0;
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
        StepBreakoutSimulation(s);
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

// Register as Screensaver ID 35
REGISTER_SCREENSAVER(
    35,
    L"Arkanoid (Breakout Neon)",
    "arkanoid",
    { "arkanoid", "breakout", "neon-breakout", "bricks" },
    WRAP_LEGACY(RenderArkanoid),
    GetBreakoutSettings()
);
