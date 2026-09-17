#include "Defaults.h"
#include "framework.h"
#include "dual-screensaver.h"
#include "ConfigUI.h"
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

const WCHAR* g_modeNames[] = {
	L"Donut", L"Game of Life", L"Matrix", L"Earth",
	L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo",
	L"Grid", L"Pong", L"Maze Generator", L"Odometer Clock",
	L"Perlin Flow Field", L"ASCII Fire", L"Hex Memory Dump", L"Sorting Algorithms",
	L"Langton's Ant Symmetrical",
	L"Boids Flocking", L"Cyclic CA", L"Pipes", L"Brian's Brain",
	L"Mandelbrot Zoom", L"Clifford Attractor", L"Curl Noise Particles"
};

using RenderFn = void(*)(HDC, ScreenData*, int, int, const RECT&);
static const RenderFn g_renderers[] = {
	RenderDonut, RenderGoL,    RenderMatrix, RenderEarth,
	RenderBlank, RenderJulia,  RenderStars,  RenderDVD,
	RenderGrid,  RenderPong,   RenderMaze,   RenderClock,
	RenderPerlin, RenderFire, RenderMemoryDump,
	RenderRandomSort, RenderLangton,
	RenderBoids, RenderCyclicCA, RenderPipes, RenderBriansBrain,
	RenderMandelbrot, RenderClifford, RenderCurlNoise
};

#define NUM_SCREENSAVERS (int)(sizeof(g_renderers) / sizeof(g_renderers[0]))

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
LRESULT CALLBACK    ConfigWindowProc(HWND, UINT, WPARAM, LPARAM);



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
		{L"brain", 20}, {L"mandelbrot", 21}, {L"clifford", 22}, {L"curl", 23}
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

	if (wcsstr(lpCmdLine, L"/pool") || wcsstr(lpCmdLine, L"/POOL") || wcsstr(lpCmdLine, L"/c") || wcsstr(lpCmdLine, L"/C"))
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
