#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cstdio>

struct SnakeParticle {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    COLORREF color;
    wchar_t ch;
};

struct SnakeState {
    bool initialized = false;
    int cols = 0;
    int rows = 0;
    int cellSize = 0;
    int offsetX = 0;
    int offsetY = 0;
    int theme = 0;
    int score = 0;
    int highScore = 0;
    int applesEaten = 0;
    int totalCells = 0;
    DWORD lastMoveTick = 0;
    DWORD winStartTime = 0;
    bool isWon = false;
    POINT food = { 0, 0 };
    int stuckFrames = 0;
    DWORD timeAccumulator = 0;
    std::vector<POINT> body;
    std::vector<int> cycle;
    std::vector<uint8_t> occupied;
    std::vector<SnakeParticle> particles;
};

// ---------------------------------------------------------------------------
// Simple Plain ASCII Self-Playing Snake (Hamiltonian Tour AI)
// ---------------------------------------------------------------------------

// Spanning-Tree Hamiltonian Cycle Construction on 2K x 2M Grid
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

// Food Spawning (Zero-allocation with linear probing)
static void SpawnFood(SnakeState& s) {
    if ((int)s.body.size() >= s.totalCells) {
        s.isWon = true;
        s.winStartTime = GetTickCount();
        return;
    }

    int start = rand() % s.totalCells;
    for (int i = 0; i < s.totalCells; ++i) {
        int idx = (start + i) % s.totalCells;
        if (!s.occupied[idx]) {
            s.food.x = idx % s.cols;
            s.food.y = idx / s.cols;
            return;
        }
    }

    s.isWon = true;
    s.winStartTime = GetTickCount();
}

