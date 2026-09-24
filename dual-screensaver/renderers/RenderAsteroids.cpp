#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/AsteroidsSettings.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace Asteroids {

// ============================================================================
// VIRTUAL VECTOR ARCADE RESOLUTION & COLOR PALETTE
// Authentic 1979 Atari QuadraScan Vector Beam Arcade Simulation
// Virtual height is 256; virtual width adapts dynamically to monitor aspect ratio
// (e.g. 456x256 for 16:9) filling 100% of the screen with ZERO black bars.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 640;
static constexpr float PI          = 3.14159265358979323846f;
static constexpr float TWO_PI      = 6.28318530717958647692f;

// 32-bit ARGB Retro Vector Phosphor Palette
static constexpr uint32_t COL_BLACK          = 0xFF020408; // Deep obsidian space
static constexpr uint32_t COL_STAR_DIM       = 0xFF37474F; // Distant space dust
static constexpr uint32_t COL_STAR_BRIGHT    = 0xFF78909C; // Twinkling vector star
static constexpr uint32_t COL_VECTOR_CORE    = 0xFFFFFFFF; // Pure white vector beam core
static constexpr uint32_t COL_WHITE_CORE     = 0xFFFFFFFF; // Pure white vector core
static constexpr uint32_t COL_VECTOR_GLOW    = 0xFF455A64; // Ambient phosphor glow
static constexpr uint32_t COL_CYAN_CORE      = 0xFF80D8FF; // Player ship electric cyan core
static constexpr uint32_t COL_CYAN_GLOW      = 0xFF0091EA; // Player ship cyan bloom
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744; // UFO / Danger vector crimson
static constexpr uint32_t COL_CRIMSON_GLOW   = 0xFF880E4F; // UFO crimson bloom
static constexpr uint32_t COL_ORANGE_THRUST  = 0xFFFF9100; // Thruster vector flame
static constexpr uint32_t COL_GREEN_LIME     = 0xFF00FF66; // Wave clear banner
static constexpr uint32_t COL_GOLD           = 0xFFFFD700; // High score / Bonus
static constexpr uint32_t COL_GRAY_LIGHT     = 0xFFCFD8DC; // Medium rock wireframe
static constexpr uint32_t COL_GRAY_MID       = 0xFF90A4AE; // Large rock wireframe

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

struct Star {
    float x, y;
    float speed;
    uint32_t color;
};

struct Bullet {
    float x, y;
    float vx, vy;
    int life;
    bool isUfo;
    bool active;
};

enum AsteroidTier {
    AST_LARGE = 3,
    AST_MEDIUM = 2,
    AST_SMALL = 1
};

struct Asteroid {
    float x, y;
    float vx, vy;
    float angle;
    float rotSpeed;
    AsteroidTier tier;
    int shapeIndex;
    float radius;
    bool active;
};

struct FlyingSaucer {
    float x, y;
    float vx, vy;
    bool isSmall; // false: Large UFO (200 pts), true: Small Sniper UFO (1000 pts)
    int fireTimer;
    int dirChangeTimer;
    bool active;
};

struct VectorShard {
    float x, y;
    float vx, vy;
    float angle;
    float rotSpeed;
    float length;
    int life;
    int maxLife;
    uint32_t color;
};

struct VectorSpark {
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

struct AsteroidsState {
    int virtualW = 0;
    int virtualH = 0;
    std::vector<uint32_t> fb;

    // Game stats
    int score = 0;
    int highScore = 50000;
    int lives = 3;
    int wave = 1;
    int state = 0; // 0: init, 1: playing, 2: wave clear pause, 3: game over pause
    int pauseTimer = 0;

    // Player Ship (Zero-G Newtonian Physics)
    float shipX = 0.0f;
    float shipY = 0.0f;
    float shipVx = 0.0f;
    float shipVy = 0.0f;
    float shipAngle = -PI * 0.5f; // Pointing upwards
    bool thrusting = false;
    int invulnTimer = 0;
    int fireCooldown = 0;
    int hyperspaceCooldown = 0;
    bool shipAlive = true;
    int respawnTimer = 0;

    // AI Pilot state (Humanized arcade play mechanics)
    float aiTargetAngle = -PI * 0.5f;
    float aiAimWobble = 0.0f;
    int aiWobbleTimer = 0;
    int aiOverThrustTimer = 0;
    int aiTunnelVision = 0;

    // Entities
    std::vector<Star> stars;
    std::vector<Bullet> bullets;
    std::vector<Asteroid> asteroids;
    FlyingSaucer ufo;
    int ufoSpawnTimer = 0;

    std::vector<VectorShard> shards;
    std::vector<VectorSpark> sparks;
    std::vector<FloatingScore> floatingScores;

