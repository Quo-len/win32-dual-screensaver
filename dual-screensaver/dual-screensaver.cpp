#include "Defaults.h"
#include "framework.h"
#include "dual-screensaver.h"
#include "ConfigUI.h"
#include "Settings.h"
#include "ScreenData.h"
#include "Perlin.h"
#include "Renderers.h"
#include "Benchmark.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <psapi.h>
#include <combaseapi.h>
#include <dxgi.h>
#include <dxgi1_4.h>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dxgi.lib")

#define MAX_LOADSTRING 100

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"DualSaver";
WCHAR szWindowClass[MAX_LOADSTRING] = L"DualSaverClass";

bool g_ShowDebugHUD = false;

const WCHAR* g_modeNames[] = {
	L"Donut", L"Game of Life", L"Matrix", L"Earth",
	L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo",
	L"Grid", L"Pong", L"Maze Generator", L"Odometer Clock",
	L"Perlin Flow Field", L"ASCII Fire", L"Hex Memory Dump", L"Sorting Algorithms",
	L"Langton's Ant Symmetrical",
	L"Boids Flocking", L"Cyclic CA", L"Pipes", L"Brian's Brain",
	L"Mandelbrot Zoom", L"Clifford Attractor", L"Curl Noise Particles",
	L"Harmonograph", L"Bad Apple (ASCII)", L"ASCIIQuarium",
	L"cbonsai (Bonsai Tree)", L"Nyan Cat (ASCII)"
};

const RenderFn g_renderers[] = {
	RenderDonut, RenderGoL,    RenderMatrix, RenderEarth,
	RenderBlank, RenderJulia,  RenderStars,  RenderDVD,
	RenderGrid,  RenderPong,   RenderMaze,   RenderClock,
	RenderPerlin, RenderFire, RenderMemoryDump,
	RenderRandomSort, RenderLangton,
	RenderBoids, RenderCyclicCA, RenderPipes, RenderBriansBrain,
	RenderMandelbrot, RenderClifford, RenderCurlNoise,
	RenderHarmonograph, RenderBadApple, RenderASCIIQuarium,
	RenderBonsai, RenderNyanCat
};

const int g_numScreensavers = (int)(sizeof(g_renderers) / sizeof(g_renderers[0]));
#define NUM_SCREENSAVERS g_numScreensavers

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

	if (wcsstr(lpCmdLine, L"/benchmark") || wcsstr(lpCmdLine, L"--benchmark") || wcsstr(lpCmdLine, L"/bench") || wcsstr(lpCmdLine, L"-bench"))
	{
		return RunBenchmark();
	}
	if (wcsstr(lpCmdLine, L"/debug") || wcsstr(lpCmdLine, L"--debug") || wcsstr(lpCmdLine, L"-debug"))
	{
		g_ShowDebugHUD = true;
	}

	const struct { const wchar_t* name; int idx; } modeMap[] = {
		{L"donut", 0}, {L"gol", 1}, {L"matrix", 2}, {L"earth", 3},
		{L"blank", 4}, {L"julia", 5}, {L"stars", 6}, {L"dvd", 7},
		{L"grid", 8}, {L"pong", 9}, {L"maze", 10}, {L"clock", 11},
		{L"perlin", 12}, {L"fire", 13}, {L"memory", 14}, {L"sort", 15},
		{L"ant", 16}, {L"boids", 17}, {L"cyclic", 18}, {L"pipes", 19},
		{L"brain", 20}, {L"mandelbrot", 21}, {L"clifford", 22}, {L"curl", 23},
		{L"harmonograph", 24}, {L"harmo", 24},
		{L"badapple", 25}, {L"bad-apple", 25}, {L"apple", 25}, {L"ascii", 25},
		{L"asciiquarium", 26}, {L"aquarium", 26}, {L"fish", 26},
		{L"cbonsai", 27}, {L"bonsai", 27}, {L"tree", 27},
		{L"nyancat", 28}, {L"nyan", 28}, {L"cat", 28}
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
			g_ModeSecondary = found1;
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
		nullptr, nullptr, hInst, (LPVOID)(UINT_PTR)(mi.dwFlags & MONITORINFOF_PRIMARY));

	ShowCursor(FALSE);
	return TRUE;
}

