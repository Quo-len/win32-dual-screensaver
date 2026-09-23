#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <random>
#include "settings/TetrisSettings.h"

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic Retro Arcade / 1989 Tetris Simulation
// Virtual height is fixed at 288 px; virtual width adapts dynamically to aspect ratio.
// ============================================================================
static constexpr int BASE_TETRIS_H = 288;
static constexpr int BOARD_COLS    = 10;
static constexpr int BOARD_ROWS    = 20;
static constexpr int BLOCK_SIZE    = 12; // 12x12 pixels per block (120x240 playfield)

// 32-bit ARGB Palette
static constexpr uint32_t TET_COL_BLACK       = 0xFF050508;
static constexpr uint32_t TET_COL_BG_DOT      = 0xFF14141E;
static constexpr uint32_t TET_COL_BOARD_BG    = 0xFF0B0B10;
static constexpr uint32_t TET_COL_BOARD_GRID  = 0xFF161622;
static constexpr uint32_t TET_COL_WHITE       = 0xFFFFFFFF;
static constexpr uint32_t TET_COL_GRAY_LIGHT  = 0xFFB0B0C0;
static constexpr uint32_t TET_COL_GRAY_DARK   = 0xFF353545;
static constexpr uint32_t TET_COL_BORDER_BLUE = 0xFF2A428C;
static constexpr uint32_t TET_COL_BORDER_CYAN = 0xFF4DEEEA;
static constexpr uint32_t TET_COL_TEXT_TITLE  = 0xFFFFD700; // Gold
static constexpr uint32_t TET_COL_TEXT_LABEL  = 0xFF9E9EB8; // Soft metallic
static constexpr uint32_t TET_COL_TEXT_RED    = 0xFFFF3B30;

// Tetromino Palette: [Type 1..7]
// 1=I(Cyan), 2=J(Blue), 3=L(Orange), 4=O(Yellow), 5=S(Green), 6=T(Purple), 7=Z(Red)
static constexpr uint32_t TET_COLORS[8] = {
    0xFF000000, // 0: None
    0xFF00E5FF, // 1: I - Cyan
    0xFF2979FF, // 2: J - Deep Blue
    0xFFFF9100, // 3: L - Bright Orange
    0xFFFFEA00, // 4: O - Vivid Yellow
    0xFF00E676, // 5: S - Neon Green
    0xFFD500F9, // 6: T - Electric Purple
    0xFFFF1744  // 7: Z - Crimson Red
};

// ============================================================================
// 5x7 RETRO BITMAP FONT
// ============================================================================
static const uint8_t* GetTetrisGlyph(char c) {
    static const uint8_t BLANK[7] = { 0 };
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
    case 'G': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0F }; return g; }
    case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
    case 'I': { static const uint8_t g[7] = { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
    case 'J': { static const uint8_t g[7] = { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E }; return g; }
    case 'K': { static const uint8_t g[7] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }; return g; }
    case 'L': { static const uint8_t g[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return g; }
    case 'M': { static const uint8_t g[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return g; }
    case 'N': { static const uint8_t g[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return g; }
    case 'O': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
    case 'P': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return g; }
    case 'Q': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x09, 0x16 }; return g; }
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
    case '+': { static const uint8_t g[7] = { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    case ':': { static const uint8_t g[7] = { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }; return g; }
    case '%': { static const uint8_t g[7] = { 0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13 }; return g; }
    case '/': { static const uint8_t g[7] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 }; return g; }
    case '.': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C }; return g; }
    case '[': { static const uint8_t g[7] = { 0x1E, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1E }; return g; }
    case ']': { static const uint8_t g[7] = { 0x0F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x0F }; return g; }
    default:  return BLANK;
    }
}

// ============================================================================
// TETROMINO SHAPES & ROTATIONS (SRS Standard 4x4 Grid)
// ============================================================================
struct BlockCoord { int x; int y; };

static const BlockCoord TETROMINO_SHAPES[8][4][4] = {
    // 0: None
    { { {0,0},{0,0},{0,0},{0,0} } },

    // 1: I (Cyan)
    {
        { {0,1}, {1,1}, {2,1}, {3,1} }, // 0 deg
        { {2,0}, {2,1}, {2,2}, {2,3} }, // 90 deg
        { {0,2}, {1,2}, {2,2}, {3,2} }, // 180 deg
        { {1,0}, {1,1}, {1,2}, {1,3} }  // 270 deg
    },

    // 2: J (Blue)
    {
        { {0,0}, {0,1}, {1,1}, {2,1} },
        { {1,0}, {2,0}, {1,1}, {1,2} },
        { {0,1}, {1,1}, {2,1}, {2,2} },
        { {1,0}, {1,1}, {0,2}, {1,2} }
    },

    // 3: L (Orange)
    {
        { {2,0}, {0,1}, {1,1}, {2,1} },
        { {1,0}, {1,1}, {1,2}, {2,2} },
        { {0,1}, {1,1}, {2,1}, {0,2} },
        { {0,0}, {1,0}, {1,1}, {1,2} }
    },

    // 4: O (Yellow)
    {
        { {1,0}, {2,0}, {1,1}, {2,1} },
        { {1,0}, {2,0}, {1,1}, {2,1} },
        { {1,0}, {2,0}, {1,1}, {2,1} },
        { {1,0}, {2,0}, {1,1}, {2,1} }
    },

    // 5: S (Green)
    {
        { {1,0}, {2,0}, {0,1}, {1,1} },
        { {1,0}, {1,1}, {2,1}, {2,2} },
        { {1,1}, {2,1}, {0,2}, {1,2} },
        { {0,0}, {0,1}, {1,1}, {1,2} }
    },

    // 6: T (Purple)
    {
        { {1,0}, {0,1}, {1,1}, {2,1} },
        { {1,0}, {1,1}, {2,1}, {1,2} },
        { {0,1}, {1,1}, {2,1}, {1,2} },
        { {1,0}, {0,1}, {1,1}, {1,2} }
    },

    // 7: Z (Red)
    {
        { {0,0}, {1,0}, {1,1}, {2,1} },
        { {2,0}, {1,1}, {2,1}, {1,2} },
        { {0,1}, {1,1}, {1,2}, {2,2} },
        { {1,0}, {0,1}, {1,1}, {0,2} }
    }
};

