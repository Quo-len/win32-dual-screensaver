#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "resource.h"
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

struct BadAppleState {
    bool loaded = false;
    bool loadAttempted = false;
    DWORD startTime = 0;
    int totalFrames = 0;
    int width = 0;
    int height = 0;
    int fps = 30;
    int levels = 16;
    uint32_t dataOffset = 0;
    std::vector<uint32_t> offsets;
    std::vector<uint8_t> rleData;
    std::vector<uint8_t> frameBuffer;
    HFONT font = nullptr;
    int fontHeight = 0;

    ~BadAppleState() {
        if (font) {
            DeleteObject(font);
            font = nullptr;
        }
    }
};

#pragma pack(push, 1)
struct BapHeader {
    char     magic[4];       // "BAP1"
    uint32_t totalFrames;
    uint16_t width;
    uint16_t height;
    uint16_t fps;
    uint16_t levels;
    uint32_t dataOffset;
    uint8_t  reserved[12];
};
#pragma pack(pop)

// 16-level ASCII density ramp
static const char ASCII_RAMP[16] = {
    ' ', ' ', '.', ':', '-', '=', '+', '*',
    'o', 'a', '#', '%', '&', '8', '$', '@'
};

static std::wstring GetModuleDir() {
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, path, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(path, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
    }
    return std::wstring(path);
}

static bool TryLoadBapData(const uint8_t* pData, size_t dataSize, BadAppleState& state) {
    if (dataSize < sizeof(BapHeader)) {
        return false;
    }

    const BapHeader* header = reinterpret_cast<const BapHeader*>(pData);
    if (memcmp(header->magic, "BAP1", 4) != 0 || header->totalFrames == 0 ||
        header->width == 0 || header->height == 0 || header->fps == 0) {
        return false;
    }

    size_t offsetsByteSize = (size_t)header->totalFrames * sizeof(uint32_t);
    size_t minExpected = sizeof(BapHeader) + offsetsByteSize;
    if (dataSize < minExpected) {
        return false;
    }

    state.totalFrames = header->totalFrames;
    state.width = header->width;
    state.height = header->height;
    state.fps = header->fps;
    state.levels = header->levels > 0 ? header->levels : 16;
    state.dataOffset = header->dataOffset;

    // Read offsets table
    state.offsets.resize(header->totalFrames);
    memcpy(state.offsets.data(), pData + sizeof(BapHeader), offsetsByteSize);

    // Read RLE data
    size_t rleSize = dataSize - minExpected;
    state.rleData.resize(rleSize);
    if (rleSize > 0) {
        memcpy(state.rleData.data(), pData + minExpected, rleSize);
    }

    // Pre-allocate frame buffer
    state.frameBuffer.assign((size_t)header->width * header->height, 0);

    state.startTime = GetTickCount();
    state.loaded = true;
    return true;
}

static bool TryLoadFromResource(BadAppleState& state) {
    HMODULE hMod = GetModuleHandle(NULL);
    HRSRC hRes = FindResource(hMod, MAKEINTRESOURCE(IDR_BAD_APPLE_BIN), RT_RCDATA);
    if (!hRes) return false;

    HGLOBAL hMem = LoadResource(hMod, hRes);
    if (!hMem) return false;

    DWORD resSize = SizeofResource(hMod, hRes);
    const void* pData = LockResource(hMem);
    if (!pData || resSize == 0) return false;

    return TryLoadBapData(reinterpret_cast<const uint8_t*>(pData), (size_t)resSize, state);
}

static bool TryLoadBapFile(const std::wstring& filePath, BadAppleState& state) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        return false;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer((size_t)fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    return TryLoadBapData(buffer.data(), buffer.size(), state);
}

static void LoadBadAppleData(BadAppleState& state) {
    if (state.loadAttempted) return;
    state.loadAttempted = true;

    // 1. Check if external bad_apple.bin exists right next to the executable (allows custom video overrides)
    std::wstring modDir = GetModuleDir();
    std::wstring directPath = modDir + L"\\bad_apple.bin";
    if (TryLoadBapFile(directPath, state)) {
        return;
    }

    // 2. Load embedded resource directly inside .exe (fully self-contained screensaver)
    if (TryLoadFromResource(state)) {
        return;
    }

    // 3. Fallback to developer script paths
    std::vector<std::wstring> searchCandidates = {
        modDir + L"\\assets\\bad_apple.bin",
        modDir + L"\\..\\assets\\bad_apple.bin",
        modDir + L"\\..\\..\\assets\\bad_apple.bin",
        modDir + L"\\..\\bad_apple.bin",
        modDir + L"\\..\\..\\bad_apple.bin",
        modDir + L"\\..\\..\\scripts\\bad_apple.bin",
        modDir + L"\\scripts\\bad_apple.bin",
        L"assets\\bad_apple.bin",
        L"bad_apple.bin",
        L"scripts\\bad_apple.bin",
        L"..\\scripts\\bad_apple.bin"
    };

    for (const auto& candidate : searchCandidates) {
        if (TryLoadBapFile(candidate, state)) {
            break;
        }
    }
}

