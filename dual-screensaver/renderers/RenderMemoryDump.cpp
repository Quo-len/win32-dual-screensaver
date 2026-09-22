#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"

struct MemoryDumpState {
    uint64_t baseAddress = 0x00007FF000000000;
    int scrollDelay = 0;
    HFONT hFont = nullptr;
    int lastWidth = 0;
    int lastHeight = 0;

    ~MemoryDumpState() {
        if (hFont) DeleteObject(hFont);
    }
};

void RenderMemoryDump(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<MemoryDumpState>(14);

    if (state.lastWidth != width || state.lastHeight != height || state.hFont == NULL) {
        if (state.hFont) DeleteObject(state.hFont);

        int fontHeight = (width / 78) * 2;
        if (fontHeight < 8) fontHeight = 8;

        state.hFont = CreateFontA(fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, "Consolas");

        state.lastWidth = width;
        state.lastHeight = height;
    }

    SelectObject(memDC, state.hFont);
    SetBkMode(memDC, TRANSPARENT);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cWidth = tm.tmAveCharWidth;
    int cHeight = tm.tmHeight;

    int lines = height / cHeight;
    if (lines == 0) lines = 1;

    int startX = (width - (78 * cWidth)) / 2;
    if (startX < 0) startX = 0;

    if (++state.scrollDelay > 1) {
        state.baseAddress += 16;
        state.scrollDelay = 0;
    }

    uint32_t seed = (uint32_t)(state.baseAddress ^ (state.baseAddress >> 32));
    uint64_t currentAddr = state.baseAddress;

    for (int i = 0; i < lines; i++) {
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

        int yPos = 0;
        if (lines > 1) {
            yPos = i * (height - cHeight) / (lines - 1);
        }

        SetTextColor(memDC, RGB(150, 150, 150));
        TextOutA(memDC, startX, yPos, addrPart, 8);

        SetTextColor(memDC, RGB(240, 240, 240));
        TextOutA(memDC, startX + 10 * cWidth, yPos, hexPart1, 24);
        TextOutA(memDC, startX + 35 * cWidth, yPos, hexPart2, 24);

        SetTextColor(memDC, RGB(150, 150, 150));
        TextOutA(memDC, startX + 60 * cWidth, yPos, asciiPart, 18);

        currentAddr += 16;
    }
}

REGISTER_SCREENSAVER(
    14,
    L"Hex Memory Dump",
    "memory",
    { "memory", "hex", "dump" },
    WRAP_LEGACY(RenderMemoryDump),
    {}
);