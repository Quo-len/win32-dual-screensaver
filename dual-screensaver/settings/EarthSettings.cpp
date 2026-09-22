#include "EarthSettings.h"
#include "../Defaults.h"

float g_EarthSpeed = DEFAULT_EARTHSPEED;

std::vector<SettingItem> GetEarthSettings() {
    return {
        { L"EarthSpeed", L"Spin:", SettingType::Float, &g_EarthSpeed, DEFAULT_EARTHSPEED, 0.001, 1.0, 3 }
    };
}
