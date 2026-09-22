#include "Settings.h"
#include "Defaults.h"
#include "ScreensaverRegistry.h"

int g_ModePrimary = DEFAULT_MODEPRIMARY;
int g_ModeSecondary = DEFAULT_MODESECONDARY;
int g_RandomMode = DEFAULT_RANDOMMODE;
uint64_t g_RandomPool = DEFAULT_RANDOM_POOL;

const WCHAR* REG_PATH = L"Software\\DualSaver";

void LoadSettings()
{
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = sizeof(int);
		RegQueryValueExW(hKey, L"ModePrimary", NULL, NULL, (LPBYTE)&g_ModePrimary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModeSecondary", NULL, NULL, (LPBYTE)&g_ModeSecondary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"RandomMode", NULL, NULL, (LPBYTE)&g_RandomMode, &size);
		DWORD type = 0;
		size = sizeof(uint64_t);
		uint64_t val64 = 0;
		if (RegQueryValueExW(hKey, L"RandomPool", NULL, &type, (LPBYTE)&val64, &size) == ERROR_SUCCESS) {
			if (type == REG_DWORD) {
				g_RandomPool = *(DWORD*)&val64;
			} else {
				g_RandomPool = val64;
			}
		} else {
			g_RandomPool = DEFAULT_RANDOM_POOL;
		}

		// Dynamically load all per-screensaver settings from registry
		for (const auto& def : ScreensaverRegistry::GetAll()) {
			for (const auto& item : def.settings) {
				DWORD itemSize = (item.type == SettingType::Int) ? sizeof(int) : sizeof(float);
				RegQueryValueExW(hKey, item.key, NULL, NULL, (LPBYTE)item.valPtr, &itemSize);
			}
		}

		RegCloseKey(hKey);
	}
}

void SaveSettings()
{
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueExW(hKey, L"ModePrimary", 0, REG_DWORD, (const BYTE*)&g_ModePrimary, sizeof(int));
		RegSetValueExW(hKey, L"ModeSecondary", 0, REG_DWORD, (const BYTE*)&g_ModeSecondary, sizeof(int));
		RegSetValueExW(hKey, L"RandomMode", 0, REG_DWORD, (const BYTE*)&g_RandomMode, sizeof(int));
		RegSetValueExW(hKey, L"RandomPool", 0, REG_QWORD, (const BYTE*)&g_RandomPool, sizeof(uint64_t));

		// Dynamically save all per-screensaver settings to registry
		for (const auto& def : ScreensaverRegistry::GetAll()) {
			for (const auto& item : def.settings) {
				if (item.type == SettingType::Int) {
					RegSetValueExW(hKey, item.key, 0, REG_DWORD, (const BYTE*)item.valPtr, sizeof(int));
				} else if (item.type == SettingType::Float) {
					RegSetValueExW(hKey, item.key, 0, REG_DWORD, (const BYTE*)item.valPtr, sizeof(float));
				} else if (item.type == SettingType::Bool) {
					int b = *(bool*)item.valPtr ? 1 : 0;
					RegSetValueExW(hKey, item.key, 0, REG_DWORD, (const BYTE*)&b, sizeof(int));
				}
			}
		}

		RegCloseKey(hKey);
	}
}