// Reset Game State
static void ResetSnakeGame(SnakeState& s, int cols, int rows) {
    s.cols = cols;
    s.rows = rows;
    s.totalCells = cols * rows;
    s.score = 0;
    s.applesEaten = 0;
    s.isWon = false;
    s.winStartTime = 0;
    s.lastMoveTick = GetTickCount();
    s.stuckFrames = 0;
    s.timeAccumulator = 0;

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

// AI Decision: Hamiltonian path with safe shortcuts (Zero allocations)
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

// ---------------------------------------------------------------------------
// Render Routine: Minimalist Plain ASCII Snake (Lag-Free & Zero Heap Allocations)
// ---------------------------------------------------------------------------
void RenderSnake(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);
    int cw = tm.tmAveCharWidth;
    int ch = tm.tmHeight;
    if (cw <= 0 || ch <= 0) return;

    int termCols = width / cw;
    int termRows = height / ch;
    if (termCols < 15 || termRows < 10) return;

    // Layout:
    // Row 0: Simple Minimalist Top HUD
    // Row 1: Single separator line '-' across screen
    // Row 2 to termRows - 1: Full-screen playfield (screen itself is border)
    // In Consolas, char width cw is ~half height ch.
    // Each logical game cell is 2 chars wide (2 * cw) by 1 char tall (ch) to form an exact square,
    // exactly like the ASCII squares in Nyan Cat.
    int hudRow = 0;
    int sepRow = 1;
    int playTop = 2;

    int availRows = termRows - playTop;
    int availSquareCols = termCols / 2;

    int playCols = (availSquareCols / 2) * 2;
    int playRows = (availRows / 2) * 2;
    if (playCols < 8) playCols = 8;
    if (playRows < 8) playRows = 8;

    int playLeftChars = (termCols - playCols * 2) / 2;

    auto& s = data->GetCustomState<SnakeState>(27);
    if (!s.initialized || s.cols != playCols || s.rows != playRows) {
        ResetSnakeGame(s, playCols, playRows);
    }

    DWORD now = GetTickCount();

    // Reset upon win
    if (s.isWon && now - s.winStartTime > 2000) {
        ResetSnakeGame(s, playCols, playRows);
    }

    // Lag-free smooth movement with time accumulator
    if (!s.isWon) {
        DWORD elapsed = now - s.lastMoveTick;
        if (elapsed > 200) elapsed = 200; // prevent catchup surge
        s.lastMoveTick = now;
        s.timeAccumulator += elapsed;

        // Rock-solid 30ms step rate (matches ~30 FPS timer perfectly with zero frame drops)
        DWORD stepInterval = 30;

        int stepsRun = 0;
        while (s.timeAccumulator >= stepInterval && stepsRun < 2) {
            s.timeAccumulator -= stepInterval;
            stepsRun++;

            POINT nextHead = ChooseNextMove(s);
            if (nextHead.x != s.body.front().x || nextHead.y != s.body.front().y) {
                s.stuckFrames = 0;
                bool ateFood = (nextHead.x == s.food.x && nextHead.y == s.food.y);
                s.body.insert(s.body.begin(), nextHead);
                s.occupied[nextHead.y * s.cols + nextHead.x] = 1;

                if (ateFood) {
                    s.score += 10;
                    s.applesEaten++;
                    if (s.score > s.highScore) s.highScore = s.score;
                    SpawnFood(s);
                } else {
                    POINT oldTail = s.body.back();
                    s.occupied[oldTail.y * s.cols + oldTail.x] = 0;
                    s.body.pop_back();
                }
            } else {
                s.stuckFrames++;
                if (s.stuckFrames > 15) {
                    ResetSnakeGame(s, playCols, playRows);
                    break;
                }
            }
        }
    }

    // 1. Black Background
    HBRUSH hBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(memDC, &rect, hBlack);

    // Color Palette: Clean Terminal with Nyan Cat style colored ASCII square blocks
    static const COLORREF COL_BORDER    = RGB(90,  100, 115); // Separator Line
    static const COLORREF COL_TEXT      = RGB(180, 190, 205); // Clean HUD Text
    static const COLORREF COL_HEAD_BG   = RGB(255, 255, 255); // White Head Background
    static const COLORREF COL_HEAD_FG   = RGB(0,   0,   0);   // Black Head Text
    static const COLORREF COL_BODY_BG   = RGB(0,   200, 80);  // Terminal Green Body Background
    static const COLORREF COL_BODY_FG   = RGB(140, 255, 170); // Light Green Body Hash Glyph
    static const COLORREF COL_TAIL_BG   = RGB(0,   130, 50);  // Dim Green Tail Background
    static const COLORREF COL_TAIL_FG   = RGB(90,  240, 140); // Tail Dot Glyph
    static const COLORREF COL_FOOD_BG   = RGB(240, 30,  30);  // Bright Red Food Background
    static const COLORREF COL_FOOD_FG   = RGB(255, 240, 180); // Food Star Glyph

    // 2. Simple Minimalist Top HUD: "SCORE: 120   HIGH: 450   LENGTH: 16/800"
    SetBkMode(memDC, TRANSPARENT);
    char hudBuf[128];
    sprintf_s(hudBuf, " SCORE: %d    HIGH: %d    LENGTH: %d/%d",
              s.score, s.highScore, (int)s.body.size(), s.totalCells);
    SetTextColor(memDC, COL_TEXT);
    TextOutA(memDC, 0, hudRow * ch, hudBuf, (int)strlen(hudBuf));

    // 3. Single Separator Line between Header and Playfield (zero heap allocation)
    static char sepBuf[512];
    static bool sepInit = false;
    if (!sepInit) {
        memset(sepBuf, '-', sizeof(sepBuf));
        sepInit = true;
    }
    int sepLen = (termCols < (int)sizeof(sepBuf)) ? termCols : (int)sizeof(sepBuf);
    SetTextColor(memDC, COL_BORDER);
    TextOutA(memDC, 0, sepRow * ch, sepBuf, sepLen);

    // 4. Draw Food: 2-character ASCII Red Square ("**") like Nyan Cat
    if (!s.isWon) {
        SetBkMode(memDC, OPAQUE);
        SetBkColor(memDC, COL_FOOD_BG);
        SetTextColor(memDC, COL_FOOD_FG);
        char foodStr[2] = { '*', '*' };
        TextOutA(memDC, (playLeftChars + s.food.x * 2) * cw, (playTop + s.food.y) * ch, foodStr, 2);
    }

    // 5. Draw Snake: 2-char ASCII Squares (Head: White ">>"; Body: Green "##"; Tail: "::")
    size_t bodyLen = s.body.size();

    // Body segments (## in solid green square)
    if (bodyLen > 2) {
        SetBkMode(memDC, OPAQUE);
        SetBkColor(memDC, COL_BODY_BG);
        SetTextColor(memDC, COL_BODY_FG);
        char bodyStr[2] = { '#', '#' };
        for (size_t i = 1; i < bodyLen - 1; ++i) {
            const POINT& pt = s.body[i];
            TextOutA(memDC, (playLeftChars + pt.x * 2) * cw, (playTop + pt.y) * ch, bodyStr, 2);
        }
    }

    // Tail tip (.. in solid dark green square)
    if (bodyLen >= 2) {
        SetBkMode(memDC, OPAQUE);
        SetBkColor(memDC, COL_TAIL_BG);
        SetTextColor(memDC, COL_TAIL_FG);
        const POINT& tailPt = s.body.back();
        char tailStr[2] = { ':', ':' };
        TextOutA(memDC, (playLeftChars + tailPt.x * 2) * cw, (playTop + tailPt.y) * ch, tailStr, 2);
    }

    // Head (>> in solid white square)
    if (bodyLen >= 1) {
        SetBkMode(memDC, OPAQUE);
        SetBkColor(memDC, COL_HEAD_BG);
        SetTextColor(memDC, COL_HEAD_FG);
        const POINT& headPt = s.body.front();
        char headStr[2] = { '>', '>' };
        if (bodyLen >= 2) {
            int dx = headPt.x - s.body[1].x;
            int dy = headPt.y - s.body[1].y;
            if      (dx > 0) { headStr[0] = '>'; headStr[1] = '>'; }
            else if (dx < 0) { headStr[0] = '<'; headStr[1] = '<'; }
            else if (dy > 0) { headStr[0] = 'v'; headStr[1] = 'v'; }
            else if (dy < 0) { headStr[0] = '^'; headStr[1] = '^'; }
        }
        TextOutA(memDC, (playLeftChars + headPt.x * 2) * cw, (playTop + headPt.y) * ch, headStr, 2);
    }

    // 6. If won: Simple ASCII message
    if (s.isWon) {
        SetBkMode(memDC, TRANSPARENT);
        const char* winMsg = "=== BOARD COMPLETED! ===";
        SetTextColor(memDC, COL_HEAD_BG);
        int winX = (playLeftChars + (playCols * 2 - (int)strlen(winMsg)) / 2);
        int winY = playTop + playRows / 2;
        TextOutA(memDC, winX * cw, winY * ch, winMsg, (int)strlen(winMsg));
    }
}

REGISTER_SCREENSAVER(
    27,
    L"Self-Playing Snake",
    "snake",
    { "snake", "ouroboros" },
    WRAP_LEGACY(RenderSnake),
    {}
);
