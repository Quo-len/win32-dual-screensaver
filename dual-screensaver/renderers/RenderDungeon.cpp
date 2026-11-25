#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/DungeonSettings.h"
#include <vector>
#include <unordered_map>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdint>

// ============================================================================
// STONE DUNGEON WALK SCREENSAVER
// Authentic procedural 3D raycaster dungeon exploration with grey stone brick themes,
// lantern illumination, wall-mounted flickering torches, soft bloom halos,
// auto-navigation exploration AI, depth descent transitions, and a retro minimap.
// ============================================================================

static constexpr int BASE_DUNGEON_H = 270;
static constexpr int MIN_DUNGEON_W  = 270;
static constexpr int MAX_DUNGEON_W  = 1280;

struct DungeonThemeDef {
    float bgR, bgG, bgB;
    float stR, stG, stB;
    float ltR, ltG, ltB;
    float tcR, tcG, tcB;
    uint32_t hudColor;
    uint32_t mapWallColor;
    uint32_t mapVisitedColor;
    uint32_t mapPlayerColor;
    uint32_t mapBorderColor;
};

// Available Themes (Default 0 = Crypt Slate Grey from dungeon-stone.bak.html)
static const DungeonThemeDef DUNGEON_THEMES[5] = {
    // 0: Crypt - Cool Slate Grey (exact grey theme from dungeon-stone.bak.html)
    {
        6.0f, 7.0f, 11.0f,
        118.0f, 126.0f, 142.0f,
        206.0f / 255.0f, 218.0f / 255.0f, 240.0f / 255.0f,
        170.0f, 200.0f, 255.0f,
        0xFFE9ECF6,
        0xFF4A5568,
        0xFF141A24,
        0xFFAAC8FF,
        0xFF2D3748
    },
    // 1: Neutral Ash Grey (True neutral grey stone with warm torches)
    {
        8.0f, 8.0f, 8.0f,
        135.0f, 135.0f, 135.0f,
        225.0f / 255.0f, 225.0f / 255.0f, 225.0f / 255.0f,
        255.0f, 176.0f, 90.0f,
        0xFFE0E0E0,
        0xFF585858,
        0xFF1E1E1E,
        0xFFFFB05A,
        0xFF383838
    },
    // 2: Warm Sandstone (Classic Stone)
    {
        8.0f, 7.0f, 6.0f,
        152.0f, 144.0f, 132.0f,
        255.0f / 255.0f, 220.0f / 255.0f, 176.0f / 255.0f,
        255.0f, 176.0f, 90.0f,
        0xFFF0E8DA,
        0xFF585450,
        0xFF242220,
        0xFFFFB05A,
        0xFF36322E
    },
    // 3: Moss
    {
        6.0f, 8.0f, 6.0f,
        124.0f, 134.0f, 116.0f,
        240.0f / 255.0f, 232.0f / 255.0f, 190.0f / 255.0f,
        255.0f, 190.0f, 100.0f,
        0xFFEEF5E2,
        0xFF4A5844,
        0xFF1A2218,
        0xFFFFBE64,
        0xFF2A3626
    },
    // 4: Ember
    {
        10.0f, 6.0f, 5.0f,
        150.0f, 120.0f, 108.0f,
        255.0f / 255.0f, 190.0f / 255.0f, 140.0f / 255.0f,
        255.0f, 140.0f, 70.0f,
        0xFFFBE9DE,
        0xFF5A443E,
        0xFF241612,
        0xFFFF8C46,
        0xFF3A2822
    }
};

// Directions: 0: +X, 1: +Y, 2: -X, 3: -Y
static const int DX[4] = { 1, 0, -1, 0 };
static const int DY[4] = { 0, 1, 0, -1 };

struct TorchGlow {
    float d;
    float x;
    float y;
    float r;
};

struct DungeonState {
    int virtualW = 0;
    int virtualH = 0;
    std::vector<uint32_t> fb;
    std::vector<float> vigTable;
    std::vector<float> rowFp;
    std::vector<float> rowI_floor;
    std::vector<float> rowI_ceil;

