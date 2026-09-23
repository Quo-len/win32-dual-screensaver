#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cstring>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Authentic 1980 Namco Pac-Man Arcade Simulation
// Virtual height is fixed at 288 (36 rows of 8x8 tiles); virtual width adapts
// dynamically to display aspect ratio (e.g. 512x288 = 64x36 tiles for 16:9).
// ============================================================================
static constexpr int BASE_PAC_H = 288;
static constexpr int TILE_SIZE  = 8; // 8x8 pixels per tile

// 32-bit ARGB Palette extracted directly from arcade reference
static constexpr uint32_t PAC_COL_BLACK      = 0xFF000000;
static constexpr uint32_t PAC_COL_WHITE      = 0xFFFFFFFF;
static constexpr uint32_t PAC_COL_YELLOW     = 0xFFFFFF00;
static constexpr uint32_t PAC_COL_MAZE_BLUE  = 0xFF2121FF; // Electric Blue Arcade Wall
static constexpr uint32_t PAC_COL_GATE_PINK  = 0xFFFFB8DE; // Ghost House Gate
static constexpr uint32_t PAC_COL_PELLET     = 0xFFFFB8AE; // Peach / Beige Pellet
static constexpr uint32_t PAC_COL_RED        = 0xFFFF0000; // Blinky
static constexpr uint32_t PAC_COL_PINK       = 0xFFFFB8FF; // Pinky
static constexpr uint32_t PAC_COL_CYAN       = 0xFF00FFFF; // Inky
static constexpr uint32_t PAC_COL_ORANGE     = 0xFFFFB852; // Clyde
static constexpr uint32_t PAC_COL_BLUE_GHOST = 0xFF2121DE; // Frightened Blue
static constexpr uint32_t PAC_COL_EYE_WHITE  = 0xFFDEDEFE; // Arcade eye sclera
static constexpr uint32_t PAC_COL_EYE_PUPIL  = 0xFF2121DE; // Arcade eye pupil
static constexpr uint32_t PAC_COL_CHERRY_RED = 0xFFFF0000;
static constexpr uint32_t PAC_COL_CHERRY_GRN = 0xFF00DE00;

// Direction Enum
enum Dir { DIR_NONE = -1, DIR_RIGHT = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_UP = 3 };
static const int DIR_DX[4] = { 1, 0, -1, 0 };
static const int DIR_DY[4] = { 0, 1, 0, -1 };

// Tile Types
enum TileType {
    TILE_EMPTY = 0,
    TILE_WALL  = 1,
    TILE_DOT   = 2,
    TILE_POWER = 3,
    TILE_GATE  = 4,
    TILE_PEN   = 5
};

// Ghost State
enum GhostMode { GHOST_CHASE = 0, GHOST_SCATTER = 1, GHOST_FRIGHTENED = 2, GHOST_EATEN = 3 };

// ============================================================================
// 5x7 RETRO BITMAP FONT
// ============================================================================
static const uint8_t* GetPacGlyph(char c) {
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
    case 'G': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x17, 0x11, 0x11, 0x0F }; return g; }
    case 'H': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }; return g; }
    case 'I': { static const uint8_t g[7] = { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }; return g; }
    case 'K': { static const uint8_t g[7] = { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }; return g; }
    case 'L': { static const uint8_t g[7] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }; return g; }
    case 'M': { static const uint8_t g[7] = { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }; return g; }
    case 'N': { static const uint8_t g[7] = { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }; return g; }
    case 'O': { static const uint8_t g[7] = { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
    case 'P': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }; return g; }
    case 'R': { static const uint8_t g[7] = { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }; return g; }
    case 'S': { static const uint8_t g[7] = { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E }; return g; }
    case 'T': { static const uint8_t g[7] = { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case 'U': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }; return g; }
    case 'V': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }; return g; }
    case 'W': { static const uint8_t g[7] = { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 }; return g; }
    case 'Y': { static const uint8_t g[7] = { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }; return g; }
    case '!': { static const uint8_t g[7] = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }; return g; }
    case '-': { static const uint8_t g[7] = { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }; return g; }
    default:  return BLANK;
    }
}

// ============================================================================
// SIMULATION STRUCTURES
// ============================================================================

struct Ghost {
    float x = 0;
    float y = 0;
    Dir dir = DIR_LEFT;
    GhostMode mode = GHOST_CHASE;
    uint32_t color = PAC_COL_RED;
    int animStep = 0;
    int homeTileX = 0;
    int homeTileY = 0;
    float speed = 1.10f;
    int releaseTimer = 0;
    bool inHouse = true;
    int lastTileX = -1;
    int lastTileY = -1;
};

struct PacmanState {
    bool initialized = false;
    uint64_t lastTick = 0;

    int virtualW = 512;
    int virtualH = BASE_PAC_H;
    int cols = 64;
    int rows = 36;

    std::vector<uint8_t> maze;
    std::vector<uint32_t> fb;
    std::vector<int> eatenDistToGate;

    // Pac-Man Entity
    float pacX = 0;
    float pacY = 0;
    Dir pacDir = DIR_LEFT;
    Dir pacNextDir = DIR_LEFT;
    int mouthAnim = 0;
    int mouthTimer = 0;
    bool pacAlive = true;
    int pacDeathTimer = 0;
    int lastPacTX = -1;
    int lastPacTY = -1;

    // 4 Ghosts: 0=Blinky, 1=Pinky, 2=Inky, 3=Clyde
    Ghost ghosts[4];
    int frightenedTimer = 0;
    int ghostEatenCount = 0;

    // Score Popup
    int popupScore = 0;
    float popupX = 0;
    float popupY = 0;
    int popupTimer = 0;

    // Bonus Fruit
    bool fruitActive = false;
    float fruitX = 0;
    float fruitY = 0;
    int fruitTimer = 0;

    // Game stats
    int score = 0;
    int highScore = 12480;
    int lives = 3;
    int round = 1;
    int dotsRemaining = 0;
    int waveClearTimer = 0;
    int gameOverTimer = 0;
    int modeTimer = 0;
    bool scatterPhase = true;
};

