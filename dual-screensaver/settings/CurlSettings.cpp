#include "CurlSettings.h"
#include <vector>

constexpr int DEFAULT_CURL_COUNT = 5000;

int g_CurlCount = DEFAULT_CURL_COUNT;

std::vector<SettingItem> GetCurlSettings() {
    return {
        { L"CurlCount", L"Particles:", SettingType::Int, &g_CurlCount, DEFAULT_CURL_COUNT, 100, 500000, 0 }
    };
}
