#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

void RenderMemoryDump(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    // Calculate a font size that fits exactly 78 characters horizontally
    if (data->hexLastWidth != width || data->hHexFont == NULL) {
        if (data->hHexFont) DeleteObject(data->hHexFont);

        int fontHeight = (width / 78) * 2; // Approximate height for standard monospace aspect ratio
        if (fontHeight < 8) fontHeight = 8; // Prevent it from getting too small in preview mode

        // Use CLEARTYPE_QUALITY so the large text stays crisp and smooth
        data->hHexFont = CreateFontA(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, "Consolas");

        data->hexLastWidth = width;
    }

    SelectObject(memDC, data->hHexFont);
    SetBkMode(memDC, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cWidth = tm.tmAveCharWidth;
    int cHeight = tm.tmHeight;

    int lines = height / cHeight;
    if (lines == 0) lines = 1;

    // Center the single column on the screen
    int startX = (width - (78 * cWidth)) / 2;
    if (startX < 0) startX = 0;

    // Control scroll speed
    if (++data->hexDumpScrollDelay > 1) {
        data->hexBaseAddress += 16;
        data->hexDumpScrollDelay = 0;
    }

    uint32_t seed = (uint32_t)(data->hexBaseAddress ^ (data->hexBaseAddress >> 32));
    uint64_t currentAddr = data->hexBaseAddress;

    // Draw lines (+1 to ensure we cover the bottom edge smoothly)
    for (int i = 0; i < lines + 1; i++) {
        char addrPart[16];
        sprintf_s(addrPart, "%08llx", currentAddr & 0xFFFFFFFF);

        char hexPart1[32] = { 0 };
        char hexPart2[32] = { 0 };
        char asciiPart[32] = { 0 };

        asciiPart[0] = '|';

        for (int j = 0; j < 16; j++) {
            seed = seed * 1664525 + 1013904223;
            unsigned char b = (unsigned char)(seed >> 24);

            if (j < 8) {
                sprintf_s(hexPart1 + j * 3, 4, "%02x ", b);
            }
            else {
                sprintf_s(hexPart2 + (j - 8) * 3, 4, "%02x ", b);
            }

            asciiPart[j + 1] = (b >= 32 && b <= 126) ? b : '.';
        }
        asciiPart[17] = '|';
        asciiPart[18] = '\0';

        // Drawing Address (Medium Gray)
        SetTextColor(memDC, RGB(150, 150, 150));
        TextOutA(memDC, startX, i * cHeight, addrPart, 8);

        // Drawing Hex Bytes (Clean White)
        SetTextColor(memDC, RGB(240, 240, 240));
        TextOutA(memDC, startX + 10 * cWidth, i * cHeight, hexPart1, 24);
        TextOutA(memDC, startX + 35 * cWidth, i * cHeight, hexPart2, 24);

        // Drawing ASCII (Medium Gray)
        SetTextColor(memDC, RGB(150, 150, 150));
        TextOutA(memDC, startX + 60 * cWidth, i * cHeight, asciiPart, 18);

        currentAddr += 16;
    }
}
