#include "MazeSettings.h"
#include "../Defaults.h"

float g_MazeBuildSpeed = DEFAULT_MAZEBUILDSPEED;
float g_MazeSolveSpeed = DEFAULT_MAZESOLVESPEED;

std::vector<SettingItem> GetMazeSettings() {
    return {
        { L"MazeBuildSpeed", L"Build Spd:", SettingType::Float, &g_MazeBuildSpeed, DEFAULT_MAZEBUILDSPEED, 1.0, 100.0, 1 },
        { L"MazeSolveSpeed", L"Solve Spd:", SettingType::Float, &g_MazeSolveSpeed, DEFAULT_MAZESOLVESPEED, 1.0, 100.0, 1 }
    };
}
