#pragma once
#include "SettingItem.h"

extern float g_DungeonWalkSpeed;
extern bool  g_DungeonMinimap;
extern int   g_DungeonTheme;

std::vector<SettingItem> GetDungeonSettings();
