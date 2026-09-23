#include "ClockSettings.h"
#include <vector>

constexpr int DEFAULT_TEXTSIZE = 20;

int g_TextSize = DEFAULT_TEXTSIZE;

std::vector<SettingItem> GetClockSettings() {
    return {
        { L"TextSize", L"Text Size:", SettingType::Int, &g_TextSize, DEFAULT_TEXTSIZE, 8, 120, 0 }
    };
}
