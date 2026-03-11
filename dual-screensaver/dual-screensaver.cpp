#include "framework.h"
#include "dual-screensaver.h"
#include "Settings.h"
#include "ScreenData.h"
#include "Perlin.h"
#include "Renderers.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"DualSaver";
WCHAR szWindowClass[MAX_LOADSTRING] = L"DualSaverClass";

float g_ASpeed = 0.04f;
float g_BSpeed = 0.02f;
float g_DonutSize = 2.0f;
int g_TextSize = 20;
int g_GolCellSize = 2;
int g_GolSpeed = 33;
float g_EarthSpeed = 0.05f;
float g_PongSpeed = 15.0f;
float g_MazeBuildSpeed = 10.0f;
float g_MazeSolveSpeed = 10.0f;
float g_PerlinScale = 0.002f;

// 0=Donut, 1=GoL, 2=Matrix, 3=Earth, 4=Blank, 5=Julia, 6=Stars, 7=DVD, 8=Grid, 9=Pong, 10=Maze, 11=Clock, 12=Perlin Flow Field
int g_ModePrimary = 11;
int g_ModeSecondary = 1;
int g_RandomMode = 0;

const WCHAR* REG_PATH = L"Software\\DualSaver";

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
LRESULT CALLBACK    ConfigWindowProc(HWND, UINT, WPARAM, LPARAM);
void                ShowSettingsWindow(HINSTANCE);

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
		RegQueryValueExW(hKey, L"MazeBuildSpeed", NULL, NULL, (LPBYTE)&g_MazeBuildSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"MazeSolveSpeed", NULL, NULL, (LPBYTE)&g_MazeSolveSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"PerlinScale", NULL, NULL, (LPBYTE)&g_PerlinScale, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModePrimary", NULL, NULL, (LPBYTE)&g_ModePrimary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModeSecondary", NULL, NULL, (LPBYTE)&g_ModeSecondary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"RandomMode", NULL, NULL, (LPBYTE)&g_RandomMode, &size);
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
		RegSetValueExW(hKey, L"TextSize", 0, REG_DWORD, (const BYTE*)&g_TextSize, sizeof(int));
		RegSetValueExW(hKey, L"GolCellSize", 0, REG_DWORD, (const BYTE*)&g_GolCellSize, sizeof(int));
		RegSetValueExW(hKey, L"GolSpeed", 0, REG_DWORD, (const BYTE*)&g_GolSpeed, sizeof(int));
		RegSetValueExW(hKey, L"EarthSpeed", 0, REG_DWORD, (const BYTE*)&g_EarthSpeed, sizeof(float));
		RegSetValueExW(hKey, L"PongSpeed", 0, REG_DWORD, (const BYTE*)&g_PongSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeBuildSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeBuildSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeSolveSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeSolveSpeed, sizeof(float));
		RegSetValueExW(hKey, L"PerlinScale", 0, REG_DWORD, (const BYTE*)&g_PerlinScale, sizeof(float));
		RegSetValueExW(hKey, L"ModePrimary", 0, REG_DWORD, (const BYTE*)&g_ModePrimary, sizeof(int));
		RegSetValueExW(hKey, L"ModeSecondary", 0, REG_DWORD, (const BYTE*)&g_ModeSecondary, sizeof(int));
		RegSetValueExW(hKey, L"RandomMode", 0, REG_DWORD, (const BYTE*)&g_RandomMode, sizeof(int));
		RegCloseKey(hKey);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	srand((unsigned int)time(NULL));
	LoadSettings();

	if (wcsstr(lpCmdLine, L"/c") || wcsstr(lpCmdLine, L"/C"))
	{
		ShowSettingsWindow(hInstance);
		return 0;
	}

	if (g_RandomMode) {
		g_ModePrimary = rand() % 13;
		g_ModeSecondary = rand() % 13;
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

using RenderFn = void(*)(HDC, ScreenData*, int, int, const RECT&);
static const RenderFn g_renderers[] = {
	RenderDonut, RenderGoL,    RenderMatrix, RenderEarth,
	RenderBlank, RenderJulia,  RenderStars,  RenderDVD,
	RenderGrid,  RenderPong,   RenderMaze,   RenderClock,
	RenderPerlin
};

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

			data->startTime = GetTickCount();

			unsigned int monitorSeed = (unsigned int)GetTickCount() + (rand() % 10000);
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

		if (mode >= 0 && mode < (int)(sizeof(g_renderers) / sizeof(g_renderers[0])))
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
		CW_USEDEFAULT, CW_USEDEFAULT, 310, 750,
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
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		HFONT hBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		int y = 10;
		HWND hL1 = CreateWindowW(L"STATIC", L"Donut Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL1, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h1 = CreateWindowW(L"STATIC", L"A Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hA = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_ASPEED, hInst, NULL); y += 30;

		HWND h2 = CreateWindowW(L"STATIC", L"B Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_BSPEED, hInst, NULL); y += 30;

		HWND h3 = CreateWindowW(L"STATIC", L"Donut Size:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_SIZE, hInst, NULL); y += 30;

		HWND h4 = CreateWindowW(L"STATIC", L"Text Size:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hT = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_TEXTSIZE, hInst, NULL); y += 35;

		HWND hL2 = CreateWindowW(L"STATIC", L"Game of Life Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL2, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h5 = CreateWindowW(L"STATIC", L"Cell Size (px):", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hG1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_GOL_SIZE, hInst, NULL); y += 30;

		HWND h6 = CreateWindowW(L"STATIC", L"Speed (ms):", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hG2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_GOL_SPEED, hInst, NULL); y += 35;

		HWND hL4 = CreateWindowW(L"STATIC", L"Earth Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL4, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h9 = CreateWindowW(L"STATIC", L"Spin Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hES = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_EARTH_SPEED, hInst, NULL); y += 35;

		HWND hL5 = CreateWindowW(L"STATIC", L"Ping Pong Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL5, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h10 = CreateWindowW(L"STATIC", L"Game Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hPS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_PONG_SPEED, hInst, NULL); y += 35;

		HWND hL6 = CreateWindowW(L"STATIC", L"Maze Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL6, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h11 = CreateWindowW(L"STATIC", L"Build Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hMB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_MAZE_BUILD_SPEED, hInst, NULL); y += 30;

		HWND h12 = CreateWindowW(L"STATIC", L"Solve Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hMS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_MAZE_SOLVE_SPEED, hInst, NULL); y += 35;

		HWND hL7 = CreateWindowW(L"STATIC", L"Perlin Noise Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL7, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h13 = CreateWindowW(L"STATIC", L"Scale:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hPerlinScale = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_PERLIN_SCALE, hInst, NULL); y += 35;

		HWND hL3 = CreateWindowW(L"STATIC", L"Monitor Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL3, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h7 = CreateWindowW(L"STATIC", L"Primary:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hC1 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, y, 100, 100, hWnd, (HMENU)IDC_COMBO_PRIMARY, hInst, NULL); y += 30;

		HWND h8 = CreateWindowW(L"STATIC", L"Secondary:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hC2 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, y, 100, 100, hWnd, (HMENU)IDC_COMBO_SECONDARY, hInst, NULL); y += 35;

		HWND hRand = CreateWindowW(L"BUTTON", L"Randomize every launch", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 20, y, 250, 20, hWnd, (HMENU)IDC_CHECK_RANDOM, hInst, NULL); y += 35;

		HWND hOk     = CreateWindowW(L"BUTTON", L"OK",     WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 30,  y, 70, 25, hWnd, (HMENU)IDOK_BTN,     hInst, NULL);
		HWND hReset  = CreateWindowW(L"BUTTON", L"Reset",  WS_CHILD | WS_VISIBLE | WS_TABSTOP,                   110, y, 70, 25, hWnd, (HMENU)IDRESET_BTN,  hInst, NULL);
		HWND hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,                   190, y, 70, 25, hWnd, (HMENU)IDCANCEL_BTN, hInst, NULL);

		const WCHAR* options[] = { L"Donut", L"Game of Life", L"Matrix", L"Earth", L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo", L"Grid", L"Pong", L"Maze Generator", L"Odometer Clock", L"Perlin Flow Field" };
		for (int i = 0; i < 13; i++) {
			SendMessage(hC1, CB_ADDSTRING, 0, (LPARAM)options[i]);
			SendMessage(hC2, CB_ADDSTRING, 0, (LPARAM)options[i]);
		}

		HWND controls[] = { h1, hA, h2, hB, h3, hS, h4, hT, h5, hG1, h6, hG2, h9, hES, h10, hPS, h11, hMB, h12, hMS, h13, hPerlinScale, h7, hC1, h8, hC2, hRand, hOk, hReset, hCancel };
		for (HWND hw : controls)
			SendMessage(hw, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		char buf[32];
		sprintf_s(buf, "%.3f", g_ASpeed);        SetWindowTextA(hA, buf);
		sprintf_s(buf, "%.3f", g_BSpeed);        SetWindowTextA(hB, buf);
		sprintf_s(buf, "%.1f", g_DonutSize);     SetWindowTextA(hS, buf);
		sprintf_s(buf, "%d",   g_TextSize);      SetWindowTextA(hT, buf);
		sprintf_s(buf, "%d",   g_GolCellSize);   SetWindowTextA(hG1, buf);
		sprintf_s(buf, "%d",   g_GolSpeed);      SetWindowTextA(hG2, buf);
		sprintf_s(buf, "%.3f", g_EarthSpeed);    SetWindowTextA(hES, buf);
		sprintf_s(buf, "%.1f", g_PongSpeed);     SetWindowTextA(hPS, buf);
		sprintf_s(buf, "%.1f", g_MazeBuildSpeed);SetWindowTextA(hMB, buf);
		sprintf_s(buf, "%.1f", g_MazeSolveSpeed);SetWindowTextA(hMS, buf);
		sprintf_s(buf, "%.4f", g_PerlinScale);   SetWindowTextA(hPerlinScale, buf);
		SendMessage(hC1, CB_SETCURSEL, g_ModePrimary, 0);
		SendMessage(hC2, CB_SETCURSEL, g_ModeSecondary, 0);
		SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_SETCHECK, g_RandomMode ? BST_CHECKED : BST_UNCHECKED, 0);
		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK_BTN)
		{
			char buf[32];
			GetDlgItemTextA(hWnd, IDC_EDIT_ASPEED,           buf, 32); g_ASpeed        = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_BSPEED,           buf, 32); g_BSpeed        = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_SIZE,             buf, 32); g_DonutSize     = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE,         buf, 32); g_TextSize      = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE,         buf, 32); g_GolCellSize   = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED,        buf, 32); g_GolSpeed      = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED,      buf, 32); g_EarthSpeed    = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED,       buf, 32); g_PongSpeed     = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf, 32); g_MazeBuildSpeed= (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf, 32); g_MazeSolveSpeed= (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SCALE,     buf, 32); g_PerlinScale   = (float)atof(buf);

			g_ModePrimary   = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY),   CB_GETCURSEL, 0, 0);
			g_ModeSecondary = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_GETCURSEL, 0, 0);
			g_RandomMode    = SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;

			if (g_GolCellSize < 1)    g_GolCellSize = 1;
			if (g_GolSpeed < 10)      g_GolSpeed = 10;
			if (g_MazeBuildSpeed < 1) g_MazeBuildSpeed = 1.0f;
			if (g_MazeSolveSpeed < 1) g_MazeSolveSpeed = 1.0f;

			SaveSettings();
			PostQuitMessage(0);
		}
		else if (LOWORD(wParam) == IDRESET_BTN)
		{
			char buf[32];
			sprintf_s(buf, "%.3f", 0.04f);  SetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf);
			sprintf_s(buf, "%.3f", 0.02f);  SetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf);
			sprintf_s(buf, "%.1f", 2.0f);   SetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf);
			sprintf_s(buf, "%d",   20);      SetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf);
			sprintf_s(buf, "%d",   2);       SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf);
			sprintf_s(buf, "%d",   33);      SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf);
			sprintf_s(buf, "%.3f", 0.05f);  SetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf);
			sprintf_s(buf, "%.1f", 15.0f);  SetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf);
			sprintf_s(buf, "%.1f", 10.0f);  SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf);
			sprintf_s(buf, "%.1f", 10.0f);  SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf);
			sprintf_s(buf, "%.4f", 0.002f); SetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SCALE, buf);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY),   CB_SETCURSEL, 11,          0);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_SETCURSEL, 1,           0);
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM),    BM_SETCHECK,  BST_UNCHECKED, 0);
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