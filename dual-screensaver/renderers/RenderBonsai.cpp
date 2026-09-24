#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "settings/BonsaiSettings.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// --- Color Constants & Palettes ---
static const COLORREF COL_BOX        = RGB(200, 200, 200);
static const COLORREF COL_SOIL       = RGB(0, 200, 0);
static const COLORREF COL_TREE_BASE  = RGB(255, 255, 0);

// Rich vibrant green moss/soil palette for pot surface
static const COLORREF PAL_SOIL[] = {
    RGB(0, 180, 0),     // Vibrant grass green
    RGB(0, 220, 50),    // Bright fresh moss
    RGB(30, 240, 60),   // Vivid lime/spring highlight
    RGB(0, 150, 0),     // Deep moss green
    RGB(40, 210, 70),   // Lush foliage green
    RGB(0, 245, 120)    // Bright terminal spring green
};

// Foliage Theme Palettes (Classic Spring, Sakura, Autumn, Ginkgo)
static const COLORREF PAL_SPRING[] = {
    RGB(0, 160, 0), RGB(0, 210, 0), RGB(30, 240, 50), RGB(0, 120, 0)
};
static const COLORREF PAL_SAKURA[] = {
    RGB(255, 182, 193), RGB(255, 135, 180), RGB(255, 220, 235), RGB(255, 105, 160)
};
static const COLORREF PAL_AUTUMN[] = {
    RGB(230, 60, 30), RGB(255, 130, 20), RGB(245, 190, 40), RGB(190, 40, 25)
};
static const COLORREF PAL_GINKGO[] = {
    RGB(255, 215, 0), RGB(240, 175, 20), RGB(255, 240, 120), RGB(215, 150, 15)
};

static const char BRANCH_CHARS[] = "~;:=";
static const char LEAF_CHARS[]   = "&%#@";

// Math Helpers matching Python random.normalvariate and uniform
static float RandNormal(float mean, float stdDev) {
    float u1 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
    float u2 = ((float)rand() + 1.0f) / ((float)RAND_MAX + 1.0f);
    float z0 = sqrtf(-2.0f * logf(u1)) * cosf(6.2831853f * u2);
    return mean + z0 * stdDev;
}

static float RandUniform(float minVal, float maxVal) {
    return minVal + ((float)rand() / (float)RAND_MAX) * (maxVal - minVal);
}

static COLORREF GetBarkColor(int theme) {
    switch (theme) {
        case 1: // Sakura
            return RGB(160 + rand() % 50, 110 + rand() % 50, 60 + rand() % 40);
        case 2: // Autumn
            return RGB(170 + rand() % 50, 100 + rand() % 50, 30 + rand() % 40);
        case 3: // Ginkgo
            return RGB(160 + rand() % 40, 130 + rand() % 40, 70 + rand() % 40);
        case 0: // Classic Spring Green (matching Python BRANCH_COLOUR range: 200..255, 150..255, 0)
        default:
            return RGB(200 + rand() % 56, 150 + rand() % 106, 0);
    }
}

static COLORREF GetLeafColor(int theme) {
    switch (theme) {
        case 1: return PAL_SAKURA[rand() % 4];
        case 2: return PAL_AUTUMN[rand() % 4];
        case 3: return PAL_GINKGO[rand() % 4];
        case 0: // Classic Green matching Python: (0, randint(75, 255), 0)
        default:
            return RGB(0, 75 + rand() % 181, 0);
    }
}

struct BonsaiCell {
    char ch = ' ';
    COLORREF color = 0;
};

struct BonsaiDrawCell {
    int x;
    int y;
    char ch;
    COLORREF color;
};

struct BonsaiPetal {
    float x, y;
    float vx, vy;
    float phase;
    char ch;
    COLORREF color;
};

struct BonsaiOptions {
    int type = 0; // 0: Classic, 1: Fibonacci, 2: OffsetFib, 3: RandomOffsetFib
    int num_layers = 8;
    float initial_len = 15.0f;
    float angle_mean = 40.0f * (3.14159265f / 180.0f);
    int leaf_len = 4;
    int theme = 0;
};

