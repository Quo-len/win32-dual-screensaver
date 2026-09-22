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

// Main Configuration Dialog Control IDs
#define IDC_COMBO_PRIMARY         1007
#define IDC_COMBO_SECONDARY       1008
#define IDOK_BTN                  1009
#define IDCANCEL_BTN              1010
#define IDRESET_BTN               1011
#define IDC_CHECK_RANDOM          1013
#define IDC_BTN_POOL              1024
#define IDC_POOL_SELECT_ALL       1024
#define IDC_POOL_DESELECT_ALL     1025
#define IDC_POOL_CHECK_BASE       1100
#define IDC_SETTINGS_BASE         1200

// Global screensaver configuration
extern int g_ModePrimary;
extern int g_ModeSecondary;
extern int g_RandomMode;
extern uint64_t g_RandomPool;
extern const WCHAR* REG_PATH;
extern const WCHAR* g_modeNames[];

void LoadSettings();
void SaveSettings();