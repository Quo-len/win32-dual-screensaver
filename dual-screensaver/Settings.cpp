#include "Settings.h"
#include "Defaults.h"

float g_ASpeed = DEFAULT_ASPEED;
float g_BSpeed = DEFAULT_BSPEED;
float g_DonutSize = DEFAULT_DONUTSIZE;
float g_DonutDistance = DEFAULT_DONUTDISTANCE;
int g_TextSize = DEFAULT_TEXTSIZE;
int g_GolCellSize = DEFAULT_GOLCELLSIZE;
int g_GolSpeed = DEFAULT_GOLSPEED;
float g_EarthSpeed = DEFAULT_EARTHSPEED;
float g_PongSpeed = DEFAULT_PONGSPEED;
float g_MazeBuildSpeed = DEFAULT_MAZEBUILDSPEED;
float g_MazeSolveSpeed = DEFAULT_MAZESOLVESPEED;
float g_PerlinScale = DEFAULT_PERLINSCALE;
float g_PerlinSpeed = DEFAULT_PERLINSPEED;
int g_AntCount = DEFAULT_ANT_COUNT;
int g_AntSpeed = DEFAULT_ANT_SPEED;
float g_DvdSpeed = DEFAULT_DVDSPEED;
int g_ModePrimary = DEFAULT_MODEPRIMARY;
int g_ModeSecondary = DEFAULT_MODESECONDARY;
int g_RandomMode = DEFAULT_RANDOMMODE;
unsigned int g_RandomPool = DEFAULT_RANDOM_POOL;

const WCHAR* REG_PATH = L"Software\\DualSaver";

void LoadSettings()
{
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = sizeof(float);
		RegQueryValueExW(hKey, L"ASpeed", NULL, NULL, (LPBYTE)&g_ASpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"BSpeed", NULL, NULL, (LPBYTE)&g_BSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"DonutSize", NULL, NULL, (LPBYTE)&g_DonutSize, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"DonutDistance", NULL, NULL, (LPBYTE)&g_DonutDistance, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"TextSize", NULL, NULL, (LPBYTE)&g_TextSize, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"GolCellSize", NULL, NULL, (LPBYTE)&g_GolCellSize, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"GolSpeed", NULL, NULL, (LPBYTE)&g_GolSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"EarthSpeed", NULL, NULL, (LPBYTE)&g_EarthSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"PongSpeed", NULL, NULL, (LPBYTE)&g_PongSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"DvdSpeed", NULL, NULL, (LPBYTE)&g_DvdSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"MazeBuildSpeed", NULL, NULL, (LPBYTE)&g_MazeBuildSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"MazeSolveSpeed", NULL, NULL, (LPBYTE)&g_MazeSolveSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"PerlinScale", NULL, NULL, (LPBYTE)&g_PerlinScale, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"PerlinSpeed", NULL, NULL, (LPBYTE)&g_PerlinSpeed, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModePrimary", NULL, NULL, (LPBYTE)&g_ModePrimary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModeSecondary", NULL, NULL, (LPBYTE)&g_ModeSecondary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"RandomMode", NULL, NULL, (LPBYTE)&g_RandomMode, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"AntCount", NULL, NULL, (LPBYTE)&g_AntCount, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"AntSpeed", NULL, NULL, (LPBYTE)&g_AntSpeed, &size);
		size = sizeof(unsigned int);
		if (RegQueryValueExW(hKey, L"RandomPool", NULL, NULL, (LPBYTE)&g_RandomPool, &size) != ERROR_SUCCESS) {
			g_RandomPool = DEFAULT_RANDOM_POOL;
		}
		RegCloseKey(hKey);
	}
}

void SaveSettings()
{
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueExW(hKey, L"ASpeed", 0, REG_DWORD, (const BYTE*)&g_ASpeed, sizeof(float));
		RegSetValueExW(hKey, L"BSpeed", 0, REG_DWORD, (const BYTE*)&g_BSpeed, sizeof(float));
		RegSetValueExW(hKey, L"DonutSize", 0, REG_DWORD, (const BYTE*)&g_DonutSize, sizeof(float));
		RegSetValueExW(hKey, L"DonutDistance", 0, REG_DWORD, (const BYTE*)&g_DonutDistance, sizeof(float));
		RegSetValueExW(hKey, L"TextSize", 0, REG_DWORD, (const BYTE*)&g_TextSize, sizeof(int));
		RegSetValueExW(hKey, L"GolCellSize", 0, REG_DWORD, (const BYTE*)&g_GolCellSize, sizeof(int));
		RegSetValueExW(hKey, L"GolSpeed", 0, REG_DWORD, (const BYTE*)&g_GolSpeed, sizeof(int));
		RegSetValueExW(hKey, L"EarthSpeed", 0, REG_DWORD, (const BYTE*)&g_EarthSpeed, sizeof(float));
		RegSetValueExW(hKey, L"PongSpeed", 0, REG_DWORD, (const BYTE*)&g_PongSpeed, sizeof(float));
		RegSetValueExW(hKey, L"DvdSpeed", 0, REG_DWORD, (const BYTE*)&g_DvdSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeBuildSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeBuildSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeSolveSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeSolveSpeed, sizeof(float));
		RegSetValueExW(hKey, L"PerlinScale", 0, REG_DWORD, (const BYTE*)&g_PerlinScale, sizeof(float));
		RegSetValueExW(hKey, L"PerlinSpeed", 0, REG_DWORD, (const BYTE*)&g_PerlinSpeed, sizeof(float));
		RegSetValueExW(hKey, L"ModePrimary", 0, REG_DWORD, (const BYTE*)&g_ModePrimary, sizeof(int));
		RegSetValueExW(hKey, L"ModeSecondary", 0, REG_DWORD, (const BYTE*)&g_ModeSecondary, sizeof(int));
		RegSetValueExW(hKey, L"RandomMode", 0, REG_DWORD, (const BYTE*)&g_RandomMode, sizeof(int));
		RegSetValueExW(hKey, L"AntCount", 0, REG_DWORD, (const BYTE*)&g_AntCount, sizeof(int));
		RegSetValueExW(hKey, L"AntSpeed", 0, REG_DWORD, (const BYTE*)&g_AntSpeed, sizeof(int));
		RegSetValueExW(hKey, L"RandomPool", 0, REG_DWORD, (const BYTE*)&g_RandomPool, sizeof(unsigned int));
		RegCloseKey(hKey);
	}
}