struct BonsaiState {
    bool initialized = false;
    DWORD lastTick = 0;
    int widthInChars = 0;
    int heightInChars = 0;
    int state = 0; // 0: growing, 1: mature/petals, 2: renew
    DWORD stateStartTime = 0;
    int treeType = 0;
    int theme = 0;
    std::vector<BonsaiCell> grid;
    std::vector<BonsaiDrawCell> drawQueue;
    size_t drawIndex = 0;
    std::vector<BonsaiPetal> petals;
    std::vector<POINT> leaves;
};

// Generator Context for procedural Python tree models
struct GeneratorContext {
    BonsaiOptions options;
    int cols = 80;
    int rows = 25;
    std::vector<std::vector<int>> branch_nums;
    std::vector<BonsaiDrawCell> queue;
    std::vector<POINT> leaves;

    void RecordCell(int x, int y, char ch, COLORREF col) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            BonsaiDrawCell dc;
            dc.x = x;
            dc.y = y;
            dc.ch = ch;
            dc.color = col;
            queue.push_back(dc);
        }
    }

    void ConstrainBranch(float x, float y, float& length, float& theta) {
        // Enforce that branches gracefully stay within the display without flat-cutting
        float max_plane_y = ((float)rows - 3.0f) * 2.0f; // Headroom for canopy dome
        float target_y = y + length * cosf(theta);

        if (target_y > max_plane_y) {
            // Rather than hard-truncating into an unnatural flat line,
            // gracefully arch the branch outward horizontally like a natural bonsai
            float sign = (sinf(theta) >= 0.0f) ? 1.0f : -1.0f;
            theta = theta + sign * 0.45f;
            float avail_y = (std::max)(1.0f, max_plane_y - y);
            if (cosf(theta) > 0.1f) {
                length = (std::min)(length, avail_y / cosf(theta));
            } else {
                length *= 0.85f;
            }
        }

        // Horizontal screen boundaries: [3, cols - 4]
        float target_x = x + length * sinf(theta);
        if (target_x < 3.0f) {
            if (sinf(theta) < 0.0f) {
                theta = fabsf(theta) * 0.4f; // Deflect back toward center
                float avail = x - 3.0f;
                if (avail > 0.0f) length = (std::min)(length, avail * 1.5f);
            }
        } else if (target_x > (float)cols - 4.0f) {
            if (sinf(theta) > 0.0f) {
                theta = -fabsf(theta) * 0.4f; // Deflect back toward center
                float avail = (float)cols - 4.0f - x;
                if (avail > 0.0f) length = (std::min)(length, avail * 1.5f);
            }
        }
    }

    void DrawLine(float x1, float y1, float x2, float y2, int width) {
        if (width < 1) width = 1;
        bool is_vertical = (fabsf(x1 - x2) < 0.0001f);
        float m = 0.0f, c = 0.0f;
        if (is_vertical) {
            c = x1;
        } else {
            m = (y2 - y1) / (x2 - x1);
            c = y1 - m * x1;
        }

        // Python get_line_char: upper = 60 deg, lower = 30 deg
        char line_char = '|';
        if (is_vertical) {
            line_char = '|';
        } else {
            float theta = atanf(m);
            float upper = 1.5707963f * (2.0f / 3.0f);
            float lower = 1.5707963f * (1.0f / 3.0f);
            if (fabsf(theta) > upper) line_char = '|';
            else if (fabsf(theta) < lower) line_char = '_';
            else if (theta > 0.0f) line_char = '/';
            else line_char = '\\';
        }

        bool is_steep = is_vertical || (fabsf(m) >= 1.0f);

        if (is_steep) {
            int start_row = (int)roundf((float)rows - y1 / 2.0f);
            int end_row   = (int)roundf((float)rows - y2 / 2.0f);
            int step = (end_row >= start_row) ? 1 : -1;

            for (int r = start_row; r != end_row + step; r += step) {
                if (r < 0 || r >= rows) continue;
                float plane_y = ((float)rows - (float)r) * 2.0f;
                float desired_x = is_vertical ? c : ((plane_y - c) / m);

                int cx = (int)roundf(desired_x);
                for (int w = 0; w < width; ++w) {
                    int col = cx;
                    if (width == 2) {
                        col = (w == 0) ? (int)floorf(desired_x) : (int)ceilf(desired_x);
                        if (col == cx && w == 1) col++;
                    } else if (width > 2) {
                        col = cx - width / 2 + w;
                    }

                    if (col >= 0 && col < cols) {
                        char ch = ((rand() % 100) < 30) ? BRANCH_CHARS[rand() % 4] : line_char;
                        COLORREF colr = GetBarkColor(options.theme);
                        RecordCell(col, r, ch, colr);
                    }
                }
            }
        } else {
            int start_col = (int)roundf(x1);
            int end_col   = (int)roundf(x2);
            int step = (end_col >= start_col) ? 1 : -1;

            // Terminal fonts are ~2x taller than wide. Scale shallow vertical row thickness
            // so horizontal branches don't visually dwarf the vertical trunk.
            int eff_width = (width > 1) ? ((width + 1) / 2) : 1;

            for (int col = start_col; col != end_col + step; col += step) {
                if (col < 0 || col >= cols) continue;
                float plane_x = (float)col;
                float desired_y = m * plane_x + c;
                float desired_row = (float)rows - desired_y / 2.0f;

                int ry = (int)roundf(desired_row);
                for (int w = 0; w < eff_width; ++w) {
                    int r = ry;
                    if (eff_width == 2) {
                        r = (w == 0) ? (int)floorf(desired_row) : (int)ceilf(desired_row);
                        if (r == ry && w == 1) r++;
                    } else if (eff_width > 2) {
                        r = ry - eff_width / 2 + w;
                    }

                    if (r >= 0 && r < rows) {
                        char ch = ((rand() % 100) < 30) ? BRANCH_CHARS[rand() % 4] : line_char;
                        COLORREF colr = GetBarkColor(options.theme);
                        RecordCell(col, r, ch, colr);
                    }
                }
            }
        }
    }

    void GenerateLeaves(float branch_x, float branch_y) {
        // Python Leaves.draw(): 4 leaves per cluster with downward gravity droop
        for (int l = 0; l < 4; ++l) {
            float vx = RandUniform(-1.0f, 1.0f);
            float vy = RandUniform(-1.0f, 1.0f);
            float mag = sqrtf(vx * vx + vy * vy);
            if (mag > 0.001f) { vx /= mag; vy /= mag; }
            else { vx = 0.0f; vy = 1.0f; }

            float px = branch_x;
            float py = branch_y;
            float gx = 0.0f;
            float gy = -1.0f; // Gravity pulling downward in plane

            for (int i = 0; i < options.leaf_len; ++i) {
                px += vx;
                py += vy;

                int sc_x = (int)roundf(px);
                int sc_y = (int)roundf((float)rows - py / 2.0f);

                // Ensure foliage stays within screen grid
                if (sc_x >= 0 && sc_x < cols && sc_y >= 0 && sc_y < rows - 4) {
                    char ch = LEAF_CHARS[rand() % 4];
                    COLORREF col = GetLeafColor(options.theme);
                    RecordCell(sc_x, sc_y, ch, col);

                    POINT pt = { sc_x, sc_y };
                    leaves.push_back(pt);
                }

                float weight = (float)i / (float)options.leaf_len;
                vx += gx * weight;
                vy += gy * weight;
            }
        }
    }
};

