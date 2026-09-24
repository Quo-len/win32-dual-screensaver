#pragma once
#include "SettingItem.h"

extern int g_BonsaiMaxWidth;
extern int g_BonsaiMaxHeight;
extern bool g_BonsaiTypeClassic;
extern bool g_BonsaiTypeFibonacci;
extern bool g_BonsaiTypeOffsetFib;
extern bool g_BonsaiTypeRndOffsetFib;
extern int g_BonsaiType;
extern int g_BonsaiStartLen;
extern int g_BonsaiLeafLen;
extern int g_BonsaiLayers;
extern int g_BonsaiAngle;

std::vector<SettingItem> GetBonsaiSettings();
