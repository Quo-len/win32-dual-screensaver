#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

void RenderPong(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int padWidth = max(5, width / 70);
    int padHeight = max(20, height / 6);
    int ballSize = padWidth;

    float speedX = g_PongSpeed;
    float speedY = g_PongSpeed;

    if (data->pBallX < 0.0f) {
        data->pBallX = (float)(width / 2);
        data->pBallY = (float)(height / 2);
        data->pBallDX = speedX;
        data->pBallDY = speedY;
        data->pPadLeftY = (float)(height / 2 - padHeight / 2);
        data->pPadRightY = (float)(height / 2 - padHeight / 2);
    }

    data->pBallDX = (data->pBallDX > 0) ? speedX : -speedX;
    data->pBallDY = (data->pBallDY > 0) ? speedY : -speedY;

    data->pBallX += data->pBallDX;
    data->pBallY += data->pBallDY;

    if (data->pBallY <= 0.0f) { data->pBallY = 0.0f; data->pBallDY *= -1.0f; }
    else if (data->pBallY + ballSize >= height) { data->pBallY = (float)(height - ballSize); data->pBallDY *= -1.0f; }

    if (data->pBallX <= padWidth) { data->pBallX = (float)padWidth; data->pBallDX *= -1.0f; }
    else if (data->pBallX + ballSize >= width - padWidth) { data->pBallX = (float)(width - padWidth - ballSize); data->pBallDX *= -1.0f; }

    data->pPadLeftY = data->pBallY + (ballSize / 2.0f) - (padHeight / 2.0f);
    data->pPadRightY = data->pBallY + (ballSize / 2.0f) - (padHeight / 2.0f);

    if (data->pPadLeftY < 0.0f) data->pPadLeftY = 0.0f;
    if (data->pPadLeftY > height - padHeight) data->pPadLeftY = (float)(height - padHeight);
    if (data->pPadRightY < 0.0f) data->pPadRightY = 0.0f;
    if (data->pPadRightY > height - padHeight) data->pPadRightY = (float)(height - padHeight);

    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

    RECT rectLeft = { 0, (int)data->pPadLeftY, padWidth, (int)data->pPadLeftY + padHeight };
    FillRect(memDC, &rectLeft, hBrush);

    RECT rectRight = { width - padWidth, (int)data->pPadRightY, width, (int)data->pPadRightY + padHeight };
    FillRect(memDC, &rectRight, hBrush);

    RECT rectBall = { (int)data->pBallX, (int)data->pBallY, (int)data->pBallX + ballSize, (int)data->pBallY + ballSize };
    FillRect(memDC, &rectBall, hBrush);

    for (int i = 0; i < height; i += padHeight) {
        RECT dash = { width / 2 - padWidth / 4, i + padHeight / 4, width / 2 + padWidth / 4, i + padHeight * 3 / 4 };
        FillRect(memDC, &dash, hBrush);
    }

    DeleteObject(hBrush);
}
