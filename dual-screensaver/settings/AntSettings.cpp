#include "AntSettings.h"
#include "../Defaults.h"

int g_AntCount = DEFAULT_ANT_COUNT;
int g_AntSpeed = DEFAULT_ANT_SPEED;

std::vector<SettingItem> GetAntSettings() {
    return {
        { L"AntCount", L"Sets (Sym):", SettingType::Int, &g_AntCount, DEFAULT_ANT_COUNT, 1, 100,  0 },
        { L"AntSpeed", L"Speed:",      SettingType::Int, &g_AntSpeed, DEFAULT_ANT_SPEED, 1, 5000, 0 }
    };
}