// ============================================================================
// PIXEL HELPERS
// ============================================================================
static inline void PacPutPixel(PacmanState& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void PacDrawText(PacmanState& s, int x, int y, const char* str, uint32_t color) {
    int curX = x;
    while (*str) {
        if (*str != ' ') {
            const uint8_t* glyph = GetPacGlyph(*str);
            for (int r = 0; r < 7; ++r) {
                uint8_t rowBits = glyph[r];
                for (int c = 0; c < 5; ++c) {
                    if ((rowBits >> (4 - c)) & 1) {
                        PacPutPixel(s, curX + c, y + r, color);
                    }
                }
            }
        }
        curX += 6;
        str++;
    }
}

static inline uint8_t GetTile(const PacmanState& s, int tx, int ty) {
    if (ty < 0 || ty >= s.rows) return TILE_WALL;
    if (tx < 0 || tx >= s.cols) return TILE_EMPTY; // Warp tunnel wrap opening
    return s.maze[ty * s.cols + tx];
}

static inline void SetTile(PacmanState& s, int tx, int ty, uint8_t val) {
    if (tx >= 0 && tx < s.cols && ty >= 0 && ty < s.rows) {
        s.maze[ty * s.cols + tx] = val;
    }
}

static inline bool IsPassable(const PacmanState& s, int tx, int ty, bool isGhost, bool isEaten) {
    if (ty < 3 || ty >= s.rows - 2) return false;
    if (tx < 0 || tx >= s.cols) return true; // Warp tunnel wrap
    uint8_t t = s.maze[ty * s.cols + tx];
    if (t == TILE_WALL) return false;
    if (t == TILE_GATE) return isEaten;
    if (t == TILE_PEN)  return isEaten;
    return true;
}

// ============================================================================
// AUTHENTIC PAC-MAN MAZE BLUEPRINT
// ============================================================================
static const char* PAC_MAP_HALF[31] = {
    //01234567890123456789012345678901
    "################################", // 0: Top outer border
    "#............##....o...##.......", // 1: Top corridor
    "#.####.#####.##.######.##.###.##", // 2: Pills & T-shapes
    "#o####.#####.##.######.##.###.##", // 3: Power pellet
    "#.####.#####.##.######.##.###.##", // 4
    "#..........................##..#", // 5: Corridor
    "#.####.##.########.##.####.###..", // 6: L-shapes
    "#.####.##.########.##.####.###.#", // 7
    "#......##....##....##..........#", // 8: Corridor
    "######.#####.##.######.#####.###", // 9: T-junctions
    "######.#####.##.######.#####.###", // 10
    "    ##.##.......................", // 11: Corridor above ghost house
    "######.##.######.######.###.###-", // 12: Ghost house gate
    "######.##.######.######.###.#PPP", // 13: Ghost house interior
    "      ....######.######.###.#PPP", // 14: Warp tunnel opening on left edge
    "######.##.######.######.###.#PPP", // 15: Ghost house interior
    "######.##.######.######.###.####", // 16: Ghost house bottom wall
    "    ##.##.......................", // 17: Corridor below ghost house (Spawn Path)
    "######.##.######.######.##.###.#", // 18: Inverted T-shape tops
    "######.##.######.######.##.###.#", // 19
    "#............##....o....##..##.#", // 20: Corridor
    "#.####.#####.##.######.####.##.#", // 21: T-stems
    "#.####.#####.##.######.####....#", // 22
    "#o..##......................##..", // 23: Power pellet
    "###.##.##.########.##.#####.####", // 24: Corner L-shapes
    "###.##.##.########.##.#####.####", // 25
    "#......##....##....##..........#", // 26: Corridor
    "#.##########.##.##########.###..", // 27: Bottom T-shapes
    "#.##########.##.##########.###.#", // 28
    "#....................o.........#", // 29: Bottom corridor
    "################################" // 30
};

// Precompute BFS distance map guiding eaten ghosts from anywhere in the maze to the ghost house entrance
static void ComputeEatenGhostPathMap(PacmanState& s) {
    s.eatenDistToGate.assign(s.cols * s.rows, 99999);
    int gateTX1 = s.cols / 2 - 1;
    int gateTX2 = s.cols / 2;
    int gateTY = 14; // Corridor row directly above the gate at row 15

    struct QItem { int x; int y; int dist; };
    std::vector<QItem> q;
    q.reserve(s.cols * s.rows);

    auto PushQ = [&](int x, int y, int d) {
        if (x >= 0 && x < s.cols && y >= 3 && y < s.rows - 2) {
            int idx = y * s.cols + x;
            if (s.eatenDistToGate[idx] > d) {
                s.eatenDistToGate[idx] = d;
                q.push_back({ x, y, d });
            }
        }
    };

    PushQ(gateTX1, gateTY, 0);
    PushQ(gateTX2, gateTY, 0);

    size_t head = 0;
    while (head < q.size()) {
        QItem cur = q[head++];
        for (int d = 0; d < 4; ++d) {
            int nx = (cur.x + DIR_DX[d] + s.cols) % s.cols;
            int ny = cur.y + DIR_DY[d];

            if (ny < 3 || ny >= s.rows - 2) continue;
            uint8_t t = GetTile(s, nx, ny);
            if (t == TILE_WALL || t == TILE_PEN || t == TILE_GATE) continue;

            int nIdx = ny * s.cols + nx;
            if (s.eatenDistToGate[nIdx] > cur.dist + 1) {
                s.eatenDistToGate[nIdx] = cur.dist + 1;
                q.push_back({ nx, ny, cur.dist + 1 });
            }
        }
    }
}

static void GenerateWidescreenMaze(PacmanState& s) {
    s.maze.assign(s.cols * s.rows, TILE_EMPTY);
    s.dotsRemaining = 0;

    int cols = s.cols;
    int offsetX = (cols - 64) / 2; // Offset horizontally to center the standard 64-width board
    if (offsetX < 0) offsetX = 0;

    for (int y = 0; y < 31; ++y) {
        const char* rowStr = PAC_MAP_HALF[y];
        int ty = y + 3; // Offset by 3 for HUD

        for (int x = 0; x < 32; ++x) {
            char c = rowStr[x];
            int leftX = offsetX + x;
            int rightX = cols - 1 - leftX;

            auto ApplyChar = [&](int gx, char ch) {
                if (gx < 0 || gx >= cols) return;
                switch (ch) {
                case '#': SetTile(s, gx, ty, TILE_WALL); break;
                case '.': SetTile(s, gx, ty, TILE_DOT); s.dotsRemaining++; break;
                case 'o': SetTile(s, gx, ty, TILE_POWER); s.dotsRemaining++; break;
                case '-': SetTile(s, gx, ty, TILE_GATE); break;
                case 'P': SetTile(s, gx, ty, TILE_PEN); break;
                default:  SetTile(s, gx, ty, TILE_EMPTY); break;
                }
            };

            ApplyChar(leftX, c);
            ApplyChar(rightX, c);
        }
    }

    ComputeEatenGhostPathMap(s);
}

// Reset characters to starting positions
static void ResetCharacters(PacmanState& s, bool fullReset) {
    int cx = s.cols / 2;
    int offsetX = (s.cols - 64) / 2;
    if (offsetX < 0) offsetX = 0;

    // Pac-Man starts directly below the ghost house, centered on the tile grid
    s.pacX = (float)(cx * TILE_SIZE);
    s.pacY = (float)((17 + 3) * TILE_SIZE);
    s.pacDir = (std::rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;
    s.pacNextDir = s.pacDir;
    s.mouthAnim = 0;
    s.mouthTimer = 0;
    s.pacAlive = true;
    s.pacDeathTimer = 0;
    s.lastPacTX = -1;
    s.lastPacTY = -1;

    // Blinky (Red) - starts outside gate
    s.ghosts[0].x = (float)(cx * TILE_SIZE);
    s.ghosts[0].y = (float)((11 + 3) * TILE_SIZE);
    s.ghosts[0].dir = (std::rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;
    s.ghosts[0].mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
    s.ghosts[0].color = PAC_COL_RED;
    s.ghosts[0].homeTileX = s.cols - 2 - offsetX;
    s.ghosts[0].homeTileY = 4;
    s.ghosts[0].releaseTimer = 0;
    s.ghosts[0].inHouse = false;
    s.ghosts[0].speed = 1.10f;
    s.ghosts[0].lastTileX = -1;
    s.ghosts[0].lastTileY = -1;

    // Pinky (Pink) - inside house
    s.ghosts[1].x = (float)((cx - 1) * TILE_SIZE);
    s.ghosts[1].y = (float)((14 + 3) * TILE_SIZE);
    s.ghosts[1].dir = DIR_UP;
    s.ghosts[1].mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
    s.ghosts[1].color = PAC_COL_PINK;
    s.ghosts[1].homeTileX = 2 + offsetX;
    s.ghosts[1].homeTileY = 4;
    s.ghosts[1].releaseTimer = 40;
    s.ghosts[1].inHouse = true;
    s.ghosts[1].speed = 1.05f;
    s.ghosts[1].lastTileX = -1;
    s.ghosts[1].lastTileY = -1;

    // Inky (Cyan) - inside house
    s.ghosts[2].x = (float)((cx - 2) * TILE_SIZE);
    s.ghosts[2].y = (float)((14 + 3) * TILE_SIZE);
    s.ghosts[2].dir = DIR_UP;
    s.ghosts[2].mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
    s.ghosts[2].color = PAC_COL_CYAN;
    s.ghosts[2].homeTileX = s.cols - 2 - offsetX;
    s.ghosts[2].homeTileY = s.rows - 4;
    s.ghosts[2].releaseTimer = 110;
    s.ghosts[2].inHouse = true;
    s.ghosts[2].speed = 1.02f;
    s.ghosts[2].lastTileX = -1;
    s.ghosts[2].lastTileY = -1;

    // Clyde (Orange) - inside house
    s.ghosts[3].x = (float)((cx + 1) * TILE_SIZE);
    s.ghosts[3].y = (float)((14 + 3) * TILE_SIZE);
    s.ghosts[3].dir = DIR_UP;
    s.ghosts[3].mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
    s.ghosts[3].color = PAC_COL_ORANGE;
    s.ghosts[3].homeTileX = 2 + offsetX;
    s.ghosts[3].homeTileY = s.rows - 4;
    s.ghosts[3].releaseTimer = 180;
    s.ghosts[3].inHouse = true;
    s.ghosts[3].speed = 1.00f;
    s.ghosts[3].lastTileX = -1;
    s.ghosts[3].lastTileY = -1;

    s.frightenedTimer = 0;
    s.ghostEatenCount = 0;
    s.popupTimer = 0;

    if (fullReset) {
        s.fruitActive = false;
        s.fruitTimer = 400;
    }
}

static void ResetPacmanGame(PacmanState& s) {
    srand((unsigned int)GetTickCount64());
    if (s.score > s.highScore) s.highScore = s.score;
    s.score = 0;
    s.lives = 3;
    s.round = 1;
    s.gameOverTimer = 0;
    s.waveClearTimer = 0;
    s.modeTimer = 0;
    s.scatterPhase = true;

    GenerateWidescreenMaze(s);
    ResetCharacters(s, true);
}

// ============================================================================
// AUTONOMOUS PAC-MAN AI
// ============================================================================

// Fast BFS to find distance to nearest pellet/power dot in maze
static int GetDistToNearestDot(const PacmanState& s, int startTX, int startTY, int maxDepth = 20) {
    uint8_t startTile = GetTile(s, startTX, startTY);
    if (startTile == TILE_DOT || startTile == TILE_POWER) return 0;

    struct QNode { int x; int y; int dist; };
    QNode queue[64];
    int qHead = 0, qTail = 0;

    bool visited[40][64] = { false };
    if (startTY >= 0 && startTY < s.rows && startTX >= 0 && startTX < s.cols) {
        visited[startTY][startTX] = true;
    }

    queue[qTail++] = { startTX, startTY, 0 };

    while (qHead < qTail) {
        QNode cur = queue[qHead++];
        if (cur.dist >= maxDepth) break;

        for (int d = 0; d < 4; ++d) {
            int nx = (cur.x + DIR_DX[d] + s.cols) % s.cols;
            int ny = cur.y + DIR_DY[d];

            if (ny < 3 || ny >= s.rows - 2) continue;
            if (visited[ny][nx]) continue;

            uint8_t t = GetTile(s, nx, ny);
            if (t == TILE_WALL || t == TILE_GATE || t == TILE_PEN) continue;

            if (t == TILE_DOT || t == TILE_POWER) {
                return cur.dist + 1;
            }

            visited[ny][nx] = true;
            if (qTail < 64) {
                queue[qTail++] = { nx, ny, cur.dist + 1 };
            }
        }
    }
    return maxDepth;
}

static void UpdatePacmanAI(PacmanState& s) {
    if (!s.pacAlive) return;

    int curTX = (int)std::round(s.pacX / (float)TILE_SIZE);
    int curTY = (int)std::round(s.pacY / (float)TILE_SIZE);

    // Only make 1 decision per tile intersection
    if (curTX == s.lastPacTX && curTY == s.lastPacTY) {
        // Exception: imminent lethal danger allows emergency reaction
        bool emergency = false;
        for (int g = 0; g < 4; ++g) {
            const auto& gh = s.ghosts[g];
            if (!gh.inHouse && gh.mode != GHOST_FRIGHTENED && gh.mode != GHOST_EATEN) {
                if (std::hypot(s.pacX - gh.x, s.pacY - gh.y) < 24.0f) {
                    emergency = true;
                    break;
                }
            }
        }
        if (!emergency) return;
    }

    float alignDist = std::hypot(s.pacX - curTX * TILE_SIZE, s.pacY - curTY * TILE_SIZE);
    if (alignDist > 2.5f) return;

    s.lastPacTX = curTX;
    s.lastPacTY = curTY;

    Dir bestDir = s.pacDir;
    float bestScore = -999999.0f;

    for (int d = 0; d < 4; ++d) {
        int nextTX = (curTX + DIR_DX[d] + s.cols) % s.cols;
        int nextTY = curTY + DIR_DY[d];

        if (!IsPassable(s, nextTX, nextTY, false, false)) continue;

        // Dynamic random noise (low weight to maintain path stability)
        float score = ((std::rand() % 1000) / 1000.0f) * 4.0f;

        // 1. Ghost Threat Evaluation
        for (int g = 0; g < 4; ++g) {
            const auto& gh = s.ghosts[g];
            if (gh.inHouse) continue; // Don't fear ghosts inside spawn!

            float gDist = std::hypot(nextTX * TILE_SIZE - gh.x, nextTY * TILE_SIZE - gh.y);

            if (gh.mode == GHOST_FRIGHTENED) {
                score += (240.0f - gDist) * 3.5f; // Hunt frightened ghosts!
            } else if (gh.mode != GHOST_EATEN) {
                if (gDist < 26.0f) {
                    score -= (26.0f - gDist) * 2000.0f; // Critical danger
                } else if (gDist < 56.0f) {
                    score -= (56.0f - gDist) * 45.0f; // Approaching threat
                }
            }
        }

        // 2. Dots & Fruit
        uint8_t targetTile = GetTile(s, nextTX, nextTY);
        if (targetTile == TILE_DOT)   score += 25.0f;
        if (targetTile == TILE_POWER) score += (s.ghosts[0].mode != GHOST_FRIGHTENED ? 120.0f : 10.0f);

        // Long-range dot guidance via BFS
        int distToDot = GetDistToNearestDot(s, nextTX, nextTY, 20);
        score += (20.0f - distToDot) * 3.0f;

        if (s.fruitActive) {
            float fDist = std::hypot(nextTX * TILE_SIZE - s.fruitX, nextTY * TILE_SIZE - s.fruitY);
            score += (220.0f - fDist) * 0.9f;
        }

        // Momentum bonus and heavy 180-reverse penalty to prevent oscillating/spinning
        if (d == s.pacDir) score += 8.0f;
        if ((d + 2) % 4 == s.pacDir) score -= 60.0f;

        if (score > bestScore) {
            bestScore = score;
            bestDir = (Dir)d;
        }
    }

    s.pacNextDir = bestDir;
}

// ============================================================================
// AUTHENTIC GHOST AI (Blinky, Pinky, Inky, Clyde)
// ============================================================================

// Returns optimal direction for an eaten ghost returning to the ghost house via BFS path map
static Dir GetEatenGhostDir(const PacmanState& s, int curTX, int curTY, Dir curDir) {
    int gateTX1 = s.cols / 2 - 1;
    int gateTX2 = s.cols / 2;

    // Directly above or on the gate: force DOWN into the ghost house
    if ((curTY == 14 || curTY == 15) && (curTX == gateTX1 || curTX == gateTX2)) {
        return DIR_DOWN;
    }

    Dir bestDir = curDir;
    int bestDist = 999999;

    for (int d = 0; d < 4; ++d) {
        if ((d + 2) % 4 == curDir) continue; // No 180 reverse

        int nx = (curTX + DIR_DX[d] + s.cols) % s.cols;
        int ny = curTY + DIR_DY[d];
        if (!IsPassable(s, nx, ny, true, true)) continue;

        int dist = s.eatenDistToGate[ny * s.cols + nx];
        if (dist < bestDist) {
            bestDist = dist;
            bestDir = (Dir)d;
        }
    }

    // Dead-end or trapped fallback: allow reverse
    if (bestDist >= 99999) {
        for (int d = 0; d < 4; ++d) {
            int nx = (curTX + DIR_DX[d] + s.cols) % s.cols;
            int ny = curTY + DIR_DY[d];
            if (!IsPassable(s, nx, ny, true, true)) continue;

            int dist = s.eatenDistToGate[ny * s.cols + nx];
            if (dist < bestDist) {
                bestDist = dist;
                bestDir = (Dir)d;
            }
        }
    }

    return bestDir;
}

static void UpdateGhostAI(PacmanState& s, int idx) {
    auto& gh = s.ghosts[idx];

    // House release
    if (gh.inHouse) {
        if (gh.releaseTimer > 0) {
            gh.releaseTimer--;
            gh.y += (gh.dir == DIR_UP ? -0.4f : 0.4f);
            if (gh.y < 16.0f * TILE_SIZE) gh.dir = DIR_DOWN;
            if (gh.y > 17.5f * TILE_SIZE) gh.dir = DIR_UP;
            return;
        }

        float gateX = (float)((s.cols / 2) * TILE_SIZE);
        float gateY = (float)((11 + 3) * TILE_SIZE);

        if (std::abs(gh.x - gateX) > 1.0f) {
            gh.x += (gh.x < gateX ? 0.8f : -0.8f);
        } else if (gh.y > gateY) {
            gh.y -= 0.8f;
        } else {
            gh.inHouse = false;
            gh.dir = (std::rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;
            gh.x = gateX;
            gh.y = gateY;
            gh.lastTileX = -1;
            gh.lastTileY = -1;
        }
        return;
    }

    int curTX = (int)std::round(gh.x / (float)TILE_SIZE);
    int curTY = (int)std::round(gh.y / (float)TILE_SIZE);

    int penMinX = s.cols / 2 - 3;
    int penMaxX = s.cols / 2 + 2;

    // Check if eaten ghost has successfully reached inside the ghost house
    if (gh.mode == GHOST_EATEN && (curTY >= 16 && curTY <= 18) && (curTX >= penMinX && curTX <= penMaxX)) {
        gh.mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
        gh.inHouse = true;
        gh.releaseTimer = 30;
        gh.lastTileX = -1;
        gh.lastTileY = -1;
        return;
    }

    // Only make 1 decision per tile intersection
    if (curTX == gh.lastTileX && curTY == gh.lastTileY) return;

    float alignDist = std::hypot(gh.x - curTX * TILE_SIZE, gh.y - curTY * TILE_SIZE);
    if (alignDist > 2.5f) return;

    gh.lastTileX = curTX;
    gh.lastTileY = curTY;

    Dir bestDir = gh.dir;

    if (gh.mode == GHOST_EATEN) {
        bestDir = GetEatenGhostDir(s, curTX, curTY, gh.dir);
    } else {
        int targetX = 0;
        int targetY = 0;

        int pacTX = (int)std::round(s.pacX / (float)TILE_SIZE);
        int pacTY = (int)std::round(s.pacY / (float)TILE_SIZE);

        if (gh.mode == GHOST_FRIGHTENED) {
            targetX = std::rand() % s.cols;
            targetY = std::rand() % s.rows;
        } else if (gh.mode == GHOST_SCATTER) {
            targetX = gh.homeTileX;
            targetY = gh.homeTileY;
        } else {
            switch (idx) {
            case 0:
                targetX = pacTX;
                targetY = pacTY;
                break;
            case 1:
                targetX = pacTX + DIR_DX[s.pacDir] * 4;
                targetY = pacTY + DIR_DY[s.pacDir] * 4;
                break;
            case 2: {
                int bTX = (int)std::round(s.ghosts[0].x / (float)TILE_SIZE);
                int bTY = (int)std::round(s.ghosts[0].y / (float)TILE_SIZE);
                int pivotX = pacTX + DIR_DX[s.pacDir] * 2;
                int pivotY = pacTY + DIR_DY[s.pacDir] * 2;
                targetX = pivotX + (pivotX - bTX);
                targetY = pivotY + (pivotY - bTY);
                break;
            }
            case 3: {
                float distToPac = std::hypot((float)(curTX - pacTX), (float)(curTY - pacTY));
                if (distToPac > 8.0f) {
                    targetX = pacTX;
                    targetY = pacTY;
                } else {
                    targetX = gh.homeTileX;
                    targetY = gh.homeTileY;
                }
                break;
            }
            }
        }

        float minDist = 999999.0f;

        for (int d = 0; d < 4; ++d) {
            if ((d + 2) % 4 == gh.dir) continue; // No 180 reverse

            int nextTX = (curTX + DIR_DX[d] + s.cols) % s.cols;
            int nextTY = curTY + DIR_DY[d];

            if (!IsPassable(s, nextTX, nextTY, true, false)) continue;

            float dist = std::hypot((float)(nextTX - targetX), (float)(nextTY - targetY)) + (std::rand() % 100) * 0.001f;
            if (dist < minDist) {
                minDist = dist;
                bestDir = (Dir)d;
            }
        }

        // Dead end fallback: allow reverse if forward is a wall
        if (minDist >= 999990.0f) {
            int revDir = (gh.dir + 2) % 4;
            int revTX = (curTX + DIR_DX[revDir] + s.cols) % s.cols;
            int revTY = curTY + DIR_DY[revDir];
            if (IsPassable(s, revTX, revTY, true, false)) {
                bestDir = (Dir)revDir;
            }
        }
    }

    // Snap to intersection on direction change
    if (bestDir != gh.dir) {
        gh.x = (float)(curTX * TILE_SIZE);
        gh.y = (float)(curTY * TILE_SIZE);
        gh.dir = bestDir;
    }
}

// ============================================================================
// GAME LOGIC UPDATE (Fixed 60 FPS)
// ============================================================================
static void UpdatePacmanGame(PacmanState& s) {
    if (s.gameOverTimer > 0) {
        s.gameOverTimer--;
        if (s.gameOverTimer == 0) ResetPacmanGame(s);
        return;
    }

    if (s.waveClearTimer > 0) {
        s.waveClearTimer--;
        if (s.waveClearTimer == 0) {
            s.round++;
            GenerateWidescreenMaze(s);
            ResetCharacters(s, false);
        }
        return;
    }

    if (!s.pacAlive) {
        s.pacDeathTimer--;
        if (s.pacDeathTimer <= 0) {
            s.lives--;
            if (s.lives <= 0) {
                s.gameOverTimer = 120;
            } else {
                ResetCharacters(s, false);
            }
        }
        return;
    }

    // 1. Pac-Man AI decision
    UpdatePacmanAI(s);

    // 2. Move Pac-Man with Strict Tile Grid Locking
    int curTX = (int)std::round(s.pacX / (float)TILE_SIZE);
    int curTY = (int)std::round(s.pacY / (float)TILE_SIZE);

    // Lock perpendicular axis strictly to tile lane
    if (s.pacDir == DIR_LEFT || s.pacDir == DIR_RIGHT) {
        s.pacY = (float)(curTY * TILE_SIZE);
    } else {
        s.pacX = (float)(curTX * TILE_SIZE);
    }

    // Handle turning at intersection or 180 reverse
    if (s.pacNextDir != s.pacDir) {
        int nextTX = (curTX + DIR_DX[s.pacNextDir] + s.cols) % s.cols;
        int nextTY = curTY + DIR_DY[s.pacNextDir];

        if (IsPassable(s, nextTX, nextTY, false, false)) {
            if ((s.pacNextDir + 2) % 4 == s.pacDir) {
                // Immediate 180 turn
                s.pacDir = s.pacNextDir;
            } else {
                float distToCenter = std::hypot(s.pacX - curTX * TILE_SIZE, s.pacY - curTY * TILE_SIZE);
                if (distToCenter <= 2.5f) {
                    s.pacX = (float)(curTX * TILE_SIZE);
                    s.pacY = (float)(curTY * TILE_SIZE);
                    s.pacDir = s.pacNextDir;
                }
            }
        }
    }

    // Check forward wall collision
    int nextTX = (curTX + DIR_DX[s.pacDir] + s.cols) % s.cols;
    int nextTY = curTY + DIR_DY[s.pacDir];
    bool canMove = IsPassable(s, nextTX, nextTY, false, false);

    float pacSpeed = 1.15f;
    if (!canMove) {
        // Clamp to tile center so Pac-Man stops cleanly at wall
        s.pacX = (float)(curTX * TILE_SIZE);
        s.pacY = (float)(curTY * TILE_SIZE);
        s.lastPacTX = -1;
        s.lastPacTY = -1; // Reset to allow turning immediately!

        // If turn was queued and passable, turn immediately
        int turnTX = (curTX + DIR_DX[s.pacNextDir] + s.cols) % s.cols;
        int turnTY = curTY + DIR_DY[s.pacNextDir];
        if (s.pacNextDir != s.pacDir && IsPassable(s, turnTX, turnTY, false, false)) {
            s.pacDir = s.pacNextDir;
        } else {
            // Find any open direction so Pac-Man never gets stuck against a wall
            for (int d = 0; d < 4; ++d) {
                int testTX = (curTX + DIR_DX[d] + s.cols) % s.cols;
                int testTY = curTY + DIR_DY[d];
                if (IsPassable(s, testTX, testTY, false, false)) {
                    s.pacDir = (Dir)d;
                    s.pacNextDir = (Dir)d;
                    break;
                }
            }
        }
    } else {
        s.pacX += DIR_DX[s.pacDir] * pacSpeed;
        s.pacY += DIR_DY[s.pacDir] * pacSpeed;

        s.mouthTimer++;
        if (s.mouthTimer >= 3) {
            s.mouthTimer = 0;
            s.mouthAnim = (s.mouthAnim + 1) % 4;
        }
    }

    // Single Warp Tunnel Exit Wrap (Only at tunnel row 14+3 = 17)
    if (curTY == 17) {
        if (s.pacX < -4.0f) s.pacX = (float)((s.cols - 1) * TILE_SIZE + 4);
        if (s.pacX > (float)((s.cols - 1) * TILE_SIZE + 4)) s.pacX = -4.0f;
    }

    // 3. Dot / Power Pellet Eating
    curTX = (int)std::round(s.pacX / (float)TILE_SIZE);
    curTY = (int)std::round(s.pacY / (float)TILE_SIZE);
    uint8_t t = GetTile(s, curTX, curTY);

    if (t == TILE_DOT) {
        SetTile(s, curTX, curTY, TILE_EMPTY);
        s.score += 10;
        s.dotsRemaining--;
        if (s.score > s.highScore) s.highScore = s.score;
        if (s.dotsRemaining <= 0) s.waveClearTimer = 90;
    } else if (t == TILE_POWER) {
        SetTile(s, curTX, curTY, TILE_EMPTY);
        s.score += 50;
        s.dotsRemaining--;
        if (s.score > s.highScore) s.highScore = s.score;
        s.frightenedTimer = 320;
        s.ghostEatenCount = 0;
        for (auto& gh : s.ghosts) {
            if (gh.mode != GHOST_EATEN && !gh.inHouse) {
                gh.mode = GHOST_FRIGHTENED;
                gh.dir = (Dir)((gh.dir + 2) % 4);
                gh.lastTileX = -1;
                gh.lastTileY = -1;
            }
        }
        if (s.dotsRemaining <= 0) s.waveClearTimer = 90;
    }

    // Scatter / Chase wave timer
    if (s.frightenedTimer == 0) {
        s.modeTimer++;
        if (s.scatterPhase && s.modeTimer > 420) { // 7 seconds scatter
            s.scatterPhase = false;
            s.modeTimer = 0;
            for (auto& gh : s.ghosts) {
                if (gh.mode == GHOST_SCATTER) {
                    gh.mode = GHOST_CHASE;
                    gh.dir = (Dir)((gh.dir + 2) % 4);
                    gh.lastTileX = -1;
                    gh.lastTileY = -1;
                }
            }
        } else if (!s.scatterPhase && s.modeTimer > 1200) { // 20 seconds chase
            s.scatterPhase = true;
            s.modeTimer = 0;
            for (auto& gh : s.ghosts) {
                if (gh.mode == GHOST_CHASE) {
                    gh.mode = GHOST_SCATTER;
                    gh.dir = (Dir)((gh.dir + 2) % 4);
                    gh.lastTileX = -1;
                    gh.lastTileY = -1;
                }
            }
        }
    }

    if (s.frightenedTimer > 0) {
        s.frightenedTimer--;
        if (s.frightenedTimer == 0) {
            for (auto& gh : s.ghosts) {
                if (gh.mode == GHOST_FRIGHTENED) {
                    gh.mode = s.scatterPhase ? GHOST_SCATTER : GHOST_CHASE;
                    gh.lastTileX = -1;
                    gh.lastTileY = -1;
                }
            }
        }
    }

    // 4. Update Ghosts with Strict Grid Movement & Wall Collision
    for (int g = 0; g < 4; ++g) {
        UpdateGhostAI(s, g);
        auto& gh = s.ghosts[g];

        if (!gh.inHouse) {
            int gTX = (int)std::round(gh.x / (float)TILE_SIZE);
            int gTY = (int)std::round(gh.y / (float)TILE_SIZE);

            // Lock perpendicular axis
            if (gh.dir == DIR_LEFT || gh.dir == DIR_RIGHT) {
                gh.y = (float)(gTY * TILE_SIZE);
            } else {
                gh.x = (float)(gTX * TILE_SIZE);
            }

            float gSpeed = gh.speed;
            if (gh.mode == GHOST_FRIGHTENED) gSpeed *= 0.65f;
            if (gh.mode == GHOST_EATEN)      gSpeed *= 2.0f;

            // Tunnel slow-down
            if (gTY == 17 && (gTX < 4 || gTX > s.cols - 5)) gSpeed *= 0.55f;

            int gNextTX = (gTX + DIR_DX[gh.dir] + s.cols) % s.cols;
            int gNextTY = gTY + DIR_DY[gh.dir];
            bool gCanMove = IsPassable(s, gNextTX, gNextTY, true, gh.mode == GHOST_EATEN);

            if (!gCanMove) {
                gh.x = (float)(gTX * TILE_SIZE);
                gh.y = (float)(gTY * TILE_SIZE);
                gh.lastTileX = -1;
                gh.lastTileY = -1; // Reset so AI can immediately re-evaluate at next tick

                // Find any passable direction so ghost never freezes
                Dir fallbackDir = DIR_NONE;
                for (int d = 0; d < 4; ++d) {
                    int nTX = (gTX + DIR_DX[d] + s.cols) % s.cols;
                    int nTY = gTY + DIR_DY[d];
                    if (IsPassable(s, nTX, nTY, true, gh.mode == GHOST_EATEN)) {
                        fallbackDir = (Dir)d;
                        if ((d + 2) % 4 != gh.dir) break;
                    }
                }
                if (fallbackDir != DIR_NONE) {
                    gh.dir = fallbackDir;
                }
            } else {
                gh.x += DIR_DX[gh.dir] * gSpeed;
                gh.y += DIR_DY[gh.dir] * gSpeed;
            }

            // Single Warp Tunnel Wrap
            if (gTY == 17) {
                if (gh.x < -4.0f) gh.x = (float)((s.cols - 1) * TILE_SIZE + 4);
                if (gh.x > (float)((s.cols - 1) * TILE_SIZE + 4)) gh.x = -4.0f;
            }
        }

        gh.animStep = (gh.animStep + 1) % 16;

        float distToPac = std::hypot(gh.x - s.pacX, gh.y - s.pacY);
        if (distToPac < 7.0f) {
            if (gh.mode == GHOST_FRIGHTENED) {
                gh.mode = GHOST_EATEN;
                s.ghostEatenCount++;
                int pts = 200 * (1 << ((std::min)(3, s.ghostEatenCount - 1)));
                s.score += pts;
                if (s.score > s.highScore) s.highScore = s.score;

                s.popupScore = pts;
                s.popupX = gh.x;
                s.popupY = gh.y;
                s.popupTimer = 35;
            } else if (gh.mode == GHOST_CHASE || gh.mode == GHOST_SCATTER) {
                s.pacAlive = false;
                s.pacDeathTimer = 60;
                break;
            }
        }
    }

    if (s.popupTimer > 0) s.popupTimer--;

    // 5. Bonus Fruit
    s.fruitTimer--;
    if (s.fruitTimer <= 0 && !s.fruitActive) {
        s.fruitActive = true;
        s.fruitX = (float)((s.cols / 2) * TILE_SIZE);
        s.fruitY = (float)((17 + 3) * TILE_SIZE);
        s.fruitTimer = 350;
    } else if (s.fruitActive) {
        if (std::hypot(s.pacX - s.fruitX, s.pacY - s.fruitY) < 8.0f) {
            s.fruitActive = false;
            s.score += 100 * s.round;
            if (s.score > s.highScore) s.highScore = s.score;
            s.popupScore = 100 * s.round;
            s.popupX = s.fruitX;
            s.popupY = s.fruitY;
            s.popupTimer = 35;
            s.fruitTimer = 500;
        } else if (s.fruitTimer <= 0) {
            s.fruitActive = false;
            s.fruitTimer = 500;
        }
    }
}

// ============================================================================
// SPRITE RENDERING
// ============================================================================

// 13x13 Pac-Man Sprite
static void RenderPacmanSprite(PacmanState& s, float posX, float posY, Dir dir, int anim) {
    float cx = posX + 4.0f;
    float cy = posY + 4.0f;
    int px = (int)std::round(cx - 6.0f);
    int py = (int)std::round(cy - 6.0f);

    static const uint16_t PAC_CIRCLE[13] = {
        0x01F0, 0x07FC, 0x0FFE, 0x0FFE, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x1FFF, 0x0FFE, 0x0FFE, 0x07FC, 0x01F0
    };

    for (int y = 0; y < 13; ++y) {
        uint16_t rowBits = PAC_CIRCLE[y];
        float dy = (float)y - 6.0f;
        for (int x = 0; x < 13; ++x) {
            if ((rowBits >> (12 - x)) & 1) {
                float dx = (float)x - 6.0f;
                bool inMouth = false;

                if (anim == 2) {
                    // Wide mouth (~60 deg)
                    if (dir == DIR_RIGHT && dx > 0.0f && std::abs(dy) <= dx * 0.85f) inMouth = true;
                    if (dir == DIR_LEFT  && dx < 0.0f && std::abs(dy) <= -dx * 0.85f) inMouth = true;
                    if (dir == DIR_DOWN  && dy > 0.0f && std::abs(dx) <= dy * 0.85f) inMouth = true;
                    if (dir == DIR_UP    && dy < 0.0f && std::abs(dx) <= -dy * 0.85f) inMouth = true;
                } else if (anim == 1 || anim == 3) {
                    // Half-open mouth (~35 deg)
                    if (dir == DIR_RIGHT && dx > 0.0f && std::abs(dy) <= dx * 0.45f) inMouth = true;
                    if (dir == DIR_LEFT  && dx < 0.0f && std::abs(dy) <= -dx * 0.45f) inMouth = true;
                    if (dir == DIR_DOWN  && dy > 0.0f && std::abs(dx) <= dy * 0.45f) inMouth = true;
                    if (dir == DIR_UP    && dy < 0.0f && std::abs(dx) <= -dy * 0.45f) inMouth = true;
                }

                if (!inMouth) {
                    PacPutPixel(s, px + x, py + y, PAC_COL_YELLOW);
                }
            }
        }
    }
}

// 14x14 Authentic Arcade Ghost Sprite
static void RenderGhostSprite(PacmanState& s, const Ghost& gh) {
    float cx = gh.x + 4.0f;
    float cy = gh.y + 4.0f;
    int gx = (int)std::round(cx - 7.0f);
    int gy = (int)std::round(cy - 7.0f);

    // Eaten Floating Eyes
    if (gh.mode == GHOST_EATEN) {
        int eyeOffX = DIR_DX[gh.dir] * 2;
        int eyeOffY = DIR_DY[gh.dir] * 2;
        for (int e = 0; e < 2; ++e) {
            int ex = gx + 2 + e * 6;
            int ey = gy + 3;
            for (int dy = 0; dy < 5; ++dy)
                for (int dx = 0; dx < 4; ++dx)
                    PacPutPixel(s, ex + dx, ey + dy, PAC_COL_EYE_WHITE);
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    PacPutPixel(s, ex + 1 + eyeOffX + dx, ey + 1 + eyeOffY + dy, PAC_COL_EYE_PUPIL);
        }
        return;
    }

    uint32_t bodyColor = gh.color;
    if (gh.mode == GHOST_FRIGHTENED) {
        bool flash = (s.frightenedTimer < 60) && ((s.frightenedTimer / 8) % 2 == 1);
        bodyColor = flash ? PAC_COL_WHITE : PAC_COL_BLUE_GHOST;
    }

    // Authentic 14x14 Ghost Body Bitmasks with Alternating Feet (Symmetric)
    static const uint16_t GHOST_BODY_F0[14] = {
        0x01E0, 0x07F8, 0x0FFC, 0x1FFE, 0x1FFE, 0x1FFE, 0x1FFE,
        0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x39E7, 0x31E3
    };
    static const uint16_t GHOST_BODY_F1[14] = {
        0x01E0, 0x07F8, 0x0FFC, 0x1FFE, 0x1FFE, 0x1FFE, 0x1FFE,
        0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x3333, 0x2331
    };

    int frame = (gh.animStep / 8) % 2;
    const uint16_t* bodyMask = (frame == 0) ? GHOST_BODY_F0 : GHOST_BODY_F1;

    for (int y = 0; y < 14; ++y) {
        uint16_t rowBits = bodyMask[y];
        for (int x = 0; x < 14; ++x) {
            if ((rowBits >> (13 - x)) & 1) {
                PacPutPixel(s, gx + x, gy + y, bodyColor);
            }
        }
    }

    // Directional Eyes & Pupils
    if (gh.mode != GHOST_FRIGHTENED) {
        int eY = 3;
        int pY = 4;
        int e1X = 2, e2X = 8;
        int p1X = 3, p2X = 9;

        switch (gh.dir) {
        case DIR_LEFT:
            e1X = 1; e2X = 7;
            p1X = 1; p2X = 7;
            break;
        case DIR_RIGHT:
            e1X = 3; e2X = 9;
            p1X = 5; p2X = 11;
            break;
        case DIR_UP:
            eY = 1; pY = 1;
            break;
        case DIR_DOWN:
            eY = 5; pY = 7;
            break;
        default:
            break;
        }

        for (int ey = 0; ey < 5; ++ey) {
            for (int ex = 0; ex < 4; ++ex) {
                PacPutPixel(s, gx + e1X + ex, gy + eY + ey, PAC_COL_EYE_WHITE);
                PacPutPixel(s, gx + e2X + ex, gy + eY + ey, PAC_COL_EYE_WHITE);
            }
        }
        for (int py = 0; py < 2; ++py) {
            for (int px = 0; px < 2; ++px) {
                PacPutPixel(s, gx + p1X + px, gy + pY + py, PAC_COL_EYE_PUPIL);
                PacPutPixel(s, gx + p2X + px, gy + pY + py, PAC_COL_EYE_PUPIL);
            }
        }
    } else {
        // Frightened Peach Eyes (2x2) & Wavy Mouth
        for (int dy = 0; dy < 2; ++dy) {
            for (int dx = 0; dx < 2; ++dx) {
                PacPutPixel(s, gx + 4 + dx, gy + 5 + dy, PAC_COL_PELLET);
                PacPutPixel(s, gx + 8 + dx, gy + 5 + dy, PAC_COL_PELLET);
            }
        }
        for (int x = 2; x <= 11; ++x) {
            int my = gy + 9 + (x % 2);
            PacPutPixel(s, gx + x, my, PAC_COL_PELLET);
        }
    }
}

// ============================================================================
// MAZE BOARD RENDERER
// Modified: Smooth 45-degree chamfered corners for the ultra-slim maze walls.
// ============================================================================
static void RenderPacmanBoard(PacmanState& s) {
    std::fill(s.fb.begin(), s.fb.end(), PAC_COL_BLACK);

    // 1. Top HUD
    PacDrawText(s, 24, 6, "1UP", PAC_COL_WHITE);
    PacDrawText(s, s.virtualW / 2 - 30, 6, "HIGH SCORE", PAC_COL_WHITE);
    PacDrawText(s, s.virtualW - 54, 6, "2UP", PAC_COL_WHITE);

    char scoreBuf[16];
    sprintf_s(scoreBuf, "%05d", s.score);
    PacDrawText(s, 24, 15, scoreBuf, PAC_COL_WHITE);

    sprintf_s(scoreBuf, "%05d", s.highScore);
    PacDrawText(s, s.virtualW / 2 - 15, 15, scoreBuf, PAC_COL_WHITE);

    // 2. Ultra-Spacious Smoothed Arcade Wall Rendering
    for (int ty = 3; ty < s.rows - 2; ++ty) {
        for (int tx = 0; tx < s.cols; ++tx) {
            uint8_t t = s.maze[ty * s.cols + tx];
            int px = tx * TILE_SIZE;
            int py = ty * TILE_SIZE;

            if (t == TILE_WALL) {
                auto isWall = [&](int ctx, int cty) { 
                    return GetTile(s, ctx, cty) == TILE_WALL; 
                };

                bool openN  = !isWall(tx,   ty-1);
                bool openS  = !isWall(tx,   ty+1);
                bool openW  = !isWall(tx-1, ty  );
                bool openE  = !isWall(tx+1, ty  );
                bool openNW = !isWall(tx-1, ty-1);
                bool openNE = !isWall(tx+1, ty-1);
                bool openSW = !isWall(tx-1, ty+1);
                bool openSE = !isWall(tx+1, ty+1);

                // Defines the tight 2-pixel wide solid skeleton mass with smooth chamfered edges
                auto isBody = [&](int x, int y) {
                    bool sx = (x >= 3 && x <= 4) || (!openW && x < 3) || (!openE && x > 4);
                    bool sy = (y >= 3 && y <= 4) || (!openN && y < 3) || (!openS && y > 4);
                    bool b = sx && sy;
                    
                    // Concave cutouts (open outer map paths completely erase corners)
                    if (x < 3 && y < 3 && openNW) b = false;
                    if (x > 4 && y < 3 && openNE) b = false;
                    if (x < 3 && y > 4 && openSW) b = false;
                    if (x > 4 && y > 4 && openSE) b = false;
                    
                    // Convex corner smoothing (shaves off the sharp outer tip of a wall bend)
                    bool gateE = (GetTile(s, tx + 1, ty) == TILE_GATE);
                    bool gateW = (GetTile(s, tx - 1, ty) == TILE_GATE);
                    if (openN && openW && !gateW && x == 3 && y == 3) b = false;
                    if (openN && openE && !gateE && x == 4 && y == 3) b = false;
                    if (openS && openW && !gateW && x == 3 && y == 4) b = false;
                    if (openS && openE && !gateE && x == 4 && y == 4) b = false;
                    
                    // Concave corner smoothing (fills in the sharp inner tip of a wall bend)
                    if (!openN && !openW && openNW && x == 2 && y == 2) b = true;
                    if (!openN && !openE && openNE && x == 5 && y == 2) b = true;
                    if (!openS && !openW && openSW && x == 2 && y == 5) b = true;
                    if (!openS && !openE && openSE && x == 5 && y == 5) b = true;

                    return b;
                };

                // Draw solid geometric perimeter
                for (int dy = 0; dy < 8; ++dy) {
                    for (int dx = 0; dx < 8; ++dx) {
                        if (isBody(dx, dy)) {
                            // Check 4-way neighbors; if any neighbor is empty space, this is a boundary pixel
                            if (!isBody(dx-1, dy) || !isBody(dx+1, dy) || 
                                !isBody(dx, dy-1) || !isBody(dx, dy+1)) {
                                PacPutPixel(s, px + dx, py + dy, PAC_COL_MAZE_BLUE);
                            }
                        }
                    }
                }

                // Connect inner corner lines and fill the inner chamfer (fixes the 3-pixel gap inside corners)
                if (!openS && !openE && openSE) {
                    PacPutPixel(s, px + 4, py + 4, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 5, py + 4, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 4, py + 5, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 5, py + 5, PAC_COL_MAZE_BLUE);
                }
                if (!openS && !openW && openSW) {
                    PacPutPixel(s, px + 3, py + 4, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 2, py + 4, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 3, py + 5, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 2, py + 5, PAC_COL_MAZE_BLUE);
                }
                if (!openN && !openE && openNE) {
                    PacPutPixel(s, px + 4, py + 3, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 5, py + 3, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 4, py + 2, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 5, py + 2, PAC_COL_MAZE_BLUE);
                }
                if (!openN && !openW && openNW) {
                    PacPutPixel(s, px + 3, py + 3, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 2, py + 3, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 3, py + 2, PAC_COL_MAZE_BLUE);
                    PacPutPixel(s, px + 2, py + 2, PAC_COL_MAZE_BLUE);
                }
            } else if (t == TILE_GATE) {
                // Ghost house gate stretches fully to meet the heavily-recessed walls without any gap
                for (int dx = -4; dx <= 11; ++dx) {
                    PacPutPixel(s, px + dx, py + 3, PAC_COL_GATE_PINK);
                    PacPutPixel(s, px + dx, py + 4, PAC_COL_GATE_PINK);
                }
            }
        }
    }

    // 3. Pellets and Power Pellets
    bool flashEnergizer = ((GetTickCount64() / 200) % 2 == 0);

    for (int y = 0; y < s.rows; ++y) {
        for (int x = 0; x < s.cols; ++x) {
            uint8_t t = GetTile(s, x, y);
            int px = x * TILE_SIZE;
            int py = y * TILE_SIZE;

            if (t == TILE_DOT) {
                PacPutPixel(s, px + 3, py + 3, PAC_COL_PELLET);
                PacPutPixel(s, px + 4, py + 3, PAC_COL_PELLET);
                PacPutPixel(s, px + 3, py + 4, PAC_COL_PELLET);
                PacPutPixel(s, px + 4, py + 4, PAC_COL_PELLET);
            } else if (t == TILE_POWER && flashEnergizer) {
                for (int dy = -3; dy <= 3; ++dy) {
                    for (int dx = -3; dx <= 3; ++dx) {
                        if (dx * dx + dy * dy <= 8) {
                            PacPutPixel(s, px + 4 + dx, py + 4 + dy, PAC_COL_PELLET);
                        }
                    }
                }
            }
        }
    }

    // 4. Bonus Cherry Fruit
    static const uint16_t CHERRY_G[12] = { 0x0300, 0x0480, 0x0840, 0x0823, 0x0414, 0x0208, 0x0110, 0, 0, 0, 0, 0 };
    static const uint16_t CHERRY_R[12] = { 0, 0, 0, 0, 0, 0, 0, 0x03B8, 0x07FC, 0x07FC, 0x03B8, 0 };

    if (s.fruitActive) {
        int fx = (int)s.fruitX - 2;
        int fy = (int)s.fruitY - 2;
        for (int y = 0; y < 12; ++y) {
            uint16_t gMask = CHERRY_G[y];
            uint16_t rMask = CHERRY_R[y];
            for (int x = 0; x < 12; ++x) {
                if ((gMask >> (11 - x)) & 1) PacPutPixel(s, fx + x, fy + y, PAC_COL_CHERRY_GRN);
                if ((rMask >> (11 - x)) & 1) PacPutPixel(s, fx + x, fy + y, PAC_COL_CHERRY_RED);
            }
        }
    }

    // 5. Render Ghosts
    for (int g = 0; g < 4; ++g) {
        RenderGhostSprite(s, s.ghosts[g]);
    }

    // 6. Render Pac-Man
    if (s.pacAlive) {
        RenderPacmanSprite(s, s.pacX, s.pacY, s.pacDir, s.mouthAnim);
    } else if (s.pacDeathTimer > 0) {
        int frame = (60 - s.pacDeathTimer) / 6;
        float r = (std::max)(1.0f, 6.0f - frame * 0.9f);
        float cx = s.pacX + 4.0f;
        float cy = s.pacY + 4.0f;
        for (int dy = -6; dy <= 6; ++dy)
            for (int dx = -6; dx <= 6; ++dx)
                if ((float)(dx * dx + dy * dy) <= r * r)
                    PacPutPixel(s, (int)(cx + dx), (int)(cy + dy), PAC_COL_YELLOW);
    }

    // 7. Score Popup (e.g. 200, 400, 800)
    if (s.popupTimer > 0) {
        char popBuf[16];
        sprintf_s(popBuf, "%d", s.popupScore);
        PacDrawText(s, (int)s.popupX - 6, (int)s.popupY - 2, popBuf, PAC_COL_CYAN);
    }

    // 8. Footer: Lives Counter & Round Fruit
    for (int l = 0; l < (std::min)(5, s.lives - 1); ++l) {
        RenderPacmanSprite(s, (float)(20 + l * 16), (float)(s.virtualH - 14), DIR_LEFT, 1);
    }
    int rx = s.virtualW - 24;
    int ry = s.virtualH - 18;
    for (int y = 0; y < 12; ++y) {
        uint16_t gMask = CHERRY_G[y];
        uint16_t rMask = CHERRY_R[y];
        for (int x = 0; x < 12; ++x) {
            if ((gMask >> (11 - x)) & 1) PacPutPixel(s, rx + x, ry + y, PAC_COL_CHERRY_GRN);
            if ((rMask >> (11 - x)) & 1) PacPutPixel(s, rx + x, ry + y, PAC_COL_CHERRY_RED);
        }
    }

    // 9. Banners
    if (s.gameOverTimer > 0) {
        PacDrawText(s, s.virtualW / 2 - 27, s.virtualH / 2 + 16, "GAME  OVER", PAC_COL_RED);
    } else if (s.waveClearTimer > 0) {
        PacDrawText(s, s.virtualW / 2 - 27, s.virtualH / 2 + 16, "BOARD CLEAR!", PAC_COL_YELLOW);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderPacman(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<PacmanState>(29);

    int vH = BASE_PAC_H;
    int vW = (int)(288.0f * (float)width / (float)height);
    vW = (vW / TILE_SIZE) * TILE_SIZE;
    if (vW < 288) vW = 288;
    if (vW > 640) vW = 640;

    int newCols = vW / TILE_SIZE;
    int newRows = vH / TILE_SIZE;

    if (!s.initialized || s.virtualW != vW || s.cols != newCols) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.cols = newCols;
        s.rows = newRows;
        s.fb.assign(vW * vH, PAC_COL_BLACK);
        s.lastTick = GetTickCount64();
        ResetPacmanGame(s);
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
        UpdatePacmanGame(s);
    }

    RenderPacmanBoard(s);

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

// Register as Screensaver ID 29
REGISTER_SCREENSAVER(
    29,
    L"Pac-Man",
    "pacman",
    { "pacman", "pac-man", "pac" },
    WRAP_LEGACY(RenderPacman),
    {}
);