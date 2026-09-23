#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic 1978/1980s Retro Arcade Space Invaders / Galaga Simulation
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio (e.g. 455x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;

// 32-bit ARGB Palette
static constexpr uint32_t COL_BLACK     = 0xFF000000;
static constexpr uint32_t COL_WHITE     = 0xFFFFFFFF;
static constexpr uint32_t COL_GREEN     = 0xFF00FF00;
static constexpr uint32_t COL_RED       = 0xFFFF2020;
static constexpr uint32_t COL_CYAN      = 0xFF40E0D0;
static constexpr uint32_t COL_YELLOW    = 0xFFFFFF00;
static constexpr uint32_t COL_DIM_GREEN = 0xFF008000;

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
    case '<': { static const uint8_t g[7] = { 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 }; return g; }
    case '>': { static const uint8_t g[7] = { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    case ':': { static const uint8_t g[7] = { 0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00 }; return g; }
    case '!': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }; return g; }
    case '*': { static const uint8_t g[7] = { 0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00 }; return g; }
    default:  return GLYPH_BLANK;
    }
}

// ============================================================================
// SPRITE DEFINITIONS (Pixel-accurate, 100% Symmetrical 1-bit Masks)
// ============================================================================

// 1. Squid (Row 0, 8x8)
static const uint8_t SPRITE_SQUID[2][8] = {
    { 0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5 }, // Frame 0
    { 0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x42, 0x24, 0x18 }  // Frame 1
};

// 2. Crab (Rows 1 & 2, 11x8) - 11-bit mask (bits 10 down to 0)
static const uint16_t SPRITE_CRAB[2][8] = {
    { 0x104, 0x088, 0x1FC, 0x376, 0x7FF, 0x5FD, 0x505, 0x0D8 }, // Frame 0
    { 0x104, 0x489, 0x5FD, 0x777, 0x7FF, 0x3FE, 0x104, 0x202 }  // Frame 1
};

// 3. Octopus (Rows 3 & 4, 12x8) - 12-bit mask (bits 11 down to 0)
static const uint16_t SPRITE_OCTOPUS[2][8] = {
    { 0x0F0, 0x7FE, 0xFFF, 0xE67, 0xFFF, 0x198, 0x36C, 0xC03 }, // Frame 0
    { 0x0F0, 0x7FE, 0xFFF, 0xE67, 0xFFF, 0x30C, 0x666, 0x30C }  // Frame 1
};

// 4. Player Cannon (15x8)
static const uint16_t SPRITE_PLAYER[8] = {
    0x0080, 0x01C0, 0x01C0, 0x3FFE, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF
};

// 5. Player Explosion (15x8, 2 frames)
static const uint16_t SPRITE_PLAYER_EXP[2][8] = {
    { 0x0240, 0x1008, 0x2424, 0x0180, 0x4812, 0x1248, 0x2004, 0x0180 },
    { 0x0810, 0x0240, 0x4002, 0x1818, 0x2424, 0x0420, 0x1188, 0x0240 }
};

// 6. Alien Explosion (13x8)
static const uint16_t SPRITE_ALIEN_EXP[8] = {
    0x0440, 0x0220, 0x0110, 0x1008, 0x0810, 0x0220, 0x0440, 0x0000
};

// 7. Mystery Flying Saucer / UFO (16x7)
static const uint16_t SPRITE_UFO[7] = {
    0x07E0, 0x1FF8, 0x3FFC, 0x6DB6, 0xFFFF, 0x1CE8, 0x0820
};

// 8. Bunker initial mask: 22 wide x 16 high (1 = solid pixel, 0 = empty)
static const uint32_t BUNKER_TEMPLATE[16] = {
    0x000FF0, // row 0:  4 empty, 14 solid, 4 empty
    0x003FFC, // row 1:  2 empty, 18 solid, 2 empty
    0x007FFE, // row 2:  1 empty, 20 solid, 1 empty
    0x00FFFF, // row 3:  22 solid
    0x00FFFF, // row 4
    0x00FFFF, // row 5
    0x00FFFF, // row 6
    0x00FFFF, // row 7
    0x00FFFF, // row 8
    0x00FFFF, // row 9
    0x00FFFF, // row 10
    0x007E7E, // row 11: 7 solid, 8 arch cutout, 7 solid
    0x007E7E, // row 12
    0x007C3E, // row 13: 6 solid, 10 arch cutout, 6 solid
    0x00781E, // row 14: 5 solid, 12 arch cutout, 5 solid
    0x00781E  // row 15
};

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================