// Type 0: ClassicTree
static void GenerateClassicBranch(GeneratorContext& ctx, float x, float y, int layer, float length, int width, float theta) {
    if (layer >= ctx.options.num_layers) {
        ctx.GenerateLeaves(x, y);
        return;
    }

    ctx.ConstrainBranch(x, y, length, theta);

    float end_x = x + length * sinf(theta);
    float end_y = y + length * cosf(theta);

    ctx.DrawLine(x, y, end_x, end_y, width);

    int sign = 1;
    int num_branches = (int)roundf(RandNormal(2.0f, 0.5f));
    if (num_branches < 0) num_branches = 0;

    float step = (num_branches != 0) ? (length / (float)num_branches) : 0.0f;
    int new_width = (std::max)(1, width - 1);
    float new_length = length * 0.75f;
    float angle_std = 8.0f * (3.14159265f / 180.0f);

    for (int i = 0; i < num_branches; ++i) {
        float dist_up = (float)(i + 1) * step;
        float new_theta = theta + (float)sign * RandNormal(ctx.options.angle_mean, angle_std);
        float bx = x + dist_up * sinf(theta);
        float by = y + dist_up * cosf(theta);

        GenerateClassicBranch(ctx, bx, by, layer + 1, new_length, new_width, new_theta);
        sign = -sign;
    }
}

