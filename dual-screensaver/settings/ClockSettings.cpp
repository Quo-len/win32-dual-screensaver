#include "ClockSettings.h"
#include "../Defaults.h"

int g_TextSize = DEFAULT_TEXTSIZE;

std::vector<SettingItem> GetClockSettings() {
    return {
        { L"TextSize", L"Text Size:", SettingType::Int, &g_TextSize, DEFAULT_TEXTSIZE, 8, 120, 0 }
    };
}