struct SpaceStar {
    float x = 0;
    float y = 0;
    float speed = 0.2f;
    uint32_t color = COL_WHITE;
    int twinkleTimer = 0;
};

struct Bunker {
    int x = 0;
    int y = 0;
    uint8_t pixels[16][22]; // 1 = intact, 0 = destroyed
};

struct Bomb {
    float x = 0;
    float y = 0;
    float vy = 1.6f;
    int type = 0; // 0 = rolling, 1 = squiggly, 2 = plunger
    int animFrame = 0;
    bool active = false;
};

struct Alien {
    bool alive = true;
    int row = 0;    // 0..4
    int col = 0;    // 0..10
    int scoreVal = 10;
};

struct InvadersState {
    bool initialized = false;
    uint64_t lastTick = 0;

    int virtualW = 455;
    int virtualH = BASE_ARCADE_H;

    // Background Stars (Galaga style)
    std::vector<SpaceStar> stars;

    // Aliens Swarm
    static constexpr int ROWS = 5;
    static constexpr int COLS = 11;
    Alien aliens[ROWS][COLS];
    int aliveCount = 55;

    float fleetX = 60.0f;
    float fleetY = 56.0f;
    float fleetDir = 1.0f; // +1 right, -1 left
    int animStep = 0;
    int stepTimer = 0;
    int stepInterval = 28; // Accelerates as aliens are destroyed

    // Player Cannon
    float playerX = 200.0f;
    float playerTargetX = 200.0f;
    bool playerAlive = true;
    int playerExplodeTimer = 0;
    int lives = 3;

    // Projectiles
    bool laserActive = false;
    float laserX = 0;
    float laserY = 0;

    std::vector<Bomb> bombs;
    int bombTimer = 0;

    // Bunkers (4 shields)
    Bunker bunkers[4];

    // Mystery UFO
    bool ufoActive = false;
    float ufoX = 0;
    float ufoY = 36;
    float ufoSpeed = 1.0f;
    int ufoSpawnTimer = 0;
    int ufoScoreDisplay = 0;
    int ufoScoreTimer = 0;
    float ufoScoreX = 0;

    // Splats / Explosions
    int alienExplodeX = -1;
    int alienExplodeY = -1;
    int alienExplodeTimer = 0;

    // Score & Wave
    int score = 0;
    int highScore = 1880;
    int wave = 1;
    int gameOverTimer = 0;
    int waveClearTimer = 0;

    // Internal Dynamic Framebuffer
    std::vector<uint32_t> fb;
};

// ============================================================================
// BITMAP DRAWING HELPERS (Direct into Virtual Framebuffer)
// ============================================================================

static inline void PutPixel(InvadersState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void DrawChar(InvadersState& s, int x, int y, char c, uint32_t color) {
    const uint8_t* glyph = GetGlyph(c);
    for (int r = 0; r < 7; ++r) {
        uint8_t rowBits = glyph[r];
        for (int col = 0; col < 5; ++col) {
            if ((rowBits >> (4 - col)) & 1) {
                PutPixel(s, x + col, y + r, color);
            }
        }
    }
}

static void DrawText(InvadersState& s, int x, int y, const char* str, uint32_t color) {
    int curX = x;
    while (*str) {
        DrawChar(s, curX, y, *str, color);
        curX += 6; // 5 width + 1 spacing
        str++;
    }
}

static void DrawHLine(InvadersState& s, int x1, int x2, int y, uint32_t color) {
    if (y < 0 || y >= s.virtualH) return;
    int start = (std::max)(0, (std::min)(x1, x2));
    int end   = (std::min)(s.virtualW - 1, (std::max)(x1, x2));
    for (int x = start; x <= end; ++x) {
        s.fb[y * s.virtualW + x] = color;
    }
}

// Carve a circular explosion crater into a bunker
static void DamageBunker(Bunker& b, int hitX, int hitY, int radius) {
    int localX = hitX - b.x;
    int localY = hitY - b.y;

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= radius * radius + 1) {
                int px = localX + dx;
                int py = localY + dy;
                if (px >= 0 && px < 22 && py >= 0 && py < 16) {
                    b.pixels[py][px] = 0;
                }
            }
        }
    }
}

