#include "Defaults.h"
#include "framework.h"
#include "dual-screensaver.h"
#include "Settings.h"
#include "ScreenData.h"
#include "Perlin.h"
#include "Renderers.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"DualSaver";
WCHAR szWindowClass[MAX_LOADSTRING] = L"DualSaverClass";

int g_TextSize = DEFAULT_TEXTSIZE;
float g_ASpeed = DEFAULT_ASPEED;
float g_BSpeed = DEFAULT_BSPEED;
float g_DonutSize = DEFAULT_DONUTSIZE;
float g_DonutDistance = DEFAULT_DONUTDISTANCE;
int g_GolCellSize = DEFAULT_GOLCELLSIZE;
int g_GolSpeed = DEFAULT_GOLSPEED;
float g_EarthSpeed = DEFAULT_EARTHSPEED;
float g_PongSpeed = DEFAULT_PONGSPEED;
float g_DvdSpeed = DEFAULT_DVDSPEED;
float g_MazeBuildSpeed = DEFAULT_MAZEBUILDSPEED;
float g_MazeSolveSpeed = DEFAULT_MAZESOLVESPEED;
float g_PerlinScale = DEFAULT_PERLINSCALE;
float g_PerlinSpeed = DEFAULT_PERLINSPEED;
int g_AntCount = DEFAULT_ANT_COUNT;
int g_AntSpeed = DEFAULT_ANT_SPEED;

int g_ModePrimary = DEFAULT_MODEPRIMARY;
int g_ModeSecondary = DEFAULT_MODESECONDARY;
int g_RandomMode = DEFAULT_RANDOMMODE;
unsigned int g_RandomPool = DEFAULT_RANDOM_POOL;

const WCHAR* REG_PATH = L"Software\\DualSaver";

static const WCHAR* g_modeNames[] = {
	L"Donut", L"Game of Life", L"Matrix", L"Earth",
	L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo",
	L"Grid", L"Pong", L"Maze Generator", L"Odometer Clock",
	L"Perlin Flow Field", L"ASCII Fire", L"Hex Memory Dump", L"Sorting Algorithms",
	L"Langton's Ant Symmetrical",
	L"Boids Flocking", L"Cyclic CA", L"Pipes", L"Brian's Brain",
	L"Mandelbrot Zoom", L"Clifford Attractor"
};

using RenderFn = void(*)(HDC, ScreenData*, int, int, const RECT&);
static const RenderFn g_renderers[] = {
	RenderDonut, RenderGoL,    RenderMatrix, RenderEarth,
	RenderBlank, RenderJulia,  RenderStars,  RenderDVD,
	RenderGrid,  RenderPong,   RenderMaze,   RenderClock,
	RenderPerlin, RenderFire, RenderMemoryDump,
	RenderRandomSort, RenderLangton,
	RenderBoids, RenderCyclicCA, RenderPipes, RenderBriansBrain,
	RenderMandelbrot, RenderClifford
};

