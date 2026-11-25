#include "DungeonSettings.h"
#include <vector>

constexpr float DEFAULT_DUNGEON_WALK_SPEED = 1.0f;
constexpr bool  DEFAULT_DUNGEON_MINIMAP    = true;
constexpr int   DEFAULT_DUNGEON_THEME      = 0; // 0: Crypt Slate Grey (Default)

float g_DungeonWalkSpeed = DEFAULT_DUNGEON_WALK_SPEED;
bool  g_DungeonMinimap    = DEFAULT_DUNGEON_MINIMAP;
int   g_DungeonTheme      = DEFAULT_DUNGEON_THEME;

std::vector<SettingItem> GetDungeonSettings() {
    return {
        { L"DungeonTheme",     L"Color Theme:", SettingType::Int,   &g_DungeonTheme,     (double)DEFAULT_DUNGEON_THEME, 0, 4, 0, L"0 = Crypt Slate Grey (from HTML)\n1 = Neutral Ash Grey\n2 = Warm Sandstone\n3 = Moss\n4 = Ember" },
        { L"DungeonWalkSpeed", L"Walk Speed:",  SettingType::Float, &g_DungeonWalkSpeed, DEFAULT_DUNGEON_WALK_SPEED,    0.5, 3.0, 1 },
        { L"DungeonMinimap",   L"Minimap",      SettingType::Bool,  &g_DungeonMinimap,   (double)DEFAULT_DUNGEON_MINIMAP, 0, 1, 0 }
    };
}