    int ci = 1, cj = 1;
    int ti = 1, tj = 1;
    float s = 1.0f;
    int dir = 0;
    float ang = 0.0f;
    int steps = 0;
    int sinceNew = 0;
    float levelTime = 0.0f;
    float phase = 0.0f;
    uint32_t seed = 1337;
    int depth = 1;
    float clock = 0.0f;
    float fade = 1.0f;
    float fadeDir = -1.0f;
    bool pending = false;
    uint64_t lastTick = 0;
    bool initialized = false;

    std::unordered_map<uint64_t, int> visits;
};

// ============================================================================
// FAST HASH & PROCEDURAL GENERATION
// ============================================================================
static inline uint64_t PackCoord(int x, int y) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
            static_cast<uint64_t>(static_cast<uint32_t>(y));
}

static inline float hh(int a, int b, int c, uint32_t seed) {
    uint32_t h = (static_cast<uint32_t>(a) * 374761393u) ^
                 (static_cast<uint32_t>(b) * 668265263u) ^
                 ((static_cast<uint32_t>(c) + seed) * 2147483629u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return static_cast<float>(h) * (1.0f / 4294967296.0f);
}

static inline float nz(int a, int b) {
    uint32_t h = (static_cast<uint32_t>(a) * 374761393u) ^ (static_cast<uint32_t>(b) * 668265263u);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return static_cast<float>(h) * (1.0f / 4294967296.0f);
}

static inline int floor_div(int a, int b) {
    int res = a / b;
    int rem = a % b;
    if (rem != 0 && ((a < 0) ^ (b < 0))) {
        res--;
    }
    return res;
}

static inline bool wallAt(int x, int y, uint32_t seed) {
    int xo = x & 1;
    int yo = y & 1;
    if (xo && yo) return false;
    int bx = floor_div(x, 10);
    int by = floor_div(y, 10);
    int lx = x - bx * 10;
    int ly = y - by * 10;
    if (lx >= 2 && lx <= 7 && ly >= 2 && ly <= 7 && hh(bx, by, 5, seed) < 0.22f) {
        return (lx == 2 || lx == 6) && (ly == 2 || ly == 6);
    }
    if (!xo && !yo) return true;
    return hh(x, y, 1, seed) > 0.60f;
}

static inline bool torchAt(int x, int y, uint32_t seed) {
    return hh(x, y, 9, seed) < 0.10f;
}

static std::vector<int> OpenDirs(const DungeonState& s, int i, int j) {
    std::vector<int> o;
    for (int d = 0; d < 4; ++d) {
        if (!wallAt(i + DX[d], j + DY[d], s.seed)) {
            o.push_back(d);
        }
    }
    return o;
}

static void SpawnDungeon(DungeonState& s) {
    int i = 1 + 2 * (rand() % 201 - 100);
    int j = 1 + 2 * (rand() % 201 - 100);
    for (int k = 0; k < 400; ++k) {
        if (OpenDirs(s, i, j).size() >= 2) break;
        i += 2 * (rand() % 3 - 1);
        j += 2 * (rand() % 3 - 1);
    }
    s.ci = s.ti = i;
    s.cj = s.tj = j;
    s.s = 1.0f;
    auto o = OpenDirs(s, i, j);
    s.dir = !o.empty() ? o[rand() % o.size()] : 0;
    s.ang = s.dir * (3.1415926535f * 0.5f);
    s.visits.clear();
    s.steps = 0;
    s.sinceNew = 0;
    s.levelTime = 0.0f;
}

static void PickNext(DungeonState& s) {
    auto o = OpenDirs(s, s.ci, s.cj);
    int opp = (s.dir + 2) & 3;
    float tot = 0.0f;
    std::vector<float> w;
    w.reserve(o.size());
    for (int d : o) {
        uint64_t key = PackCoord(s.ci + 2 * DX[d], s.cj + 2 * DY[d]);
        auto it = s.visits.find(key);
        int v = (it != s.visits.end()) ? it->second : 0;
        float base = (d == s.dir) ? 3.0f : ((d == opp) ? 0.12f : 2.0f);
        float q = base / (1.0f + v * 1.6f);
        w.push_back(q);
        tot += q;
    }
    float r = (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f)) * tot;
    int chosenDir = !o.empty() ? o.back() : opp;
    for (size_t k = 0; k < o.size(); ++k) {
        r -= w[k];
        if (r <= 0.0f) {
            chosenDir = o[k];
            break;
        }
    }
    s.dir = chosenDir;
    uint64_t nextKey = PackCoord(s.ci + 2 * DX[chosenDir], s.cj + 2 * DY[chosenDir]);
    if (s.visits.find(nextKey) == s.visits.end()) {
        s.sinceNew = 0;
    } else {
        s.sinceNew++;
    }
    s.visits[nextKey]++;
    if (s.visits.size() > 6000) s.visits.clear();
    s.ti = s.ci + 2 * DX[chosenDir];
    s.tj = s.cj + 2 * DY[chosenDir];
    s.s = 0.0f;
    s.steps++;
}

