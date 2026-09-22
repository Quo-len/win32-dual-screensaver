#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"

struct MatrixState {
    std::vector<int> drops;
    std::vector<wchar_t> chars;
    std::vector<unsigned char> intensity;
    HFONT hFont = nullptr;
    int lastFontSize = 0;

    ~MatrixState() {
        if (hFont) DeleteObject(hFont);
    }
};

void RenderMatrix(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<MatrixState>(2);

    int m_fontSize = data->isPreview ? 10 : g_TextSize;
    if (m_fontSize < 5) m_fontSize = 5;

    int m_cols = width / m_fontSize;
    int m_rows = height / m_fontSize;

    if (m_cols <= 0) m_cols = 1;
    if (m_rows <= 0) m_rows = 1;

    if (state.drops.size() != (size_t)m_cols || state.chars.size() != (size_t)(m_cols * m_rows)) {
        state.drops.assign(m_cols, 0);
        state.chars.assign(m_cols * m_rows, L' ');
        state.intensity.assign(m_cols * m_rows, 0);
        for (int i = 0; i < m_cols; ++i) state.drops[i] = rand() % m_rows;
    }

    for (int x = 0; x < m_cols; x++) {
        for (int y = 0; y < m_rows; y++) {
            int idx = y * m_cols + x;
            if (state.intensity[idx] > 0) {
                int v = state.intensity[idx] - (rand() % 25 + 5);
                state.intensity[idx] = v < 0 ? 0 : v;
            }
        }

        int headY = state.drops[x];
        if (headY >= 0 && headY < m_rows) {
            int idx = headY * m_cols + x;
            state.chars[idx] = 0x30A0 + (rand() % 96);
            state.intensity[idx] = 255;
        }

        state.drops[x]++;
        if (state.drops[x] >= m_rows || (rand() % 100 > 95)) {
            state.drops[x] = 0;
        }
    }

    if (!state.hFont || state.lastFontSize != m_fontSize) {
        if (state.hFont) DeleteObject(state.hFont);
        state.hFont = CreateFontW(m_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH | FF_MODERN, L"MS Gothic");
        state.lastFontSize = m_fontSize;
    }

    SelectObject(memDC, state.hFont);
    SetBkMode(memDC, TRANSPARENT);

    for (int y = 0; y < m_rows; y++) {
        for (int x = 0; x < m_cols; x++) {
            int idx = y * m_cols + x;
            int brightness = state.intensity[idx];
            if (brightness > 0) {
                COLORREF color = (brightness > 240) ? RGB(200, 255, 200) : RGB(0, brightness, 0);
                SetTextColor(memDC, color);
                TextOutW(memDC, x * m_fontSize, y * m_fontSize, &state.chars[idx], 1);
            }
        }
    }
}

REGISTER_SCREENSAVER(
    2,
    L"Matrix",
    "matrix",
    { "matrix" },
    WRAP_LEGACY(RenderMatrix),
    {}
);
