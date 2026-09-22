#include "PerlinSettings.h"
#include "../Defaults.h"

float g_PerlinScale = DEFAULT_PERLINSCALE;
float g_PerlinSpeed = DEFAULT_PERLINSPEED;

std::vector<SettingItem> GetPerlinSettings() {
    return {
        { L"PerlinScale", L"Scale:", SettingType::Float, &g_PerlinScale, DEFAULT_PERLINSCALE, 0.0001, 0.1, 4 },
        { L"PerlinSpeed", L"Speed:", SettingType::Float, &g_PerlinSpeed, DEFAULT_PERLINSPEED, 0.1,    50.0, 2 }
    };
}
