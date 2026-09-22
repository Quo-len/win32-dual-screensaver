#include "GolSettings.h"
#include "../Defaults.h"

int g_GolCellSize = DEFAULT_GOLCELLSIZE;
int g_GolSpeed = DEFAULT_GOLSPEED;

std::vector<SettingItem> GetGolSettings() {
    return {
        { L"GolSize",  L"Cell (px):",   SettingType::Int, &g_GolCellSize, DEFAULT_GOLCELLSIZE, 1,   50,   0 },
        { L"GolSpeed", L"Speed (ms):", SettingType::Int, &g_GolSpeed,    DEFAULT_GOLSPEED,    10,  1000, 0 }
    };
}
