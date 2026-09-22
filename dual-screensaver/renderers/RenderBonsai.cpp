#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>

struct BonsaiCell {
    char ch = ' ';
    COLORREF color = 0;
};

struct BonsaiShoot {
    float x, y;
    float dx, dy;
    int age;
    int maxAge;
    int generation;
    int thickness;
};

struct BonsaiPetal {
    float x, y;
    float vx, vy;
    float phase;
    char ch;
    COLORREF color;
};

struct BonsaiState {
    bool initialized = false;
    DWORD lastTick = 0;
    int widthInChars = 0;
    int heightInChars = 0;
    int state = 0; // 0: growing, 1: mature/petals, 2: renew
    DWORD stateStartTime = 0;
    int theme = 0; // 0: Spring, 1: Sakura, 2: Autumn, 3: Ginkgo
    std::vector<BonsaiCell> grid;
    std::vector<BonsaiShoot> activeShoots;
    std::vector<BonsaiPetal> petals;
    std::vector<POINT> leaves;
};

// Pot and Soil Colors
static const COLORREF COL_POT_OUTER = RGB(170, 95, 45);
static const COLORREF COL_POT_INNER = RGB(125, 65, 30);
static const COLORREF COL_POT_RIM   = RGB(195, 120, 60);
static const COLORREF COL_SOIL      = RGB(80, 50, 25);
static const COLORREF COL_MOSS      = RGB(55, 125, 45);

// Bark Gradients by Generation
static const COLORREF WOOD_COLORS[4] = {
    RGB(130, 80, 40),   // Gen 0: Main trunk
    RGB(155, 100, 50),  // Gen 1: Major boughs
    RGB(180, 120, 65),  // Gen 2: Branches
    RGB(205, 145, 80)   // Gen 3: Fine twigs
};

// Foliage Palettes
static const COLORREF PAL_SPRING[] = {
    RGB(40, 180, 70), RGB(28, 140, 50), RGB(85, 215, 100), RGB(20, 110, 35)
};
static const COLORREF PAL_SAKURA[] = {
    RGB(255, 182, 193), RGB(255, 135, 180), RGB(255, 220, 235), RGB(255, 95, 150)
};
static const COLORREF PAL_AUTUMN[] = {
    RGB(225, 50, 35), RGB(255, 130, 20), RGB(245, 190, 40), RGB(180, 35, 25)
};
static const COLORREF PAL_GINKGO[] = {
    RGB(255, 215, 0), RGB(240, 175, 20), RGB(255, 240, 120), RGB(215, 150, 15)
};

static COLORREF GetFoliageColor(int theme) {
    int idx = rand() % 4;
    switch (theme) {
        case 1:  return PAL_SAKURA[idx];
        case 2:  return PAL_AUTUMN[idx];
        case 3:  return PAL_GINKGO[idx];
        case 0:
        default: return PAL_SPRING[idx];
    }
}

static void DrawPot(BonsaiState& s, int cols, int rows, int potY, int potHalfW) {
    auto putCell = [&](int x, int y, char c, COLORREF col) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            s.grid[y * cols + x].ch = c;
            s.grid[y * cols + x].color = col;
        }
    };

    int cx = cols / 2;
    int left = cx - potHalfW;
    int right = cx + potHalfW;


    // Rim: (---___________________---)
    putCell(left,     potY, '(', COL_POT_RIM);
    putCell(left + 1, potY, '-', COL_POT_RIM);
    putCell(left + 2, potY, '-', COL_POT_RIM);
    for (int x = left + 3; x <= right - 3; ++x) {
        putCell(x, potY, '_', COL_POT_RIM);
    }
    putCell(right - 2, potY, '-', COL_POT_RIM);
    putCell(right - 1, potY, '-', COL_POT_RIM);
    putCell(right,     potY, ')', COL_POT_RIM);

    // Soil & Moss inside
    for (int x = left + 4; x <= right - 4; ++x) {
        char s = (x % 3 == 0) ? '^' : ((x % 5 == 0) ? '~' : '.');
        COLORREF c = (x % 2 == 0) ? COL_MOSS : COL_SOIL;
        putCell(x, potY - 1, s, c);
    }

    // Walls: \                     /
    putCell(left + 1, potY + 1, '\\', COL_POT_OUTER);
    for (int x = left + 2; x <= right - 2; ++x) {
        putCell(x, potY + 1, ' ', COL_POT_INNER);
    }
    putCell(right - 1, potY + 1, '/', COL_POT_OUTER);

    // Bottom contour: \___________________/
    putCell(left + 2, potY + 2, '\\', COL_POT_OUTER);
    for (int x = left + 3; x <= right - 3; ++x) {
        putCell(x, potY + 2, '_', COL_POT_OUTER);
    }
    putCell(right - 2, potY + 2, '/', COL_POT_OUTER);

    // Feet:   |_______________|
    putCell(left + 4, potY + 3, '|', COL_POT_OUTER);
    for (int x = left + 5; x <= right - 5; ++x) {
        putCell(x, potY + 3, '_', COL_POT_OUTER);
    }
    putCell(right - 4, potY + 3, '|', COL_POT_OUTER);
}