// Types 1, 2, 3: Fibonacci, Offset Fibonacci, Random Offset Fibonacci
static void GenerateFibBranch(GeneratorContext& ctx, float x, float y, int layer_inx, int branch_inx, float length, int width, float theta) {
    if (layer_inx > ctx.options.num_layers) {
        ctx.GenerateLeaves(x, y);
        return;
    }

    ctx.ConstrainBranch(x, y, length, theta);

    float end_x = x + length * sinf(theta);
    float end_y = y + length * cosf(theta);

    ctx.DrawLine(x, y, end_x, end_y, width);

    int num_branches = 1;
    if (layer_inx < (int)ctx.branch_nums.size()) {
        int idx = branch_inx % (int)ctx.branch_nums[layer_inx].size();
        num_branches = ctx.branch_nums[layer_inx][idx];
    }

    int new_width = (std::max)(1, width - 1);
    float new_length = length * 0.75f;
    float angle_std = 8.0f * (3.14159265f / 180.0f);
    int sign = 1;

    if (ctx.options.type == 1) {
        // Type 1: FibonacciTree - branches grow from the TIP of parent
        for (int i = 0; i < num_branches; ++i) {
            float angle = RandNormal(ctx.options.angle_mean, angle_std);
            float new_theta = theta + (float)sign * angle;
            GenerateFibBranch(ctx, end_x, end_y, layer_inx + 1, branch_inx + i, new_length, new_width, new_theta);
            sign = -sign;
        }
    } else if (ctx.options.type == 2) {
        // Type 2: OffsetFibTree - branches linearly distributed along parent
        float step = (num_branches != 0) ? (length / (float)num_branches) : 0.0f;
        for (int i = 0; i < num_branches; ++i) {
            float dist_up = (float)(i + 1) * step;
            float angle = RandNormal(ctx.options.angle_mean, angle_std);
            float new_theta = theta + (float)sign * angle;
            float bx = x + dist_up * sinf(theta);
            float by = y + dist_up * cosf(theta);
            GenerateFibBranch(ctx, bx, by, layer_inx + 1, branch_inx + i, new_length, new_width, new_theta);
            sign = -sign;
        }
    } else if (ctx.options.type == 3) {
        // Type 3: RandomOffsetFibTree - branches randomly placed along parent or at end
        bool need_leaves = true;
        for (int i = 0; i < num_branches; ++i) {
            bool grow_at_end = (((float)rand() / (float)RAND_MAX) < 0.5f);
            float dist_up;
            if (grow_at_end) {
                need_leaves = false;
                dist_up = length;
            } else {
                dist_up = RandUniform(length * 0.3f, length * 0.9f);
            }
            float angle = RandNormal(ctx.options.angle_mean, angle_std);
            float new_theta = theta + (float)sign * angle;
            float bx = x + dist_up * sinf(theta);
            float by = y + dist_up * cosf(theta);
            GenerateFibBranch(ctx, bx, by, layer_inx + 1, branch_inx + i, new_length, new_width, new_theta);
            sign = -sign;
        }
        if (need_leaves) {
            ctx.GenerateLeaves(end_x, end_y);
        }
    }
}

// Generate Fibonacci branch allocation matrix (Python FibonacciTree.generate_branch_nums)
static std::vector<std::vector<int>> GenerateBranchNumsMatrix(int layers) {
    std::vector<int> fib = { 1, 1 };
    for (int i = 0; i < layers + 2; ++i) {
        fib.push_back(fib[fib.size() - 1] + fib[fib.size() - 2]);
    }

    std::vector<std::vector<int>> branch_nums = { { 1 } };
    for (int i = 0; i < layers; ++i) {
        int num_branches = fib[i + 2];
        int num_parents = 0;
        for (int n : branch_nums.back()) num_parents += n;
        if (num_parents <= 0) num_parents = 1;

        int base = num_branches / num_parents;
        int diff = num_branches - base * num_parents;
        std::vector<int> current_nums;
        for (int x = 0; x < num_parents; ++x) {
            current_nums.push_back((x < diff) ? (base + 1) : base);
        }
        // Shuffle
        for (int j = (int)current_nums.size() - 1; j > 0; --j) {
            int k = rand() % (j + 1);
            std::swap(current_nums[j], current_nums[k]);
        }
        branch_nums.push_back(current_nums);
    }
    return branch_nums;
}