static void DescendDungeon(DungeonState& s) {
    s.seed = static_cast<uint32_t>(rand()) ^ (static_cast<uint32_t>(rand()) << 15);
    s.depth++;
    SpawnDungeon(s);
}

// ============================================================================
// COLOR & PIXEL PRIMITIVES
// ============================================================================
static inline uint32_t PackColor(float r, float g, float b) {
    int ir = static_cast<int>(r + 0.5f);
    int ig = static_cast<int>(g + 0.5f);
    int ib = static_cast<int>(b + 0.5f);
    if (ir < 0) ir = 0; else if (ir > 255) ir = 255;
    if (ig < 0) ig = 0; else if (ig > 255) ig = 255;
    if (ib < 0) ib = 0; else if (ib > 255) ib = 255;
    return 0xFF000000u | (static_cast<uint32_t>(ir) << 16) | (static_cast<uint32_t>(ig) << 8) | static_cast<uint32_t>(ib);
}

// 5x7 Retro Bitmap Font for HUD
static const uint8_t* GetDungeonGlyph(char c) {
    static const uint8_t GLYPH_BLANK[7] = { 0 };
    switch (c) {
    case '0': { static const uint8_t g[7] = { 0x1F, 0x11, 0x13, 0x15, 0x19, 0x11, 0x1F }; return g; }
    case '1': { static const uint8_t g[7] = { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
    case '2': { static const uint8_t g[7] = { 0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F }; return g; }
    case '3': { static const uint8_t g[7] = { 0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x1F }; return g; }
    case '4': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01 }; return g; }
    case '5': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F }; return g; }
    case '6': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F }; return g; }
    case '7': { static const uint8_t g[7] = { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }; return g; }
    case '8': { static const uint8_t g[7] = { 0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F }; return g; }
    case '9': { static const uint8_t g[7] = { 0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F }; return g; }
    case 'D': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E }; return g; }
    case 'E': { static const uint8_t g[7] = { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }; return g; }
    case 'P': { static const uint8_t g[7] = { 0x1F, 0x11, 0x11, 0x1F, 0x10, 0x10, 0x10 }; return g; }
    case 'T': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
    case ' ': { return GLYPH_BLANK; }
    default: return GLYPH_BLANK;
    }
}

static void DrawDungeonText(DungeonState& s, int px, int py, const char* str, uint32_t color) {
    while (*str) {
        const uint8_t* rows = GetDungeonGlyph(*str);
        for (int r = 0; r < 7; ++r) {
            uint8_t bits = rows[r];
            for (int col = 0; col < 5; ++col) {
                if (bits & (0x10 >> col)) {
                    int x = px + col;
                    int y = py + r;
                    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
                        s.fb[y * s.virtualW + x] = color;
                    }
                }
            }
        }
        px += 6;
        str++;
    }
}

