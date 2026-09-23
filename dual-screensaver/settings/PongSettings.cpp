#include "PongSettings.h"
#include <vector>

constexpr float DEFAULT_PONGSPEED = 25.0f;

float g_PongSpeed = DEFAULT_PONGSPEED;

std::vector<SettingItem> GetPongSettings() {
    return {
        { L"PongSpeed", L"Speed:", SettingType::Float, &g_PongSpeed, DEFAULT_PONGSPEED, 1.0, 200.0, 1 }
    };
}
