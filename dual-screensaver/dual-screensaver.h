#pragma once

#include "resource.h"
#include <windows.h>

struct ScreenData;
using RenderFn = void(*)(HDC, ScreenData*, int, int, const RECT&);

extern const WCHAR* g_modeNames[];
extern const RenderFn g_renderers[];
extern const int g_numScreensavers;

extern int g_TextSize;
extern int g_Speed;
extern bool g_ShowDebugHUD;
void initPerlin(unsigned int seed, int* perm);
