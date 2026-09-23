#include "TetrisSettings.h"
#include <vector>

constexpr float DEFAULT_TETRIS_INITIAL_SPEED = 0.035f;

float g_TetrisInitialSpeed = DEFAULT_TETRIS_INITIAL_SPEED;

std::vector<SettingItem> GetTetrisSettings() {
    return {
        { L"TetrisInitialSpeed", L"Initial Speed:", SettingType::Float, &g_TetrisInitialSpeed, DEFAULT_TETRIS_INITIAL_SPEED, 0.005, 0.200, 3 }
    };
}
