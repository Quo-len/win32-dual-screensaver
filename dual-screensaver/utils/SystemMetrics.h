#pragma once
#include <windows.h>
#include <cstdint>

struct ProcessMetrics {
    double cpuPercent = 0.0;
    double ramMB = 0.0;
    double peakRamMB = 0.0;
    double commitMB = 0.0;
    double vramMB = 0.0;
    char gpuName[128] = { 0 };
    DWORD gdiHandles = 0;
};

namespace SystemMetrics {
    double GetCpuUsage();
    double GetVramUsageMB(char* outAdapterName = nullptr, size_t nameBufSize = 0);
    ProcessMetrics GetCurrentMetrics();
}
