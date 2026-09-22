#include "DonutSettings.h"
#include "../Defaults.h"

float g_ASpeed = DEFAULT_ASPEED;
float g_BSpeed = DEFAULT_BSPEED;
float g_DonutSize = DEFAULT_DONUTSIZE;
float g_DonutDistance = DEFAULT_DONUTDISTANCE;

std::vector<SettingItem> GetDonutSettings() {
    return {
        { L"ASpeed",        L"A Speed:",    SettingType::Float, &g_ASpeed,        DEFAULT_ASPEED,        0.001, 0.5, 3 },
        { L"BSpeed",        L"B Speed:",    SettingType::Float, &g_BSpeed,        DEFAULT_BSPEED,        0.001, 0.5, 3 },
        { L"Size",          L"Donut Size:", SettingType::Float, &g_DonutSize,     DEFAULT_DONUTSIZE,     0.5,   10.0, 1 },
        { L"DonutDistance", L"Distance:",   SettingType::Float, &g_DonutDistance, DEFAULT_DONUTDISTANCE, 0.5,   20.0, 1 }
    };
}