static double GetCurrentProcessCpuUsage()
{
	static ULONGLONG s_lastSystemTime = 0;
	static ULONGLONG s_lastProcessTime = 0;
	static double s_cpuPercent = 0.0;
	static DWORD s_numCores = 0;

	if (s_numCores == 0)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		s_numCores = sysInfo.dwNumberOfProcessors;
		if (s_numCores == 0) s_numCores = 1;
	}

	FILETIME ftCreation, ftExit, ftKernel, ftUser, ftNow;
	GetSystemTimeAsFileTime(&ftNow);
	if (!GetProcessTimes(GetCurrentProcess(), &ftCreation, &ftExit, &ftKernel, &ftUser))
	{
		return 0.0;
	}

	ULARGE_INTEGER now, kernel, user;
	now.LowPart = ftNow.dwLowDateTime; now.HighPart = ftNow.dwHighDateTime;
	kernel.LowPart = ftKernel.dwLowDateTime; kernel.HighPart = ftKernel.dwHighDateTime;
	user.LowPart = ftUser.dwLowDateTime; user.HighPart = ftUser.dwHighDateTime;

	ULONGLONG currentSystemTime = now.QuadPart;
	ULONGLONG currentProcessTime = kernel.QuadPart + user.QuadPart;

	if (s_lastSystemTime != 0)
	{
		ULONGLONG sysDiff = currentSystemTime - s_lastSystemTime;
		ULONGLONG procDiff = currentProcessTime - s_lastProcessTime;
		// Refresh every ~250ms
		if (sysDiff >= 2500000)
		{
			double percent = ((double)procDiff / (double)(sysDiff * s_numCores)) * 100.0;
			s_cpuPercent = (percent < 0.0) ? 0.0 : ((percent > 100.0) ? 100.0 : percent);
			s_lastSystemTime = currentSystemTime;
			s_lastProcessTime = currentProcessTime;
		}
	}
	else
	{
		s_lastSystemTime = currentSystemTime;
		s_lastProcessTime = currentProcessTime;
	}

	return s_cpuPercent;
}

static double GetProcessVramUsageMB(char* outAdapterName, size_t nameBufSize)
{
	static IDXGIFactory4* s_pFactory = nullptr;
	static IDXGIAdapter3* s_pAdapter = nullptr;
	static bool s_inited = false;
	static char s_adapterName[128] = "GPU";

	if (!s_inited)
	{
		s_inited = true;
		if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&s_pFactory))))
		{
			IDXGIAdapter* pAdapt = nullptr;
			if (SUCCEEDED(s_pFactory->EnumAdapters(0, &pAdapt)))
			{
				DXGI_ADAPTER_DESC desc;
				if (SUCCEEDED(pAdapt->GetDesc(&desc)))
				{
					WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, s_adapterName, sizeof(s_adapterName), NULL, NULL);
				}
				pAdapt->QueryInterface(IID_PPV_ARGS(&s_pAdapter));
				pAdapt->Release();
			}
		}
	}

	if (outAdapterName && nameBufSize > 0)
	{
		strncpy_s(outAdapterName, nameBufSize, s_adapterName, _TRUNCATE);
	}

	if (s_pAdapter)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO memInfo = {};
		if (SUCCEEDED(s_pAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &memInfo)))
		{
			return (double)memInfo.CurrentUsage / (1024.0 * 1024.0);
		}
	}
	return 0.0;
}

