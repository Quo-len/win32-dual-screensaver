#include "BonsaiSettings.h"
#include <vector>

constexpr int DEFAULT_BONSAI_WIDTH = 140;
constexpr int DEFAULT_BONSAI_HEIGHT = 50;
constexpr bool DEFAULT_BONSAI_CLASSIC = true;
constexpr bool DEFAULT_BONSAI_FIB = true;
constexpr bool DEFAULT_BONSAI_OFFSET_FIB = true;
constexpr bool DEFAULT_BONSAI_RND_OFFSET = true;
constexpr int DEFAULT_BONSAI_TYPE = -1; // -1 for random
constexpr int DEFAULT_BONSAI_START_LEN = 28;
constexpr int DEFAULT_BONSAI_LEAF_LEN = 5;
constexpr int DEFAULT_BONSAI_LAYERS = 8;
constexpr int DEFAULT_BONSAI_ANGLE = 40;

int g_BonsaiMaxWidth = DEFAULT_BONSAI_WIDTH;
int g_BonsaiMaxHeight = DEFAULT_BONSAI_HEIGHT;
bool g_BonsaiTypeClassic = DEFAULT_BONSAI_CLASSIC;
bool g_BonsaiTypeFibonacci = DEFAULT_BONSAI_FIB;
bool g_BonsaiTypeOffsetFib = DEFAULT_BONSAI_OFFSET_FIB;
bool g_BonsaiTypeRndOffsetFib = DEFAULT_BONSAI_RND_OFFSET;
int g_BonsaiType = DEFAULT_BONSAI_TYPE;
int g_BonsaiStartLen = DEFAULT_BONSAI_START_LEN;
int g_BonsaiLeafLen = DEFAULT_BONSAI_LEAF_LEN;
int g_BonsaiLayers = DEFAULT_BONSAI_LAYERS;
int g_BonsaiAngle = DEFAULT_BONSAI_ANGLE;

std::vector<SettingItem> GetBonsaiSettings() {
    return {
        { L"BonsaiWidth",       L"Max Width (-x):",       SettingType::Int,  &g_BonsaiMaxWidth,         DEFAULT_BONSAI_WIDTH,       40, 300, 0,
            L"Maximum width of the bonsai tree in terminal character columns. Recommended: 120-160 to fit modern monitors." },
        { L"BonsaiHeight",      L"Max Height (-y):",      SettingType::Int,  &g_BonsaiMaxHeight,        DEFAULT_BONSAI_HEIGHT,      25, 120, 0,
            L"Maximum height of the bonsai tree in terminal character rows. Recommended: 45-60 to fill monitor height." },
        { L"BonsaiClassic",     L"Type 0: Classic",       SettingType::Bool, &g_BonsaiTypeClassic,      1.0,                        0,  1,   0,
            L"Include Classic style (traditional balanced bonsai branching) in the random tree type selection pool." },
        { L"BonsaiFib",         L"Type 1: Fibonacci",     SettingType::Bool, &g_BonsaiTypeFibonacci,    1.0,                        0,  1,   0,
            L"Include Fibonacci style (spiral mathematical branching pattern) in the random tree type pool." },
        { L"BonsaiOffsetFib",   L"Type 2: Offset Fib",    SettingType::Bool, &g_BonsaiTypeOffsetFib,    1.0,                        0,  1,   0,
            L"Include Offset Fibonacci style (asymmetric organic branching) in the random tree type pool." },
        { L"BonsaiRndOffset",   L"Type 3: Rnd Offset",    SettingType::Bool, &g_BonsaiTypeRndOffsetFib, 1.0,                        0,  1,   0,
            L"Include Randomized Offset Fibonacci style (wild, chaotic natural branching) in the random tree type pool." },
        { L"BonsaiType",        L"Type (-1=Rnd) (-t):",   SettingType::Int,  &g_BonsaiType,             (double)DEFAULT_BONSAI_TYPE,-1, 3,   0,
            L"Generator algorithm to use: -1 picks randomly from enabled types above, 0 = Classic, 1 = Fibonacci, 2 = Offset Fib, 3 = Rnd Offset." },
        { L"BonsaiStartLen",    L"Start Len (-S):",       SettingType::Int,  &g_BonsaiStartLen,         DEFAULT_BONSAI_START_LEN,   10, 70,  0,
            L"Initial trunk length in character steps before the trunk begins to branch out into limbs. Recommended: 30-40." },
        { L"BonsaiLeafLen",     L"Leaf Len (-L):",        SettingType::Int,  &g_BonsaiLeafLen,          DEFAULT_BONSAI_LEAF_LEN,    2,  15,  0,
            L"Branch length threshold below which branches stop splitting and sprout foliage leaves. Recommended: 4-6." },
        { L"BonsaiLayers",      L"Layers (-l):",          SettingType::Int,  &g_BonsaiLayers,           DEFAULT_BONSAI_LAYERS,      4,  12,  0,
            L"Maximum branching depth (number of recursive branch split iterations from trunk to canopy). Recommended: 7-9." },
        { L"BonsaiAngle",       L"Angle (deg) (-a):",     SettingType::Int,  &g_BonsaiAngle,            DEFAULT_BONSAI_ANGLE,       20, 65,  0,
            L"Average branching angle in degrees. Higher values produce wider spreading canopies; lower values produce tall, upright trees. Recommended: 35-45." }
    };
}
