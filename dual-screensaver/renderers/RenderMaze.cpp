#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/MazeSettings.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdint>

// ============================================================================
// VIRTUAL ARCADE RESOLUTION & COLOR PALETTE
// Retro Arcade Maze Simulation (Pure Maze inside Neon Box, Zero Text)
// Virtual height is fixed at 256; virtual width adapts dynamically to monitor
// aspect ratio filling the screen fullscreen with a sleek symmetrical margin.
// ============================================================================
static constexpr int BASE_ARCADE_H = 256;
static constexpr int MIN_ARCADE_W  = 256;
static constexpr int MAX_ARCADE_W  = 1280;
static constexpr int CELL_SIZE     = 7; // 7x7 pixels per maze cell
static constexpr int BOX_MARGIN    = 4; // Sleek 4px margin around screen edge

// 32-bit ARGB Retro Arcade Palette
static constexpr uint32_t COL_BLACK          = 0xFF050508;
static constexpr uint32_t COL_WHITE          = 0xFFFFFFFF;
static constexpr uint32_t COL_GOLD           = 0xFFFFD700;
static constexpr uint32_t COL_CRIMSON        = 0xFFFF1744;
static constexpr uint32_t COL_CYAN_NEON      = 0xFF00E5FF;
static constexpr uint32_t COL_CYAN_CORE      = 0xFF80D8FF;
static constexpr uint32_t COL_GREEN_EMERALD  = 0xFF00E676;
static constexpr uint32_t COL_FLOOR_VISITED  = 0xFF0A111E;
static constexpr uint32_t COL_FLOOR_DOT      = 0xFF142238;
static constexpr uint32_t COL_BORDER_OUTER   = 0xFF152040;
static constexpr uint32_t COL_BORDER_INNER   = 0xFF00E5FF;
static constexpr uint32_t COL_DEAD_END       = 0xFF5C2566;
static constexpr uint32_t COL_DEAD_END_CORE  = 0xFF8E24AA;

// ============================================================================
// SIMULATION DATA STRUCTURES
// ============================================================================
struct MazeSparkle {
    float x, y;
    float vx, vy;
    int life;
    int maxLife;
    uint32_t color;
};

struct MazeCellRetro {
    bool visited;
    bool wallTop, wallRight, wallBottom, wallLeft;
    bool solveVisited;
    bool inPath;
};

struct MazeStateRetro {
    int virtualW = 0;
    int virtualH = 0;
    std::vector<uint32_t> fb;

    int cols = 0;
    int rows = 0;
    int state = 0; // 0: reset, 1: generating, 2: solving, 3: solved celebration
    int start = 0;
    int end = 0;
    std::vector<MazeCellRetro> grid;
    std::vector<int> stack;
    std::vector<int> solveStack;

    int celebrationTimer = 0;
    uint64_t lastTick = 0;

    std::vector<MazeSparkle> sparkles;
    bool initialized = false;
};

// ============================================================================
// FAST LOW-LEVEL DRAWING PRIMITIVES
// ============================================================================
static inline void PutPixel(MazeStateRetro& s, int x, int y, uint32_t color) {
    if (x >= 0 && x < s.virtualW && y >= 0 && y < s.virtualH) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void DrawHLine(MazeStateRetro& s, int x1, int x2, int y, uint32_t color) {
    if (y < 0 || y >= s.virtualH) return;
    if (x1 > x2) std::swap(x1, x2);
    x1 = (std::max)(0, x1);
    x2 = (std::min)(s.virtualW - 1, x2);
    uint32_t* row = &s.fb[y * s.virtualW];
    for (int x = x1; x <= x2; ++x) {
        row[x] = color;
    }
}

static void DrawVLine(MazeStateRetro& s, int x, int y1, int y2, uint32_t color) {
    if (x < 0 || x >= s.virtualW) return;
    if (y1 > y2) std::swap(y1, y2);
    y1 = (std::max)(0, y1);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        s.fb[y * s.virtualW + x] = color;
    }
}

static void FillRect(MazeStateRetro& s, int x1, int y1, int x2, int y2, uint32_t color) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    x1 = (std::max)(0, x1);
    y1 = (std::max)(0, y1);
    x2 = (std::min)(s.virtualW - 1, x2);
    y2 = (std::min)(s.virtualH - 1, y2);
    for (int y = y1; y <= y2; ++y) {
        uint32_t* row = &s.fb[y * s.virtualW];
        for (int x = x1; x <= x2; ++x) {
            row[x] = color;
        }
    }
}

