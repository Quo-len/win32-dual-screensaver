#include "framework.h"
#include "dual-screensaver.h"
#include "ConfigUI.h"
#include "Settings.h"
#include "ScreenData.h"
#include "Benchmark.h"
#include "utils/SystemMetrics.h"
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
bool g_Force4K = false;


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
		int width = 1920;
		int height = 1080;
		if (wcsstr(lpCmdLine, L"4k") || wcsstr(lpCmdLine, L"4K") || wcsstr(lpCmdLine, L"2160") || wcsstr(lpCmdLine, L"3840"))
		{
			width = 3840;
			height = 2160;
		}
		else if (wcsstr(lpCmdLine, L"720") || wcsstr(lpCmdLine, L"720p"))
		{
			width = 1280;
			height = 720;
		}
		bool visual = (wcsstr(lpCmdLine, L"visual") != NULL || wcsstr(lpCmdLine, L"show") != NULL || wcsstr(lpCmdLine, L"gui") != NULL || wcsstr(lpCmdLine, L"window") != NULL);
		return RunBenchmark(width, height, 25, 75, visual);
	}
	if (wcsstr(lpCmdLine, L"/debug") || wcsstr(lpCmdLine, L"--debug") || wcsstr(lpCmdLine, L"-debug"))
	{
		g_ShowDebugHUD = true;
	}
	if (wcsstr(lpCmdLine, L"/4k") || wcsstr(lpCmdLine, L"--4k") || wcsstr(lpCmdLine, L"-4k") || wcsstr(lpCmdLine, L" 4k"))
	{
		g_Force4K = true;
	}

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
		if (foundArgs[0]) {
			const auto* def1 = ScreensaverRegistry::FindByAlias(foundArgs[0]);
			if (def1) found1 = def1->id;
		}
		if (foundCount == 2 && foundArgs[1]) {
			const auto* def2 = ScreensaverRegistry::FindByAlias(foundArgs[1]);
			if (def2) found2 = def2->id;
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
		int numScreensavers = ScreensaverRegistry::GetCount();
		for (int i = 0; i < numScreensavers; ++i) {
			if (g_RandomPool & (1ULL << i)) {
				pool.push_back(i);
			}
		}
		if (pool.empty()) {
			for (int i = 0; i < numScreensavers; ++i) pool.push_back(i);
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
	if (g_Force4K)
	{
		CreateWindowExW(WS_EX_TOPMOST, szWindowClass, szTitle,
			WS_POPUP | WS_VISIBLE,
			0, 0, 3840, 2160,
			nullptr, nullptr, hInst, (LPVOID)(UINT_PTR)1);
		ShowCursor(FALSE);
		return TRUE;
	}
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
	ProcessMetrics pm = SystemMetrics::GetCurrentMetrics();
	double ramMB = pm.ramMB;
	double cpuPercent = pm.cpuPercent;
	double vramMB = pm.vramMB;
	DWORD gdiHandles = pm.gdiHandles;

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

			data->startTime = GetTickCount64();
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

		RenderContext ctx;
		ctx.hdc = memDC;
		ctx.data = data;
		ctx.width = width;
		ctx.height = height;
		ctx.rect = rect;

		if (data->hFont) {
			TEXTMETRIC tm;
			HGDIOBJ oldF = SelectObject(memDC, data->hFont);
			if (GetTextMetrics(memDC, &tm) && tm.tmAveCharWidth > 0 && tm.tmHeight > 0) {
				ctx.charWidth = tm.tmAveCharWidth;
				ctx.charHeight = tm.tmHeight;
				ctx.termCols = width / tm.tmAveCharWidth;
				ctx.termRows = height / tm.tmHeight;
			}
			SelectObject(memDC, oldF);
		}

		ctx.deltaTime = (data->lastRenderTimeMs > 0.0) ? (float)(data->lastRenderTimeMs / 1000.0) : 0.033f;
		ctx.totalTime = (double)(GetTickCount64() - data->startTime) / 1000.0;
		ctx.frameIndex = data->frameCounter;

		LARGE_INTEGER tStart, tEnd, tFreq;
		QueryPerformanceFrequency(&tFreq);
		QueryPerformanceCounter(&tStart);

		ScreensaverRegistry::Execute(mode, ctx);

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

