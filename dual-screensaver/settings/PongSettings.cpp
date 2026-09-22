#include "PongSettings.h"
#include "../Defaults.h"

float g_PongSpeed = DEFAULT_PONGSPEED;

std::vector<SettingItem> GetPongSettings() {
    return {
        { L"PongSpeed", L"Speed:", SettingType::Float, &g_PongSpeed, DEFAULT_PONGSPEED, 1.0, 200.0, 1 }
    };
}
