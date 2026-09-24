#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/PongSettings.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic Retro Arcade Cyber-Pong Simulation
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio (e.g. 455x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;

// 32-bit ARGB Palette (Rich Arcade Neon & Metallic Palette matching Snake & Tetris)
static constexpr uint32_t COL_BLACK          = 0xFF050508;
static constexpr uint32_t COL_WHITE          = 0xFFFFFFFF;
static constexpr uint32_t COL_CYAN_NEON      = 0xFF00E5FF;
static constexpr uint32_t COL_CYAN_GLOW      = 0xFF18FFFF;
static constexpr uint32_t COL_CYAN_DARK      = 0xFF006064;
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744;
static constexpr uint32_t COL_CRIMSON_GLOW   = 0xFFFF5252;
static constexpr uint32_t COL_CRIMSON_DARK   = 0xFF880E4F;
static constexpr uint32_t COL_GOLD           = 0xFFFFD700;
static constexpr uint32_t COL_YELLOW         = 0xFFFFEA00;
static constexpr uint32_t COL_ORANGE         = 0xFFFF9100;
static constexpr uint32_t COL_GREEN_LIME     = 0xFF00FF66;
static constexpr uint32_t COL_GREEN_EMERALD  = 0xFF00E676;
static constexpr uint32_t COL_GRAY_LIGHT     = 0xFFB0B0C0;
static constexpr uint32_t COL_GRAY_DARK      = 0xFF353545;
static constexpr uint32_t COL_COURT_DOT      = 0xFF0D1424;
static constexpr uint32_t COL_BORDER_OUTER   = 0xFF1C2848;
static constexpr uint32_t COL_BORDER_INNER   = 0xFF00E5FF;

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
    default:  return GLYPH_BLANK;
    }
}

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================

struct BallTrailNode {
    float x;
    float y;
    uint32_t color;
};

struct PongSparkle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    uint32_t color;
};

struct PongPopup {
    float x, y;
    char text[16];
    int timer;
    uint32_t color;
};

struct PongState {
    bool initialized = false;
    uint64_t lastTick = 0;

    int virtualW = 455;
    int virtualH = BASE_ARCADE_H;
    std::vector<uint32_t> fb;

    // Ball Dynamics
    float ballX = -1.0f;
    float ballY = -1.0f;
    float ballDX = 0.0f;
    float ballDY = 0.0f;
    float ballSpeed = 2.8f;
    int lastHitter = 1; // 1 = Left (Cyan), 2 = Right (Crimson)
    int goalPauseTimer = 0;

    // Paddle Dynamics (Left: Cyan, Right: Crimson)
    float padLeftY = 0.0f;
    float padRightY = 0.0f;
    float padLeftTargetY = 0.0f;
    float padRightTargetY = 0.0f;
    float padLeftError = 0.0f;
    float padRightError = 0.0f;
    int padLeftHitTimer = 0;
    int padRightHitTimer = 0;
    int padLeftLag = 0;
    int padRightLag = 0;

    // Scoring & Match State
    int scoreP1 = 0;
    int scoreP2 = 0;
    int currentRally = 0;
    int maxRally = 0;
    int matchWinTimer = 0;
    int matchWinner = 0;

    // FX & Aesthetics
    std::vector<BallTrailNode> trail;
    std::vector<PongSparkle> sparkles;
    std::vector<PongPopup> popups;
};

// ============================================================================
// FRAMEBUFFER DRAWING UTILITIES
// ============================================================================

