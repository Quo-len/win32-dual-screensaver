#include "MazeSettings.h"
#include <vector>

constexpr float DEFAULT_MAZEBUILDSPEED = 10.0f;
constexpr float DEFAULT_MAZESOLVESPEED = 10.0f;

float g_MazeBuildSpeed = DEFAULT_MAZEBUILDSPEED;
float g_MazeSolveSpeed = DEFAULT_MAZESOLVESPEED;

std::vector<SettingItem> GetMazeSettings() {
    return {
        { L"MazeBuildSpeed", L"Build Spd:", SettingType::Float, &g_MazeBuildSpeed, DEFAULT_MAZEBUILDSPEED, 1.0, 100.0, 1 },
        { L"MazeSolveSpeed", L"Solve Spd:", SettingType::Float, &g_MazeSolveSpeed, DEFAULT_MAZESOLVESPEED, 1.0, 100.0, 1 }
    };
}