void RenderBadApple(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<BadAppleState>(23);

    // Fill entire screen with solid black
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(memDC, &rect, blackBrush);

    if (!state.loaded) {
        LoadBadAppleData(state);
    }

    if (!state.loaded) {
        // Display graceful error/setup message
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(220, 50, 50));
        HFONT hErrFont = CreateFontW(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(memDC, hErrFont);

        RECT r = rect;
        r.top += height / 3;
        DrawTextW(memDC,
            L"[ Bad Apple ASCII Screensaver ]\n\n"
            L"Could not locate 'bad_apple.bin'.\n"
            L"Please run: python scripts\\convert_video.py\n"
            L"to generate and deploy the video data file.",
            -1, &r, DT_CENTER | DT_WORDBREAK);

        SelectObject(memDC, oldFont);
        DeleteObject(hErrFont);
        return;
    }

    int cols = state.width;
    int rows = state.height;
    if (cols <= 0 || rows <= 0 || state.totalFrames <= 0) return;

    // Advance frame based on elapsed time and framerate (seamless looping)
    DWORD now = GetTickCount();
    if (state.startTime == 0) {
        state.startTime = now;
    }
    DWORD elapsed = now - state.startTime;
    int frameIdx = (int)(((uint64_t)elapsed * (uint64_t)state.fps) / 1000ULL) % state.totalFrames;

    // Decode RLE frame
    uint32_t offset = state.offsets[frameIdx];
    if (offset < state.rleData.size()) {
        const uint8_t* ptr = state.rleData.data() + offset;
        const uint8_t* endPtr = state.rleData.data() + state.rleData.size();
        uint8_t* dst = state.frameBuffer.data();
        int target = cols * rows;
        int decoded = 0;

        while (decoded < target && ptr + 1 < endPtr) {
            uint8_t count = *ptr++;
            uint8_t val = *ptr++;
            int toFill = (std::min)((int)count, target - decoded);
            memset(dst + decoded, val, toFill);
            decoded += toFill;
        }
    }

    // Determine font size to fit display nicely
    // Monospace fonts have roughly width = height * 0.58
    int maxFontHByScreenH = height / (rows + 1);
    int maxFontHByScreenW = (int)(width / (cols * 0.58f));
    int fontH = (std::max)(4, (std::min)(maxFontHByScreenH, maxFontHByScreenW));

    if (!state.font || state.fontHeight != fontH) {
        if (state.font) {
            DeleteObject(state.font);
        }
        state.font = CreateFontW(
            fontH, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
        );
        state.fontHeight = fontH;
    }

    HGDIOBJ oldFont = SelectObject(memDC, state.font);

    TEXTMETRICW tm;
    GetTextMetricsW(memDC, &tm);
    int charW = tm.tmAveCharWidth;
    int charH = tm.tmHeight;

    // Ensure it strictly fits within the window boundaries
    while ((charW * cols > width || charH * rows > height) && fontH > 3) {
        fontH--;
        SelectObject(memDC, oldFont);
        DeleteObject(state.font);
        state.font = CreateFontW(
            fontH, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
        );
        state.fontHeight = fontH;
        oldFont = SelectObject(memDC, state.font);
        GetTextMetricsW(memDC, &tm);
        charW = tm.tmAveCharWidth;
        charH = tm.tmHeight;
    }

    int totalW = charW * cols;
    int totalH = charH * rows;
    int startX = (std::max)(0, (width - totalW) / 2);
    int startY = (std::max)(0, (height - totalH) / 2);

    SetBkMode(memDC, OPAQUE);
    SetBkColor(memDC, RGB(0, 0, 0));
    SetTextColor(memDC, RGB(235, 235, 235));

    std::vector<char> lineBuf(cols + 1, ' ');
    const uint8_t* frameBuf = state.frameBuffer.data();

    for (int y = 0; y < rows; ++y) {
        const uint8_t* rowSrc = frameBuf + (y * cols);
        for (int x = 0; x < cols; ++x) {
            uint8_t val = rowSrc[x];
            if (val >= 16) val = 15;
            lineBuf[x] = ASCII_RAMP[val];
        }
        TextOutA(memDC, startX, startY + y * charH, lineBuf.data(), cols);
    }

    SelectObject(memDC, oldFont);
}

REGISTER_SCREENSAVER(
    23,
    L"Bad Apple (ASCII)",
    "badapple",
    { "badapple", "bad-apple", "apple", "ascii" },
    WRAP_LEGACY(RenderBadApple),
    {}
);