// Reset 4 bunkers to pristine condition, distributed evenly across virtual width
static void ResetBunkers(InvadersState& s) {
    const int bunkerCount = 4;
    const int bunkerWidth = 22;
    const int bunkerY = 194;
    const int totalBunkersW = bunkerCount * bunkerWidth;
    const int spacing = (s.virtualW - totalBunkersW) / (bunkerCount + 1);

    for (int i = 0; i < bunkerCount; ++i) {
        s.bunkers[i].x = spacing + i * (bunkerWidth + spacing);
        s.bunkers[i].y = bunkerY;
        for (int r = 0; r < 16; ++r) {
            uint32_t rowBits = BUNKER_TEMPLATE[r];
            for (int c = 0; c < 22; ++c) {
                s.bunkers[i].pixels[r][c] = ((rowBits >> (21 - c)) & 1) ? 1 : 0;
            }
        }
    }
}

// Initialize twinkling background starfield
static void InitStars(InvadersState& s) {
    s.stars.clear();
    int count = s.virtualW / 6; // ~70-90 stars
    static const uint32_t starPalette[] = {
        0xFFFFFFFF, 0xFFA0C0FF, 0xFFFFE080, 0xFFFF9090, 0xFF90FFA0
    };
    for (int i = 0; i < count; ++i) {
        SpaceStar st;
        st.x = (float)(rand() % s.virtualW);
        st.y = (float)(rand() % s.virtualH);
        st.speed = 0.15f + ((rand() % 100) / 350.0f);
        st.color = starPalette[rand() % 5];
        st.twinkleTimer = rand() % 60;
        s.stars.push_back(st);
    }
}

// Reset alien fleet for new wave
static void ResetFleet(InvadersState& s, bool fullReset) {
    s.aliveCount = 55;
    // Center fleet horizontally across the virtual widescreen
    const float fleetTotalW = (InvadersState::COLS - 1) * 16.0f + 12.0f; // ~172 px
    s.fleetX = (std::max)(20.0f, (s.virtualW - fleetTotalW) * 0.5f);
    s.fleetY = (float)(52 + ((s.wave - 1) % 4) * 6); // Each wave starts slightly lower
    s.fleetDir = 1.0f;
    s.animStep = 0;
    s.stepTimer = 0;
    s.stepInterval = (std::max)(6, 26 - (s.wave - 1) * 3);

    for (int r = 0; r < InvadersState::ROWS; ++r) {
        for (int c = 0; c < InvadersState::COLS; ++c) {
            s.aliens[r][c].alive = true;
            s.aliens[r][c].row = r;
            s.aliens[r][c].col = c;
            if (r == 0) s.aliens[r][c].scoreVal = 30;      // Squid
            else if (r <= 2) s.aliens[r][c].scoreVal = 20; // Crab
            else s.aliens[r][c].scoreVal = 10;            // Octopus
        }
    }

    s.bombs.clear();
    s.laserActive = false;
    s.ufoActive = false;
    s.ufoSpawnTimer = rand() % 300 + 200;

    if (fullReset) {
        s.playerX = s.virtualW * 0.5f - 7.0f;
        s.playerTargetX = s.playerX;
        s.playerAlive = true;
        s.playerExplodeTimer = 0;
        ResetBunkers(s);
    }
}

// Full game reset
static void ResetGame(InvadersState& s) {
    if (s.score > s.highScore) s.highScore = s.score;
    s.score = 0;
    s.lives = 3;
    s.wave = 1;
    s.gameOverTimer = 0;
    s.waveClearTimer = 0;
    s.alienExplodeTimer = 0;
    InitStars(s);
    ResetFleet(s, true);
}

// ============================================================================
// AUTONOMOUS AI CONTROLLER (Self-Playing Agent)
// ============================================================================