// ============================================================================
// SIMULATION STEP
// ============================================================================
static void StepDungeonSimulation(DungeonState& s, float dt) {
    s.clock += dt;
    s.levelTime += dt;
    if (s.fade < 1.0f) {
        s.s += dt * 1.25f * 0.5f * g_DungeonWalkSpeed;
        s.phase += dt * 1.25f * 3.1f * g_DungeonWalkSpeed;
        if (s.s >= 1.0f) {
            s.s = 1.0f;
            s.ci = s.ti;
            s.cj = s.tj;
            PickNext(s);
        }
    }
    float target = s.dir * (3.1415926535f * 0.5f);
    float diff = atan2f(sinf(target - s.ang), cosf(target - s.ang));
    s.ang += diff * (1.0f - expf(-dt * 2.4f)) + sinf(s.clock * 0.45f) * 0.0012f;

    if (!s.pending && (s.sinceNew > 260 || s.levelTime > 120.0f)) {
        s.pending = true;
        s.fadeDir = 1.0f;
    }
    if (s.fadeDir > 0.0f) {
        s.fade = (std::min)(1.0f, s.fade + dt * 0.5f);
        if (s.fade >= 1.0f && s.pending) {
            DescendDungeon(s);
            s.pending = false;
            s.fadeDir = -1.0f;
        }
    } else if (s.fade > 0.0f) {
        s.fade = (std::max)(0.0f, s.fade - dt / 2.2f);
    }
}

