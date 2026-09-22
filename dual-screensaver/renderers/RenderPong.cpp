#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/PongSettings.h"

struct PongState {
    float ballX = -1.0f;
    float ballY = -1.0f;
    float ballDX = 0.0f;
    float ballDY = 0.0f;
    float padLeftY = 0.0f;
    float padRightY = 0.0f;
};

void RenderPong(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<PongState>(9);

    int padWidth = max(5, width / 70);
    int padHeight = max(20, height / 6);
    int ballSize = padWidth;

    float speedX = g_PongSpeed;
    float speedY = g_PongSpeed;

    if (state.ballX < 0.0f) {
        state.ballX = (float)(width / 2);
        state.ballY = (float)(height / 2);
        state.ballDX = speedX;
        state.ballDY = speedY;
        state.padLeftY = (float)(height / 2 - padHeight / 2);
        state.padRightY = (float)(height / 2 - padHeight / 2);
    }

    state.ballDX = (state.ballDX > 0) ? speedX : -speedX;
    state.ballDY = (state.ballDY > 0) ? speedY : -speedY;

    state.ballX += state.ballDX;
    state.ballY += state.ballDY;

    if (state.ballY <= 0.0f) { state.ballY = 0.0f; state.ballDY *= -1.0f; }
    else if (state.ballY + ballSize >= height) { state.ballY = (float)(height - ballSize); state.ballDY *= -1.0f; }

    if (state.ballX <= padWidth) { state.ballX = (float)padWidth; state.ballDX *= -1.0f; }
    else if (state.ballX + ballSize >= width - padWidth) { state.ballX = (float)(width - padWidth - ballSize); state.ballDX *= -1.0f; }

    state.padLeftY = state.ballY + (ballSize / 2.0f) - (padHeight / 2.0f);
    state.padRightY = state.ballY + (ballSize / 2.0f) - (padHeight / 2.0f);

    if (state.padLeftY < 0.0f) state.padLeftY = 0.0f;
    if (state.padLeftY > height - padHeight) state.padLeftY = (float)(height - padHeight);
    if (state.padRightY < 0.0f) state.padRightY = 0.0f;
    if (state.padRightY > height - padHeight) state.padRightY = (float)(height - padHeight);

    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

    RECT rectLeft = { 0, (int)state.padLeftY, padWidth, (int)state.padLeftY + padHeight };
    FillRect(memDC, &rectLeft, hBrush);

    RECT rectRight = { width - padWidth, (int)state.padRightY, width, (int)state.padRightY + padHeight };
    FillRect(memDC, &rectRight, hBrush);

    RECT rectBall = { (int)state.ballX, (int)state.ballY, (int)state.ballX + ballSize, (int)state.ballY + ballSize };
    FillRect(memDC, &rectBall, hBrush);

    for (int i = 0; i < height; i += padHeight) {
        RECT dash = { width / 2 - padWidth / 4, i + padHeight / 4, width / 2 + padWidth / 4, i + padHeight * 3 / 4 };
        FillRect(memDC, &dash, hBrush);
    }

    DeleteObject(hBrush);
}

REGISTER_SCREENSAVER(
    9,
    L"Pong",
    "pong",
    { "pong" },
    WRAP_LEGACY(RenderPong),
    GetPongSettings()
);