    uint64_t lastTick = 0;
    bool initialized = false;
};

// ============================================================================
// TOROIDAL WRAP-AROUND SPACE CALCULATIONS
// ============================================================================
static inline float WrapCoord(float val, float maxVal) {
    while (val < 0.0f) val += maxVal;
    while (val >= maxVal) val -= maxVal;
    return val;
}

static inline float ToroidalDistX(float x1, float x2, float W) {
    float dx = x2 - x1;
    if (dx > W * 0.5f) dx -= W;
    else if (dx < -W * 0.5f) dx += W;
    return dx;
}

static inline float ToroidalDistY(float y1, float y2, float H) {
    float dy = y2 - y1;
    if (dy > H * 0.5f) dy -= H;
    else if (dy < -H * 0.5f) dy += H;
    return dy;
}

// ============================================================================
// HIGH-PERFORMANCE VECTOR RENDERER & PHOSPHOR BLOOM
// ============================================================================
static inline void PutPixel(AsteroidsState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static inline void PutGlowPixel(AsteroidsState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        uint32_t& dst = s.fb[y * s.virtualW + x];
        uint32_t r = (std::min)(255u, ((dst >> 16) & 0xFF) + ((color >> 16) & 0xFF));
        uint32_t g = (std::min)(255u, ((dst >> 8)  & 0xFF) + ((color >> 8)  & 0xFF));
        uint32_t b = (std::min)(255u, (dst & 0xFF)         + (color & 0xFF));
        dst = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}

// Bresenham Vector Line with authentic phosphor bloom halo
static void DrawVectorLine(AsteroidsState& s, int x1, int y1, int x2, int y2, uint32_t color, bool glow) {
    if (glow) {
        uint32_t glowCol = ((color & 0xFEFEFE) >> 1); // 50% phosphor intensity bloom
        int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        int x = x1, y = y1;
        while (true) {
            PutGlowPixel(s, x + 1, y, glowCol);
            PutGlowPixel(s, x - 1, y, glowCol);
            PutGlowPixel(s, x, y + 1, glowCol);
            PutGlowPixel(s, x, y - 1, glowCol);
            if (x == x2 && y == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x += sx; }
            if (e2 <= dx) { err += dx; y += sy; }
        }
    }

    // High intensity vector beam core
    int dx = std::abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -std::abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x1, y = y1;
    while (true) {
        PutPixel(s, x, y, color);
        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

// Draw polygon with full 4-quadrant toroidal screen wrapping (zero edge clipping)
static void DrawVectorPolyWrapped(AsteroidsState& s, const std::vector<std::pair<float, float>>& pts,
                                  float cx, float cy, float angle, float scale, uint32_t color, bool closed, bool glow) {
    float offsetsX[3] = { 0.0f, (float)s.virtualW, (float)-s.virtualW };
    float offsetsY[3] = { 0.0f, (float)s.virtualH, (float)-s.virtualH };

    float cosA = std::cos(angle) * scale;
    float sinA = std::sin(angle) * scale;
    size_t n = pts.size();

    for (int ox = 0; ox < 3; ++ox) {
        float tx = cx + offsetsX[ox];
        if (tx < -scale * 2.5f || tx > s.virtualW + scale * 2.5f) continue;
        for (int oy = 0; oy < 3; ++oy) {
            float ty = cy + offsetsY[oy];
            if (ty < -scale * 2.5f || ty > s.virtualH + scale * 2.5f) continue;

            for (size_t i = 0; i < n; ++i) {
                if (!closed && i + 1 == n) break;
                size_t j = (i + 1) % n;
                int x1 = (int)(tx + pts[i].first * cosA - pts[i].second * sinA);
                int y1 = (int)(ty + pts[i].first * sinA + pts[i].second * cosA);
                int x2 = (int)(tx + pts[j].first * cosA - pts[j].second * sinA);
                int y2 = (int)(ty + pts[j].first * sinA + pts[j].second * cosA);
                DrawVectorLine(s, x1, y1, x2, y2, color, glow);
            }
        }
    }
}

static void DrawGlyphChar(AsteroidsState& s, int px, int py, char c, uint32_t color) {
    const uint8_t* rows = GetGlyph(c);
    for (int r = 0; r < 7; ++r) {
        uint8_t bits = rows[r];
        for (int col = 0; col < 5; ++col) {
            if (bits & (0x10 >> col)) PutPixel(s, px + col, py + r, color);
        }
    }
}

static void DrawText(AsteroidsState& s, int px, int py, const char* str, uint32_t color) {
    while (*str) {
        DrawGlyphChar(s, px, py, *str, color);
        px += 6;
        str++;
    }
}

// ============================================================================
// 1979 RETRO VECTOR GEOMETRIES
// ============================================================================

// Authentic Asteroids Player Ship
static const std::vector<std::pair<float, float>> SHIP_VERTS = {
    { 0.0f, -9.0f },    // Nose
    { 5.5f, 7.0f },     // Right wingtip
    { 0.0f, 4.0f },     // Thruster notch
    { -5.5f, 7.0f }     // Left wingtip
};

// Flickering Thruster Jet Flame
static const std::vector<std::pair<float, float>> THRUSTER_VERTS = {
    { -3.0f, 4.5f },
    { 0.0f, 10.5f },
    { 3.0f, 4.5f }
};

// 4 Classic 1979 Jagged Asteroid Faceted Shapes (normalized)
static const std::vector<std::pair<float, float>> ASTEROID_SHAPES[4] = {
    // Variant 0
    { { 0.0f, -1.0f }, { 0.5f, -0.9f }, { 1.0f, -0.4f }, { 0.75f, 0.2f }, { 1.0f, 0.7f },
      { 0.4f, 1.0f }, { -0.3f, 0.8f }, { -0.9f, 1.0f }, { -1.0f, 0.4f }, { -0.6f, 0.0f },
      { -1.0f, -0.4f }, { -0.5f, -0.9f } },
    // Variant 1
    { { 0.2f, -1.0f }, { 0.9f, -0.6f }, { 0.6f, -0.2f }, { 1.0f, 0.3f }, { 0.7f, 0.9f },
      { 0.0f, 0.7f }, { -0.5f, 1.0f }, { -0.9f, 0.6f }, { -0.7f, 0.1f }, { -1.0f, -0.5f },
      { -0.6f, -0.8f } },
    // Variant 2
    { { 0.0f, -1.0f }, { 0.7f, -0.8f }, { 1.0f, -0.2f }, { 0.5f, 0.1f }, { 1.0f, 0.5f },
      { 0.6f, 1.0f }, { -0.1f, 0.9f }, { -0.7f, 1.0f }, { -1.0f, 0.3f }, { -0.8f, -0.3f },
      { -0.9f, -0.7f } },
    // Variant 3
    { { 0.3f, -1.0f }, { 0.8f, -0.7f }, { 1.0f, 0.0f }, { 0.85f, 0.6f }, { 0.3f, 0.85f },
      { -0.2f, 1.0f }, { -0.8f, 0.8f }, { -1.0f, 0.2f }, { -0.6f, -0.2f }, { -0.9f, -0.6f },
      { -0.3f, -0.9f } }
};

// 1979 Flying Saucer (UFO) Geometry
static const std::vector<std::pair<float, float>> UFO_BODY = {
    { -1.0f, 0.2f }, { -0.5f, -0.3f }, { -0.25f, -0.8f }, { 0.25f, -0.8f },
    { 0.5f, -0.3f }, { 1.0f, 0.2f }, { 0.6f, 0.8f }, { -0.6f, 0.8f }
};

// ============================================================================
// STAGE & WAVE GENERATION
// ============================================================================
static void SpawnAsteroid(AsteroidsState& s, float x, float y, AsteroidTier tier) {
    float spd = 0.6f;
    float rad = 18.0f;
    if (tier == AST_MEDIUM) { spd = 1.3f; rad = 10.0f; }
    else if (tier == AST_SMALL) { spd = 2.1f; rad = 5.5f; }

    float moveAngle = (float)(rand() % 628) / 100.0f;
    float spdVar = spd * (0.8f + (rand() % 40) * 0.01f);
    float vx = std::cos(moveAngle) * spdVar;
    float vy = std::sin(moveAngle) * spdVar;
    float rotSpd = ((rand() % 100) - 50) * 0.0008f;
    int shp = rand() % 4;

    s.asteroids.push_back({ x, y, vx, vy, 0.0f, rotSpd, tier, shp, rad, true });
}

static void SetupWave(AsteroidsState& s, int waveNum) {
    s.asteroids.clear();
    s.bullets.clear();
    s.ufo.active = false;
    s.ufoSpawnTimer = 350 + (rand() % 250);

    // Number of large asteroids scales with wave: 4, 5, 6, max 8
    int rockCount = g_AsteroidsStartRocks + (waveNum - 1);
    if (rockCount > 8) rockCount = 8;

    float safeRadius = 75.0f;
    for (int i = 0; i < rockCount; ++i) {
        float ax, ay;
        int attempts = 0;
        do {
            ax = (float)(rand() % s.virtualW);
            ay = (float)(rand() % s.virtualH);
            float dx = ToroidalDistX(s.shipX, ax, (float)s.virtualW);
            float dy = ToroidalDistY(s.shipY, ay, (float)s.virtualH);
            if (std::sqrt(dx * dx + dy * dy) > safeRadius) break;
            attempts++;
        } while (attempts < 50);

        SpawnAsteroid(s, ax, ay, AST_LARGE);
    }

    // Reset ship position safely at center
    s.shipX = s.virtualW * 0.5f;
    s.shipY = s.virtualH * 0.5f;
    s.shipVx = 0.0f;
    s.shipVy = 0.0f;
    s.shipAngle = -PI * 0.5f;
    s.thrusting = false;
    s.invulnTimer = 45; // ~0.75s initial wave spawn shield
    s.shipAlive = true;
    s.respawnTimer = 0;
    s.fireCooldown = 0;
}

static void ResetGame(AsteroidsState& s) {
    s.score = 0;
    s.lives = 3;
    s.wave = 1;
    s.state = 1; // Playing
    SetupWave(s, s.wave);
}

// ============================================================================
// SIMULATION & AUTOPLAY AI PILOT
// ============================================================================
static void StepAsteroidsSimulation(AsteroidsState& s) {
    // 1. Update Distant Starfield Parallax
    for (auto& star : s.stars) {
        star.x -= star.speed;
        if (star.x < 0) star.x += s.virtualW;
    }

    // 2. Update Vector Line Shards (Explosion Shrapnel)
    for (size_t i = 0; i < s.shards.size(); ) {
        s.shards[i].x += s.shards[i].vx;
        s.shards[i].y += s.shards[i].vy;
        s.shards[i].x = WrapCoord(s.shards[i].x, (float)s.virtualW);
        s.shards[i].y = WrapCoord(s.shards[i].y, (float)s.virtualH);
        s.shards[i].angle += s.shards[i].rotSpeed;
        s.shards[i].life++;
        if (s.shards[i].life >= s.shards[i].maxLife) {
            s.shards.erase(s.shards.begin() + i);
        } else {
            ++i;
        }
    }

    // 3. Update Vector Sparks
    for (size_t i = 0; i < s.sparks.size(); ) {
        s.sparks[i].x += s.sparks[i].vx;
        s.sparks[i].y += s.sparks[i].vy;
        s.sparks[i].vx *= 0.96f;
        s.sparks[i].vy *= 0.96f;
        s.sparks[i].life++;
        if (s.sparks[i].life >= s.sparks[i].maxLife) {
            s.sparks.erase(s.sparks.begin() + i);
        } else {
            ++i;
        }
    }

    // 4. Update Floating Scores
    for (size_t i = 0; i < s.floatingScores.size(); ) {
        s.floatingScores[i].y -= 0.35f;
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

    // Pause Handlers (Wave Clear or Game Over)
    if (s.state == 2) {
        s.pauseTimer--;
        if (s.pauseTimer <= 0) {
            s.wave++;
            SetupWave(s, s.wave);
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

    if (s.invulnTimer > 0) s.invulnTimer--;
    if (s.fireCooldown > 0) s.fireCooldown--;
    if (s.hyperspaceCooldown > 0) s.hyperspaceCooldown--;

    // Handle respawn delay after ship destruction
    if (!s.shipAlive && s.state == 1) {
        if (s.respawnTimer > 0) {
            s.respawnTimer--;
            if (s.respawnTimer <= 0 && s.lives > 0) {
                // Respawn safely at center
                s.shipX = s.virtualW * 0.5f;
                s.shipY = s.virtualH * 0.5f;
                s.shipVx = 0.0f; s.shipVy = 0.0f;
                s.shipAngle = -PI * 0.5f;
                s.invulnTimer = 50; // Protective respawn shield
                s.hyperspaceCooldown = 60;
                s.shipAlive = true;
            }
        }
    }

    // 5. AUTOPLAY AI PILOT: Threat Evaluation & Intercept Ballistics
    if (s.shipAlive) {
        float closestDist = 999999.0f;
        float targetX = 0.0f;
        float targetY = 0.0f;
        float targetVx = 0.0f;
        float targetVy = 0.0f;
        bool hasTarget = false;

        // Threat 1: Small Saucer (Extremely high priority sniper)
        if (s.ufo.active && s.ufo.isSmall) {
            targetX = s.ufo.x; targetY = s.ufo.y;
            targetVx = s.ufo.vx; targetVy = s.ufo.vy;
            hasTarget = true;
        }
        // Threat 2: Large Saucer
        else if (s.ufo.active) {
            targetX = s.ufo.x; targetY = s.ufo.y;
            targetVx = s.ufo.vx; targetVy = s.ufo.vy;
            hasTarget = true;
        }
        // Threat 3: Nearest threatening Asteroids (prioritizing small fast rocks)
        else {
            for (const auto& a : s.asteroids) {
                if (!a.active) continue;
                float dx = ToroidalDistX(s.shipX, a.x, (float)s.virtualW);
                float dy = ToroidalDistY(s.shipY, a.y, (float)s.virtualH);
                float dist = std::sqrt(dx * dx + dy * dy);

                // Small rocks are urgent threats; large rocks are easier to avoid
                float threatScore = dist - (3 - a.tier) * 20.0f;
                if (threatScore < closestDist) {
                    closestDist = threatScore;
                    targetX = a.x; targetY = a.y;
                    targetVx = a.vx; targetVy = a.vy;
                    hasTarget = true;
                }
            }
        }

        // Update human aim wobble (simulates human hand/stick inaccuracy)
        s.aiWobbleTimer--;
        if (s.aiWobbleTimer <= 0) {
            s.aiWobbleTimer = 14 + (rand() % 20);
            float skillMod = (2.2f - g_AsteroidsAiSkill);
            s.aiAimWobble = ((rand() % 100) - 50) * 0.007f * skillMod; // ~±20° wobble
        }

        // Ballistic Intercept Aim Calculation with human aim imperfection
        if (hasTarget) {
            float dx = ToroidalDistX(s.shipX, targetX, (float)s.virtualW);
            float dy = ToroidalDistY(s.shipY, targetY, (float)s.virtualH);
            float dist = std::sqrt(dx * dx + dy * dy);

            // Time for bullet to travel to target (bullet speed = 6.2)
            float bulletSpeed = 6.2f;
            float flightTime = dist / bulletSpeed;

            // Predicted intercept coordinate
            float leadX = targetX + targetVx * flightTime;
            float leadY = targetY + targetVy * flightTime;
            float leadDx = ToroidalDistX(s.shipX, leadX, (float)s.virtualW);
            float leadDy = ToroidalDistY(s.shipY, leadY, (float)s.virtualH);

            s.aiTargetAngle = std::atan2(leadDy, leadDx) + PI * 0.5f + s.aiAimWobble;
        }

        // Smooth Angular Steering towards Intercept
        float angleDiff = s.aiTargetAngle - s.shipAngle;
        while (angleDiff > PI) angleDiff -= TWO_PI;
        while (angleDiff < -PI) angleDiff += TWO_PI;

        float turnRate = 0.08f * g_AsteroidsAiSkill;
        if (std::abs(angleDiff) > turnRate) {
            s.shipAngle += (angleDiff > 0) ? turnRate : -turnRate;
        } else {
            s.shipAngle = s.aiTargetAngle;
        }

        // Keep angle in [-PI..PI]
        while (s.shipAngle > PI) s.shipAngle -= TWO_PI;
        while (s.shipAngle < -PI) s.shipAngle += TWO_PI;

        // Firing Discipline: Fire when nose is aligned with intercept target
        if (hasTarget && std::abs(angleDiff) < 0.28f && s.fireCooldown <= 0) {
            int activeBullets = 0;
            for (const auto& b : s.bullets) if (b.active && !b.isUfo) activeBullets++;
            if (activeBullets < 4) {
                float bAngle = s.shipAngle - PI * 0.5f;
                float noseX = s.shipX + std::cos(bAngle) * 9.0f;
                float noseY = s.shipY + std::sin(bAngle) * 9.0f;
                float bSpeed = 6.2f;
                s.bullets.push_back({ noseX, noseY, s.shipVx * 0.35f + std::cos(bAngle) * bSpeed,
                                      s.shipVy * 0.35f + std::sin(bAngle) * bSpeed, 50, false, true });
                s.fireCooldown = 16;
                s.aiTunnelVision = 18; // Tunnel vision when firing
            }
        }

        if (s.aiTunnelVision > 0) s.aiTunnelVision--;

        // Evasive Maneuvers & Hazard Assessment
        float nearestHazardDist = 999999.0f;
        float hazardDx = 0.0f, hazardDy = 0.0f;
        int nearbyHazardCount = 0;

        for (const auto& a : s.asteroids) {
            if (!a.active) continue;
            float dx = ToroidalDistX(s.shipX, a.x, (float)s.virtualW);
            float dy = ToroidalDistY(s.shipY, a.y, (float)s.virtualH);
            float d = std::sqrt(dx * dx + dy * dy);
            if (d < 28.0f) nearbyHazardCount++;
            if (d < nearestHazardDist) {
                nearestHazardDist = d;
                hazardDx = dx; hazardDy = dy;
            }
        }

        s.thrusting = false;
        float currentSpeed = std::sqrt(s.shipVx * s.shipVx + s.shipVy * s.shipVy);

        // Tunnel vision: when firing intently, detection threshold is reduced
        float hazardThreshold = (s.aiTunnelVision > 0) ? 18.0f : 26.0f;

        // Emergency Hyperspace Jump when trapped in close pincer
        if (nearestHazardDist < 13.0f && nearbyHazardCount >= 2 && s.hyperspaceCooldown <= 0) {
            // Blinding hyperspace warp flash
            for (int k = 0; k < 16; ++k) {
                float a = (float)(rand() % 628) / 100.0f;
                float spd = 1.0f + (rand() % 150) * 0.01f;
                s.sparks.push_back({ s.shipX, s.shipY, std::cos(a) * spd, std::sin(a) * spd, 0, 20, COL_CYAN_CORE });
            }

            s.shipX = (float)(rand() % s.virtualW);
            s.shipY = (float)(rand() % s.virtualH);
            s.shipVx = 0.0f; s.shipVy = 0.0f;
            s.invulnTimer = 0; // Authentic arcade: entering hyperspace has no shield
            s.hyperspaceCooldown = 320;
        }
        // Over-thrust handler: holding thrust a bit too long in panic
        else if (s.aiOverThrustTimer > 0) {
            s.aiOverThrustTimer--;
            s.thrusting = true;
        }
        // Dodge thrust: evade away from immediate hazard (with realistic human hesitation)
        else if (nearestHazardDist < hazardThreshold) {
            // 25% human reaction lag: pilot hesitates before hitting thrust
            if (rand() % 100 >= 25) {
                float escapeAngle = std::atan2(-hazardDy, -hazardDx) + PI * 0.5f;

                // In multi-hazard pincer, player can panic-turn slightly off-angle
                if (nearbyHazardCount >= 2) {
                    escapeAngle += ((rand() % 100) - 50) * 0.008f;
                }

                float headingDiff = escapeAngle - s.shipAngle;
                while (headingDiff > PI) headingDiff -= TWO_PI;
                while (headingDiff < -PI) headingDiff += TWO_PI;

                if (std::abs(headingDiff) < 0.65f && currentSpeed < 3.2f) {
                    s.thrusting = true;
                    // 15% chance of human over-thrusting panic
                    if (rand() % 100 < 15) {
                        s.aiOverThrustTimer = 6 + (rand() % 6);
                    }
                }
            }
        }
        // Intercept thrust: approach distant asteroids if ship is moving too slowly
        else if (currentSpeed < 0.9f && closestDist > 100.0f && std::abs(angleDiff) < 0.4f) {
            s.thrusting = true;
        }

        // Apply Newtonian acceleration
        if (s.thrusting) {
            float thrustMag = g_AsteroidsShipThrust;
            float noseAngle = s.shipAngle - PI * 0.5f;
            s.shipVx += std::cos(noseAngle) * thrustMag;
            s.shipVy += std::sin(noseAngle) * thrustMag;

            // Thruster particle embers
            if (rand() % 2 == 0) {
                float tailAngle = s.shipAngle + PI * 0.5f + ((rand() % 40) - 20) * 0.01f;
                float tailSpd = 1.5f + (rand() % 100) * 0.01f;
                s.sparks.push_back({ s.shipX - std::cos(noseAngle) * 5.0f,
                                     s.shipY - std::sin(noseAngle) * 5.0f,
                                     std::cos(tailAngle) * tailSpd + s.shipVx * 0.2f,
                                     std::sin(tailAngle) * tailSpd + s.shipVy * 0.2f,
                                     0, 10, COL_ORANGE_THRUST });
            }
        }

        // Cosmic zero-G inertia & drag
        s.shipVx *= 0.988f;
        s.shipVy *= 0.988f;
        s.shipX += s.shipVx;
        s.shipY += s.shipVy;
        s.shipX = WrapCoord(s.shipX, (float)s.virtualW);
        s.shipY = WrapCoord(s.shipY, (float)s.virtualH);
    }

    // 6. Update Flying Saucers (UFO)
    if (!s.ufo.active) {
        s.ufoSpawnTimer--;
        if (s.ufoSpawnTimer <= 0) {
            s.ufo.active = true;
            s.ufo.isSmall = (s.wave >= 2 && (rand() % 100 < 55));
            s.ufo.y = (float)(40 + rand() % (s.virtualH - 80));
            s.ufo.vx = (rand() % 2 == 0) ? 1.5f : -1.5f;
            if (s.ufo.isSmall) s.ufo.vx *= 1.3f;
            s.ufo.x = (s.ufo.vx > 0) ? 0.0f : (float)(s.virtualW - 1);
            s.ufo.vy = 0.0f;
            s.ufo.fireTimer = s.ufo.isSmall ? 40 : 65;
            s.ufo.dirChangeTimer = 60;
        }
    } else {
        s.ufo.x += s.ufo.vx;
        s.ufo.y += s.ufo.vy;

        // UFO direction changes
        s.ufo.dirChangeTimer--;
        if (s.ufo.dirChangeTimer <= 0) {
            s.ufo.dirChangeTimer = 50 + (rand() % 50);
            float dirs[3] = { -0.8f, 0.0f, 0.8f };
            s.ufo.vy = dirs[rand() % 3];
        }

        // UFO firing
        s.ufo.fireTimer--;
        if (s.ufo.fireTimer <= 0) {
            s.ufo.fireTimer = s.ufo.isSmall ? 50 : 75;
            float shotAngle = 0.0f;
            if (s.ufo.isSmall && s.shipAlive) {
                // Small UFO: accurate sniper shot targeting player ship!
                float dx = ToroidalDistX(s.ufo.x, s.shipX, (float)s.virtualW);
                float dy = ToroidalDistY(s.ufo.y, s.shipY, (float)s.virtualH);
                shotAngle = std::atan2(dy, dx) + ((rand() % 20) - 10) * 0.015f;
            } else {
                // Large UFO: wild inaccurate shots
                shotAngle = (float)(rand() % 628) / 100.0f;
            }
            float bSpd = 4.2f;
            s.bullets.push_back({ s.ufo.x, s.ufo.y, std::cos(shotAngle) * bSpd,
                                  std::sin(shotAngle) * bSpd, 65, true, true });
        }

        // Saucer exits screen boundary
        if ((s.ufo.vx > 0 && s.ufo.x > s.virtualW + 20) || (s.ufo.vx < 0 && s.ufo.x < -20)) {
            s.ufo.active = false;
            s.ufoSpawnTimer = 700 + (rand() % 500);
        }
    }

    // 7. Update Bullets
    for (auto& b : s.bullets) {
        if (!b.active) continue;
        b.x += b.vx;
        b.y += b.vy;
        b.x = WrapCoord(b.x, (float)s.virtualW);
        b.y = WrapCoord(b.y, (float)s.virtualH);
        b.life--;
        if (b.life <= 0) b.active = false;
    }

    // 8. Update Asteroids
    int activeRockCount = 0;
    for (auto& a : s.asteroids) {
        if (!a.active) continue;
        activeRockCount++;
        a.x += a.vx;
        a.y += a.vy;
        a.x = WrapCoord(a.x, (float)s.virtualW);
        a.y = WrapCoord(a.y, (float)s.virtualH);
        a.angle += a.rotSpeed;
    }

    // 9. COLLISIONS: Player Bullets vs Asteroids & UFO
    for (auto& b : s.bullets) {
        if (!b.active || b.isUfo) continue;

        // Bullet vs Asteroid
        for (auto& a : s.asteroids) {
            if (!a.active) continue;
            float dx = ToroidalDistX(b.x, a.x, (float)s.virtualW);
            float dy = ToroidalDistY(b.y, a.y, (float)s.virtualH);
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= a.radius + 2.0f) {
                b.active = false;
                a.active = false;

                int pts = (a.tier == AST_LARGE) ? 20 : (a.tier == AST_MEDIUM ? 50 : 100);
                s.score += pts;
                if (s.score > s.highScore) s.highScore = s.score;
                s.floatingScores.push_back({ a.x, a.y, pts, 35, COL_VECTOR_CORE });

                // Authentic Vector Line Shatter Shrapnel
                for (int k = 0; k < 6; ++k) {
                    float sAngle = (float)(rand() % 628) / 100.0f;
                    float sSpd = 0.8f + (rand() % 150) * 0.01f;
                    s.shards.push_back({ a.x, a.y, a.vx * 0.4f + std::cos(sAngle) * sSpd,
                                         a.vy * 0.4f + std::sin(sAngle) * sSpd,
                                         sAngle, ((rand() % 100) - 50) * 0.003f,
                                         a.radius * 0.5f, 0, 25, COL_VECTOR_CORE });
                }

                // Vector Sparks
                for (int k = 0; k < 12; ++k) {
                    float sAngle = (float)(rand() % 628) / 100.0f;
                    float sSpd = 1.2f + (rand() % 200) * 0.01f;
                    s.sparks.push_back({ a.x, a.y, std::cos(sAngle) * sSpd, std::sin(sAngle) * sSpd, 0, 16, COL_GRAY_LIGHT });
                }

                // Split into smaller asteroids
                if (a.tier == AST_LARGE) {
                    SpawnAsteroid(s, a.x, a.y, AST_MEDIUM);
                    SpawnAsteroid(s, a.x, a.y, AST_MEDIUM);
                } else if (a.tier == AST_MEDIUM) {
                    SpawnAsteroid(s, a.x, a.y, AST_SMALL);
                    SpawnAsteroid(s, a.x, a.y, AST_SMALL);
                }
                break;
            }
        }

        // Bullet vs UFO
        if (s.ufo.active && b.active) {
            float dx = ToroidalDistX(b.x, s.ufo.x, (float)s.virtualW);
            float dy = ToroidalDistY(b.y, s.ufo.y, (float)s.virtualH);
            float ufoRad = s.ufo.isSmall ? 8.0f : 14.0f;

            if (std::sqrt(dx * dx + dy * dy) <= ufoRad) {
                b.active = false;
                s.ufo.active = false;
                int pts = s.ufo.isSmall ? 1000 : 200;
                s.score += pts;
                if (s.score > s.highScore) s.highScore = s.score;
                s.floatingScores.push_back({ s.ufo.x, s.ufo.y, pts, 45, COL_GOLD });
                s.ufoSpawnTimer = 800 + (rand() % 500);

                // UFO vector explosion
                for (int k = 0; k < 8; ++k) {
                    float sAngle = (float)(rand() % 628) / 100.0f;
                    float sSpd = 1.0f + (rand() % 180) * 0.01f;
                    s.shards.push_back({ s.ufo.x, s.ufo.y, std::cos(sAngle) * sSpd, std::sin(sAngle) * sSpd,
                                         sAngle, ((rand() % 100) - 50) * 0.004f, 8.0f, 0, 30, COL_CRIMSON });
                }
            }
        }
    }

    // 10. COLLISIONS: Player Ship vs Asteroids, UFO, & UFO Bullets
    if (s.shipAlive && s.invulnTimer <= 0) {
        bool shipDestroyed = false;

        // Ship vs Asteroids
        for (const auto& a : s.asteroids) {
            if (!a.active) continue;
            float dx = ToroidalDistX(s.shipX, a.x, (float)s.virtualW);
            float dy = ToroidalDistY(s.shipY, a.y, (float)s.virtualH);
            if (std::sqrt(dx * dx + dy * dy) <= a.radius + 6.5f) {
                shipDestroyed = true;
                break;
            }
        }

        // Ship vs UFO
        if (!shipDestroyed && s.ufo.active) {
            float dx = ToroidalDistX(s.shipX, s.ufo.x, (float)s.virtualW);
            float dy = ToroidalDistY(s.shipY, s.ufo.y, (float)s.virtualH);
            float ufoRad = s.ufo.isSmall ? 8.0f : 14.0f;
            if (std::sqrt(dx * dx + dy * dy) <= ufoRad + 6.5f) {
                shipDestroyed = true;
            }
        }

        // Ship vs UFO Bullets
        if (!shipDestroyed) {
            for (auto& b : s.bullets) {
                if (!b.active || !b.isUfo) continue;
                float dx = ToroidalDistX(s.shipX, b.x, (float)s.virtualW);
                float dy = ToroidalDistY(s.shipY, b.y, (float)s.virtualH);
                if (std::sqrt(dx * dx + dy * dy) <= 9.0f) {
                    b.active = false;
                    shipDestroyed = true;
                    break;
                }
            }
        }

        if (shipDestroyed) {
            s.lives--;
            s.shipAlive = false;

            // Dramatic Vector Ship Shatter (Ship lines breaking apart in zero-G)
            for (size_t i = 0; i < SHIP_VERTS.size(); ++i) {
                size_t j = (i + 1) % SHIP_VERTS.size();
                float mx = (SHIP_VERTS[i].first + SHIP_VERTS[j].first) * 0.5f;
                float my = (SHIP_VERTS[i].second + SHIP_VERTS[j].second) * 0.5f;
                float sAngle = (float)(rand() % 628) / 100.0f;
                float sSpd = 1.0f + (rand() % 160) * 0.01f;
                s.shards.push_back({ s.shipX + mx, s.shipY + my,
                                     s.shipVx * 0.3f + std::cos(sAngle) * sSpd,
                                     s.shipVy * 0.3f + std::sin(sAngle) * sSpd,
                                     sAngle, ((rand() % 100) - 50) * 0.005f, 10.0f, 0, 45, COL_CYAN_CORE });
            }

            for (int k = 0; k < 24; ++k) {
                float a = (float)(rand() % 628) / 100.0f;
                float spd = 1.5f + (rand() % 220) * 0.01f;
                s.sparks.push_back({ s.shipX, s.shipY, std::cos(a) * spd, std::sin(a) * spd, 0, 24, COL_CYAN_CORE });
            }

            // Floating "SHIP LOST" text popup
            s.floatingScores.push_back({ s.shipX, s.shipY - 6.0f, -1, 50, COL_CRIMSON });

            if (s.lives > 0) {
                s.respawnTimer = 45; // ~0.75s dramatic death pause while shrapnel drifts
            } else {
                // GAME OVER
                s.state = 3;
                s.pauseTimer = 100; // ~3.3 seconds pause
            }
        }
    }

    // 11. Check Wave Clear
    if (activeRockCount == 0 && !s.ufo.active) {
        s.state = 2;
        s.pauseTimer = 80; // ~2.6 seconds pause
        s.score += 1000;   // Wave completion bonus
        if (s.score > s.highScore) s.highScore = s.score;

        // Celebratory vector particle bursts
        for (int k = 0; k < 36; ++k) {
            float fx = (float)(rand() % s.virtualW);
            float fy = (float)(rand() % s.virtualH);
            float a = (float)(rand() % 628) / 100.0f;
            float spd = 1.0f + (rand() % 180) * 0.01f;
            s.sparks.push_back({ fx, fy, std::cos(a) * spd, std::sin(a) * spd, 0, 30, COL_GREEN_LIME });
        }
    }
}

// ============================================================================
// COMPLETE 1979 VECTOR ARCADE FRAME RENDERING
// ============================================================================
static void RenderVectorFrame(AsteroidsState& s) {
    // 1. Clear Framebuffer to Obsidian Cosmic Space
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 2. Render Distant Twinkling Starfield
    for (const auto& star : s.stars) {
        PutPixel(s, (int)star.x, (int)star.y, star.color);
    }

    // 3. Render Asteroid Wireframe Rocks
    for (const auto& a : s.asteroids) {
        if (!a.active) continue;
        uint32_t rockCol = (a.tier == AST_LARGE) ? COL_GRAY_MID :
                           (a.tier == AST_MEDIUM ? COL_GRAY_LIGHT : COL_VECTOR_CORE);
        DrawVectorPolyWrapped(s, ASTEROID_SHAPES[a.shapeIndex], a.x, a.y, a.angle, a.radius, rockCol, true, true);
    }

    // 4. Render Flying Saucer (UFO)
    if (s.ufo.active) {
        float ufoScale = s.ufo.isSmall ? 7.0f : 12.0f;
        uint32_t ufoCol = s.ufo.isSmall ? COL_GOLD : COL_CRIMSON;
        DrawVectorPolyWrapped(s, UFO_BODY, s.ufo.x, s.ufo.y, 0.0f, ufoScale, ufoCol, true, true);
        // Saucer center equator line
        DrawVectorLine(s, (int)(s.ufo.x - ufoScale), (int)(s.ufo.y + ufoScale * 0.2f),
                          (int)(s.ufo.x + ufoScale), (int)(s.ufo.y + ufoScale * 0.2f), ufoCol, true);
    }

    // 5. Render Vector Bullets (Photon Pulses)
    for (const auto& b : s.bullets) {
        if (!b.active) continue;
        int bx = (int)b.x;
        int by = (int)b.y;
        uint32_t bCol = b.isUfo ? COL_CRIMSON : COL_VECTOR_CORE;

        // Glowing vector line bullet dash
        int bxTail = (int)(b.x - b.vx * 0.8f);
        int byTail = (int)(b.y - b.vy * 0.8f);
        DrawVectorLine(s, bxTail, byTail, bx, by, bCol, true);
        PutPixel(s, bx, by, COL_WHITE_CORE);
    }

    // 6. Render Floating Vector Shards (Shattered Rock & Ship Fragments)
    for (const auto& sh : s.shards) {
        float hx = std::cos(sh.angle) * sh.length * 0.5f;
        float hy = std::sin(sh.angle) * sh.length * 0.5f;
        int x1 = (int)(sh.x - hx); int y1 = (int)(sh.y - hy);
        int x2 = (int)(sh.x + hx); int y2 = (int)(sh.y + hy);
        DrawVectorLine(s, x1, y1, x2, y2, sh.color, false);
    }

    // 7. Render Vector Sparks
    for (const auto& sp : s.sparks) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }

    // 8. Render Player Ship & Thruster Jet
    if (s.shipAlive) {
        // Ship wireframe
        DrawVectorPolyWrapped(s, SHIP_VERTS, s.shipX, s.shipY, s.shipAngle, 1.0f, COL_CYAN_CORE, true, true);

        // Flickering Thruster Jet Flame
        if (s.thrusting) {
            DrawVectorPolyWrapped(s, THRUSTER_VERTS, s.shipX, s.shipY, s.shipAngle, 1.0f, COL_ORANGE_THRUST, false, true);
        }

        // Protective Invulnerability Pulsing Diamond Shield
        if (s.invulnTimer > 0) {
            float shieldPulse = 14.0f + std::sin((float)s.invulnTimer * 0.35f) * 2.5f;
            static const std::vector<std::pair<float, float>> SHIELD_VERTS = {
                { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }
            };
            uint32_t sCol = (s.invulnTimer % 4 < 2) ? COL_CYAN_CORE : COL_CYAN_GLOW;
            DrawVectorPolyWrapped(s, SHIELD_VERTS, s.shipX, s.shipY, 0.0f, shieldPulse, sCol, true, false);
        }
    }

    // 9. Floating Score & Status Popups
    for (const auto& fs : s.floatingScores) {
        char sBuf[16];
        if (fs.score > 0) {
            sprintf_s(sBuf, "+%d", fs.score);
            DrawText(s, (int)fs.x - 8, (int)fs.y, sBuf, fs.color);
        } else if (fs.score == -1) {
            DrawText(s, (int)fs.x - 26, (int)fs.y, "SHIP LOST", fs.color);
        }
    }

    // 10. Authentic 1979 Vector Arcade HUD
    // Top Left: Live Score
    char scoreBuf[32];
    sprintf_s(scoreBuf, "SCORE: %06d", s.score);
    DrawText(s, 14, 6, scoreBuf, COL_VECTOR_CORE);

    // Top Center: High Score
    int midX = s.virtualW / 2;
    char highBuf[32];
    sprintf_s(highBuf, "HIGH: %06d", s.highScore);
    int hw = (int)strlen(highBuf) * 6;
    DrawText(s, midX - hw / 2, 6, highBuf, COL_GOLD);

    // Top Right: Wave Number & Health Icons (Only mini vector ship icons, no text counter)
    char waveBuf[32];
    sprintf_s(waveBuf, "WAVE: %02d", s.wave);
    int ww = (int)strlen(waveBuf) * 6;

    int maxSlots = (std::max)(3, s.lives);
    int iconsWidth = maxSlots * 12;
    int iconStartX = s.virtualW - 14 - iconsWidth;

    DrawText(s, iconStartX - 16 - ww, 6, waveBuf, COL_VECTOR_CORE);

    // Mini vector ship icons showing health (decreases visibly as ships are lost)
    uint32_t livesCol = (s.lives > 1) ? COL_CYAN_CORE : COL_CRIMSON;
    for (int l = 0; l < s.lives; ++l) {
        float ix = (float)(iconStartX + l * 12 + 6);
        DrawVectorPolyWrapped(s, SHIP_VERTS, ix, 10.0f, 0.0f, 0.70f, livesCol, true, true);
    }

    // Center Stage Banners
    if (s.state == 2) {
        const char* clrTxt = "WAVE CLEAR!";
        int tw = (int)strlen(clrTxt) * 6;
        DrawText(s, midX - tw / 2, 120, clrTxt, COL_GREEN_LIME);
    } else if (s.state == 3) {
        const char* goTxt = "GAME OVER";
        int tw = (int)strlen(goTxt) * 6;
        DrawText(s, midX - tw / 2, 120, goTxt, COL_CRIMSON);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderAsteroids(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<AsteroidsState>(36);

    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    vW = (vW + 1) & ~1; // Force strictly even virtual width for exact bilateral symmetry
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;

    if (!s.initialized || s.virtualW != vW || s.fb.size() != (size_t)(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, COL_BLACK);

        // Initialize distant starfield
        s.stars.clear();
        for (int i = 0; i < 50; ++i) {
            float sx = (float)(rand() % s.virtualW);
            float sy = (float)(rand() % s.virtualH);
            float spd = 0.05f + (rand() % 50) * 0.002f;
            uint32_t col = (rand() % 3 == 0) ? COL_STAR_BRIGHT : COL_STAR_DIM;
            s.stars.push_back({ sx, sy, spd, col });
        }

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
        StepAsteroidsSimulation(s);
    }

    // Render virtual vector arcade framebuffer
    RenderVectorFrame(s);

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

} // namespace Asteroids

// Register as Screensaver ID 36
REGISTER_SCREENSAVER(
    36,
    L"Asteroids (1979 Vector Arcade)",
    "asteroids",
    { "asteroids", "asteroid", "vector-asteroids", "rocks" },
    WRAP_LEGACY(Asteroids::RenderAsteroids),
    GetAsteroidsSettings()
);