static void AddFoliageCluster(BonsaiState& s, int cx, int cy, int radius) {
    int cols = s.widthInChars;
    int rows = s.heightInChars;
    static const char LEAF_CHARS[] = { '&', '~', '*', '%', '@' };

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius * 2; dx <= radius * 2; ++dx) {
            float dist = (float)(dx * dx) / 3.0f + (float)(dy * dy);
            if (dist <= (float)(radius * radius) + 0.5f) {
                if (rand() % 100 < 80) {
                    int x = cx + dx;
                    int y = cy + dy;
                    if (x >= 0 && x < cols && y >= 0 && y < rows) {
                        // Don't overwrite wood trunk if it's already there
                        char cur = s.grid[y * cols + x].ch;
                        if (cur == ' ' || cur == '~' || cur == '*' || cur == '&' || cur == '%') {
                            char ch = LEAF_CHARS[rand() % 5];
                            s.grid[y * cols + x].ch = ch;
                            s.grid[y * cols + x].color = GetFoliageColor(s.theme);
                            POINT pt = { x, y };
                            s.leaves.push_back(pt);
                        }
                    }
                }
            }
        }
    }
}

static void StartNewBonsai(BonsaiState& s, int cols, int rows) {
    s.widthInChars = cols;
    s.heightInChars = rows;
    s.grid.assign(cols * rows, BonsaiCell());
    s.activeShoots.clear();
    s.petals.clear();
    s.leaves.clear();

    // Random theme: 0=Spring, 1=Sakura, 2=Autumn, 3=Ginkgo
    s.theme = rand() % 4;

    int potY = rows - 6;
    int potHalfW = (std::min)(22, cols / 4);
    if (potHalfW < 12) potHalfW = 12;

    DrawPot(s, cols, rows, potY, potHalfW);

    // Root shoot at base of trunk
    BonsaiShoot root;
    root.x = (float)(cols / 2);
    root.y = (float)(potY - 2);
    root.dx = ((float)(rand() % 20) - 10.0f) / 50.0f; // slight natural lean
    root.dy = -1.0f;
    root.age = 0;
    root.maxAge = (int)(rows * 0.35f + (rand() % 5));
    root.generation = 0;
    root.thickness = 3;
    s.activeShoots.push_back(root);

    s.state = 0; // Growing
    s.stateStartTime = GetTickCount();
    s.lastTick = GetTickCount();
    s.initialized = true;
}

