#include "framework.h"
#include "Renderers.h"
#include <stdio.h>
#include <string.h>
#include <string>

void RenderBlank(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    DWORD elapsed = GetTickCount() - data->startTime;
    int seconds = (elapsed / 1000) % 60;
    int minutes = (elapsed / 60000) % 60;
    int hours = (elapsed / 3600000);

    char msg[128];
    sprintf_s(msg, "< Away from PC for %02d hours, %02d minutes, %02d seconds >", hours, minutes, seconds);
    int msgLen = (int)strlen(msg);

    std::string topDashes(msgLen - 2, '_');
    std::string bottomDashes(msgLen - 2, '-');

    std::string cow = " " + topDashes + "\n" +
        msg + "\n" +
        " " + bottomDashes + "\n" +
        "        \\   ^__^\n" +
        "         \\  (oo)\\_______\n" +
        "            (__)\\       )\\/\\\n" +
        "                ||----w |\n" +
        "                ||     ||";

    SelectObject(memDC, data->hFont);
    SetTextColor(memDC, RGB(200, 200, 200));
    SetBkMode(memDC, TRANSPARENT);

    RECT calcRect = { 0, 0, 0, 0 };
    DrawTextA(memDC, cow.c_str(), -1, &calcRect, DT_CALCRECT | DT_LEFT);

    int cowWidth = calcRect.right - calcRect.left;
    int cowHeight = calcRect.bottom - calcRect.top;

    RECT drawRect;
    drawRect.left = (width / 2) + (int)(width * 0.05f);
    if (drawRect.left + cowWidth > width) drawRect.left = width - cowWidth - 20;

    drawRect.right = drawRect.left + cowWidth;
    drawRect.bottom = height;
    drawRect.top = drawRect.bottom - cowHeight;

    DrawTextA(memDC, cow.c_str(), -1, &drawRect, DT_LEFT);
}
