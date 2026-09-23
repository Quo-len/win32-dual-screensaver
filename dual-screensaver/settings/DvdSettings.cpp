#include "DvdSettings.h"
#include <vector>

constexpr float DEFAULT_DVDSPEED = 10.0f;

float g_DvdSpeed = DEFAULT_DVDSPEED;

std::vector<SettingItem> GetDvdSettings() {
    return {
        { L"DvdSpeed", L"Speed:", SettingType::Float, &g_DvdSpeed, DEFAULT_DVDSPEED, 0.5, 100.0, 1 }
    };
}