static void StepGrowth(BonsaiState& s) {
    if (s.activeShoots.empty()) {
        // Growth finished, transition to mature serene phase
        s.state = 1; // Mature / Petals
        s.stateStartTime = GetTickCount();
        return;
    }

    int cols = s.widthInChars;
    int rows = s.heightInChars;
    std::vector<BonsaiShoot> nextShoots;

    auto putCell = [&](int x, int y, char c, COLORREF col) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            s.grid[y * cols + x].ch = c;
            s.grid[y * cols + x].color = col;
        }
    };

    for (auto& shoot : s.activeShoots) {
        // Draw wood at current position
        int ix = (int)roundf(shoot.x);
        int iy = (int)roundf(shoot.y);

        COLORREF woodCol = WOOD_COLORS[(std::min)(shoot.generation, 3)];

        // Choose character based on slope
        char woodChar = '|';
        if (shoot.dx > 0.4f)       woodChar = '\\';
        else if (shoot.dx < -0.4f) woodChar = '/';
        else if (shoot.age % 4 == 0) woodChar = (shoot.dx >= 0.0f) ? ')' : '(';

        putCell(ix, iy, woodChar, woodCol);

        // Add thickness for trunk and early boughs
        if (shoot.thickness >= 3) {
            putCell(ix - 1, iy, '/', woodCol);
            putCell(ix + 1, iy, '\\', woodCol);
        } else if (shoot.thickness == 2) {
            putCell(ix + ((shoot.dx >= 0) ? 1 : -1), iy, woodChar, woodCol);
        }

        shoot.age++;
        shoot.x += shoot.dx;
        shoot.y += shoot.dy;

        // Organic curve perturbation
        shoot.dx += ((float)(rand() % 41) - 20.0f) / 100.0f;
        // Dampen excessive horizontal speed
        if (shoot.dx > 1.4f) shoot.dx = 1.4f;
        if (shoot.dx < -1.4f) shoot.dx = -1.4f;

        // Terminal condition or branching
        if (shoot.age >= shoot.maxAge || shoot.y <= 4.0f) {
            // Bloom foliage at termination
            int radius = (shoot.generation <= 1) ? 3 : 2;
            AddFoliageCluster(s, ix, iy, radius);
        } else {
            // Bifurcation / Branching probability
            bool shouldBranch = (shoot.age > 4 && rand() % 100 < (22 - shoot.generation * 4));
            if (shouldBranch && shoot.generation < 3) {
                // Spawn two child branches
                BonsaiShoot b1 = shoot;
                b1.generation = shoot.generation + 1;
                b1.thickness = (std::max)(1, shoot.thickness - 1);
                b1.age = 0;
                b1.maxAge = (int)(shoot.maxAge * 0.7f + (rand() % 4));
                b1.dx = shoot.dx - (0.5f + (float)(rand() % 30) / 100.0f);
                b1.dy = shoot.dy * 0.85f;
                nextShoots.push_back(b1);

                BonsaiShoot b2 = shoot;
                b2.generation = shoot.generation + 1;
                b2.thickness = (std::max)(1, shoot.thickness - 1);
                b2.age = 0;
                b2.maxAge = (int)(shoot.maxAge * 0.7f + (rand() % 4));
                b2.dx = shoot.dx + (0.5f + (float)(rand() % 30) / 100.0f);
                b2.dy = shoot.dy * 0.85f;
                nextShoots.push_back(b2);

                // Add foliage node at bifurcation
                if (rand() % 2 == 0) {
                    AddFoliageCluster(s, ix, iy, 1);
                }
            } else {
                nextShoots.push_back(shoot);
            }
        }
    }

    s.activeShoots = nextShoots;
}