// ============================================================================
// SIMULATION STATE
// ============================================================================
struct FloatingPopup {
    char text[24];
    float x;
    float y;
    int timer;
    uint32_t color;
};

struct TetrisState {
    bool initialized = false;
    uint64_t lastTick = 0;

    int virtualW = 512;
    int virtualH = BASE_TETRIS_H;
    std::vector<uint32_t> fb;

    // Board matrix: 0=empty, 1..7=tetromino block color
    uint8_t board[BOARD_ROWS][BOARD_COLS] = { 0 };

    // Active falling piece
    int pieceType = 1;
    int pieceRot = 0;
    int pieceX = 3;
    float pieceY = 0.0f;

    // Next piece
    int nextPieceType = 1;

    // AI Planned Destination
    int targetX = 3;
    int targetRot = 0;
    bool aiPlanReady = false;

    // 7-Bag Randomizer
    std::vector<int> bag;
    std::mt19937 rng;

    // Scoring & Stats
    int score = 0;
    int highScore = 85240;
    int linesCleared = 0;
    int level = 1;
    int pieceCounts[8] = { 0 }; // Statistics for pieces 1..7

    // Line Clear Animation State
    bool clearingLines = false;
    int clearTimer = 0;
    int linesToClear[4] = { -1, -1, -1, -1 };
    int linesToClearCount = 0;

    // Game Over Curtain State
    bool gameOver = false;
    int gameOverRow = BOARD_ROWS - 1;
    int gameOverTimer = 0;

    // Popups
    std::vector<FloatingPopup> popups;

    // Timers & speed
    float dropSpeed = 0.035f; // cells per tick (relaxed starter speed)
    int moveDelay = 0;

    // Clears Breakdown
    int singleClears = 0;
    int doubleClears = 0;
    int tripleClears = 0;
    int tetrisClears = 0;

    int frameTick = 0;
};

