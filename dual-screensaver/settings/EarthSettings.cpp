#include "EarthSettings.h"
#include <vector>

constexpr float DEFAULT_EARTHSPEED = 0.05f;

float g_EarthSpeed = DEFAULT_EARTHSPEED;

std::vector<SettingItem> GetEarthSettings() {
    return {
        { L"EarthSpeed", L"Spin:", SettingType::Float, &g_EarthSpeed, DEFAULT_EARTHSPEED, 0.001, 1.0, 3 }
    };
}