static void DrawDebugHUD(HDC hdc, ScreenData* data, int width, int height, int mode)
{
	int boxW = 310;
	int boxH = 158;
	int boxX = width - boxW - 20;
	int boxY = 20;

	// Background card
	HBRUSH bgBrush = CreateSolidBrush(RGB(15, 20, 28));
	HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(55, 75, 110));
	HGDIOBJ oldBrush = SelectObject(hdc, bgBrush);
	HGDIOBJ oldPen = SelectObject(hdc, borderPen);

	RoundRect(hdc, boxX, boxY, boxX + boxW, boxY + boxH, 8, 8);

	SelectObject(hdc, oldBrush);
	SelectObject(hdc, oldPen);
	DeleteObject(bgBrush);
	DeleteObject(borderPen);

	// Font & text setup
	HFONT hHudFont = CreateFontA(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FIXED_PITCH | FF_MODERN, "Consolas");
	HGDIOBJ oldFont = SelectObject(hdc, hHudFont);
	SetBkMode(hdc, TRANSPARENT);

	// Line 1: Title
	SetTextColor(hdc, RGB(255, 205, 50));
	TextOutA(hdc, boxX + 12, boxY + 10, "[F5] PERFORMANCE HUD", 20);

	// Metrics
	DWORD gdiHandles = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
	PROCESS_MEMORY_COUNTERS pmc = { sizeof(pmc) };
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	double ramMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
	double cpuPercent = GetCurrentProcessCpuUsage();
	char gpuName[64] = "GPU";
	double vramMB = GetProcessVramUsageMB(gpuName, sizeof(gpuName));

	char buf[128];

	// Line 2: Mode name
	SetTextColor(hdc, RGB(220, 230, 245));
	char modeNameA[64];
	WideCharToMultiByte(CP_UTF8, 0, g_modeNames[mode], -1, modeNameA, sizeof(modeNameA), NULL, NULL);
	int len = sprintf_s(buf, "Mode: %s (#%02d)", modeNameA, mode);
	TextOutA(hdc, boxX + 12, boxY + 30, buf, len);

	// Line 3: Render time
	double lastMs = data->lastRenderTimeMs;
	double avgMs = data->avgRenderTimeMs;
	COLORREF perfColor = (avgMs > 16.67) ? RGB(255, 90, 90) : (avgMs > 8.0 ? RGB(255, 180, 50) : RGB(80, 230, 120));
	SetTextColor(hdc, perfColor);
	len = sprintf_s(buf, "Render: %.2f ms (Avg: %.2f ms)", lastMs, avgMs);
	TextOutA(hdc, boxX + 12, boxY + 50, buf, len);

	// Line 4: FPS
	double maxFps = (avgMs > 0.001) ? (1000.0 / avgMs) : 9999.0;
	SetTextColor(hdc, RGB(180, 200, 220));
	len = sprintf_s(buf, "FPS: %.0f / 30 (Max: %.0f)", data->currentFps, maxFps);
	TextOutA(hdc, boxX + 12, boxY + 70, buf, len);

	// Line 5: CPU & RAM
	SetTextColor(hdc, RGB(130, 210, 255));
	len = sprintf_s(buf, "CPU: %.1f%% | RAM: %.1f MB", cpuPercent, ramMB);
	TextOutA(hdc, boxX + 12, boxY + 90, buf, len);

	// Line 6: GPU & VRAM
	SetTextColor(hdc, RGB(180, 160, 255));
	len = sprintf_s(buf, "GPU: ~0%% (GDI) | VRAM: %.1f MB", vramMB);
	TextOutA(hdc, boxX + 12, boxY + 110, buf, len);

	// Line 7: GDI Handles & Resolution
	SetTextColor(hdc, RGB(140, 170, 200));
	len = sprintf_s(buf, "GDI: %lu objs | Res: %dx%d", gdiHandles, width, height);
	TextOutA(hdc, boxX + 12, boxY + 130, buf, len);

	SelectObject(hdc, oldFont);
	DeleteObject(hHudFont);
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

		LARGE_INTEGER tStart, tEnd, tFreq;
		QueryPerformanceFrequency(&tFreq);
		QueryPerformanceCounter(&tStart);

		if (mode >= 0 && mode < NUM_SCREENSAVERS)
			g_renderers[mode](memDC, data, width, height, rect);

		QueryPerformanceCounter(&tEnd);
		double frameMs = (double)(tEnd.QuadPart - tStart.QuadPart) * 1000.0 / (double)tFreq.QuadPart;
		data->lastRenderTimeMs = frameMs;
		if (data->avgRenderTimeMs <= 0.0) data->avgRenderTimeMs = frameMs;
		else data->avgRenderTimeMs = data->avgRenderTimeMs * 0.9 + frameMs * 0.1;

		data->frameCounter++;
		DWORD now = GetTickCount();
		if (now - data->lastFpsUpdateTick >= 500) {
			data->currentFps = (data->frameCounter * 1000.0) / (double)(now - data->lastFpsUpdateTick);
			data->frameCounter = 0;
			data->lastFpsUpdateTick = now;
		}

		if (g_ShowDebugHUD && !data->isPreview) {
			DrawDebugHUD(memDC, data, width, height, mode);
		}

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
		if (wParam == VK_F5) {
			g_ShowDebugHUD = !g_ShowDebugHUD;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (data && !data->isPreview) PostQuitMessage(0);
		break;
	case WM_DESTROY:
		if (data) {
			if (data->hFont) DeleteObject(data->hFont);
			if (data->hMatrixFont) DeleteObject(data->hMatrixFont);
			if (data->nyanFont) DeleteObject(data->nyanFont);
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

