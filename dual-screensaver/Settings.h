#pragma once
#include "framework.h"
#include <cstdint>

// Modular per-screensaver settings
#include "settings/DonutSettings.h"
#include "settings/GolSettings.h"
#include "settings/EarthSettings.h"
#include "settings/DvdSettings.h"
#include "settings/PongSettings.h"
#include "settings/MazeSettings.h"
#include "settings/ClockSettings.h"
#include "settings/PerlinSettings.h"
#include "settings/AntSettings.h"
#include "settings/CurlSettings.h"
#include "settings/TetrisSettings.h"
#include "settings/BonsaiSettings.h"

// Global screensaver configuration
extern int g_ModePrimary;
extern int g_ModeSecondary;
extern int g_RandomMode;
extern uint64_t g_RandomPool;
extern const WCHAR* REG_PATH;
extern const WCHAR* g_modeNames[];

void LoadSettings();
void SaveSettings();