// ============================================================================
// PIXEL HELPERS
// ============================================================================
static inline void TetPutPixel(TetrisState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void TetDrawRect(TetrisState& s, int rx, int ry, int rw, int rh, uint32_t color) {
    for (int y = ry; y < ry + rh; ++y) {
        for (int x = rx; x < rx + rw; ++x) {
            TetPutPixel(s, x, y, color);
        }
    }
}

static void TetDrawText(TetrisState& s, int x, int y, const char* str, uint32_t color, int scale = 1) {
    if (scale <= 1) {
        int curX = x;
        while (*str) {
            if (*str != ' ') {
                const uint8_t* glyph = GetTetrisGlyph(*str);
                for (int r = 0; r < 7; ++r) {
                    uint8_t rowBits = glyph[r];
                    for (int c = 0; c < 5; ++c) {
                        if ((rowBits >> (4 - c)) & 1) {
                            TetPutPixel(s, curX + c, y + r, color);
                        }
                    }
                }
            }
            curX += 6;
            str++;
        }
    } else {
        int curX = x;
        while (*str) {
            if (*str != ' ') {
                const uint8_t* glyph = GetTetrisGlyph(*str);
                for (int r = 0; r < 7; ++r) {
                    uint8_t rowBits = glyph[r];
                    for (int c = 0; c < 5; ++c) {
                        if ((rowBits >> (4 - c)) & 1) {
                            for (int dy = 0; dy < scale; ++dy) {
                                for (int dx = 0; dx < scale; ++dx) {
                                    TetPutPixel(s, curX + c * scale + dx, y + r * scale + dy, color);
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
}

// Draw framed retro arcade card/panel with bevel border and title ribbon
static void TetDrawPanel(TetrisState& s, int x, int y, int w, int h, const char* title = nullptr, uint32_t headerCol = TET_COL_BORDER_CYAN) {
    if (w <= 0 || h <= 0) return;
    // Dark slate arcade backing
    TetDrawRect(s, x, y, w, h, 0xFF0B1020);
    // Outer border
    TetDrawRect(s, x, y, w, 1, TET_COL_BORDER_BLUE);
    TetDrawRect(s, x, y + h - 1, w, 1, TET_COL_BORDER_BLUE);
    TetDrawRect(s, x, y, 1, h, TET_COL_BORDER_BLUE);
    TetDrawRect(s, x + w - 1, y, 1, h, TET_COL_BORDER_BLUE);
    // Inner border
    TetDrawRect(s, x + 2, y + 2, w - 4, 1, 0xFF1C2848);
    TetDrawRect(s, x + 2, y + h - 3, w - 4, 1, 0xFF1C2848);
    TetDrawRect(s, x + 2, y + 2, 1, h - 4, 0xFF1C2848);
    TetDrawRect(s, x + w - 3, y + 2, 1, h - 4, 0xFF1C2848);

    if (title && title[0]) {
        // Title header ribbon
        TetDrawRect(s, x + 3, y + 3, w - 6, 11, 0xFF141F3C);
        int titleLen = (int)strlen(title);
        int titleW = titleLen * 6 - 1;
        int titleX = x + (w - titleW) / 2;
        if (titleX < x + 4) titleX = x + 4;
        TetDrawText(s, titleX, y + 5, title, headerCol);
    }
}

// Draw authentic 3D beveled retro arcade block (12x12)
static void DrawBlock(TetrisState& s, int px, int py, int type, bool isGhost = false) {
    if (type <= 0 || type > 7) return;

    uint32_t baseCol = TET_COLORS[type];

    if (isGhost) {
        // Subtle dotted wireframe outline for ghost piece
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            if (i % 2 == 0) {
                TetPutPixel(s, px + i, py, baseCol);
                TetPutPixel(s, px + i, py + BLOCK_SIZE - 1, baseCol);
                TetPutPixel(s, px, py + i, baseCol);
                TetPutPixel(s, px + BLOCK_SIZE - 1, py + i, baseCol);
            }
        }
        return;
    }

    // Color derivation: Highlight (top/left) and Shadow (bottom/right)
    uint32_t r = (baseCol >> 16) & 0xFF;
    uint32_t g = (baseCol >> 8) & 0xFF;
    uint32_t b = baseCol & 0xFF;

    uint32_t hiR = (std::min)(255, (int)(r * 1.55f));
    uint32_t hiG = (std::min)(255, (int)(g * 1.55f));
    uint32_t hiB = (std::min)(255, (int)(b * 1.55f));
    uint32_t hiCol = 0xFF000000 | (hiR << 16) | (hiG << 8) | hiB;

    uint32_t shR = (uint32_t)(r * 0.45f);
    uint32_t shG = (uint32_t)(g * 0.45f);
    uint32_t shB = (uint32_t)(b * 0.45f);
    uint32_t shCol = 0xFF000000 | (shR << 16) | (shG << 8) | shB;

    // Fill inner core
    TetDrawRect(s, px, py, BLOCK_SIZE, BLOCK_SIZE, baseCol);

    // Beveled edges
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        // Top edge
        TetPutPixel(s, px + i, py, hiCol);
        TetPutPixel(s, px + i, py + 1, hiCol);
        // Left edge
        TetPutPixel(s, px, py + i, hiCol);
        TetPutPixel(s, px + 1, py + i, hiCol);

        // Bottom edge
        TetPutPixel(s, px + i, py + BLOCK_SIZE - 1, shCol);
        TetPutPixel(s, px + i, py + BLOCK_SIZE - 2, shCol);
        // Right edge
        TetPutPixel(s, px + BLOCK_SIZE - 1, py + i, shCol);
        TetPutPixel(s, px + BLOCK_SIZE - 2, py + i, shCol);
    }

    // Authentic retro center glossy pip
    TetDrawRect(s, px + 3, py + 3, 3, 3, hiCol);
}

// ============================================================================
// TETRIS BOARD LOGIC & COLLISION
// ============================================================================
static bool CheckCollision(const uint8_t board[BOARD_ROWS][BOARD_COLS], int type, int rot, int bx, int by) {
    if (type <= 0 || type > 7) return false;
    for (int i = 0; i < 4; ++i) {
        int x = bx + TETROMINO_SHAPES[type][rot][i].x;
        int y = by + TETROMINO_SHAPES[type][rot][i].y;

        if (x < 0 || x >= BOARD_COLS || y >= BOARD_ROWS) return true;
        if (y >= 0 && board[y][x] != 0) return true;
    }
    return false;
}

static int GetHardDropY(const uint8_t board[BOARD_ROWS][BOARD_COLS], int type, int rot, int bx, int startY) {
    int curY = startY;
    while (!CheckCollision(board, type, rot, bx, curY + 1)) {
        curY++;
    }
    return curY;
}

// ============================================================================
// PIERRE DELLACHERIE HEURISTIC EVALUATION AI
// ============================================================================
// Weighted heuristic features that promote flat surfaces, zero buried cavities,
// fast line clears, and 4-line Tetris setup.
static double EvaluateBoardState(const uint8_t board[BOARD_ROWS][BOARD_COLS], int landingHeight, int linesCleared, int erodedPieceCells) {
    // 1. Landing Height penalty
    double score = -4.5001588 * (double)landingHeight;

    // 2. Eroded Piece Cells bonus (massive boost for clears, especially Tetris)
    score += 3.4181268 * (double)(linesCleared * erodedPieceCells);
    if (linesCleared == 4) score += 250.0; // Big bonus for 4-line TETRIS clear!

    // 3. Row Transitions (fewer transitions = denser, cleaner horizontal rows)
    int rowTransitions = 0;
    for (int y = 0; y < BOARD_ROWS; ++y) {
        int prev = 1; // Left boundary acts as filled
        for (int x = 0; x < BOARD_COLS; ++x) {
            int cur = (board[y][x] != 0) ? 1 : 0;
            if (cur != prev) rowTransitions++;
            prev = cur;
        }
        if (prev == 0) rowTransitions++; // Right boundary acts as filled
    }
    score -= 3.2178882 * (double)rowTransitions;

    // 4. Column Transitions (fewer transitions = smooth vertical profile)
    int colTransitions = 0;
    for (int x = 0; x < BOARD_COLS; ++x) {
        int prev = 0; // Top boundary acts as empty
        for (int y = 0; y < BOARD_ROWS; ++y) {
            int cur = (board[y][x] != 0) ? 1 : 0;
            if (cur != prev) colTransitions++;
            prev = cur;
        }
        if (prev == 0) colTransitions++; // Floor acts as filled
    }
    score -= 9.3486953 * (double)colTransitions;

    // 5. Number of Holes (empty cell covered by filled cells above)
    int holes = 0;
    int wellsSum = 0;
    for (int x = 0; x < BOARD_COLS; ++x) {
        bool blockAbove = false;
        int wellDepth = 0;
        for (int y = 0; y < BOARD_ROWS; ++y) {
            if (board[y][x] != 0) {
                blockAbove = true;
                wellDepth = 0;
            } else {
                if (blockAbove) holes++;

                // Well depth calculation
                bool leftWall = (x == 0) || (board[y][x - 1] != 0);
                bool rightWall = (x == BOARD_COLS - 1) || (board[y][x + 1] != 0);
                if (leftWall && rightWall) {
                    wellDepth++;
                    wellsSum += wellDepth;
                } else {
                    wellDepth = 0;
                }
            }
        }
    }
    score -= 7.899265 * (double)holes;
    score -= 3.3855972 * (double)wellsSum;

    return score;
}

// Find optimal (bestRot, bestX) placement using Dellacherie evaluation
static void ComputeBestMove(TetrisState& s) {
    double bestScore = -1e9;
    int bestX = s.pieceX;
    int bestRot = 0;

    for (int rot = 0; rot < 4; ++rot) {
        // Determine valid horizontal span for this rotation
        int minX = 0, maxX = BOARD_COLS - 1;
        for (int i = 0; i < 4; ++i) {
            int ox = TETROMINO_SHAPES[s.pieceType][rot][i].x;
            if (minX + ox < 0) minX = -ox;
            if (maxX + ox >= BOARD_COLS) maxX = BOARD_COLS - 1 - ox;
        }

        for (int x = minX; x <= maxX; ++x) {
            if (CheckCollision(s.board, s.pieceType, rot, x, 0)) continue;

            int dropY = GetHardDropY(s.board, s.pieceType, rot, x, 0);

            // Simulate placement in scratch board
            uint8_t simBoard[BOARD_ROWS][BOARD_COLS];
            memcpy(simBoard, s.board, sizeof(simBoard));

            for (int i = 0; i < 4; ++i) {
                int bx = x + TETROMINO_SHAPES[s.pieceType][rot][i].x;
                int by = dropY + TETROMINO_SHAPES[s.pieceType][rot][i].y;
                if (by >= 0 && by < BOARD_ROWS && bx >= 0 && bx < BOARD_COLS) {
                    simBoard[by][bx] = s.pieceType;
                }
            }

            // Simulate line clears and count eroded piece cells
            int linesCleared = 0;
            int erodedCells = 0;
            for (int y = BOARD_ROWS - 1; y >= 0; --y) {
                bool full = true;
                for (int c = 0; c < BOARD_COLS; ++c) {
                    if (simBoard[y][c] == 0) { full = false; break; }
                }
                if (full) {
                    linesCleared++;
                    for (int i = 0; i < 4; ++i) {
                        int by = dropY + TETROMINO_SHAPES[s.pieceType][rot][i].y;
                        if (by == y) erodedCells++;
                    }
                    // Collapse board down
                    for (int row = y; row > 0; --row) {
                        memcpy(simBoard[row], simBoard[row - 1], BOARD_COLS);
                    }
                    memset(simBoard[0], 0, BOARD_COLS);
                    y++; // Recheck same row index after collapse
                }
            }

            int landingHeight = BOARD_ROWS - dropY;
            double score = EvaluateBoardState(simBoard, landingHeight, linesCleared, erodedCells);

            if (score > bestScore) {
                bestScore = score;
                bestX = x;
                bestRot = rot;
            }
        }
    }

    s.targetX = bestX;
    s.targetRot = bestRot;
    s.aiPlanReady = true;
}

// 7-Bag Randomizer
static int DrawNextPiece(TetrisState& s) {
    if (s.bag.empty()) {
        s.bag = { 1, 2, 3, 4, 5, 6, 7 };
        std::shuffle(s.bag.begin(), s.bag.end(), s.rng);
    }
    int p = s.bag.back();
    s.bag.pop_back();
    return p;
}

static void SpawnPiece(TetrisState& s) {
    s.pieceType = s.nextPieceType;
    s.nextPieceType = DrawNextPiece(s);
    s.pieceRot = 0;
    s.pieceX = 3;
    s.pieceY = 0.0f;
    s.aiPlanReady = false;
    s.pieceCounts[s.pieceType]++;

    // Check immediate top-out game over
    if (CheckCollision(s.board, s.pieceType, s.pieceRot, s.pieceX, (int)s.pieceY)) {
        s.gameOver = true;
        s.gameOverRow = BOARD_ROWS - 1;
        s.gameOverTimer = 0;
        return;
    }

    // Let AI compute destination
    ComputeBestMove(s);
}

static void ResetTetrisGame(TetrisState& s) {
    memset(s.board, 0, sizeof(s.board));
    s.score = 0;
    s.linesCleared = 0;
    s.level = 1;
    s.clearingLines = false;
    s.clearTimer = 0;
    s.gameOver = false;
    s.popups.clear();
    memset(s.pieceCounts, 0, sizeof(s.pieceCounts));

    s.singleClears = 0;
    s.doubleClears = 0;
    s.tripleClears = 0;
    s.tetrisClears = 0;

    std::random_device rd;
    s.rng.seed(rd());
    s.bag.clear();

    s.nextPieceType = DrawNextPiece(s);
    SpawnPiece(s);
}

// ============================================================================
// SIMULATION UPDATE
// ============================================================================
static void UpdateTetrisGame(TetrisState& s) {
    s.frameTick++;

    // 1. Handle Game Over curtain animation
    if (s.gameOver) {
        s.gameOverTimer++;
        if (s.gameOverTimer >= 3) {
            s.gameOverTimer = 0;
            if (s.gameOverRow >= 0) {
                // Fill row with gray blocks
                for (int x = 0; x < BOARD_COLS; ++x) s.board[s.gameOverRow][x] = 8; // Gray
                s.gameOverRow--;
            } else {
                // Restart clean game
                ResetTetrisGame(s);
            }
        }
        return;
    }

    // 2. Handle Line Clear Flash Animation
    if (s.clearingLines) {
        s.clearTimer--;
        if (s.clearTimer <= 0) {
            s.clearingLines = false;

            // Collapse rows
            for (int i = 0; i < s.linesToClearCount; ++i) {
                int clearRow = s.linesToClear[i];
                for (int r = clearRow; r > 0; --r) {
                    memcpy(s.board[r], s.board[r - 1], BOARD_COLS);
                }
                memset(s.board[0], 0, BOARD_COLS);
            }

            s.linesToClearCount = 0;
            SpawnPiece(s);
        }
        return;
    }

    // 3. Update Floating Popups
    for (size_t i = 0; i < s.popups.size();) {
        s.popups[i].timer--;
        s.popups[i].y -= 0.6f;
        if (s.popups[i].timer <= 0) {
            s.popups.erase(s.popups.begin() + i);
        } else {
            ++i;
        }
    }

    if (!s.aiPlanReady) ComputeBestMove(s);

    // 4. Autonomous Maneuvering: Rotate & Slide toward AI target
    s.moveDelay++;
    if (s.moveDelay >= 4) {
        s.moveDelay = 0;

        // Rotate piece
        if (s.pieceRot != s.targetRot) {
            int nextRot = (s.pieceRot + 1) % 4;
            if (!CheckCollision(s.board, s.pieceType, nextRot, s.pieceX, (int)s.pieceY)) {
                s.pieceRot = nextRot;
            }
        }

        // Slide horizontally
        if (s.pieceX < s.targetX) {
            if (!CheckCollision(s.board, s.pieceType, s.pieceRot, s.pieceX + 1, (int)s.pieceY)) {
                s.pieceX++;
            }
        } else if (s.pieceX > s.targetX) {
            if (!CheckCollision(s.board, s.pieceType, s.pieceRot, s.pieceX - 1, (int)s.pieceY)) {
                s.pieceX--;
            }
        }
    }

    // 5. Piece Falling / Drop
    // Starter speed is loaded from Settings; progression and capped speed auto-scale proportionally
    float initialSpeed = (g_TetrisInitialSpeed > 0.001f) ? g_TetrisInitialSpeed : 0.035f;
    s.dropSpeed = initialSpeed;

    int levelStep = (std::min)(s.level - 1, 15);
    // Base progression auto-scales so level 16 reaches ~6.14x the initial speed
    float baseSpeed = initialSpeed * (1.0f + levelStep * 0.34286f);
    float speed = baseSpeed;

    // Once aligned with AI target column & rotation, smoothly accelerate drop (~2.57x base, reaching ~15.4x initial speed at cap)
    if (s.pieceX == s.targetX && s.pieceRot == s.targetRot) {
        float alignedInitial = initialSpeed * 2.5714f;
        speed = alignedInitial * (1.0f + levelStep * 0.33333f);
    }

    // Safety ceiling: prevent tunneling through cells in a single frame
    speed = (std::min)(speed, 0.95f);

    s.pieceY += speed;

    // Check floor / block collision
    if (CheckCollision(s.board, s.pieceType, s.pieceRot, s.pieceX, (int)std::floor(s.pieceY) + 1)) {
        int lockY = (int)std::floor(s.pieceY);

        // Lock piece into board
        for (int i = 0; i < 4; ++i) {
            int bx = s.pieceX + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].x;
            int by = lockY + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].y;
            if (by >= 0 && by < BOARD_ROWS && bx >= 0 && bx < BOARD_COLS) {
                s.board[by][bx] = s.pieceType;
            }
        }

        // Check for completed lines
        s.linesToClearCount = 0;
        for (int y = 0; y < BOARD_ROWS; ++y) {
            bool full = true;
            for (int x = 0; x < BOARD_COLS; ++x) {
                if (s.board[y][x] == 0) { full = false; break; }
            }
            if (full) {
                s.linesToClear[s.linesToClearCount++] = y;
            }
        }

        if (s.linesToClearCount > 0) {
            s.clearingLines = true;
            s.clearTimer = 16; // Flash duration (~0.27s pause to appreciate line clear)

            s.linesCleared += s.linesToClearCount;
            s.level = 1 + (s.linesCleared / 10);

            // Scoring: 100, 300, 500, 800 * level
            int pts = 0;
            const char* label = "LINE!";
            uint32_t popCol = TET_COL_WHITE;

            switch (s.linesToClearCount) {
            case 1: s.singleClears++; pts = 100 * s.level; label = "SINGLE +100"; popCol = TET_COL_WHITE; break;
            case 2: s.doubleClears++; pts = 300 * s.level; label = "DOUBLE +300"; popCol = 0xFF4DEEEA; break;
            case 3: s.tripleClears++; pts = 500 * s.level; label = "TRIPLE +500"; popCol = 0xFFFFEA00; break;
            case 4: s.tetrisClears++; pts = 800 * s.level; label = "TETRIS!! +800"; popCol = 0xFFFFD700; break;
            }

            s.score += pts;
            if (s.score > s.highScore) s.highScore = s.score;

            // Spawn floating text
            int boardX = (s.virtualW - BOARD_COLS * BLOCK_SIZE) / 2;
            FloatingPopup p;
            strcpy_s(p.text, label);
            p.x = (float)(boardX + 16);
            p.y = (float)(24 + s.linesToClear[0] * BLOCK_SIZE);
            p.timer = 40;
            p.color = popCol;
            s.popups.push_back(p);
        } else {
            SpawnPiece(s);
        }
    }
}

// ============================================================================
// FRAMEBUFFER RENDERER
// ============================================================================
static void RenderTetrisFrame(TetrisState& s) {
    // 1. Dark Space Background
    std::fill(s.fb.begin(), s.fb.end(), TET_COL_BLACK);

    // Subtle background grid stars/dots
    for (int y = 8; y < s.virtualH; y += 16) {
        for (int x = 8; x < s.virtualW; x += 16) {
            TetPutPixel(s, x, y, TET_COL_BG_DOT);
        }
    }

    int boardW = BOARD_COLS * BLOCK_SIZE;
    int boardH = BOARD_ROWS * BLOCK_SIZE;
    int boardX = (s.virtualW - boardW) / 2;
    int boardY = (s.virtualH - boardH) / 2; // = 24

    // 2. Playfield Frame & Bezel
    // Outer shadow & metallic blue bevel
    TetDrawRect(s, boardX - 5, boardY - 5, boardW + 10, boardH + 10, TET_COL_BORDER_BLUE);
    TetDrawRect(s, boardX - 3, boardY - 3, boardW + 6, boardH + 6, TET_COL_BORDER_CYAN);
    TetDrawRect(s, boardX - 2, boardY - 2, boardW + 4, boardH + 4, 0xFF182245);
    TetDrawRect(s, boardX, boardY, boardW, boardH, TET_COL_BOARD_BG);

    // Inner subtle playfield grid lines
    for (int r = 1; r < BOARD_ROWS; ++r) {
        for (int c = 0; c < boardW; ++c) {
            TetPutPixel(s, boardX + c, boardY + r * BLOCK_SIZE, TET_COL_BOARD_GRID);
        }
    }
    for (int c = 1; c < BOARD_COLS; ++c) {
        for (int r = 0; r < boardH; ++r) {
            TetPutPixel(s, boardX + c * BLOCK_SIZE, boardY + r, TET_COL_BOARD_GRID);
        }
    }

    // 3. Draw Locked Blocks on Board
    for (int r = 0; r < BOARD_ROWS; ++r) {
        // Flash completed lines bright white
        bool isClearingRow = false;
        if (s.clearingLines) {
            for (int i = 0; i < s.linesToClearCount; ++i) {
                if (s.linesToClear[i] == r) { isClearingRow = true; break; }
            }
        }

        for (int c = 0; c < BOARD_COLS; ++c) {
            int type = s.board[r][c];
            if (type > 0) {
                int bx = boardX + c * BLOCK_SIZE;
                int by = boardY + r * BLOCK_SIZE;
                if (isClearingRow && (s.clearTimer % 2 == 0)) {
                    TetDrawRect(s, bx, by, BLOCK_SIZE, BLOCK_SIZE, TET_COL_WHITE);
                } else if (type == 8) {
                    DrawBlock(s, bx, by, 7); // Gray/Red game over curtain
                } else {
                    DrawBlock(s, bx, by, type);
                }
            }
        }
    }

    // 4. Draw Ghost Piece & Active Falling Piece (Only when not in line clear)
    if (!s.clearingLines && !s.gameOver) {
        // Ghost Piece Projection
        int ghostY = GetHardDropY(s.board, s.pieceType, s.pieceRot, s.pieceX, (int)std::floor(s.pieceY));
        for (int i = 0; i < 4; ++i) {
            int gx = s.pieceX + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].x;
            int gy = ghostY + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].y;
            if (gy >= 0 && gy < BOARD_ROWS && gx >= 0 && gx < BOARD_COLS) {
                DrawBlock(s, boardX + gx * BLOCK_SIZE, boardY + gy * BLOCK_SIZE, s.pieceType, true);
            }
        }

        // Active Falling Piece
        int curIntY = (int)std::floor(s.pieceY);
        for (int i = 0; i < 4; ++i) {
            int bx = s.pieceX + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].x;
            int by = curIntY + TETROMINO_SHAPES[s.pieceType][s.pieceRot][i].y;
            if (by >= 0 && by < BOARD_ROWS && bx >= 0 && bx < BOARD_COLS) {
                DrawBlock(s, boardX + bx * BLOCK_SIZE, boardY + by * BLOCK_SIZE, s.pieceType, false);
            }
        }
    }

    // 5. Left Side: Sized Arcade Statistics Panel (Enlarged & scaled for far viewing)
    int cardW = 104;
    // Playfield outer border is at boardX - 5. Place panel with a tight 6px gap.
    int panelLeftX = boardX - 11 - cardW;
    if (panelLeftX < 4) panelLeftX = 4;
    int panelLeftW = cardW;

    // Statistics Table: full playfield height 240 with large scale-2 digits
    TetDrawPanel(s, panelLeftX, boardY, panelLeftW, 240, "STATISTICS", 0xFF00E5FF);
    static const int STAT_PIECES[7] = { 6, 2, 7, 4, 5, 3, 1 }; // T, J, Z, O, S, L, I
    static const char* STAT_NAMES[7] = { "T", "J", "Z", "O", "S", "L", "I" };

    for (int pIdx = 0; pIdx < 7; ++pIdx) {
        int pType = STAT_PIECES[pIdx];
        int rowY = boardY + 18 + pIdx * 31;

        // Slot preview box (width 28, height 18)
        int slotX = panelLeftX + 6;
        int slotY = rowY + 3;
        TetDrawRect(s, slotX, slotY, 28, 18, 0xFF080D1A);
        TetDrawRect(s, slotX, slotY, 28, 1, 0xFF162038);
        TetDrawRect(s, slotX, slotY + 17, 28, 1, 0xFF162038);
        TetDrawRect(s, slotX, slotY, 1, 18, 0xFF162038);
        TetDrawRect(s, slotX + 27, slotY, 1, 18, 0xFF162038);

        // Compute bounding box of rotation 0 to perfectly center the mini-sprite inside slot
        int minX = 99, minY = 99, maxX = -1, maxY = -1;
        for (int i = 0; i < 4; ++i) {
            int bx = TETROMINO_SHAPES[pType][0][i].x;
            int by = TETROMINO_SHAPES[pType][0][i].y;
            if (bx < minX) minX = bx;
            if (by < minY) minY = by;
            if (bx > maxX) maxX = bx;
            if (by > maxY) maxY = by;
        }

        int sprW = (maxX - minX + 1) * 4;
        int sprH = (maxY - minY + 1) * 4;
        int offX = slotX + (28 - sprW) / 2;
        int offY = slotY + (18 - sprH) / 2;

        for (int i = 0; i < 4; ++i) {
            int bx = offX + (TETROMINO_SHAPES[pType][0][i].x - minX) * 4;
            int by = offY + (TETROMINO_SHAPES[pType][0][i].y - minY) * 4;
            TetDrawRect(s, bx, by, 4, 4, TET_COLORS[pType]);
            TetPutPixel(s, bx, by, TET_COL_WHITE);
        }

        // Piece letter (scale 2: 14px tall)
        TetDrawText(s, panelLeftX + 38, rowY + 5, STAT_NAMES[pIdx], TET_COLORS[pType], 2);

        // Piece count in red (scale 2: 14px tall, bold & highly visible from afar)
        char countBuf[16];
        sprintf_s(countBuf, "%03d", s.pieceCounts[pType]);
        TetDrawText(s, panelLeftX + 54, rowY + 5, countBuf, TET_COL_TEXT_RED, 2);
    }

    // Top Header Banner above board
    int topTitleX = boardX + (boardW - 100) / 2;
    TetDrawText(s, topTitleX, 8, "WORLD ARCADE '89", 0xFF64748B);

    // 6. Right Side: Sized Arcade Panels (Enlarged & scaled for far viewing)
    // Playfield outer border is at boardX + boardW + 5. Place panel with a tight 6px gap.
    int panelRightX = boardX + boardW + 11;
    int panelRightW = cardW;

    // A. NEXT Piece Box (Height 60, centered upcoming tetromino with chunky 10px blocks)
    TetDrawPanel(s, panelRightX, boardY, panelRightW, 60, "NEXT", 0xFFFFD700);
    if (s.nextPieceType >= 1 && s.nextPieceType <= 7) {
        int minX = 99, minY = 99, maxX = -1, maxY = -1;
        for (int i = 0; i < 4; ++i) {
            int bx = TETROMINO_SHAPES[s.nextPieceType][0][i].x;
            int by = TETROMINO_SHAPES[s.nextPieceType][0][i].y;
            if (bx < minX) minX = bx;
            if (by < minY) minY = by;
            if (bx > maxX) maxX = bx;
            if (by > maxY) maxY = by;
        }

        int blkStep = 10;
        int sprW = (maxX - minX + 1) * blkStep;
        int sprH = (maxY - minY + 1) * blkStep;
        int centerX = panelRightX + panelRightW / 2;
        int centerY = boardY + 36;
        int offX = centerX - sprW / 2;
        int offY = centerY - sprH / 2;

        for (int i = 0; i < 4; ++i) {
            int nx = offX + (TETROMINO_SHAPES[s.nextPieceType][0][i].x - minX) * blkStep;
            int ny = offY + (TETROMINO_SHAPES[s.nextPieceType][0][i].y - minY) * blkStep;
            TetDrawRect(s, nx, ny, 9, 9, TET_COLORS[s.nextPieceType]);
            TetPutPixel(s, nx + 1, ny + 1, TET_COL_WHITE);
            TetPutPixel(s, nx + 2, ny + 1, TET_COL_WHITE);
            TetPutPixel(s, nx + 1, ny + 2, TET_COL_WHITE);
        }
    }

    // B. Score & Level Card (Generous margin top, scale 2 score, lines, and level)
    TetDrawPanel(s, panelRightX, boardY + 67, panelRightW, 88, "SCORE & RANK", 0xFF4DEEEA);

    TetDrawText(s, panelRightX + 10, boardY + 87, "SCORE", TET_COL_TEXT_LABEL);
    char scoreBuf[16];
    sprintf_s(scoreBuf, "%06d", s.score);
    TetDrawText(s, panelRightX + 10, boardY + 96, scoreBuf, 0xFFFFFFFF, 2);

    TetDrawText(s, panelRightX + 10, boardY + 115, "LINES", TET_COL_TEXT_LABEL);
    sprintf_s(scoreBuf, "%04d", s.linesCleared);
    TetDrawText(s, panelRightX + 10, boardY + 124, scoreBuf, 0xFF00E5FF, 2);

    TetDrawText(s, panelRightX + 66, boardY + 115, "LEVEL", TET_COL_TEXT_LABEL);
    sprintf_s(scoreBuf, "%02d", s.level);
    TetDrawText(s, panelRightX + 66, boardY + 124, scoreBuf, 0xFFFFEA00, 2);

    TetDrawText(s, panelRightX + 10, boardY + 143, "TOP", TET_COL_TEXT_LABEL);
    sprintf_s(scoreBuf, "%06d", s.highScore);
    TetDrawText(s, panelRightX + 34, boardY + 143, scoreBuf, 0xFFFFD700);

    // C. Line Clears Card (Height 79, matching bottom of board and statistics table)
    TetDrawPanel(s, panelRightX, boardY + 161, panelRightW, 79, "LINE CLEARS", 0xFFFF9100);

    struct ClearStat { const char* label; int count; uint32_t col; };
    ClearStat clears[4] = {
        { "SINGLE", s.singleClears, 0xFFFFFFFF },
        { "DOUBLE", s.doubleClears, 0xFF00E5FF },
        { "TRIPLE", s.tripleClears, 0xFFFFEA00 },
        { "TETRIS", s.tetrisClears, 0xFFFFD700 }
    };

    for (int c = 0; c < 4; ++c) {
        int cy = boardY + 179 + c * 13;
        TetDrawRect(s, panelRightX + 8, cy + 1, 5, 5, clears[c].col);
        TetDrawText(s, panelRightX + 17, cy, clears[c].label, clears[c].col);

        char cBuf[16];
        sprintf_s(cBuf, "%03d", clears[c].count);
        TetDrawText(s, panelRightX + panelRightW - 26, cy, cBuf, 0xFFFFFFFF);
    }

    // 8. Floating Score Popups
    for (const auto& pop : s.popups) {
        TetDrawText(s, (int)pop.x, (int)pop.y, pop.text, pop.color);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderTetris(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<TetrisState>(30);

    int vH = BASE_TETRIS_H;
    int vW = (int)(288.0f * (float)width / (float)height);
    vW = (vW / 8) * 8; // Snap to 8-pixel alignment
    if (vW < 360) vW = 360;
    if (vW > 640) vW = 640;

    if (!s.initialized || s.virtualW != vW) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, TET_COL_BLACK);
        ResetTetrisGame(s);
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
        UpdateTetrisGame(s);
    }

    RenderTetrisFrame(s);

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

// Register as Screensaver ID 30
REGISTER_SCREENSAVER(
    30,
    L"Tetris",
    "tetris",
    { "tetris", "blocks", "tetramino", "brick" },
    WRAP_LEGACY(RenderTetris),
    GetTetrisSettings()
);