// ============================================================================
// RAYCASTING RENDER PASS
// ============================================================================
static void RenderDungeonFrame(DungeonState& s) {
    int tIdx = g_DungeonTheme;
    if (tIdx < 0 || tIdx >= 5) tIdx = 0;
    const auto& th = DUNGEON_THEMES[tIdx];

    float px = s.ci + 0.5f + (s.ti - s.ci) * s.s;
    float py = s.cj + 0.5f + (s.tj - s.cj) * s.s;

    float bob = sinf(s.phase) * s.virtualH * 0.0022f;
    float dx = cosf(s.ang);
    float dy = sinf(s.ang);
    static constexpr float PL = 0.95f;
    float plx = -dy * PL;
    float ply = dx * PL;
    float focal = (s.virtualW * 0.5f) / PL * 0.6f;
    float mid = s.virtualH * 0.5f + bob;
    float flick = 1.0f + 0.05f * sinf(s.clock * 7.1f) + 0.04f * sinf(s.clock * 13.7f) + 0.03f * sinf(s.clock * 3.3f);

    // Precalculate row distances and light terms
    for (int y = 0; y < s.virtualH; ++y) {
        float o = fabsf(y + 0.5f - mid);
        float fp = (o < 0.5f) ? 80.0f : (focal * 0.5f / o);
        s.rowFp[y] = fp;
        s.rowI_floor[y] = flick / (1.0f + 0.26f * fp * fp) * 0.75f;
        s.rowI_ceil[y]  = flick / (1.0f + 0.26f * fp * fp) * 0.36f;
    }

    std::unordered_map<uint64_t, TorchGlow> glows;

    // DDA Raycasting per column
    for (int c = 0; c < s.virtualW; ++c) {
        float camX = 2.0f * (c + 0.5f) / static_cast<float>(s.virtualW) - 1.0f;
        float rx = dx + plx * camX;
        float ry = dy + ply * camX;

        int mx = static_cast<int>(floorf(px));
        int my = static_cast<int>(floorf(py));

        float ddx = fabsf(1.0f / (fabsf(rx) > 1e-9f ? rx : 1e-9f));
        float ddy = fabsf(1.0f / (fabsf(ry) > 1e-9f ? ry : 1e-9f));

        int sx, sy;
        float sdx, sdy;
        if (rx < 0.0f) {
            sx = -1;
            sdx = (px - mx) * ddx;
        } else {
            sx = 1;
            sdx = (mx + 1.0f - px) * ddx;
        }
        if (ry < 0.0f) {
            sy = -1;
            sdy = (py - my) * ddy;
        } else {
            sy = 1;
            sdy = (my + 1.0f - py) * ddy;
        }

        int side = 0;
        bool hit = false;
        int guard = 0;
        while (!hit && guard++ < 46) {
            if (sdx < sdy) {
                sdx += ddx;
                mx += sx;
                side = 0;
            } else {
                sdy += ddy;
                my += sy;
                side = 1;
            }
            if (wallAt(mx, my, s.seed)) {
                hit = true;
            }
        }

        float perp = (std::max)(0.08f, hit ? (side == 0 ? sdx - ddx : sdy - ddy) : 46.0f);
        float wu = (side == 0) ? (py + perp * ry) : (px + perp * rx);
        float wallX = wu - floorf(wu);
        float wallPx = focal / perp;
        float y0 = mid - wallPx * 0.5f;
        float y1 = mid + wallPx * 0.5f;

        bool torch = hit && torchAt(mx, my, s.seed);
        if (torch) {
            uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(mx)) << 33) |
                           (static_cast<uint64_t>(static_cast<uint32_t>(my)) << 1) |
                           (side & 1);
            float distCenter = fabsf(wallX - 0.5f);
            auto it = glows.find(key);
            if (it == glows.end() || distCenter < it->second.d) {
                glows[key] = TorchGlow{ distCenter, static_cast<float>(c), y0 + wallPx * 0.42f, wallPx * 0.5f };
            }
        }

        float wl = flick / (1.0f + 0.26f * perp * perp) * (side ? 0.7f : 1.0f);
        float edgeD = 1.0f - 0.5f * expf(-(std::min)(wallX, 1.0f - wallX) * 16.0f);
        float mFade = (std::max)(0.0f, (std::min)(1.0f, 1.0f - perp / 9.0f));

        int yStart = static_cast<int>(ceilf(y0));
        int yEnd = static_cast<int>(floorf(y1));
        int yStartClamped = (std::max)(0, (std::min)(s.virtualH, yStart));
        int yEndClamped = (std::max)(0, (std::min)(s.virtualH - 1, yEnd));

        float invWallPx = (wallPx > 1e-4f) ? (1.0f / wallPx) : 1.0f;

        // 1. Ceiling (Flagstone)
        for (int y = 0; y < yStartClamped; ++y) {
            float fp = s.rowFp[y];
            float wx = px + rx * fp;
            float wy = py + ry * fp;
            int fj = static_cast<int>(floorf(wy * 2.0f));
            float fu = wx * 2.0f + (fj & 1) * 0.5f;
            int fi = static_cast<int>(floorf(fu));
            float mort = (fu - fi < 0.07f || wy * 2.0f - fj < 0.07f) ? (1.0f - 0.5f * mFade) : 1.0f;
            float tone = (0.72f + 0.32f * nz(fi + 977, fj)) * mort *
                         (0.9f + 0.1f * nz(static_cast<int>(floorf(wx * 9.0f)), static_cast<int>(floorf(wy * 9.0f))));
            float I = s.rowI_ceil[y] * tone;
            float r = th.bgR + th.stR * th.ltR * I;
            float g = th.bgG + th.stG * th.ltG * I;
            float b = th.bgB + th.stB * th.ltB * I;

            float dth = static_cast<float>(((c * 7 + y * 13) & 3) - 1.5f);
            float vig = s.vigTable[y * s.virtualW + c];
            s.fb[y * s.virtualW + c] = PackColor((r + dth) * vig, (g + dth) * vig, (b + dth) * vig);
        }

        // 2. Wall (Grey Bricks)
        for (int y = yStartClamped; y <= yEndClamped; ++y) {
            float yc = y + 0.5f;
            float v = (yc - y0) * invWallPx;
            float vv = v * 5.0f;
            int course = static_cast<int>(floorf(vv));
            float fv = vv - course;
            float uu = wu * 2.0f + (course & 1) * 0.5f + nz(course, 11) * 0.35f;
            int bi = static_cast<int>(floorf(uu));
            float bf = uu - bi;
            float mort = (fv < 0.08f || bf < 0.045f) ? (1.0f - 0.55f * mFade) : 1.0f;
            float tone = (0.74f + 0.34f * nz(bi + (side ? 300 : 0), course * 7 + (side ? mx : my) * 3)) *
                         mort * (0.9f + 0.1f * nz(static_cast<int>(floorf(wu * 16.0f)), static_cast<int>(floorf(v * 36.0f))));
            float q = 2.0f * v - 1.0f;
            float q2 = q * q;
            float ao = 1.0f - 0.5f * q2 * q2;
            float I = wl * tone * edgeD * ao;

            float trAdd = 0.0f, tgAdd = 0.0f, tbAdd = 0.0f;
            if (torch) {
                float dxW = wallX - 0.5f;
                float dyV = v - 0.40f;
                float gl = expf(-(dxW * dxW / 0.05f + dyV * dyV / 0.03f));
                float ex = 1.25f * gl * flick / (1.0f + 0.03f * perp * perp);
                I += ex * tone;

                float dyV2 = v - 0.38f;
                float fl2 = expf(-(dxW * dxW / 0.0014f + dyV2 * dyV2 / 0.0024f)) * (0.85f + 0.3f * (flick - 1.0f) * 6.0f);
                trAdd = th.tcR * fl2 * 1.3f;
                tgAdd = th.tcG * fl2 * 1.3f;
                tbAdd = th.tcB * fl2 * 1.3f;
            }

            float r = th.bgR + th.stR * th.ltR * I + trAdd;
            float g = th.bgG + th.stG * th.ltG * I + tgAdd;
            float b = th.bgB + th.stB * th.ltB * I + tbAdd;

            float dth = static_cast<float>(((c * 7 + y * 13) & 3) - 1.5f);
            float vig = s.vigTable[y * s.virtualW + c];
            s.fb[y * s.virtualW + c] = PackColor((r + dth) * vig, (g + dth) * vig, (b + dth) * vig);
        }

        // 3. Floor (Flagstone)
        for (int y = yEndClamped + 1; y < s.virtualH; ++y) {
            float fp = s.rowFp[y];
            float wx = px + rx * fp;
            float wy = py + ry * fp;
            int fj = static_cast<int>(floorf(wy * 2.0f));
            float fu = wx * 2.0f + (fj & 1) * 0.5f;
            int fi = static_cast<int>(floorf(fu));
            float mort = (fu - fi < 0.07f || wy * 2.0f - fj < 0.07f) ? (1.0f - 0.5f * mFade) : 1.0f;
            float tone = (0.72f + 0.32f * nz(fi, fj)) * mort *
                         (0.9f + 0.1f * nz(static_cast<int>(floorf(wx * 9.0f)), static_cast<int>(floorf(wy * 9.0f))));
            float I = s.rowI_floor[y] * tone;
            float r = th.bgR + th.stR * th.ltR * I;
            float g = th.bgG + th.stG * th.ltG * I;
            float b = th.bgB + th.stB * th.ltB * I;

            float dth = static_cast<float>(((c * 7 + y * 13) & 3) - 1.5f);
            float vig = s.vigTable[y * s.virtualW + c];
            s.fb[y * s.virtualW + c] = PackColor((r + dth) * vig, (g + dth) * vig, (b + dth) * vig);
        }
    }

    // Soft torch halos (bloom)
    for (const auto& kv : glows) {
        const auto& g = kv.second;
        float gx = g.x;
        float gy = g.y;
        float rr = g.r * 1.7f;
        if (rr < 2.0f) continue;
        if (rr > s.virtualW * 0.45f) rr = s.virtualW * 0.45f;

        int minX = (std::max)(0, static_cast<int>(gx - rr));
        int maxX = (std::min)(s.virtualW - 1, static_cast<int>(gx + rr));
        int minY = (std::max)(0, static_cast<int>(gy - rr));
        int maxY = (std::min)(s.virtualH - 1, static_cast<int>(gy + rr));
        float invR = 1.0f / rr;

        for (int y = minY; y <= maxY; ++y) {
            float dyH = (y - gy) * invR;
            float dy2 = dyH * dyH;
            if (dy2 >= 1.0f) continue;
            for (int x = minX; x <= maxX; ++x) {
                float dxH = (x - gx) * invR;
                float d2 = dxH * dxH + dy2;
                if (d2 >= 1.0f) continue;
                float distNorm = sqrtf(d2);
                float alpha = 0.0f;
                if (distNorm < 0.45f) {
                    float t = distNorm / 0.45f;
                    alpha = (0.16f * flick) * (1.0f - t) + 0.04f * t;
                } else {
                    float t = (distNorm - 0.45f) / 0.55f;
                    alpha = 0.04f * (1.0f - t);
                }
                uint32_t pix = s.fb[y * s.virtualW + x];
                int pr = ((pix >> 16) & 0xFF) + static_cast<int>(th.tcR * alpha + 0.5f);
                int pg = ((pix >> 8)  & 0xFF) + static_cast<int>(th.tcG * alpha + 0.5f);
                int pb = (pix & 0xFF)         + static_cast<int>(th.tcB * alpha + 0.5f);
                if (pr > 255) pr = 255;
                if (pg > 255) pg = 255;
                if (pb > 255) pb = 255;
                s.fb[y * s.virtualW + x] = 0xFF000000u | (pr << 16) | (pg << 8) | pb;
            }
        }
    }

    // Optional Minimap in top-right corner
    if (g_DungeonMinimap) {
        const int cell = 3;
        const int R = 8;
        int ox = s.virtualW - (2 * R + 1) * cell - 8;
        int oy = 8;
        int pcx = static_cast<int>(floorf(px));
        int pcy = static_cast<int>(floorf(py));

        for (int my = -1; my <= (2 * R + 1) * cell; ++my) {
            for (int mx = -1; mx <= (2 * R + 1) * cell; ++mx) {
                int pxMap = ox + mx;
                int pyMap = oy + my;
                if (pxMap >= 0 && pxMap < s.virtualW && pyMap >= 0 && pyMap < s.virtualH) {
                    if (my == -1 || my == (2 * R + 1) * cell || mx == -1 || mx == (2 * R + 1) * cell) {
                        s.fb[pyMap * s.virtualW + pxMap] = th.mapBorderColor;
                    } else {
                        uint32_t cur = s.fb[pyMap * s.virtualW + pxMap];
                        uint32_t cr = ((cur >> 16) & 0xFF) >> 1;
                        uint32_t cg = ((cur >> 8)  & 0xFF) >> 1;
                        uint32_t cb = (cur & 0xFF) >> 1;
                        s.fb[pyMap * s.virtualW + pxMap] = 0xFF000000u | (cr << 16) | (cg << 8) | cb;
                    }
                }
            }
        }

        for (int j = -R; j <= R; ++j) {
            for (int i = -R; i <= R; ++i) {
                int gx = pcx + i;
                int gy = pcy + j;
                uint32_t col = 0;
                if (wallAt(gx, gy, s.seed)) {
                    col = th.mapWallColor;
                } else if (s.visits.find(PackCoord(gx, gy)) != s.visits.end()) {
                    col = th.mapVisitedColor;
                } else {
                    continue;
                }
                int bx = ox + (i + R) * cell;
                int by = oy + (j + R) * cell;
                for (int cy = 0; cy < cell - 1; ++cy) {
                    for (int cx = 0; cx < cell - 1; ++cx) {
                        int pxMap = bx + cx;
                        int pyMap = by + cy;
                        if (pxMap >= 0 && pxMap < s.virtualW && pyMap >= 0 && pyMap < s.virtualH) {
                            s.fb[pyMap * s.virtualW + pxMap] = col;
                        }
                    }
                }
            }
        }

        // Player torch position blip
        int pBlipX = ox + R * cell;
        int pBlipY = oy + R * cell;
        for (int cy = 0; cy < cell - 1; ++cy) {
            for (int cx = 0; cx < cell - 1; ++cx) {
                int pxMap = pBlipX + cx;
                int pyMap = pBlipY + cy;
                if (pxMap >= 0 && pxMap < s.virtualW && pyMap >= 0 && pyMap < s.virtualH) {
                    s.fb[pyMap * s.virtualW + pxMap] = th.mapPlayerColor;
                }
            }
        }
    }

    // Depth display in HUD
    char depthBuf[32];
    snprintf(depthBuf, sizeof(depthBuf), "DEPTH %d", s.depth);
    DrawDungeonText(s, 10, 10, depthBuf, th.hudColor);

    // Fade descent transition (fade to dark dungeon background)
    if (s.fade > 0.0f) {
        float f = (std::min)(1.0f, s.fade);
        float invF = 1.0f - f;
        for (auto& pix : s.fb) {
            int pr = (pix >> 16) & 0xFF;
            int pg = (pix >> 8)  & 0xFF;
            int pb = pix & 0xFF;
            pr = static_cast<int>(pr * invF + th.bgR * f);
            pg = static_cast<int>(pg * invF + th.bgG * f);
            pb = static_cast<int>(pb * invF + th.bgB * f);
            pix = 0xFF000000u | (pr << 16) | (pg << 8) | pb;
        }
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderDungeon(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<DungeonState>(37);

    int vH = BASE_DUNGEON_H;
    int vW = static_cast<int>(270.0f * static_cast<float>(width) / static_cast<float>(height));
    if (vW < MIN_DUNGEON_W) vW = MIN_DUNGEON_W;
    if (vW > MAX_DUNGEON_W) vW = MAX_DUNGEON_W;
    if (vW % 2 != 0) vW++;

    if (!s.initialized || s.virtualW != vW || s.virtualH != vH || s.fb.size() != static_cast<size_t>(vW * vH)) {
        s.virtualW = vW;
        s.virtualH = vH;
        int tIdx = g_DungeonTheme;
        if (tIdx < 0 || tIdx >= 5) tIdx = 0;
        const auto& th = DUNGEON_THEMES[tIdx];
        s.fb.assign(vW * vH, PackColor(th.bgR, th.bgG, th.bgB));
        s.rowFp.resize(vH);
        s.rowI_floor.resize(vH);
        s.rowI_ceil.resize(vH);

        // Precompute vignette table
        s.vigTable.resize(vW * vH);
        float cx = vW * 0.5f;
        float cy = vH * 0.5f;
        for (int y = 0; y < vH; ++y) {
            float vy = (y - cy) / cy;
            float vy2 = vy * vy;
            for (int x = 0; x < vW; ++x) {
                float vx = (x - cx) / cx;
                float d2 = vx * vx + vy2;
                float vig = 1.0f;
                if (d2 > 0.3025f) { // 0.55^2
                    float d = sqrtf(d2);
                    vig = 1.0f - 0.45f * ((d - 0.55f) / 0.45f);
                    if (vig < 0.38f) vig = 0.38f;
                }
                s.vigTable[y * vW + x] = vig;
            }
        }

        s.seed = static_cast<uint32_t>(rand()) ^ (static_cast<uint32_t>(rand()) << 15);
        s.depth = 1;
        SpawnDungeon(s);
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200;
    s.lastTick = now;

    float dt = elapsed > 0 ? (elapsed / 1000.0f) : 0.016f;
    if (dt > 0.05f) dt = 0.05f;

    StepDungeonSimulation(s, dt);
    RenderDungeonFrame(s);

    // Fullscreen DIB blit
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = s.virtualW;
    bmi.bmiHeader.biHeight = -s.virtualH; // Top-down
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

// Register as Screensaver ID 37
REGISTER_SCREENSAVER(
    37,
    L"Stone Dungeon",
    "dungeon",
    { "dungeon", "stone", "stonedungeon", "dungeonwalk" },
    WRAP_LEGACY(RenderDungeon),
    GetDungeonSettings()
);