// Clean Double-Line Neon Arcade Box (Framing Screen with Sleek Uniform Margins)
static void DrawArcadeBox(MazeStateRetro& s) {
    int x1 = BOX_MARGIN;
    int y1 = BOX_MARGIN;
    int x2 = s.virtualW - 1 - BOX_MARGIN;
    int y2 = s.virtualH - 1 - BOX_MARGIN;

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

// Layout Calculation: Symmetrical Coordinates for Maze inside Box
static inline void GetMazeLayout(const MazeStateRetro& s, int& outLeft, int& outTop) {
    int courtLeft = BOX_MARGIN + 3;
    int courtRight = s.virtualW - 1 - BOX_MARGIN - 3;
    int courtWidth = courtRight - courtLeft + 1;
    outLeft = courtLeft + (courtWidth - s.cols * CELL_SIZE) / 2;

    int courtTop = BOX_MARGIN + 3;
    int courtBottom = s.virtualH - 1 - BOX_MARGIN - 3;
    int courtHeight = courtBottom - courtTop + 1;
    outTop = courtTop + (courtHeight - s.rows * CELL_SIZE) / 2;
}

// ============================================================================
// SIMULATION ENGINE: GENERATION & PATHFINDING
// ============================================================================
static void ResetMaze(MazeStateRetro& s) {
    int courtLeft = BOX_MARGIN + 3;
    int courtRight = s.virtualW - 1 - BOX_MARGIN - 3;
    int courtWidth = courtRight - courtLeft + 1;

    int courtTop = BOX_MARGIN + 3;
    int courtBottom = s.virtualH - 1 - BOX_MARGIN - 3;
    int courtHeight = courtBottom - courtTop + 1;

    s.cols = courtWidth / CELL_SIZE;
    s.rows = courtHeight / CELL_SIZE;

    if (s.cols < 4) s.cols = 4;
    if (s.rows < 4) s.rows = 4;

    int totalCells = s.cols * s.rows;
    s.grid.clear();
    s.grid.assign(totalCells, { false, true, true, true, true, false, false });

    s.stack.clear();
    s.solveStack.clear();
    s.sparkles.clear();

    // Select start and end nodes on opposite borders for maximum journey length
    s.start = rand() % s.cols; // Top row
    s.end = (s.rows - 1) * s.cols + (rand() % s.cols); // Bottom row

    s.stack.push_back(s.start);
    s.grid[s.start].visited = true;
    s.celebrationTimer = 0;
    s.state = 1; // Generating
}

static void StepMazeSimulation(MazeStateRetro& s) {
    // 1. Update Sparkles
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

    if (s.state == 0) {
        ResetMaze(s);
        return;
    }

    int courtLeft = 0, courtTop = 0;
    GetMazeLayout(s, courtLeft, courtTop);

    // 2. Generation Phase (Recursive Backtracker)
    if (s.state == 1) {
        int steps = (std::max)(1, (int)(g_MazeBuildSpeed * 1.5f));
        for (int st = 0; st < steps && !s.stack.empty(); ++st) {
            int current = s.stack.back();
            int cx = current % s.cols;
            int cy = current / s.cols;

            std::vector<int> neighbors, dirs;
            if (cy > 0 && !s.grid[current - s.cols].visited) { neighbors.push_back(current - s.cols); dirs.push_back(0); }
            if (cx < s.cols - 1 && !s.grid[current + 1].visited) { neighbors.push_back(current + 1); dirs.push_back(1); }
            if (cy < s.rows - 1 && !s.grid[current + s.cols].visited) { neighbors.push_back(current + s.cols); dirs.push_back(2); }
            if (cx > 0 && !s.grid[current - 1].visited) { neighbors.push_back(current - 1); dirs.push_back(3); }

            if (!neighbors.empty()) {
                int r = rand() % (int)neighbors.size();
                int next = neighbors[r];
                int dir = dirs[r];

                if (dir == 0) { s.grid[current].wallTop = false; s.grid[next].wallBottom = false; }
                else if (dir == 1) { s.grid[current].wallRight = false; s.grid[next].wallLeft = false; }
                else if (dir == 2) { s.grid[current].wallBottom = false; s.grid[next].wallTop = false; }
                else if (dir == 3) { s.grid[current].wallLeft = false; s.grid[next].wallRight = false; }

                s.grid[next].visited = true;
                s.stack.push_back(next);

                // Spurt carving sparkle
                if (s.sparkles.size() < 24 && (rand() % 4 == 0)) {
                    float px = (float)(courtLeft + (next % s.cols) * CELL_SIZE + CELL_SIZE / 2);
                    float py = (float)(courtTop + (next / s.cols) * CELL_SIZE + CELL_SIZE / 2);
                    float vx = ((rand() % 100) - 50) * 0.02f;
                    float vy = ((rand() % 100) - 50) * 0.02f;
                    s.sparkles.push_back({ px, py, vx, vy, 0, 10, COL_GOLD });
                }
            } else {
                s.stack.pop_back();
            }
        }

        if (s.stack.empty()) {
            s.state = 2; // Transition to solving
            s.solveStack.push_back(s.start);
            s.grid[s.start].solveVisited = true;
            s.grid[s.start].inPath = true;
        }
    }
    // 3. Solving Phase (Depth-First Search Navigator)
    else if (s.state == 2) {
        int steps = (std::max)(1, (int)(g_MazeSolveSpeed * 1.0f));
        for (int st = 0; st < steps && !s.solveStack.empty(); ++st) {
            int current = s.solveStack.back();

            if (current == s.end) {
                s.state = 3; // Solved!
                s.celebrationTimer = 90; // ~1.5s pause to admire

                // Spray burst of celebration sparks around exit
                float ex = (float)(courtLeft + (s.end % s.cols) * CELL_SIZE + CELL_SIZE / 2);
                float ey = (float)(courtTop + (s.end / s.cols) * CELL_SIZE + CELL_SIZE / 2);
                for (int sp = 0; sp < 28; sp++) {
                    float angle = (float)(rand() % 628) / 100.0f;
                    float speed = 0.5f + ((rand() % 150) / 100.0f);
                    s.sparkles.push_back({ ex, ey, cosf(angle) * speed, sinf(angle) * speed, 0, 24, (rand() % 2 == 0) ? COL_GOLD : COL_GREEN_EMERALD });
                }
                break;
            }

            int cx = current % s.cols;
            int cy = current / s.cols;

            std::vector<int> nextDirs;
            if (!s.grid[current].wallTop && cy > 0 && !s.grid[current - s.cols].solveVisited) nextDirs.push_back(current - s.cols);
            if (!s.grid[current].wallRight && cx < s.cols - 1 && !s.grid[current + 1].solveVisited) nextDirs.push_back(current + 1);
            if (!s.grid[current].wallBottom && cy < s.rows - 1 && !s.grid[current + s.cols].solveVisited) nextDirs.push_back(current + s.cols);
            if (!s.grid[current].wallLeft && cx > 0 && !s.grid[current - 1].solveVisited) nextDirs.push_back(current - 1);

            if (!nextDirs.empty()) {
                int next = nextDirs[0];
                s.grid[next].solveVisited = true;
                s.grid[next].inPath = true;
                s.solveStack.push_back(next);
            } else {
                s.grid[current].inPath = false;
                s.solveStack.pop_back();
            }
        }
    }
    // 4. Celebration / Solved State
    else if (s.state == 3) {
        s.celebrationTimer--;
        if (s.celebrationTimer <= 0) {
            s.state = 0; // Trigger reset on next frame
        }
    }
}

// ============================================================================
// COMPLETE RETRO ARCADE FRAME RENDERING (Pure Maze in Symmetrical Box, No Text)
// ============================================================================
static void RenderArcadeFrame(MazeStateRetro& s) {
    // 1. Clear Framebuffer to Obsidian Black
    std::fill(s.fb.begin(), s.fb.end(), COL_BLACK);

    // 2. Playfield Arena Double-Line Neon Box
    DrawArcadeBox(s);

    int courtLeft = 0, courtTop = 0;
    GetMazeLayout(s, courtLeft, courtTop);

    // 3. Render Maze Grid Cells & Walls
    for (int cy = 0; cy < s.rows; ++cy) {
        for (int cx = 0; cx < s.cols; ++cx) {
            int i = cy * s.cols + cx;
            int px = courtLeft + cx * CELL_SIZE;
            int py = courtTop + cy * CELL_SIZE;

            const auto& cell = s.grid[i];

            // Floor background
            if (cell.visited) {
                FillRect(s, px, py, px + CELL_SIZE - 1, py + CELL_SIZE - 1, COL_FLOOR_VISITED);
                // Subtle center floor tech pin
                PutPixel(s, px + CELL_SIZE / 2, py + CELL_SIZE / 2, COL_FLOOR_DOT);
            }

            // Pathfinding energy trail (Active Solution vs Backtracked Dead-Ends)
            if (cell.inPath) {
                // Steady Golden Solution Conduit
                FillRect(s, px + 2, py + 2, px + CELL_SIZE - 3, py + CELL_SIZE - 3, COL_GOLD);
                PutPixel(s, px + CELL_SIZE / 2, py + CELL_SIZE / 2, COL_WHITE);
            } else if (cell.solveVisited) {
                // Steady Calm Violet Dead-End Trail (No flickering)
                FillRect(s, px + 2, py + 2, px + CELL_SIZE - 3, py + CELL_SIZE - 3, COL_DEAD_END);
                PutPixel(s, px + CELL_SIZE / 2, py + CELL_SIZE / 2, COL_DEAD_END_CORE);
            }

            // High-Tech Cyber Neon Walls (Cyan with Corner Nodes)
            if (cell.visited) {
                if (cell.wallTop)    DrawHLine(s, px, px + CELL_SIZE - 1, py, COL_CYAN_NEON);
                if (cell.wallBottom) DrawHLine(s, px, px + CELL_SIZE - 1, py + CELL_SIZE - 1, COL_CYAN_NEON);
                if (cell.wallLeft)   DrawVLine(s, px, py, py + CELL_SIZE - 1, COL_CYAN_NEON);
                if (cell.wallRight)  DrawVLine(s, px + CELL_SIZE - 1, py, py + CELL_SIZE - 1, COL_CYAN_NEON);

                // Bright glowing corner nodes
                PutPixel(s, px, py, COL_CYAN_CORE);
                PutPixel(s, px + CELL_SIZE - 1, py, COL_CYAN_CORE);
                PutPixel(s, px, py + CELL_SIZE - 1, COL_CYAN_CORE);
                PutPixel(s, px + CELL_SIZE - 1, py + CELL_SIZE - 1, COL_CYAN_CORE);
            }
        }
    }

    // 4. Render Start & Exit Warp Gates
    if (s.cols > 0 && s.rows > 0) {
        // Start Gate: Neon Emerald Portal
        int sx = courtLeft + (s.start % s.cols) * CELL_SIZE;
        int sy = courtTop + (s.start / s.cols) * CELL_SIZE;
        FillRect(s, sx + 1, sy + 1, sx + CELL_SIZE - 2, sy + CELL_SIZE - 2, COL_GREEN_EMERALD);
        PutPixel(s, sx + CELL_SIZE / 2, sy + CELL_SIZE / 2, COL_WHITE);

        // Exit Gate: Neon Crimson Beacon
        int ex = courtLeft + (s.end % s.cols) * CELL_SIZE;
        int ey = courtTop + (s.end / s.cols) * CELL_SIZE;
        FillRect(s, ex + 1, ey + 1, ex + CELL_SIZE - 2, ey + CELL_SIZE - 2, COL_CRIMSON);
        PutPixel(s, ex + CELL_SIZE / 2, ey + CELL_SIZE / 2, COL_WHITE);
    }

    // 5. Active Beacon Head (Generator Carver or Solver Drone)
    if (s.state == 1 && !s.stack.empty()) {
        int curr = s.stack.back();
        int hx = courtLeft + (curr % s.cols) * CELL_SIZE;
        int hy = courtTop + (curr / s.cols) * CELL_SIZE;
        FillRect(s, hx + 1, hy + 1, hx + CELL_SIZE - 2, hy + CELL_SIZE - 2, COL_GOLD);
        PutPixel(s, hx + CELL_SIZE / 2, hy + CELL_SIZE / 2, COL_WHITE);
    } else if (s.state == 2 && !s.solveStack.empty()) {
        int curr = s.solveStack.back();
        int hx = courtLeft + (curr % s.cols) * CELL_SIZE;
        int hy = courtTop + (curr / s.cols) * CELL_SIZE;
        FillRect(s, hx + 1, hy + 1, hx + CELL_SIZE - 2, hy + CELL_SIZE - 2, COL_CYAN_NEON);
        PutPixel(s, hx + CELL_SIZE / 2, hy + CELL_SIZE / 2, COL_WHITE);
    }

    // 6. Sparkle Particles
    for (const auto& sp : s.sparkles) {
        PutPixel(s, (int)sp.x, (int)sp.y, sp.color);
    }
}

// ============================================================================
// MAIN SCREENSAVER ENTRY POINT
// ============================================================================
void RenderMaze(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<MazeStateRetro>(10);

    int vH = BASE_ARCADE_H;
    int vW = (int)(256.0f * (float)width / (float)height);
    if (vW < MIN_ARCADE_W) vW = MIN_ARCADE_W;
    if (vW > MAX_ARCADE_W) vW = MAX_ARCADE_W;
    if (vW % 2 != 0) vW++; // Ensure even width for crisp DIB row alignment

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

    // Single step per frame prevents rapid multi-step strobing and jitter
    StepMazeSimulation(s);

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

// Register as Screensaver ID 10
REGISTER_SCREENSAVER(
    10,
    L"Maze Generator",
    "maze",
    { "maze" },
    WRAP_LEGACY(RenderMaze),
    GetMazeSettings()
);