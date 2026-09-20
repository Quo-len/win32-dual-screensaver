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
	int gdiLeaks;
	bool passed;
	const char* status;
};

int RunBenchmark(int width, int height, int warmupFrames, int benchFrames)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE || hStdOut == NULL || GetFileType(hStdOut) != FILE_TYPE_PIPE) {
		if (AttachConsole(ATTACH_PARENT_PROCESS)) {
			FILE* fpOut;
			FILE* fpErr;
			freopen_s(&fpOut, "CONOUT$", "w", stdout);
			freopen_s(&fpErr, "CONOUT$", "w", stderr);
		}
	}

	SetConsoleOutputCP(CP_UTF8);

	std::ofstream reportFile("benchmark_report.txt", std::ios::out | std::ios::trunc);

	auto logBoth = [&](const std::string& text) {
		std::cout << text;
		if (reportFile.is_open()) {
			reportFile << text;
			reportFile.flush();
		}
		OutputDebugStringA(text.c_str());
	};

	std::stringstream header;
	header << "\n"
	       << "========================================================================================\n"
	       << "                 DUAL SCREENSAVER HEADLESS PERFORMANCE BENCHMARK                        \n"
	       << "                  Resolution: " << width << "x" << height 
	       << " | Frames: " << benchFrames << " (" << warmupFrames << " warmup)\n"
	       << "========================================================================================\n"
	       << " #   Renderer Name                 Avg (ms)   P95 (ms)   Max FPS  GDI Leaks  Status     \n"
	       << "----------------------------------------------------------------------------------------\n";
	logBoth(header.str());

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

		// 1. Warmup iterations (settles simulation and initial GDI subsystem cache)
		for (int w = 0; w < warmupFrames; ++w) {
			g_renderers[mode](memDC, data, width, height, rect);
		}

		// 2. Timed benchmark iterations
		std::vector<double> frameTimes;
		frameTimes.reserve(benchFrames);
		double totalMs = 0.0;
		double minMs = 1e9;
		double maxMs = 0.0;

		DWORD gdiMidPoint = 0;
		int halfFrames = benchFrames / 2;

		for (int f = 0; f < benchFrames; ++f) {
			if (f == halfFrames) {
				gdiMidPoint = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
			}

			LARGE_INTEGER t0, t1;
			QueryPerformanceCounter(&t0);
			g_renderers[mode](memDC, data, width, height, rect);
			QueryPerformanceCounter(&t1);

			double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
			frameTimes.push_back(ms);
			totalMs += ms;
			if (ms < minMs) minMs = ms;
			if (ms > maxMs) maxMs = ms;
		}

		DWORD gdiAfterBench = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);

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
		     << std::setw(10) << res.gdiLeaks << "  "
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

	// Summary
	std::stringstream summary;
	summary << "----------------------------------------------------------------------------------------\n"
	        << " Summary: " << g_numScreensavers << " screensavers tested | "
	        << (g_numScreensavers - totalFails) << " Passed | "
	        << totalFails << " Failed | Duration: "
	        << std::fixed << std::setprecision(2) << totalDurationSec << "s\n"
	        << "========================================================================================\n\n";
	logBoth(summary.str());

	return (totalFails == 0) ? 0 : 1;
}