static inline void PutPixel(PongState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static inline void PutPixelBlend(PongState& s, int x, int y, uint32_t color, float alpha) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        if (alpha >= 1.0f) {
            s.fb[y * s.virtualW + x] = color;
            return;
        }
        uint32_t bg = s.fb[y * s.virtualW + x];
        uint32_t r1 = (bg >> 16) & 0xFF;
        uint32_t g1 = (bg >> 8) & 0xFF;
        uint32_t b1 = bg & 0xFF;
        uint32_t r2 = (color >> 16) & 0xFF;
        uint32_t g2 = (color >> 8) & 0xFF;
        uint32_t b2 = color & 0xFF;
        uint32_t r = (uint32_t)(r1 * (1.0f - alpha) + r2 * alpha);
        uint32_t g = (uint32_t)(g1 * (1.0f - alpha) + g2 * alpha);
        uint32_t b = (uint32_t)(b1 * (1.0f - alpha) + b2 * alpha);
        s.fb[y * s.virtualW + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

static inline void DrawHLine(PongState& s, int x1, int x2, int y, uint32_t col) {
    if (y < 0 || y >= s.virtualH) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = (std::max)(0, x1);
    x2 = (std::min)(s.virtualW - 1, x2);
    for (int x = x1; x <= x2; ++x) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static inline void DrawVLine(PongState& s, int x, int y1, int y2, uint32_t col) {
    if (x < 0 || x >= s.virtualW) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = (std::max)(0, y1);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        s.fb[y * s.virtualW + x] = col;
    }
}

static void DrawText(PongState& s, int x, int y, const char* str, uint32_t color, int scale = 1) {
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

// Double-Line Neon Arcade Borders (Snake & Tetris styling)
static void DrawPongBorder(PongState& s) {
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

// Center Partition Net (Authentic Retro Arcade Dotted/Dashed Line dividing the two court halves)
static void DrawCenterNet(PongState& s) {
    int midX = s.virtualW / 2;
    int dashH = 5;
    int dashGap = 5;
    int startY = 36;
    int endY = 240; // Strictly inside court bounds (court is 29..247, inner borders at 28 and 248)

    for (int y = startY; y + dashH - 1 <= endY; y += dashH + dashGap) {
        DrawVLine(s, midX - 1, y, y + dashH - 1, COL_CYAN_DARK);
        DrawVLine(s, midX,     y, y + dashH - 1, COL_CYAN_NEON);
        DrawVLine(s, midX + 1, y, y + dashH - 1, COL_CYAN_DARK);

        // Power node core
        PutPixel(s, midX, y + dashH / 2, COL_WHITE);
    }
}

// ============================================================================
// TEXTURED PADDLES & HIGH-ENERGY BALL
// ============================================================================

static void DrawPaddle(PongState& s, int px, int py, int w, int h, bool isLeft, int hitTimer) {
    bool flash = (hitTimer > 0);

    for (int y = 0; y < h; ++y) {
        bool isCap = (y < 2 || y >= h - 2);
        bool isCore = (y >= h / 2 - 3 && y <= h / 2 + 2);

        for (int x = 0; x < w; ++x) {
            uint32_t col;

            if (isLeft) {
                // Player 1: Neon Cyan Cyber-Paddle
                if (flash) {
                    col = (x == w - 1 || isCore) ? COL_WHITE : COL_CYAN_GLOW;
                } else if (isCap) {
                    col = COL_CYAN_DARK;
                } else if (x == w - 1) {
                    col = COL_WHITE; // Striking face highlight
                } else if (x == w - 2) {
                    col = isCore ? COL_WHITE : COL_CYAN_GLOW;
                } else if (x == 1) {
                    col = COL_CYAN_NEON;
                } else {
                    col = COL_CYAN_DARK; // Outer chassis
                }
            } else {
                // Player 2: Vivid Crimson / Amber Cyber-Paddle
                if (flash) {
                    col = (x == 0 || isCore) ? COL_WHITE : COL_CRIMSON_GLOW;
                } else if (isCap) {
                    col = COL_CRIMSON_DARK;
                } else if (x == 0) {
                    col = COL_WHITE; // Striking face highlight
                } else if (x == 1) {
                    col = isCore ? COL_WHITE : COL_CRIMSON_GLOW;
                } else if (x == w - 2) {
                    col = COL_CRIMSON;
                } else {
                    col = COL_CRIMSON_DARK; // Outer chassis
                }
            }

            PutPixel(s, px + x, py + y, col);
        }
    }
}

static void DrawPlasmaBall(PongState& s, int bx, int by, int size, uint32_t color) {
    // 4x4 high-energy plasma ball with pure white core and outer glow
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool isCore = (x >= 1 && x <= size - 2 && y >= 1 && y <= size - 2);
            uint32_t col = isCore ? COL_WHITE : color;
            PutPixel(s, bx + x, by + y, col);
        }
    }
}

// ============================================================================
// SIMULATION & AUTONOMOUS AI
// ============================================================================

static void ServeBall(PongState& s, int scoringPlayer) {
    s.goalPauseTimer = 28; // ~0.45s pause before serve
    s.ballX = (float)(s.virtualW / 2 - 2);
    s.ballY = (float)(s.virtualH / 2 - 2);

    float baseSpd = (g_PongSpeed / 25.0f) * 2.8f;
    if (baseSpd < 1.8f) baseSpd = 1.8f;
    if (baseSpd > 5.5f) baseSpd = 5.5f;
    s.ballSpeed = baseSpd;

    // Serve towards player who conceded the point
    float angle = ((rand() % 40) - 20) * (3.14159f / 180.0f);
    float dirX = (scoringPlayer == 1) ? 1.0f : -1.0f;
    s.ballDX = dirX * cosf(angle) * baseSpd;
    s.ballDY = sinf(angle) * baseSpd;
    s.lastHitter = (scoringPlayer == 1) ? 1 : 2;
    s.currentRally = 0;
    s.padLeftError = (float)((rand() % 13) - 6);
    s.padRightError = (float)((rand() % 13) - 6);
    s.padLeftLag = 0;
    s.padRightLag = 0;
    s.trail.clear();
}

static void ResetPongMatch(PongState& s) {
    s.scoreP1 = 0;
    s.scoreP2 = 0;
    s.currentRally = 0;
    s.maxRally = 0;
    s.matchWinTimer = 0;
    s.matchWinner = 0;

    int padH = 32;
    s.padLeftY = (float)(s.virtualH / 2 - padH / 2);
    s.padRightY = (float)(s.virtualH / 2 - padH / 2);
    s.padLeftTargetY = s.padLeftY;
    s.padRightTargetY = s.padRightY;
    s.padLeftError = 0.0f;
    s.padRightError = 0.0f;

    s.sparkles.clear();
    s.popups.clear();
    ServeBall(s, 1);
}

static void UpdatePongGame(PongState& s) {
    int padW = 5;
    int padH = 32;
    int ballSz = 4;

    int courtTop = 29;
    int courtBottom = 247;
    int padLeftX = 22;
    int padRightX = s.virtualW - 27;

    // Match Won Celebration
    if (s.matchWinTimer > 0) {
        s.matchWinTimer--;
        if (s.matchWinTimer == 0) {
            ResetPongMatch(s);
        }
        return;
    }

    if (s.goalPauseTimer > 0) {
        s.goalPauseTimer--;
        return;
    }

    if (s.padLeftHitTimer > 0) s.padLeftHitTimer--;
    if (s.padRightHitTimer > 0) s.padRightHitTimer--;

    // 1. Motion Trail Update
    uint32_t trailCol = (s.lastHitter == 1) ? COL_CYAN_NEON : COL_CRIMSON;
    s.trail.push_back({ s.ballX, s.ballY, trailCol });
    size_t maxTrail = 6 + (size_t)((std::min)(10, s.currentRally / 2));
    if (s.trail.size() > maxTrail) {
        s.trail.erase(s.trail.begin());
    }

    // 2. Ball Movement
    s.ballX += s.ballDX;
    s.ballY += s.ballDY;

    // Top / Bottom Wall Bounces
    if (s.ballY <= (float)courtTop) {
        s.ballY = (float)courtTop;
        s.ballDY = fabsf(s.ballDY);

        // Wall spark burst
        for (int p = 0; p < 6; ++p) {
            float ang = (float)p * (3.14159f / 5.0f);
            s.sparkles.push_back({ s.ballX + 2.0f, (float)courtTop, cosf(ang) * 1.5f, fabsf(sinf(ang)) * 1.5f, 0, 12, COL_CYAN_NEON });
        }
    } else if (s.ballY + ballSz >= (float)courtBottom) {
        s.ballY = (float)(courtBottom - ballSz);
        s.ballDY = -fabsf(s.ballDY);

        for (int p = 0; p < 6; ++p) {
            float ang = (float)p * (3.14159f / 5.0f);
            s.sparkles.push_back({ s.ballX + 2.0f, (float)courtBottom, cosf(ang) * 1.5f, -fabsf(sinf(ang)) * 1.5f, 0, 12, COL_CYAN_NEON });
        }
    }

    // 3. Autonomous Paddle AI with Realistic Reaction & Deflection
    float courtMidX = (float)(s.virtualW / 2);
    float paddleMaxSpd = 2.45f;

    // Left Paddle AI: reacts when ball is heading left and has crossed into tracking zone
    if (s.ballDX < 0.0f && s.ballX < courtMidX + 75.0f) {
        if (s.padLeftLag > 0) {
            s.padLeftLag--;
            s.padLeftTargetY = s.padLeftY; // brief hesitation / human reaction window
        } else {
            s.padLeftTargetY = s.ballY + (ballSz / 2.0f) - (padH / 2.0f) + s.padLeftError;
        }
    } else {
        // Return towards neutral center position
        s.padLeftTargetY = (float)(s.virtualH / 2 - padH / 2);
    }
    float diffL = s.padLeftTargetY - s.padLeftY;
    if (fabsf(diffL) <= paddleMaxSpd) s.padLeftY = s.padLeftTargetY;
    else s.padLeftY += (diffL > 0 ? 1.0f : -1.0f) * paddleMaxSpd;

    // Right Paddle AI: reacts when ball is heading right and has crossed into tracking zone
    if (s.ballDX > 0.0f && s.ballX > courtMidX - 75.0f) {
        if (s.padRightLag > 0) {
            s.padRightLag--;
            s.padRightTargetY = s.padRightY;
        } else {
            s.padRightTargetY = s.ballY + (ballSz / 2.0f) - (padH / 2.0f) + s.padRightError;
        }
    } else {
        // Return towards neutral center position
        s.padRightTargetY = (float)(s.virtualH / 2 - padH / 2);
    }
    float diffR = s.padRightTargetY - s.padRightY;
    if (fabsf(diffR) <= paddleMaxSpd) s.padRightY = s.padRightTargetY;
    else s.padRightY += (diffR > 0 ? 1.0f : -1.0f) * paddleMaxSpd;

    // Clamp Paddles within Court
    if (s.padLeftY < (float)courtTop) s.padLeftY = (float)courtTop;
    if (s.padLeftY + padH > (float)courtBottom) s.padLeftY = (float)(courtBottom - padH);
    if (s.padRightY < (float)courtTop) s.padRightY = (float)courtTop;
    if (s.padRightY + padH > (float)courtBottom) s.padRightY = (float)(courtBottom - padH);

    // 4. Paddle Collision Detection
    // Left Paddle Hit
    if (s.ballDX < 0.0f && s.ballX <= (float)(padLeftX + padW) && s.ballX >= (float)(padLeftX - 2)) {
        if (s.ballY + ballSz >= s.padLeftY && s.ballY <= s.padLeftY + padH) {
            s.ballX = (float)(padLeftX + padW);
            float hitRel = ((s.ballY + ballSz / 2.0f) - (s.padLeftY + padH / 2.0f)) / (padH / 2.0f);
            float angle = hitRel * 0.95f; // up to ~54 degree angle

            s.currentRally++;
            if (s.currentRally > s.maxRally) s.maxRally = s.currentRally;

            // Noticeable acceleration each hit up to 2.1x base speed
            float spd = s.ballSpeed * (1.0f + (std::min)(1.10f, s.currentRally * 0.08f));

            s.ballDX = fabsf(cosf(angle) * spd);
            s.ballDY = sinf(angle) * spd;
            s.lastHitter = 1;
            s.padLeftHitTimer = 6;

            // Opponent AI uncertainty scales with rally speed and cut angle
            float errRange = 7.0f + (std::min)(18.0f, s.currentRally * 1.5f);
            if (fabsf(hitRel) > 0.65f) errRange += 4.0f; // steep angle cut shot
            float errSign = (rand() % 2 == 0) ? 1.0f : -1.0f;
            float errFrac = (float)(rand() % 100) / 100.0f;
            s.padRightError = errSign * errFrac * errRange;

            // Human reaction delay occasionally introduced on higher rallies
            s.padRightLag = (s.currentRally >= 4 && (rand() % 3 == 0)) ? (3 + rand() % 6) : (rand() % 3);

            // Particle Starburst
            for (int p = 0; p < 8; ++p) {
                float ang = (float)p * (3.14159f / 4.0f);
                s.sparkles.push_back({ (float)(padLeftX + padW), s.ballY + 2.0f, cosf(ang) * 1.8f, sinf(ang) * 1.8f, 0, 14, COL_CYAN_NEON });
            }
        }
    }

    // Right Paddle Hit
    if (s.ballDX > 0.0f && s.ballX + ballSz >= (float)padRightX && s.ballX + ballSz <= (float)(padRightX + padW + 2)) {
        if (s.ballY + ballSz >= s.padRightY && s.ballY <= s.padRightY + padH) {
            s.ballX = (float)(padRightX - ballSz);
            float hitRel = ((s.ballY + ballSz / 2.0f) - (s.padRightY + padH / 2.0f)) / (padH / 2.0f);
            float angle = hitRel * 0.95f;

            s.currentRally++;
            if (s.currentRally > s.maxRally) s.maxRally = s.currentRally;

            float spd = s.ballSpeed * (1.0f + (std::min)(1.10f, s.currentRally * 0.08f));

            s.ballDX = -fabsf(cosf(angle) * spd);
            s.ballDY = sinf(angle) * spd;
            s.lastHitter = 2;
            s.padRightHitTimer = 6;

            float errRange = 7.0f + (std::min)(18.0f, s.currentRally * 1.5f);
            if (fabsf(hitRel) > 0.65f) errRange += 4.0f;
            float errSign = (rand() % 2 == 0) ? 1.0f : -1.0f;
            float errFrac = (float)(rand() % 100) / 100.0f;
            s.padLeftError = errSign * errFrac * errRange;

            s.padLeftLag = (s.currentRally >= 4 && (rand() % 3 == 0)) ? (3 + rand() % 6) : (rand() % 3);

            // Particle Starburst
            for (int p = 0; p < 8; ++p) {
                float ang = (float)p * (3.14159f / 4.0f);
                s.sparkles.push_back({ (float)padRightX, s.ballY + 2.0f, cosf(ang) * 1.8f, sinf(ang) * 1.8f, 0, 14, COL_CRIMSON });
            }
        }
    }

    // 5. Goal Detection / Scoring
    // Player 2 Scores!
    if (s.ballX < 8.0f) {
        s.scoreP2++;
        for (int p = 0; p < 16; ++p) {
            float ang = (float)p * (6.2831853f / 16.0f);
            s.sparkles.push_back({ 14.0f, s.ballY, cosf(ang) * 2.2f, sinf(ang) * 2.2f, 0, 20, COL_CRIMSON });
        }
        s.popups.push_back({ 24.0f, s.ballY - 4.0f, "POINT!", 0, COL_CRIMSON });

        if (s.scoreP2 >= 11 && s.scoreP2 - s.scoreP1 >= 2) {
            s.matchWinTimer = 110;
            s.matchWinner = 2;
        } else {
            ServeBall(s, 2);
        }
    }
    // Player 1 Scores!
    else if (s.ballX > (float)(s.virtualW - 12)) {
        s.scoreP1++;
        for (int p = 0; p < 16; ++p) {
            float ang = (float)p * (6.2831853f / 16.0f);
            s.sparkles.push_back({ (float)(s.virtualW - 15), s.ballY, cosf(ang) * 2.2f, sinf(ang) * 2.2f, 0, 20, COL_CYAN_NEON });
        }
        s.popups.push_back({ (float)(s.virtualW - 65), s.ballY - 4.0f, "POINT!", 0, COL_CYAN_NEON });

        if (s.scoreP1 >= 11 && s.scoreP1 - s.scoreP2 >= 2) {
            s.matchWinTimer = 110;
            s.matchWinner = 1;
        } else {
            ServeBall(s, 1);
        }
    }

    // 6. FX Updates
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
}

// ============================================================================
// COMPLETE ARCADE FRAME RENDERING
// ============================================================================

static void RenderArcadeFrame(PongState& s) {
    // 1. Clear Framebuffer to Obsidian Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 2. Playfield Arena Double-Line Neon Borders & Court Net
    DrawPongBorder(s);
    DrawCenterNet(s);

    // 3. Render Motion Trail
    for (size_t i = 0; i < s.trail.size(); ++i) {
        float alpha = (float)(i + 1) / (float)s.trail.size() * 0.5f;
        int tx = (int)s.trail[i].x;
        int ty = (int)s.trail[i].y;
        PutPixelBlend(s, tx + 1, ty + 1, s.trail[i].color, alpha);
        PutPixelBlend(s, tx + 2, ty + 1, s.trail[i].color, alpha);
        PutPixelBlend(s, tx + 1, ty + 2, s.trail[i].color, alpha);
        PutPixelBlend(s, tx + 2, ty + 2, s.trail[i].color, alpha);
    }

    // 4. Render Textured Paddles
    int padW = 5;
    int padH = 32;
    int padLeftX = 22;
    int padRightX = s.virtualW - 27;

    DrawPaddle(s, padLeftX, (int)s.padLeftY, padW, padH, true, s.padLeftHitTimer);
    DrawPaddle(s, padRightX, (int)s.padRightY, padW, padH, false, s.padRightHitTimer);

    // 5. Render Plasma Ball
    uint32_t ballCol = (s.lastHitter == 1) ? COL_CYAN_NEON : COL_CRIMSON;
    DrawPlasmaBall(s, (int)s.ballX, (int)s.ballY, 4, ballCol);

    // 6. Particle Sparkles
    for (const auto& sp : s.sparkles) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }

    // 7. Floating Score Popups
    for (const auto& pop : s.popups) {
        DrawText(s, (int)pop.x, (int)pop.y, pop.text, pop.color);
    }

    // 8. Top HUD (Clean Snake & Tetris styling)
    int playLeft = 14;

    // Player 1 Header & 2x Score
    DrawText(s, playLeft + 12, 4, "PLAYER 1", COL_CYAN_NEON);
    char p1Buf[8];
    sprintf_s(p1Buf, "%02d", s.scoreP1);
    DrawText(s, playLeft + 18, 12, p1Buf, COL_WHITE, 1);

    // Center Rally Counter
    int midX = s.virtualW / 2;
    char rallyBuf[16];
    sprintf_s(rallyBuf, "RALLY %d", s.currentRally);
    int rw = (int)strlen(rallyBuf) * 6;
    DrawText(s, midX - rw / 2, 4, rallyBuf, COL_GOLD);

    char bestBuf[16];
    sprintf_s(bestBuf, "BEST %d", s.maxRally);
    int bw = (int)strlen(bestBuf) * 6;
    DrawText(s, midX - bw / 2, 13, bestBuf, COL_GRAY_LIGHT);

    // Player 2 Header & 2x Score
    DrawText(s, s.virtualW - playLeft - 56, 4, "PLAYER 2", COL_CRIMSON);
    char p2Buf[8];
    sprintf_s(p2Buf, "%02d", s.scoreP2);
    DrawText(s, s.virtualW - playLeft - 38, 12, p2Buf, COL_WHITE, 1);

    // 9. Match Win Banner
    if (s.matchWinTimer > 0) {
        const char* winMsg = (s.matchWinner == 1) ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
        uint32_t winCol = (s.matchWinner == 1) ? COL_CYAN_NEON : COL_CRIMSON;
        int mw = (int)strlen(winMsg) * 6;
        DrawText(s, midX - mw / 2, 116, winMsg, winCol);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================

void RenderPong(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<PongState>(9);

    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    if (!s.initialized || s.virtualW != vW || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);
        ResetPongMatch(s);
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200;
    s.lastTick = now;

    int steps = (int)(elapsed / 16);
    if (steps < 1) steps = 1;
    if (steps > 4) steps = 4;

    for (int i = 0; i < steps; ++i) {
        UpdatePongGame(s);
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

// Register as Screensaver ID 9
REGISTER_SCREENSAVER(
    9,
    L"Pong",
    "pong",
    { "pong" },
    WRAP_LEGACY(RenderPong),
    GetPongSettings()
);