static void UpdatePlayerAI(InvadersState& s) {
    if (!s.playerAlive) return;

    const float cannonW = 15.0f;
    float bestX = s.playerX;
    float safestCost = 999999.0f;
    const float minX = 12.0f;
    const float maxX = (float)(s.virtualW - 27);

    // 1. Scan candidate positions across playfield (step by 3 pixels)
    for (float candX = minX; candX <= maxX; candX += 3.0f) {
        float candCenter = candX + cannonW * 0.5f;
        float cost = 0.0f;

        // A. Bomb Danger Avoidance (Heavily penalize positions under falling bombs)
        for (const auto& b : s.bombs) {
            if (!b.active) continue;

            float distY = 216.0f - b.y; // distance from ground
            if (distY > 0 && distY < 120.0f) {
                float distX = std::abs(candCenter - b.x);
                if (distX < 15.0f) {
                    // Check if sheltered by an intact bunker
                    bool sheltered = false;
                    for (const auto& bk : s.bunkers) {
                        if (b.x >= bk.x && b.x < bk.x + 22 && b.y < bk.y) {
                            int localX = (int)(b.x - bk.x);
                            for (int r = 0; r < 6; ++r) {
                                if (bk.pixels[r][localX]) {
                                    sheltered = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (!sheltered) {
                        // Severe danger penalty: closer bomb = exponential danger
                        cost += (120.0f - distY) * (15.0f - distX) * 50.0f;
                    } else {
                        // Safe behind shield, slight penalty only
                        cost += 8.0f;
                    }
                }
            }
        }

        // B. Target Alignment Bonus (Target lowest aliens or UFO)
        if (s.ufoActive) {
            // UFO is high-value target!
            float ufoCenter = s.ufoX + 8.0f;
            float diff = std::abs(candCenter - ufoCenter);
            cost += diff * 1.5f;
        } else {
            // Find lowest alive alien column
            float bestColX = -1.0f;
            int lowestRow = -1;

            for (int c = 0; c < InvadersState::COLS; ++c) {
                for (int r = InvadersState::ROWS - 1; r >= 0; --r) {
                    if (s.aliens[r][c].alive) {
                        if (r > lowestRow) {
                            lowestRow = r;
                            bestColX = s.fleetX + c * 16.0f + 6.0f;
                        }
                        break;
                    }
                }
            }

            if (bestColX >= 0) {
                float diff = std::abs(candCenter - bestColX);
                cost += diff * 1.0f;
            }
        }

        // C. Inertia penalty: prefer small smooth movements rather than jitter
        cost += std::abs(candX - s.playerX) * 0.15f;

        if (cost < safestCost) {
            safestCost = cost;
            bestX = candX;
        }
    }

    s.playerTargetX = bestX;

    // Smooth movement towards target
    float dx = s.playerTargetX - s.playerX;
    float speed = 2.0f;
    if (std::abs(dx) <= speed) {
        s.playerX = s.playerTargetX;
    } else {
        s.playerX += (dx > 0 ? speed : -speed);
    }

    // Clamp player bounds
    if (s.playerX < minX) s.playerX = minX;
    if (s.playerX > maxX) s.playerX = maxX;

    // 2. Autonomous Opportunistic Shooting
    if (!s.laserActive) {
        float cannonCenter = s.playerX + 7.0f;
        bool shouldShoot = false;

        if (s.ufoActive && std::abs(cannonCenter - (s.ufoX + 8.0f)) < 8.0f) {
            shouldShoot = true;
        } else {
            // Check if aligned with an alive alien column
            for (int c = 0; c < InvadersState::COLS; ++c) {
                float colCenter = s.fleetX + c * 16.0f + 6.0f;
                if (std::abs(cannonCenter - colCenter) < 6.0f) {
                    for (int r = 0; r < InvadersState::ROWS; ++r) {
                        if (s.aliens[r][c].alive) {
                            shouldShoot = true;
                            break;
                        }
                    }
                    if (shouldShoot) break;
                }
            }
        }

        // Fire missile!
        if (shouldShoot || (rand() % 35 == 0)) {
            s.laserActive = true;
            s.laserX = cannonCenter;
            s.laserY = 212.0f;
        }
    }
}

// ============================================================================
// SIMULATION UPDATE
// ============================================================================

static void UpdateSpaceInvaders(InvadersState& s) {
    // 1. Handle Game Over or Wave Clear pauses
    if (s.gameOverTimer > 0) {
        s.gameOverTimer--;
        if (s.gameOverTimer == 0) {
            ResetGame(s);
        }
        return;
    }

    if (s.waveClearTimer > 0) {
        s.waveClearTimer--;
        if (s.waveClearTimer == 0) {
            s.wave++;
            ResetFleet(s, false);
        }
        return;
    }

    // 2. Handle Player Death explosion
    if (!s.playerAlive) {
        s.playerExplodeTimer--;
        if (s.playerExplodeTimer <= 0) {
            s.lives--;
            if (s.lives <= 0) {
                s.gameOverTimer = 100; // Show GAME OVER for ~3 seconds
            } else {
                s.playerAlive = true;
                s.playerX = s.virtualW * 0.5f - 7.0f;
                s.playerTargetX = s.playerX;
                s.bombs.clear();
            }
        }
        return;
    }

    // 3. Update Autopilot AI
    UpdatePlayerAI(s);

    // 4. Update Player Laser
    if (s.laserActive) {
        s.laserY -= 4.2f;

        // Check Bunker Collision with Player Laser
        for (auto& b : s.bunkers) {
            if (s.laserX >= b.x && s.laserX < b.x + 22 && s.laserY >= b.y && s.laserY < b.y + 16) {
                int bx = (int)(s.laserX - b.x);
                int by = (int)(s.laserY - b.y);
                if (bx >= 0 && bx < 22 && by >= 0 && by < 16 && b.pixels[by][bx]) {
                    DamageBunker(b, (int)s.laserX, (int)s.laserY, 2);
                    s.laserActive = false;
                    break;
                }
            }
        }

        // Check Alien Hit
        if (s.laserActive) {
            for (int r = 0; r < InvadersState::ROWS; ++r) {
                for (int c = 0; c < InvadersState::COLS; ++c) {
                    if (s.aliens[r][c].alive) {
                        float ax = s.fleetX + c * 16.0f;
                        float ay = s.fleetY + r * 14.0f;
                        if (s.laserX >= ax && s.laserX <= ax + 12.0f &&
                            s.laserY >= ay && s.laserY <= ay + 8.0f) {
                            // Alien Destroyed!
                            s.aliens[r][c].alive = false;
                            s.aliveCount--;
                            s.score += s.aliens[r][c].scoreVal;
                            if (s.score > s.highScore) s.highScore = s.score;

                            s.alienExplodeX = (int)ax;
                            s.alienExplodeY = (int)ay;
                            s.alienExplodeTimer = 6;
                            s.laserActive = false;

                            // Accelerate fleet as aliens are destroyed
                            s.stepInterval = (std::max)(1, (s.aliveCount * 26) / 55 + 2);
                            break;
                        }
                    }
                }
                if (!s.laserActive) break;
            }
        }

        // Check UFO Hit
        if (s.laserActive && s.ufoActive) {
            if (s.laserX >= s.ufoX && s.laserX <= s.ufoX + 16.0f &&
                s.laserY >= s.ufoY && s.laserY <= s.ufoY + 7.0f) {
                static const int ufoScores[] = { 50, 100, 150, 300 };
                int award = ufoScores[rand() % 4];
                s.score += award;
                if (s.score > s.highScore) s.highScore = s.score;

                s.ufoActive = false;
                s.laserActive = false;
                s.ufoScoreDisplay = award;
                s.ufoScoreTimer = 35;
                s.ufoScoreX = s.ufoX;
                s.ufoSpawnTimer = rand() % 400 + 300;
            }
        }

        // Offscreen top
        if (s.laserY < 32.0f) {
            s.laserActive = false;
        }
    }

    // 5. Update Alien Fleet March
    s.stepTimer++;
    if (s.stepTimer >= s.stepInterval) {
        s.stepTimer = 0;
        s.animStep = 1 - s.animStep; // Toggle walk frame

        // Check fleet bounds across virtual widescreen
        float minAlienX = 9999.0f;
        float maxAlienX = -9999.0f;
        float maxAlienY = 0.0f;

        for (int r = 0; r < InvadersState::ROWS; ++r) {
            for (int c = 0; c < InvadersState::COLS; ++c) {
                if (s.aliens[r][c].alive) {
                    float ax = s.fleetX + c * 16.0f;
                    float ay = s.fleetY + r * 14.0f;
                    if (ax < minAlienX) minAlienX = ax;
                    if (ax + 12.0f > maxAlienX) maxAlienX = ax + 12.0f;
                    if (ay + 8.0f > maxAlienY) maxAlienY = ay + 8.0f;
                }
            }
        }

        if (s.aliveCount == 0) {
            s.waveClearTimer = 60; // 2 seconds victory pause
            return;
        }

        // Drop down & reverse when hitting edge of screen
        const float rightBoundary = (float)(s.virtualW - 14);
        const float leftBoundary  = 14.0f;
        if ((s.fleetDir > 0 && maxAlienX >= rightBoundary) || (s.fleetDir < 0 && minAlienX <= leftBoundary)) {
            s.fleetDir = -s.fleetDir;
            s.fleetY += 8.0f;

            // Invaders reach bunker/cannon level = Instant game over
            if (maxAlienY >= 200.0f) {
                s.playerAlive = false;
                s.playerExplodeTimer = 40;
                s.lives = 0;
                s.gameOverTimer = 100;
                return;
            }
        } else {
            s.fleetX += s.fleetDir * 2.5f;
        }
    }

    // 6. Alien Bomb Dropping
    s.bombTimer++;
    if (s.bombTimer >= 20 && s.bombs.size() < 5) {
        s.bombTimer = 0;
        // Find columns with alive aliens
        std::vector<int> aliveCols;
        for (int c = 0; c < InvadersState::COLS; ++c) {
            for (int r = 0; r < InvadersState::ROWS; ++r) {
                if (s.aliens[r][c].alive) {
                    aliveCols.push_back(c);
                    break;
                }
            }
        }

        if (!aliveCols.empty()) {
            int pickCol = aliveCols[rand() % aliveCols.size()];
            // Find lowest alien in this column
            for (int r = InvadersState::ROWS - 1; r >= 0; --r) {
                if (s.aliens[r][pickCol].alive) {
                    Bomb b;
                    b.x = s.fleetX + pickCol * 16.0f + 5.0f;
                    b.y = s.fleetY + r * 14.0f + 8.0f;
                    b.vy = 1.6f + (s.wave - 1) * 0.15f;
                    b.type = rand() % 3;
                    b.active = true;
                    s.bombs.push_back(b);
                    break;
                }
            }
        }
    }

    // 7. Update Alien Bombs
    for (size_t i = 0; i < s.bombs.size(); ) {
        Bomb& b = s.bombs[i];
        b.y += b.vy;
        b.animFrame = (b.animFrame + 1) % 4;

        bool destroyed = false;

        // Check Bunker Collision with Alien Bomb
        for (auto& bk : s.bunkers) {
            if (b.x >= bk.x && b.x < bk.x + 22 && b.y >= bk.y && b.y < bk.y + 16) {
                int bx = (int)(b.x - bk.x);
                int by = (int)(b.y - bk.y);
                if (bx >= 0 && bx < 22 && by >= 0 && by < 16 && bk.pixels[by][bx]) {
                    DamageBunker(bk, (int)b.x, (int)b.y, 3);
                    destroyed = true;
                    break;
                }
            }
        }

        // Check Player Collision
        if (!destroyed && s.playerAlive) {
            if (b.x >= s.playerX && b.x <= s.playerX + 15.0f &&
                b.y >= 216.0f && b.y <= 224.0f) {
                s.playerAlive = false;
                s.playerExplodeTimer = 50;
                destroyed = true;
            }
        }

        // Offscreen bottom
        if (b.y >= 236.0f) {
            destroyed = true;
        }

        if (destroyed) {
            s.bombs.erase(s.bombs.begin() + i);
        } else {
            ++i;
        }
    }

    // 8. Mystery UFO Saucer Logic
    if (!s.ufoActive) {
        s.ufoSpawnTimer--;
        if (s.ufoSpawnTimer <= 0) {
            s.ufoActive = true;
            s.ufoSpeed = (rand() % 2 == 0) ? 1.2f : -1.2f;
            s.ufoX = (s.ufoSpeed > 0) ? -16.0f : (float)s.virtualW;
        }
    } else {
        s.ufoX += s.ufoSpeed;
        if (s.ufoSpeed > 0 && s.ufoX > s.virtualW + 10) {
            s.ufoActive = false;
            s.ufoSpawnTimer = rand() % 400 + 350;
        } else if (s.ufoSpeed < 0 && s.ufoX < -20) {
            s.ufoActive = false;
            s.ufoSpawnTimer = rand() % 400 + 350;
        }
    }

    if (s.ufoScoreTimer > 0) s.ufoScoreTimer--;
    if (s.alienExplodeTimer > 0) s.alienExplodeTimer--;
}

// ============================================================================
// RENDER COMPONENT
// ============================================================================

static void RenderArcadeFrame(InvadersState& s) {
    // 0. Clear Framebuffer to Pure Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 1. Draw Twinkling Background Starfield (Galaga style)
    for (auto& st : s.stars) {
        st.y += st.speed;
        if (st.y >= (float)s.virtualH) {
            st.y = 0;
            st.x = (float)(rand() % s.virtualW);
        }
        st.twinkleTimer = (st.twinkleTimer + 1) % 60;
        uint32_t col = (st.twinkleTimer < 10) ? (st.color & 0xFF7F7F7F) : st.color;
        PutPixel(s, (int)st.x, (int)st.y, col);
    }

    // 2. Top HUD Header: "SCORE<1>   HI-SCORE   SCORE<2>"
    DrawText(s, 16, 10, "SCORE<1>", COL_WHITE);
    DrawText(s, s.virtualW / 2 - 24, 10, "HI-SCORE", COL_WHITE);
    DrawText(s, s.virtualW - 68, 10, "SCORE<2>", COL_WHITE);

    // Score Values
    char scoreBuf[16];
    sprintf_s(scoreBuf, "%04d", s.score);
    DrawText(s, 28, 22, scoreBuf, COL_WHITE);

    sprintf_s(scoreBuf, "%04d", s.highScore);
    DrawText(s, s.virtualW / 2 - 12, 22, scoreBuf, COL_WHITE);

    // 3. Mystery UFO (Red) or Bonus Score
    if (s.ufoActive) {
        int ux = (int)s.ufoX;
        int uy = (int)s.ufoY;
        for (int r = 0; r < 7; ++r) {
            uint16_t rowBits = SPRITE_UFO[r];
            for (int c = 0; c < 16; ++c) {
                if ((rowBits >> (15 - c)) & 1) {
                    PutPixel(s, ux + c, uy + r, COL_RED);
                }
            }
        }
    } else if (s.ufoScoreTimer > 0) {
        char ufoBuf[8];
        sprintf_s(ufoBuf, "%d", s.ufoScoreDisplay);
        DrawText(s, (int)s.ufoScoreX, (int)s.ufoY, ufoBuf, COL_RED);
    }

    // 4. Alien Invaders Grid (Symmetrical, pixel-perfect)
    for (int r = 0; r < InvadersState::ROWS; ++r) {
        for (int c = 0; c < InvadersState::COLS; ++c) {
            if (s.aliens[r][c].alive) {
                int ax = (int)(s.fleetX + c * 16.0f);
                int ay = (int)(s.fleetY + r * 14.0f);

                if (r == 0) {
                    // Squid (8x8, White, centered at offset + 2)
                    const uint8_t* sprite = SPRITE_SQUID[s.animStep];
                    for (int dy = 0; dy < 8; ++dy) {
                        uint8_t rowBits = sprite[dy];
                        for (int dx = 0; dx < 8; ++dx) {
                            if ((rowBits >> (7 - dx)) & 1) {
                                PutPixel(s, ax + dx + 2, ay + dy, COL_WHITE);
                            }
                        }
                    }
                } else if (r <= 2) {
                    // Crab (11x8, White, centered at offset + 1)
                    const uint16_t* sprite = SPRITE_CRAB[s.animStep];
                    for (int dy = 0; dy < 8; ++dy) {
                        uint16_t rowBits = sprite[dy];
                        for (int dx = 0; dx < 11; ++dx) {
                            if ((rowBits >> (10 - dx)) & 1) {
                                PutPixel(s, ax + dx + 1, ay + dy, COL_WHITE);
                            }
                        }
                    }
                } else {
                    // Octopus (12x8, White, centered at offset + 0)
                    const uint16_t* sprite = SPRITE_OCTOPUS[s.animStep];
                    for (int dy = 0; dy < 8; ++dy) {
                        uint16_t rowBits = sprite[dy];
                        for (int dx = 0; dx < 12; ++dx) {
                            if ((rowBits >> (11 - dx)) & 1) {
                                PutPixel(s, ax + dx, ay + dy, COL_WHITE);
                            }
                        }
                    }
                }
            }
        }
    }

    // Alien Death Splat
    if (s.alienExplodeTimer > 0) {
        for (int dy = 0; dy < 8; ++dy) {
            uint16_t rowBits = SPRITE_ALIEN_EXP[dy];
            for (int dx = 0; dx < 13; ++dx) {
                if ((rowBits >> (12 - dx)) & 1) {
                    PutPixel(s, s.alienExplodeX + dx, s.alienExplodeY + dy, COL_WHITE);
                }
            }
        }
    }

    // 5. Bunkers / Defense Shields (Green, Destructible)
    for (const auto& b : s.bunkers) {
        for (int r = 0; r < 16; ++r) {
            for (int c = 0; c < 22; ++c) {
                if (b.pixels[r][c]) {
                    PutPixel(s, b.x + c, b.y + r, COL_GREEN);
                }
            }
        }
    }

    // 6. Player Cannon (Green) or Death Explosion
    if (s.playerAlive) {
        int px = (int)s.playerX;
        int py = 216;
        for (int r = 0; r < 8; ++r) {
            uint16_t rowBits = SPRITE_PLAYER[r];
            for (int c = 0; c < 15; ++c) {
                if ((rowBits >> (14 - c)) & 1) {
                    PutPixel(s, px + c, py + r, COL_GREEN);
                }
            }
        }
    } else if (s.playerExplodeTimer > 0) {
        int px = (int)s.playerX;
        int py = 216;
        int frame = (s.playerExplodeTimer / 6) % 2;
        const uint16_t* sprite = SPRITE_PLAYER_EXP[frame];
        for (int r = 0; r < 8; ++r) {
            uint16_t rowBits = sprite[r];
            for (int c = 0; c < 15; ++c) {
                if ((rowBits >> (14 - c)) & 1) {
                    PutPixel(s, px + c, py + r, COL_GREEN);
                }
            }
        }
    }

    // 7. Player Laser Beam (White)
    if (s.laserActive) {
        int lx = (int)s.laserX;
        int ly = (int)s.laserY;
        PutPixel(s, lx, ly,     COL_WHITE);
        PutPixel(s, lx, ly + 1, COL_WHITE);
        PutPixel(s, lx, ly + 2, COL_WHITE);
        PutPixel(s, lx, ly + 3, COL_WHITE);
    }

    // 8. Alien Bombs (White, Animated)
    for (const auto& b : s.bombs) {
        int bx = (int)b.x;
        int by = (int)b.y;
        if (b.type == 0) {
            // Rolling bomb
            int offset = (b.animFrame % 2 == 0) ? 0 : 1;
            PutPixel(s, bx - offset, by,     COL_WHITE);
            PutPixel(s, bx + offset, by + 1, COL_WHITE);
            PutPixel(s, bx - offset, by + 2, COL_WHITE);
            PutPixel(s, bx + offset, by + 3, COL_WHITE);
        } else {
            // Plunger bomb
            PutPixel(s, bx, by,     COL_WHITE);
            PutPixel(s, bx, by + 1, COL_WHITE);
            PutPixel(s, bx, by + 2, COL_WHITE);
            PutPixel(s, bx - 1, by + 3, COL_WHITE);
            PutPixel(s, bx + 1, by + 3, COL_WHITE);
        }
    }

    // 9. Ground Baseline (Green horizontal line spanning entire widescreen at Y = 236)
    DrawHLine(s, 0, s.virtualW - 1, 236, COL_GREEN);

    // 10. Bottom Status Bar: Lives & Credit (Below Ground Line)
    char livesBuf[8];
    sprintf_s(livesBuf, "%d", s.lives);
    DrawText(s, 16, 242, livesBuf, COL_WHITE);

    // Mini Cannon Icons for Remaining Lives
    for (int l = 0; l < (std::min)(5, s.lives - 1); ++l) {
        int lx = 30 + l * 18;
        int ly = 241;
        for (int r = 0; r < 8; ++r) {
            uint16_t rowBits = SPRITE_PLAYER[r];
            for (int c = 0; c < 15; ++c) {
                if ((rowBits >> (14 - c)) & 1) {
                    PutPixel(s, lx + c, ly + r, COL_GREEN);
                }
            }
        }
    }

    // Credit Counter / Round Indicator (Right edge of widescreen)
    char creditBuf[16];
    sprintf_s(creditBuf, "CREDIT %02d", s.wave);
    DrawText(s, s.virtualW - 74, 242, creditBuf, COL_WHITE);

    // 11. Game Over Banner (Centered)
    if (s.gameOverTimer > 0) {
        DrawText(s, s.virtualW / 2 - 27, 110, "GAME OVER", COL_RED);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================

void RenderSpaceInvaders(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<InvadersState>(28);

    // Calculate virtual arcade resolution to match the display's exact aspect ratio
    // Locking height to 256 for genuine retro scanline chunky pixel density
    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    if (!s.initialized || s.virtualW != vW || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);
        ResetGame(s);
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    // Fixed-step simulation update (60 updates per second)
    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200; // avoid spiral of death
    s.lastTick = now;

    int steps = (int)(elapsed / 16);
    if (steps < 1) steps = 1;
    if (steps > 4) steps = 4;

    for (int i = 0; i < steps; ++i) {
        UpdateSpaceInvaders(s);
    }

    // Render virtual arcade framebuffer
    RenderArcadeFrame(s);

    // Fullscreen Pixel-Perfect Stretched Blit (Fills 100% of the screen, NO blank bars)
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

// Register as Screensaver ID 28
REGISTER_SCREENSAVER(
    28,
    L"Space Invaders",
    "invaders",
    { "invaders", "space", "galaga", "spaceinvaders" },
    WRAP_LEGACY(RenderSpaceInvaders),
    {}
);