#define NUM_SCREENSAVERS (int)(sizeof(g_renderers) / sizeof(g_renderers[0]))

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
LRESULT CALLBACK    ConfigWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PoolWindowProc(HWND, UINT, WPARAM, LPARAM);
void                ShowSettingsWindow(HINSTANCE);
void                ShowPoolWindow(HWND, HINSTANCE);

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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	hInst = hInstance;
	srand((unsigned int)time(NULL));
	LoadSettings();

	const struct { const wchar_t* name; int idx; } modeMap[] = {
		{L"donut", 0}, {L"gol", 1}, {L"matrix", 2}, {L"earth", 3},
		{L"blank", 4}, {L"julia", 5}, {L"stars", 6}, {L"dvd", 7},
		{L"grid", 8}, {L"pong", 9}, {L"maze", 10}, {L"clock", 11},
		{L"perlin", 12}, {L"fire", 13}, {L"memory", 14}, {L"sort", 15},
		{L"ant", 16}, {L"boids", 17}, {L"cyclic", 18}, {L"pipes", 19},
		{L"brain", 20}, {L"mandelbrot", 21}, {L"clifford", 22}
	};

	WCHAR* cmdCopy = _wcsdup(lpCmdLine);
	WCHAR* context = NULL;
	WCHAR* token = wcstok_s(cmdCopy, L" \t", &context);
	WCHAR* foundArgs[2] = { NULL, NULL };
	int foundCount = 0;
	while (token && foundCount < 2) {
		if (token[0] != L'/' && token[0] != L'-') {
			foundArgs[foundCount++] = token;
		}
		token = wcstok_s(NULL, L" \t", &context);
	}
	if (foundCount == 1 || foundCount == 2) {
		int found1 = -1, found2 = -1;
		for (int i = 0; i < (int)(sizeof(modeMap) / sizeof(modeMap[0])); ++i) {
			if (foundArgs[0] && _wcsicmp(foundArgs[0], modeMap[i].name) == 0) found1 = modeMap[i].idx;
			if (foundCount == 2 && foundArgs[1] && _wcsicmp(foundArgs[1], modeMap[i].name) == 0) found2 = modeMap[i].idx;
		}
		if (found1 >= 0 && foundCount == 1) {
			g_ModePrimary = found1;
			g_RandomMode = 0;
		}
		else if (found1 >= 0 && found2 >= 0 && foundCount == 2) {
			g_ModePrimary = found1;
			g_ModeSecondary = found2;
			g_RandomMode = 0;
		}
	}
	free(cmdCopy);

	if (wcsstr(lpCmdLine, L"/pool") || wcsstr(lpCmdLine, L"/POOL"))
	{
		ShowPoolWindow(NULL, hInstance);
		return 0;
	}

	if (wcsstr(lpCmdLine, L"/c") || wcsstr(lpCmdLine, L"/C"))
	{
		ShowSettingsWindow(hInstance);
		return 0;
	}

	if (g_RandomMode) {
		std::vector<int> pool;
		for (int i = 0; i < NUM_SCREENSAVERS; ++i) {
			if (g_RandomPool & (1u << i)) {
				pool.push_back(i);
			}
		}
		if (pool.empty()) {
			for (int i = 0; i < NUM_SCREENSAVERS; ++i) pool.push_back(i);
		}
		g_ModePrimary = pool[rand() % pool.size()];
		g_ModeSecondary = pool[rand() % pool.size()];
	}

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DUALSCREENSAVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (wcsstr(lpCmdLine, L"/p") || wcsstr(lpCmdLine, L"/P"))
	{
		WCHAR* spacePos = wcschr(lpCmdLine, L' ');
		if (spacePos)
		{
			HWND hParent = (HWND)_wcstoui64(spacePos + 1, NULL, 10);
			RECT rc;
			GetClientRect(hParent, &rc);
			CreateWindowExW(0, szWindowClass, szTitle, WS_CHILD | WS_VISIBLE,
				0, 0, rc.right, rc.bottom, hParent, NULL, hInstance, (LPVOID)TRUE);

			MSG msg;
			while (GetMessage(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return 0;
	}

	if (!InitInstance(hInstance, nCmdShow)) return FALSE;

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DUALSCREENSAVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
	return TRUE;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hMonitor, &mi);

	CreateWindowExW(WS_EX_TOPMOST, szWindowClass, szTitle,
		WS_POPUP | WS_VISIBLE,
		mi.rcMonitor.left, mi.rcMonitor.top,
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top,
		nullptr, nullptr, hInst, (LPVOID)(mi.dwFlags & MONITORINFOF_PRIMARY));

	ShowCursor(FALSE);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static POINT initialMousePos;
	static bool mouseInitialized = false;

	ScreenData* data = (ScreenData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		ScreenData* newData = new ScreenData();
		newData->isPrimary = (bool)cs->lpCreateParams;
		newData->isPreview = (cs->style & WS_CHILD) != 0;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)newData);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_CREATE:
		if (data) {
			data->hFont = CreateFontA(data->isPreview ? 10 : g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FIXED_PITCH | FF_MODERN, "Consolas");

			data->hMatrixFont = CreateFontW(data->isPreview ? 10 : g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FIXED_PITCH | FF_MODERN, L"MS Gothic");

			data->startTime = GetTickCount64();

			unsigned int monitorSeed = (unsigned int)GetTickCount64() + (rand() % 10000);
			initPerlin(monitorSeed, data->perm);
		}
		SetTimer(hWnd, 1, 33, NULL);
		break;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, width, height);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

		FillRect(memDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
		SetBkMode(memDC, OPAQUE);
		SetBkColor(memDC, RGB(0, 0, 0));

		int mode = data->isPreview ? 0 : (data->isPrimary ? g_ModePrimary : g_ModeSecondary);

		if (mode >= 0 && mode < NUM_SCREENSAVERS)
			g_renderers[mode](memDC, data, width, height, rect);

		BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

		SelectObject(memDC, hOldBmp);
		DeleteObject(hMemBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_MOUSEMOVE:
	{
		if (data && data->isPreview) break;
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		if (!mouseInitialized) {
			initialMousePos = pt;
			mouseInitialized = true;
		}
		else if (abs(pt.x - initialMousePos.x) > 5 || abs(pt.y - initialMousePos.y) > 5) {
			PostQuitMessage(0);
		}
	}
	break;
	case WM_KEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (data && !data->isPreview) PostQuitMessage(0);
		break;
	case WM_DESTROY:
		if (data) {
			if (data->hFont) DeleteObject(data->hFont);
			if (data->hMatrixFont) DeleteObject(data->hMatrixFont);
			delete data;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK PoolWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		HFONT hFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		HFONT hBold = CreateFontW(19, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		HWND hTitle = CreateWindowW(L"STATIC", L"Select screensavers to include in the random pool:",
			WS_CHILD | WS_VISIBLE, 20, 15, 380, 25, hWnd, NULL, hInst, NULL);
		SendMessage(hTitle, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0));

		int col1X = 25;
		int col2X = 215;
		int startY = 48;
		int rowH = 30;
		int chkW = 175;
		int half  = (NUM_SCREENSAVERS + 1) / 2; // split into two equal columns

		for (int i = 0; i < NUM_SCREENSAVERS; i++)
		{
			int x = (i < half) ? col1X : col2X;
			int y = startY + ((i < half) ? i : (i - half)) * rowH;

			HWND hChk = CreateWindowW(L"BUTTON", g_modeNames[i],
				WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
				x, y, chkW, 26, hWnd, (HMENU)(INT_PTR)(IDC_POOL_CHECK_BASE + i), hInst, NULL);
			SendMessage(hChk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

			if (g_RandomPool & (1u << i))
			{
				SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);
			}
		}

		int btnY = startY + half * rowH + 15;
		HWND hSelAll = CreateWindowW(L"BUTTON", L"Select All", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			25, btnY, 100, 30, hWnd, (HMENU)IDC_POOL_SELECT_ALL, hInst, NULL);
		SendMessage(hSelAll, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		HWND hDeselAll = CreateWindowW(L"BUTTON", L"Deselect All", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			135, btnY, 100, 30, hWnd, (HMENU)IDC_POOL_DESELECT_ALL, hInst, NULL);
		SendMessage(hDeselAll, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		int actY = btnY + 45;
		HWND hOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			180, actY, 95, 32, hWnd, (HMENU)IDOK, hInst, NULL);
		SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		HWND hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			285, actY, 95, 32, hWnd, (HMENU)IDCANCEL, hInst, NULL);
		SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		break;
	}
	case WM_COMMAND:
	{
		WORD id = LOWORD(wParam);
		if (id == IDC_POOL_SELECT_ALL)
		{
			for (int i = 0; i < NUM_SCREENSAVERS; i++)
			{
				CheckDlgButton(hWnd, IDC_POOL_CHECK_BASE + i, BST_CHECKED);
			}
		}
		else if (id == IDC_POOL_DESELECT_ALL)
		{
			for (int i = 0; i < NUM_SCREENSAVERS; i++)
			{
				CheckDlgButton(hWnd, IDC_POOL_CHECK_BASE + i, BST_UNCHECKED);
			}
		}
		else if (id == IDOK)
		{
			unsigned int newMask = 0;
			for (int i = 0; i < NUM_SCREENSAVERS; i++)
			{
				if (IsDlgButtonChecked(hWnd, IDC_POOL_CHECK_BASE + i) == BST_CHECKED)
				{
					newMask |= (1u << i);
				}
			}
			g_RandomPool = newMask;
			SaveSettings();
			DestroyWindow(hWnd);
		}
		else if (id == IDCANCEL)
		{
			DestroyWindow(hWnd);
		}
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ShowPoolWindow(HWND hWndParent, HINSTANCE hInstance)
{
	static bool s_classRegistered = false;
	if (!s_classRegistered)
	{
		WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = PoolWindowProc;
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wcex.lpszClassName = L"SaverPoolSettingsClass";
		RegisterClassExW(&wcex);
		s_classRegistered = true;
	}

	if (hWndParent) EnableWindow(hWndParent, FALSE);

	HWND hWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SaverPoolSettingsClass", L"Randomizer Pool Selection",
		WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		CW_USEDEFAULT, CW_USEDEFAULT, 420, 560,
		hWndParent, nullptr, hInstance, nullptr);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (hWndParent)
	{
		EnableWindow(hWndParent, TRUE);
		SetForegroundWindow(hWndParent);
	}
}

void ShowSettingsWindow(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ConfigWindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszClassName = L"SaverSettingsClass";

	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SaverSettingsClass", L"Screensaver Settings",
		WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		CW_USEDEFAULT, CW_USEDEFAULT, 750, 650,
		nullptr, nullptr, hInstance, nullptr);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT CALLBACK ConfigWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		HFONT hBold = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		const int col1X = 25;
		const int col2X = 380;
		const int lblW = 140;
		const int edtW = 140;
		const int rowH = 38;
		const int secH = 50;

		int y = 5;

		HWND hL1 = CreateWindowW(L"STATIC", L"Donut Settings", WS_CHILD | WS_VISIBLE, col1X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL1, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h1 = CreateWindowW(L"STATIC", L"A Speed:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hA = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_ASPEED, hInst, NULL); y += rowH;

		HWND h2 = CreateWindowW(L"STATIC", L"B Speed:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_BSPEED, hInst, NULL); y += rowH;

		HWND h3 = CreateWindowW(L"STATIC", L"Donut Size:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_SIZE, hInst, NULL); y += rowH;

		HWND hDistLbl = CreateWindowW(L"STATIC", L"Distance:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hDist = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_DONUT_DISTANCE, hInst, NULL); y += rowH;

		HWND h4 = CreateWindowW(L"STATIC", L"Text Size:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hT = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_TEXTSIZE, hInst, NULL); y += secH;

		HWND hL2 = CreateWindowW(L"STATIC", L"Game of Life", WS_CHILD | WS_VISIBLE, col1X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL2, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h5 = CreateWindowW(L"STATIC", L"Cell (px):", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hG1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_GOL_SIZE, hInst, NULL); y += rowH;

		HWND h6 = CreateWindowW(L"STATIC", L"Speed (ms):", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hG2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_GOL_SPEED, hInst, NULL); y += secH;

		HWND hL4 = CreateWindowW(L"STATIC", L"Earth Settings", WS_CHILD | WS_VISIBLE, col1X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL4, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h9 = CreateWindowW(L"STATIC", L"Spin:", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hES = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_EARTH_SPEED, hInst, NULL);

		y += secH;
		HWND hL8 = CreateWindowW(L"STATIC", L"Langton's Ant", WS_CHILD | WS_VISIBLE, col1X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL8, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND hAnt1 = CreateWindowW(L"STATIC", L"Sets (Symmetry):", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hAntCount = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_ANT_COUNT, hInst, NULL); y += rowH;

		HWND hAnt2 = CreateWindowW(L"STATIC", L"Speed (Ops/f):", WS_CHILD | WS_VISIBLE, col1X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hAntSpeed = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER, col1X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_ANT_SPEED, hInst, NULL);

		y = 5;

		HWND hL5 = CreateWindowW(L"STATIC", L"Ping Pong", WS_CHILD | WS_VISIBLE, col2X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL5, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h10 = CreateWindowW(L"STATIC", L"Speed:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hPS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_PONG_SPEED, hInst, NULL); y += rowH;

		HWND h10b = CreateWindowW(L"STATIC", L"DVD Speed:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hDS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_DVD_SPEED, hInst, NULL); y += secH;

		HWND hL6 = CreateWindowW(L"STATIC", L"Maze Settings", WS_CHILD | WS_VISIBLE, col2X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL6, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h11 = CreateWindowW(L"STATIC", L"Build Spd:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hMB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_MAZE_BUILD_SPEED, hInst, NULL); y += rowH;

		HWND h12 = CreateWindowW(L"STATIC", L"Solve Spd:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hMS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_MAZE_SOLVE_SPEED, hInst, NULL); y += secH;

		HWND hL7 = CreateWindowW(L"STATIC", L"Perlin Noise", WS_CHILD | WS_VISIBLE, col2X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL7, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h13 = CreateWindowW(L"STATIC", L"Scale:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hPerlinScale = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_PERLIN_SCALE, hInst, NULL);
		y += rowH;

		HWND h14 = CreateWindowW(L"STATIC", L"Speed (1-10):", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hPerlinSpeed = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, col2X + 150, y, edtW, 30, hWnd, (HMENU)IDC_EDIT_PERLIN_SPEED, hInst, NULL);
		y += secH;

		HWND hL3 = CreateWindowW(L"STATIC", L"Monitors", WS_CHILD | WS_VISIBLE, col2X, y, 250, 30, hWnd, NULL, hInst, NULL);
		SendMessage(hL3, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 40;

		HWND h7 = CreateWindowW(L"STATIC", L"Primary:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hC1 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, col2X + 150, y, edtW, 300, hWnd, (HMENU)IDC_COMBO_PRIMARY, hInst, NULL); y += rowH;

		HWND h8 = CreateWindowW(L"STATIC", L"Secondary:", WS_CHILD | WS_VISIBLE, col2X + 10, y, lblW, 25, hWnd, NULL, hInst, NULL);
		HWND hC2 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, col2X + 150, y, edtW, 300, hWnd, (HMENU)IDC_COMBO_SECONDARY, hInst, NULL);

		int btnW = 100;
		int btnH = 35;
		int rightBtnX = 400;

		HWND hRand = CreateWindowW(L"BUTTON", L"Randomize every launch", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
			rightBtnX, 517, 200, 30, hWnd, (HMENU)IDC_CHECK_RANDOM, hInst, NULL);

		HWND hPoolBtn = CreateWindowW(L"BUTTON", L"Random Pool...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			rightBtnX + 210, 516, 115, 32, hWnd, (HMENU)IDC_BTN_POOL, hInst, NULL);

		y = 560;

		HWND hOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			rightBtnX, y, btnW, btnH, hWnd, (HMENU)IDOK_BTN, hInst, NULL);

		HWND hReset = CreateWindowW(L"BUTTON", L"Reset", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			rightBtnX + 110, y, btnW, btnH, hWnd, (HMENU)IDRESET_BTN, hInst, NULL);

		HWND hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			rightBtnX + 220, y, btnW, btnH, hWnd, (HMENU)IDCANCEL_BTN, hInst, NULL);

		HWND controls[] = { h1, hA, h2, hB, h3, hS, hDistLbl, hDist, h4, hT, h5, hG1, h6, hG2, h9, hES, h10, hPS, h10b, hDS, h11, hMB, h12, hMS, h13, hPerlinScale, h14, hPerlinSpeed, h7, hC1, h8, hC2, hAnt1, hAntCount, hAnt2, hAntSpeed, hRand, hPoolBtn, hOk, hReset, hCancel };
		for (HWND hw : controls) SendMessage(hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		for (int i = 0; i < NUM_SCREENSAVERS; i++) {
			SendMessage(hC1, CB_ADDSTRING, 0, (LPARAM)g_modeNames[i]);
			SendMessage(hC2, CB_ADDSTRING, 0, (LPARAM)g_modeNames[i]);
		}

		char buf[32];
		sprintf_s(buf, "%.3f", g_ASpeed);        SetWindowTextA(hA, buf);
		sprintf_s(buf, "%.3f", g_BSpeed);        SetWindowTextA(hB, buf);
		sprintf_s(buf, "%.1f", g_DonutSize);     SetWindowTextA(hS, buf);
		sprintf_s(buf, "%.1f", g_DonutDistance); SetWindowTextA(hDist, buf);
		sprintf_s(buf, "%d", g_TextSize);      SetWindowTextA(hT, buf);
		sprintf_s(buf, "%d", g_GolCellSize);   SetWindowTextA(hG1, buf);
		sprintf_s(buf, "%d", g_GolSpeed);      SetWindowTextA(hG2, buf);
		sprintf_s(buf, "%.3f", g_EarthSpeed);    SetWindowTextA(hES, buf);
		sprintf_s(buf, "%.1f", g_PongSpeed);     SetWindowTextA(hPS, buf);
		sprintf_s(buf, "%.1f", g_MazeBuildSpeed);SetWindowTextA(hMB, buf);
		sprintf_s(buf, "%.1f", g_MazeSolveSpeed);SetWindowTextA(hMS, buf);
		sprintf_s(buf, "%.4f", g_PerlinScale);   SetWindowTextA(hPerlinScale, buf);
		sprintf_s(buf, "%.2f", g_PerlinSpeed);     SetWindowTextA(hPerlinSpeed, buf);
		sprintf_s(buf, "%.1f", g_DvdSpeed); SetWindowTextA(hDS, buf);
		sprintf_s(buf, "%d", g_AntCount); SetWindowTextA(hAntCount, buf);
		sprintf_s(buf, "%d", g_AntSpeed); SetWindowTextA(hAntSpeed, buf);

		SendMessage(hC1, CB_SETCURSEL, g_ModePrimary, 0);
		SendMessage(hC2, CB_SETCURSEL, g_ModeSecondary, 0);
		SendMessage(hRand, BM_SETCHECK, g_RandomMode ? BST_CHECKED : BST_UNCHECKED, 0);
		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BTN_POOL)
		{
			ShowPoolWindow(hWnd, hInst);
		}
		else if (LOWORD(wParam) == IDOK_BTN)
		{
			char buf[32];
			GetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf, 32); g_ASpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf, 32); g_BSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf, 32); g_DonutSize = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_DONUT_DISTANCE, buf, 32); g_DonutDistance = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf, 32); g_TextSize = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf, 32); g_GolCellSize = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf, 32); g_GolSpeed = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf, 32); g_EarthSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf, 32); g_PongSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_DVD_SPEED, buf, 32); g_DvdSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf, 32); g_MazeBuildSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf, 32); g_MazeSolveSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SCALE, buf, 32); g_PerlinScale = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SPEED, buf, 32); g_PerlinSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_ANT_COUNT, buf, 32); g_AntCount = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_ANT_SPEED, buf, 32); g_AntSpeed = atoi(buf);

			g_ModePrimary = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY), CB_GETCURSEL, 0, 0);
			g_ModeSecondary = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_GETCURSEL, 0, 0);
			g_RandomMode = SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;

			if (g_GolCellSize < 1)    g_GolCellSize = 1;
			if (g_GolSpeed < 10)      g_GolSpeed = 10;
			if (g_MazeBuildSpeed < 1) g_MazeBuildSpeed = 1.0f;
			if (g_MazeSolveSpeed < 1) g_MazeSolveSpeed = 1.0f;
			if (g_PerlinSpeed < 0.0f) g_PerlinSpeed = 0.1f;
			if (g_AntCount < 1) g_AntCount = 1;

			SaveSettings();
			PostQuitMessage(0);
		}
		else if (LOWORD(wParam) == IDRESET_BTN)
		{
			char buf[32];
			sprintf_s(buf, "%.3f", DEFAULT_ASPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf);
			sprintf_s(buf, "%.3f", DEFAULT_BSPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf);
			sprintf_s(buf, "%.1f", DEFAULT_DONUTSIZE);   SetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf);
			sprintf_s(buf, "%.1f", DEFAULT_DONUTDISTANCE);   SetDlgItemTextA(hWnd, IDC_EDIT_DONUT_DISTANCE, buf);
			sprintf_s(buf, "%d", DEFAULT_TEXTSIZE);      SetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf);
			sprintf_s(buf, "%d", DEFAULT_GOLCELLSIZE);       SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf);
			sprintf_s(buf, "%d", DEFAULT_GOLSPEED);      SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf);
			sprintf_s(buf, "%.3f", DEFAULT_EARTHSPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf);
			sprintf_s(buf, "%.1f", DEFAULT_PONGSPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf);
			sprintf_s(buf, "%.1f", DEFAULT_DVDSPEED);   SetDlgItemTextA(hWnd, IDC_EDIT_DVD_SPEED, buf);
			sprintf_s(buf, "%.1f", DEFAULT_MAZEBUILDSPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf);
			sprintf_s(buf, "%.1f", DEFAULT_MAZESOLVESPEED);  SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf);
			sprintf_s(buf, "%.4f", DEFAULT_PERLINSCALE); SetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SCALE, buf);
			sprintf_s(buf, "%.2f", DEFAULT_PERLINSPEED); SetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SPEED, buf);
			sprintf_s(buf, "%d", DEFAULT_ANT_COUNT); SetDlgItemTextA(hWnd, IDC_EDIT_ANT_COUNT, buf);
			sprintf_s(buf, "%d", DEFAULT_ANT_SPEED); SetDlgItemTextA(hWnd, IDC_EDIT_ANT_SPEED, buf);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY), CB_SETCURSEL, DEFAULT_MODEPRIMARY, 0);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_SETCURSEL, DEFAULT_MODESECONDARY, 0);
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_SETCHECK, DEFAULT_RANDOMMODE ? BST_CHECKED : BST_UNCHECKED, 0);
			g_RandomPool = DEFAULT_RANDOM_POOL;
		}
		else if (LOWORD(wParam) == IDCANCEL_BTN)
		{
			PostQuitMessage(0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}