void RenderBonsai(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& s = data->GetCustomState<BonsaiState>(25);

    SelectObject(memDC, data->hFont);
    SetBkMode(memDC, OPAQUE);
    SetBkColor(memDC, RGB(0, 0, 0));

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cW = tm.tmAveCharWidth;
    int cH = tm.tmHeight;
    if (cW <= 0) cW = 8;
    if (cH <= 0) cH = 16;

    int cols = width / cW;
    int rows = height / cH;
    if (cols < 24) cols = 24;
    if (rows < 15) rows = 15;

    if (!s.initialized || s.widthInChars != cols || s.heightInChars != rows) {
        StartNewBonsai(s, cols, rows);
    }

    DWORD now = GetTickCount();
    float dt = (now - s.lastTick) / 1000.0f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.033f;
    s.lastTick = now;

    // Fill screen with black
    FillRect(memDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Growth / Lifecycle State Machine
    if (s.state == 0) {
        // Growth phase: advance 1-2 steps per frame for organic pacing
        StepGrowth(s);
        if (rand() % 2 == 0) StepGrowth(s);
    } else if (s.state == 1) {
        // Mature serene phase: gentle foliage rustle & drifting petals/leaves
        DWORD elapsed = now - s.stateStartTime;

        // Subtle foliage shimmer / rustle in the breeze
        if (!s.leaves.empty() && (rand() % 3 == 0)) {
            int flutterCount = 1 + (rand() % 3);
            for (int k = 0; k < flutterCount; ++k) {
                const auto& pt = s.leaves[rand() % s.leaves.size()];
                int idx = pt.y * cols + pt.x;
                char cur = s.grid[idx].ch;
                if (cur != ' ' && cur != '|' && cur != '/' && cur != '\\' && cur != '(' && cur != ')' && cur != '_') {
                    static const char FLUTTER_CHARS[] = { '&', '~', '*', '%', '@' };
                    s.grid[idx].ch = FLUTTER_CHARS[rand() % 5];
                }
            }
        }

        // Spawn falling petals strictly from actual foliage clusters on the tree
        if (!s.leaves.empty() && (rand() % 3 == 0)) {
            const auto& leaf = s.leaves[rand() % s.leaves.size()];
            BonsaiPetal p;
            p.x = (float)leaf.x;
            p.y = (float)leaf.y;
            p.vx = 0.20f + (float)(rand() % 25) / 100.0f;
            p.vy = 0.20f + (float)(rand() % 20) / 100.0f;
            p.phase = (float)(rand() % 628) / 100.0f;
            static const char PETAL_CHARS[] = { '*', '.', '~', '+' };
            p.ch = PETAL_CHARS[rand() % 4];
            p.color = s.grid[leaf.y * cols + leaf.x].color;
            s.petals.push_back(p);
        }

        // Serene viewing duration: 18 seconds before renewal
        if (elapsed > 18000) {
            s.state = 2; // Renewal
            s.stateStartTime = now;
        }
    } else if (s.state == 2) {
        // Renewal: gentle fade transition, then sprout a new tree
        DWORD elapsed = now - s.stateStartTime;
        if (elapsed > 1800) {
            StartNewBonsai(s, cols, rows);
        }
    }

    // Update Petals
    for (size_t i = 0; i < s.petals.size(); ) {
        auto& p = s.petals[i];
        p.phase += dt * 2.0f;
        p.x += (p.vx + sinf(p.phase) * 0.4f) * (dt * 30.0f);
        p.y += p.vy * (dt * 30.0f);

        if (p.y >= (float)(rows - 2) || p.x >= (float)cols || p.x < 0) {
            s.petals.erase(s.petals.begin() + i);
        } else {
            ++i;
        }
    }

    // Render tree grid with span batching
    for (int y = 0; y < rows; ++y) {
        int x = 0;
        while (x < cols) {
            if (s.grid[y * cols + x].ch == ' ') {
                x++;
                continue;
            }

            int startX = x;
            COLORREF col = s.grid[y * cols + x].color;
            char span[512];
            int spanLen = 0;

            while (x < cols && s.grid[y * cols + x].ch != ' ' &&
                   s.grid[y * cols + x].color == col && spanLen < 500) {
                span[spanLen++] = s.grid[y * cols + x].ch;
                x++;
            }
            span[spanLen] = '\0';

            SetTextColor(memDC, col);
            TextOutA(memDC, startX * cW, y * cH, span, spanLen);
        }
    }

    // Render drifting petals on top
    for (const auto& p : s.petals) {
        int px = (int)p.x;
        int py = (int)p.y;
        if (px >= 0 && px < cols && py >= 0 && py < rows) {
            SetTextColor(memDC, p.color);
            TextOutA(memDC, px * cW, py * cH, &p.ch, 1);
        }
    }
}

REGISTER_SCREENSAVER(
    25,
    L"cbonsai (Bonsai Tree)",
    "cbonsai",
    { "cbonsai", "bonsai", "tree" },
    WRAP_LEGACY(RenderBonsai),
    {}
);

