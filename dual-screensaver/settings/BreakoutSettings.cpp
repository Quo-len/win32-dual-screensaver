#include "BreakoutSettings.h"
#include <vector>

constexpr float DEFAULT_BREAKOUT_BALL_SPEED = 3.5f;
constexpr float DEFAULT_BREAKOUT_PADDLE_SPEED = 5.0f;

float g_BreakoutBallSpeed = DEFAULT_BREAKOUT_BALL_SPEED;
float g_BreakoutPaddleSpeed = DEFAULT_BREAKOUT_PADDLE_SPEED;

std::vector<SettingItem> GetBreakoutSettings() {
    return {
        { L"BreakoutBallSpeed", L"Ball Speed:", SettingType::Float, &g_BreakoutBallSpeed, DEFAULT_BREAKOUT_BALL_SPEED, 1.0, 10.0, 1 },
        { L"BreakoutPaddleSpeed", L"Paddle Speed:", SettingType::Float, &g_BreakoutPaddleSpeed, DEFAULT_BREAKOUT_PADDLE_SPEED, 2.0, 15.0, 1 }
    };
}
