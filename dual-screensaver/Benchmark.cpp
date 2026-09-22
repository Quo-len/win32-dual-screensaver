#include "Benchmark.h"
#include "ScreenData.h"
#include "dual-screensaver.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <stdio.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

struct BenchResult {
	int index;
	const wchar_t* name;
	double avgMs;
	double minMs;
	double maxMs;
	double p95Ms;
	double maxFps;
	double ramMB;
	double cpuPercent;
	int gdiLeaks;
	bool passed;
	std::string status;
};

static bool s_abortVisual = false;

static LRESULT CALLBACK BenchWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
		s_abortVisual = true;
		DestroyWindow(hWnd);
		return 0;
	}
	if (msg == WM_CLOSE || msg == WM_DESTROY) {
		s_abortVisual = true;
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int RunBenchmark(int width, int height, int warmupFrames, int benchFrames, bool visual)
{
	AllocConsole();
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);

	std::ofstream outFile("benchmark_report.txt", std::ios::out | std::ios::trunc);

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	DWORD numCores = sysInfo.dwNumberOfProcessors ? sysInfo.dwNumberOfProcessors : 1;

	auto logBoth = [&](const std::string& text) {
		std::cout << text;
		if (outFile.is_open()) {
			outFile << text;
			outFile.flush();
		}
		OutputDebugStringA(text.c_str());
	};

	std::stringstream header;
	header << "\n"
	       << "==========================================================================================================\n"
	       << "                         DUAL SCREENSAVER " << (visual ? "VISUAL SHOWCASE &" : "HEADLESS") << " BENCHMARK\n"
	       << "                Resolution: " << width << "x" << height 
	       << " | Frames: " << benchFrames << " (" << warmupFrames << " warmup) | Engine: Win32 GDI (0% GPU)\n"
	       << "==========================================================================================================\n"
	       << " #   Renderer Name                 Avg (ms)   P95 (ms)   Max FPS   CPU %     RAM (MB)   GDI Leaks  Status     \n"
	       << "----------------------------------------------------------------------------------------------------------\n";
	logBoth(header.str());

	HWND benchWnd = NULL;
	HDC winHdc = NULL;
	s_abortVisual = false;

	if (visual) {
		WNDCLASSEXW wc = { sizeof(wc) };
		wc.lpfnWndProc = BenchWndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = L"DualSaverVisualBench";
		RegisterClassExW(&wc);

		int screenW = GetSystemMetrics(SM_CXSCREEN);
		int screenH = GetSystemMetrics(SM_CYSCREEN);
		int dispW = (width <= screenW) ? width : screenW;
		int dispH = (height <= screenH) ? height : screenH;

		benchWnd = CreateWindowExW(WS_EX_APPWINDOW, L"DualSaverVisualBench",
			L"Dual Screensaver - Visual Benchmark Showcase (Press ESC to Exit)",
			WS_POPUP | WS_VISIBLE,
			(screenW - dispW) / 2, (screenH - dispH) / 2, dispW, dispH,
			NULL, NULL, GetModuleHandle(NULL), NULL);
		if (benchWnd) {
			winHdc = GetDC(benchWnd);
			ShowWindow(benchWnd, SW_SHOW);
			UpdateWindow(benchWnd);
		}
	}

	HDC screenDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(screenDC);
	HBITMAP memBmp = CreateCompatibleBitmap(screenDC, width, height);
	HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

	RECT rect = { 0, 0, width, height };

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	std::vector<BenchResult> results;
	int totalFails = 0;
	LARGE_INTEGER totalStart, totalEnd;
	QueryPerformanceCounter(&totalStart);

	for (int mode = 0; mode < g_numScreensavers; ++mode)
	{
		if (s_abortVisual) break;
		DWORD gdiInitial = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);

		ScreenData* data = new ScreenData();
		data->isPrimary = true;
		data->isPreview = false;
		data->hFont = CreateFontA(g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, "Consolas");
		data->hMatrixFont = CreateFontW(g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH | FF_MODERN, L"MS Gothic");
		data->startTime = GetTickCount64();
		initPerlin(12345, data->perm);

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
		ctx.deltaTime = 0.033f;
		ctx.totalTime = 0.0;
		ctx.frameIndex = 0;

		// 1. Warmup iterations (settles simulation and initial GDI subsystem cache)
		for (int w = 0; w < warmupFrames; ++w) {
			if (s_abortVisual) break;
			ctx.frameIndex = w;
			ctx.totalTime = w * 0.033;
			ScreensaverRegistry::Execute(mode, ctx);
			if (visual && winHdc) {
				BitBlt(winHdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		// 2. Timed benchmark iterations
		std::vector<double> frameTimes;
		frameTimes.reserve(benchFrames);
		double totalMs = 0.0;
		double minMs = 1e9;
		double maxMs = 0.0;

		DWORD gdiMidPoint = 0;
		int halfFrames = benchFrames / 2;

		FILETIME ftCreate, ftExit, k0, u0, k1, u1;
		GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &k0, &u0);

		for (int f = 0; f < benchFrames; ++f) {
			if (s_abortVisual) break;
			if (f == halfFrames) {
				gdiMidPoint = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
			}

			ctx.frameIndex = warmupFrames + f;
			ctx.totalTime = (warmupFrames + f) * 0.033;

			LARGE_INTEGER t0, t1;
			QueryPerformanceCounter(&t0);
			ScreensaverRegistry::Execute(mode, ctx);
			QueryPerformanceCounter(&t1);

			double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
			frameTimes.push_back(ms);
			totalMs += ms;
			if (ms < minMs) minMs = ms;
			if (ms > maxMs) maxMs = ms;

			if (visual && winHdc) {
				char hudBuf[128];
				sprintf_s(hudBuf, " [%02d/%02d] %ls | Frame: %.2f ms | ESC to exit ", mode, g_numScreensavers - 1, g_modeNames[mode], ms);
				RECT card = { 20, 20, 460, 52 };
				HBRUSH bg = CreateSolidBrush(RGB(15, 22, 32));
				FillRect(memDC, &card, bg);
				DeleteObject(bg);
				SetBkMode(memDC, TRANSPARENT);
				SetTextColor(memDC, RGB(100, 230, 140));
				DrawTextA(memDC, hudBuf, -1, &card, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

				BitBlt(winHdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &k1, &u1);
		DWORD gdiAfterBench = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);

		// Compute RAM
		PROCESS_MEMORY_COUNTERS pmc = { sizeof(pmc) };
		GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
		double ramMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);

		// Compute CPU utilization % during the loop
		ULARGE_INTEGER ku0, uu0, ku1, uu1;
		ku0.LowPart = k0.dwLowDateTime; ku0.HighPart = k0.dwHighDateTime;
		uu0.LowPart = u0.dwLowDateTime; uu0.HighPart = u0.dwHighDateTime;
		ku1.LowPart = k1.dwLowDateTime; ku1.HighPart = k1.dwHighDateTime;
		uu1.LowPart = u1.dwLowDateTime; uu1.HighPart = u1.dwHighDateTime;

		ULONGLONG proc100ns = (ku1.QuadPart - ku0.QuadPart) + (uu1.QuadPart - uu0.QuadPart);
		double procMs = (double)proc100ns / 10000.0;
		double cpuPercent = (totalMs > 0.0) ? ((procMs / totalMs) * 100.0 / (double)numCores) : 0.0;
		double maxSingleCore = 100.0 / (double)numCores;
		if (cpuPercent > maxSingleCore) cpuPercent = maxSingleCore;

		// 3. Cleanup ScreenData
		if (data->hFont) DeleteObject(data->hFont);
		if (data->hMatrixFont) DeleteObject(data->hMatrixFont);
		if (data->nyanFont) DeleteObject(data->nyanFont);
		delete data;

		// Compute metrics
		double avgMs = totalMs / (double)benchFrames;
		std::sort(frameTimes.begin(), frameTimes.end());
		int p95Index = (int)((double)benchFrames * 0.95);
		if (p95Index >= benchFrames) p95Index = benchFrames - 1;
		double p95Ms = frameTimes[p95Index];
		double maxFps = (avgMs > 0.0001) ? (1000.0 / avgMs) : 99999.0;

		// A real leak accumulates every frame across the second half of execution
		int sustainedLeak = (int)(gdiAfterBench - gdiMidPoint);
		if (sustainedLeak < 0) sustainedLeak = 0;

		BenchResult res;
		res.index = mode;
		res.name = g_modeNames[mode];
		res.avgMs = avgMs;
		res.minMs = minMs;
		res.maxMs = maxMs;
		res.p95Ms = p95Ms;
		res.maxFps = maxFps;
		res.ramMB = ramMB;
		res.cpuPercent = cpuPercent;
		res.gdiLeaks = sustainedLeak;

		if (sustainedLeak > 0) {
			res.passed = false;
			res.status = "FAIL (Leak)";
			totalFails++;
		} else if (avgMs > 16.67) {
			res.passed = true;
			res.status = "WARN (>60fps)";
		} else {
			res.passed = true;
			res.status = "PASS";
		}

		results.push_back(res);

		// Convert wide name for output
		char narrowName[64];
		WideCharToMultiByte(CP_UTF8, 0, res.name, -1, narrowName, sizeof(narrowName), NULL, NULL);

		std::stringstream line;
		line << " " << std::setw(2) << std::setfill('0') << mode << "  "
		     << std::setfill(' ') << std::left << std::setw(28) << narrowName
		     << std::right << std::fixed << std::setprecision(2)
		     << std::setw(7) << avgMs << " ms"
		     << std::setw(8) << p95Ms << " ms"
		     << std::setw(9) << (int)maxFps
		     << std::setw(7) << std::setprecision(1) << cpuPercent << " %"
		     << std::setw(8) << std::setprecision(1) << ramMB << " MB"
		     << std::setw(11) << res.gdiLeaks << "  "
		     << std::left << std::setw(10) << res.status
		     << "\n";
		logBoth(line.str());
	}

	QueryPerformanceCounter(&totalEnd);
	double totalDurationSec = (double)(totalEnd.QuadPart - totalStart.QuadPart) / (double)freq.QuadPart;

	SelectObject(memDC, oldBmp);
	DeleteObject(memBmp);
	DeleteDC(memDC);
	ReleaseDC(NULL, screenDC);

	if (benchWnd) {
		if (winHdc) ReleaseDC(benchWnd, winHdc);
		DestroyWindow(benchWnd);
	}

	// Summary
	std::stringstream summary;
	summary << "----------------------------------------------------------------------------------------------------------\n"
	        << " Summary: " << g_numScreensavers << " screensavers tested | "
	        << (g_numScreensavers - totalFails) << " Passed | "
	        << totalFails << " Failed | Duration: "
	        << std::fixed << std::setprecision(2) << totalDurationSec << "s\n"
	        << "==========================================================================================================\n\n";
	logBoth(summary.str());

	return (totalFails == 0) ? 0 : 1;
}
