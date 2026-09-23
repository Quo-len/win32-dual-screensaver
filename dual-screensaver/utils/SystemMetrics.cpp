#include "SystemMetrics.h"
#include <psapi.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <stdio.h>
#include <cstring>

#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dxgi.lib")

namespace SystemMetrics {

double GetCpuUsage()
{
	static ULONGLONG s_lastSystemTime = 0;
	static ULONGLONG s_lastProcessTime = 0;
	static double s_cpuPercent = 0.0;
	static DWORD s_numCores = 0;

	if (s_numCores == 0)
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		s_numCores = sysInfo.dwNumberOfProcessors ? sysInfo.dwNumberOfProcessors : 1;
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
		// Refresh every ~200ms
		if (sysDiff >= 2000000)
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

double GetVramUsageMB(char* outAdapterName, size_t nameBufSize)
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

ProcessMetrics GetCurrentMetrics()
{
	ProcessMetrics metrics;
	metrics.cpuPercent = GetCpuUsage();
	metrics.vramMB = GetVramUsageMB(metrics.gpuName, sizeof(metrics.gpuName));

	PROCESS_MEMORY_COUNTERS pmc = { sizeof(pmc) };
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
	{
		metrics.ramMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
		metrics.peakRamMB = (double)pmc.PeakWorkingSetSize / (1024.0 * 1024.0);
		metrics.commitMB = (double)pmc.PagefileUsage / (1024.0 * 1024.0);
	}

	metrics.gdiHandles = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
	return metrics;
}

} // namespace SystemMetrics