static int s_primaryBonsaiType = -1;
static int s_primaryBonsaiTheme = -1;
static int s_secondaryBonsaiType = -1;
static int s_secondaryBonsaiTheme = -1;

static void StartNewBonsai(BonsaiState& s, int cols, int rows, bool isPrimary) {
    s.widthInChars = cols;
    s.heightInChars = rows;
    s.grid.assign(cols * rows, BonsaiCell());
    s.drawQueue.clear();
    s.drawIndex = 0;
    s.petals.clear();
    s.leaves.clear();

    // Determine tree type according to settings and enabled pool
    if (g_BonsaiType >= 0 && g_BonsaiType <= 3) {
        s.treeType = g_BonsaiType;
    } else {
        std::vector<int> pool;
        if (g_BonsaiTypeClassic) pool.push_back(0);
        if (g_BonsaiTypeFibonacci) pool.push_back(1);
        if (g_BonsaiTypeOffsetFib) pool.push_back(2);
        if (g_BonsaiTypeRndOffsetFib) pool.push_back(3);
        if (pool.empty()) {
            for (int i = 0; i < 4; ++i) pool.push_back(i);
        }

        // On dual screens, avoid picking the same tree type as the other monitor
        int otherType = isPrimary ? s_secondaryBonsaiType : s_primaryBonsaiType;
        if (pool.size() > 1 && otherType >= 0) {
            std::vector<int> filtered;
            for (int t : pool) {
                if (t != otherType) filtered.push_back(t);
            }
            if (!filtered.empty()) {
                s.treeType = filtered[rand() % filtered.size()];
            } else {
                s.treeType = pool[rand() % pool.size()];
            }
        } else {
            s.treeType = pool[rand() % pool.size()];
        }
    }

    if (isPrimary) s_primaryBonsaiType = s.treeType;
    else s_secondaryBonsaiType = s.treeType;

    // Random theme: 0=Classic Spring, 1=Sakura, 2=Autumn, 3=Ginkgo
    // Ensure both screens have contrasting seasonal color themes
    int otherTheme = isPrimary ? s_secondaryBonsaiTheme : s_primaryBonsaiTheme;
    if (otherTheme >= 0) {
        s.theme = (otherTheme + 1 + (rand() % 3)) % 4;
    } else {
        s.theme = rand() % 4;
    }

    if (isPrimary) s_primaryBonsaiTheme = s.theme;
    else s_secondaryBonsaiTheme = s.theme;

    auto putGridCell = [&](int x, int y, char c, COLORREF col) {
        if (x >= 0 && x < cols && y >= 0 && y < rows) {
            s.grid[y * cols + x].ch = c;
            s.grid[y * cols + x].color = col;
        }
    };

    // 1. Draw Pot / Box (matching cbonsai base art, with detailed green moss/soil)
    int pot_row = rows - 4;
    int root_inx1 = pot_row;
    int root_inx2 = cols / 2;

    int pot_width = 31;
    if (cols < 35) {
        pot_width = (std::max)(17, cols - 4);
        if (pot_width % 2 == 0) pot_width--;
    }

    int dirt_len = (pot_width - 9) / 2; // 11 when pot_width == 31
    int left_col = root_inx2 - (dirt_len + 4);
    int right_col = root_inx2 + (dirt_len + 4);

    // Row 0: Pot rim, detailed textured green soil/moss, and tree trunk anchor
    putGridCell(left_col, root_inx1, ':', COL_BOX);
    putGridCell(right_col, root_inx1, ':', COL_BOX);

    // Detailed soil texture with natural mix of ground, grains, and mounds in lush green tones
    static const char SOIL_TEXTURE[] = "____~~~--..,";
    for (int x = left_col + 1; x < root_inx2 - 3; ++x) {
        char ch = SOIL_TEXTURE[rand() % (sizeof(SOIL_TEXTURE) - 1)];
        if ((rand() % 100) < 5) ch = '*';
        COLORREF col = PAL_SOIL[rand() % (sizeof(PAL_SOIL) / sizeof(PAL_SOIL[0]))];
        putGridCell(x, root_inx1, ch, col);
    }
    for (int x = root_inx2 + 4; x < right_col; ++x) {
        char ch = SOIL_TEXTURE[rand() % (sizeof(SOIL_TEXTURE) - 1)];
        if ((rand() % 100) < 5) ch = '*';
        COLORREF col = PAL_SOIL[rand() % (sizeof(PAL_SOIL) / sizeof(PAL_SOIL[0]))];
        putGridCell(x, root_inx1, ch, col);
    }

    // Trunk base anchor: "./~~~\." matching cbonsai wood color
    static const char BASE_ANCHOR[] = "./~~~\\.";
    COLORREF barkCol = GetBarkColor(s.theme);
    for (int k = 0; k < 7; ++k) {
        putGridCell(root_inx2 - 3 + k, root_inx1, BASE_ANCHOR[k], barkCol);
    }

    // Row 1: Pot upper bowl walls (" \                           / ")
    putGridCell(left_col + 1, root_inx1 + 1, '\\', COL_BOX);
    putGridCell(right_col - 1, root_inx1 + 1, '/', COL_BOX);

    // Row 2: Pot lower bowl walls and bottom rim ("  \_________________________/ ")
    putGridCell(left_col + 2, root_inx1 + 2, '\\', COL_BOX);
    for (int x = left_col + 3; x <= right_col - 3; ++x) {
        putGridCell(x, root_inx1 + 2, '_', COL_BOX);
    }
    putGridCell(right_col - 2, root_inx1 + 2, '/', COL_BOX);

    // Row 3: Pot feet ("  (_)                     (_) ")
    putGridCell(left_col + 2, root_inx1 + 3, '(', COL_BOX);
    putGridCell(left_col + 3, root_inx1 + 3, '_', COL_BOX);
    putGridCell(left_col + 4, root_inx1 + 3, ')', COL_BOX);

    putGridCell(right_col - 4, root_inx1 + 3, '(', COL_BOX);
    putGridCell(right_col - 3, root_inx1 + 3, '_', COL_BOX);
    putGridCell(right_col - 2, root_inx1 + 3, ')', COL_BOX);

    // 2. Procedural Tree Generation & Scale Calibration
    GeneratorContext ctx;
    ctx.cols = cols;
    ctx.rows = rows;
    ctx.options.type = s.treeType;
    ctx.options.theme = s.theme;
    ctx.options.num_layers = (std::max)(1, (std::min)(20, g_BonsaiLayers));
    ctx.options.angle_mean = (float)g_BonsaiAngle * (3.14159265f / 180.0f);
    ctx.options.leaf_len = (std::max)(1, (std::min)(50, g_BonsaiLeafLen));

    // Allow initial_len to scale so the tree comfortably fills the monitor height and width
    float root_y = 8.0f; // Plane Y corresponding to pot_row
    float max_plane_height = ((float)rows - 2.5f) * 2.0f - root_y;
    float max_len_by_height = (max_plane_height - 4.0f) / 1.8f;
    float max_len_by_width  = (((float)cols / 2.0f) - 4.0f) / 1.2f;

    float initial_len = (float)g_BonsaiStartLen;
    initial_len = (std::min)({ initial_len, max_len_by_height, max_len_by_width });
    if (initial_len < 4.0f) initial_len = 4.0f;
    ctx.options.initial_len = initial_len;

    int initial_width = (std::max)(2, (std::min)(4, (int)roundf(initial_len / 9.0f)));

    float angle_std = 8.0f * (3.14159265f / 180.0f);
    float initial_angle = RandNormal(0.0f, angle_std);

    // Execute tree generation according to selected type
    float root_x = (float)root_inx2;
    if (s.treeType == 0) {
        GenerateClassicBranch(ctx, root_x, root_y, 1, initial_len, initial_width, initial_angle);
    } else {
        ctx.branch_nums = GenerateBranchNumsMatrix(ctx.options.num_layers);
        GenerateFibBranch(ctx, root_x, root_y, 1, 0, initial_len, initial_width, initial_angle);
    }

    s.drawQueue = std::move(ctx.queue);
    s.leaves = std::move(ctx.leaves);

    s.state = 0; // Growing phase
    s.stateStartTime = GetTickCount();
    s.lastTick = GetTickCount();
    s.initialized = true;
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

    int screenCols = width / cW;
    int screenRows = height / cH;
    if (screenCols < 24) screenCols = 24;
    if (screenRows < 15) screenRows = 15;

    int cols = (std::min)(screenCols, g_BonsaiMaxWidth);
    int rows = (std::min)(screenRows, g_BonsaiMaxHeight);
    if (cols < 24) cols = 24;
    if (rows < 15) rows = 15;

    int offsetX = (screenCols - cols) / 2;
    int offsetY = screenRows - rows;

    bool isPrimary = data ? data->isPrimary : true;

    if (!s.initialized || s.widthInChars != cols || s.heightInChars != rows) {
        StartNewBonsai(s, cols, rows, isPrimary);
    }

    DWORD now = GetTickCount();
    float dt = (now - s.lastTick) / 1000.0f;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.033f;
    s.lastTick = now;

    // Fill screen with black
    FillRect(memDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Growth / Lifecycle State Machine
    if (s.state == 0) {
        // Growth phase: reveal cells sequentially from drawQueue
        if (!s.drawQueue.empty()) {
            int stepCount = (std::max)(2, (int)(s.drawQueue.size() / 140));
            for (int k = 0; k < stepCount && s.drawIndex < s.drawQueue.size(); ++k) {
                const auto& dc = s.drawQueue[s.drawIndex++];
                if (dc.x >= 0 && dc.x < cols && dc.y >= 0 && dc.y < rows) {
                    s.grid[dc.y * cols + dc.x].ch = dc.ch;
                    s.grid[dc.y * cols + dc.x].color = dc.color;
                }
            }
        }

        if (s.drawIndex >= s.drawQueue.size()) {
            s.state = 1; // Mature Serene Phase
            s.stateStartTime = now;
        }
    } else if (s.state == 1) {
        // Mature serene phase: foliage rustle & drifting petals
        DWORD elapsed = now - s.stateStartTime;

        // Subtle foliage shimmer / breeze
        if (!s.leaves.empty() && (rand() % 3 == 0)) {
            int flutterCount = 1 + (rand() % 3);
            for (int k = 0; k < flutterCount; ++k) {
                const auto& pt = s.leaves[rand() % s.leaves.size()];
                int idx = pt.y * cols + pt.x;
                char cur = s.grid[idx].ch;
                if (cur != ' ' && cur != '|' && cur != '/' && cur != '\\' && cur != '_') {
                    s.grid[idx].ch = LEAF_CHARS[rand() % 4];
                }
            }
        }

        // Spawn falling petals from foliage clusters
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
        // Renewal: transition pause, then sprout a new tree
        DWORD elapsed = now - s.stateStartTime;
        if (elapsed > 1500) {
            StartNewBonsai(s, cols, rows, isPrimary);
        }
    }

    // Update Petals
    for (size_t i = 0; i < s.petals.size(); ) {
        auto& p = s.petals[i];
        p.phase += dt * 2.0f;
        p.x += (p.vx + sinf(p.phase) * 0.4f) * (dt * 30.0f);
        p.y += p.vy * (dt * 30.0f);

        int screenX = offsetX + (int)p.x;
        int screenY = offsetY + (int)p.y;

        if (screenY >= screenRows || screenX < 0 || screenX >= screenCols) {
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
            TextOutA(memDC, (offsetX + startX) * cW, (offsetY + y) * cH, span, spanLen);
        }
    }

    // Render drifting petals on top
    for (const auto& p : s.petals) {
        int screenX = offsetX + (int)p.x;
        int screenY = offsetY + (int)p.y;
        if (screenX >= 0 && screenX < screenCols && screenY >= 0 && screenY < screenRows) {
            SetTextColor(memDC, p.color);
            TextOutA(memDC, screenX * cW, screenY * cH, &p.ch, 1);
        }
    }
}

REGISTER_SCREENSAVER(
    25,
    L"cbonsai (Bonsai Tree)",
    "cbonsai",
    { "cbonsai", "bonsai", "tree" },
    WRAP_LEGACY(RenderBonsai),
    GetBonsaiSettings()
);

