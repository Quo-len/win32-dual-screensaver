#pragma once
#include "framework.h"

#define IDC_EDIT_ASPEED           1001
#define IDC_EDIT_BSPEED           1002
#define IDC_EDIT_SIZE             1003
#define IDC_EDIT_TEXTSIZE         1004
#define IDC_EDIT_GOL_SIZE         1005
#define IDC_EDIT_GOL_SPEED        1006
#define IDC_COMBO_PRIMARY         1007
#define IDC_COMBO_SECONDARY       1008
#define IDOK_BTN                  1009
#define IDCANCEL_BTN              1010
#define IDRESET_BTN               1011
#define IDC_EDIT_EARTH_SPEED      1012
#define IDC_CHECK_RANDOM          1013
#define IDC_EDIT_PONG_SPEED       1014
#define IDC_EDIT_MAZE_BUILD_SPEED 1015
#define IDC_EDIT_MAZE_SOLVE_SPEED 1016
#define IDC_EDIT_PERLIN_SCALE     1017
#define IDC_EDIT_DONUT_DISTANCE   1018
#define IDC_EDIT_DVD_SPEED        1019
#define IDC_EDIT_ANT_COUNT        1020
#define IDC_EDIT_ANT_SPEED        1021
#define IDC_EDIT_PERLIN_SPEED	  1022
#define IDC_BTN_POOL              1023
#define IDC_POOL_SELECT_ALL       1024
#define IDC_POOL_DESELECT_ALL     1025
#define IDC_POOL_CHECK_BASE       1100

extern float g_ASpeed;
extern float g_BSpeed;
extern float g_DonutSize;
extern float g_DonutDistance;
extern int g_TextSize;
extern int g_GolCellSize;
extern int g_GolSpeed;
extern float g_EarthSpeed;
extern float g_PongSpeed;
extern float g_MazeBuildSpeed;
extern float g_MazeSolveSpeed;
extern float g_PerlinScale;
extern float g_PerlinSpeed;
extern int g_AntCount;
extern int g_AntSpeed;
extern int g_ModePrimary;
extern int g_ModeSecondary;
extern int g_RandomMode;
extern unsigned int g_RandomPool;
extern const WCHAR* REG_PATH;

void LoadSettings();
void SaveSettings();
void ShowPoolWindow(HWND hWndParent, HINSTANCE hInstance);