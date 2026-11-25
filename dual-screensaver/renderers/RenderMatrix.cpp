#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "GlyphAtlas.h"
#include <vector>

constexpr int MATRIX_GLYPH_COUNT = 96;
constexpr wchar_t MATRIX_GLYPH_BASE = 0x30A0;

struct MatrixState {
    std::vector<int> drops;
    std::vector<unsigned char> glyphs;
    std::vector<unsigned char> intensity;

    GlyphAtlas atlas;
    int atlasFontSize = 0;
    PixelCanvas canvas;
};

void RenderMatrix(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;
    auto& state = data->GetCustomState<MatrixState>(2);

    int m_fontSize = data->isPreview ? 10 : g_TextSize;
    if (m_fontSize < 5) m_fontSize = 5;

    int m_cols = width / m_fontSize;
    int m_rows = height / m_fontSize;

    if (m_cols <= 0) m_cols = 1;
    if (m_rows <= 0) m_rows = 1;

    if (state.drops.size() != (size_t)m_cols || state.glyphs.size() != (size_t)(m_cols * m_rows)) {
        state.drops.assign(m_cols, 0);
        state.glyphs.assign(m_cols * m_rows, 0);
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
            state.glyphs[idx] = static_cast<unsigned char>(rand() % MATRIX_GLYPH_COUNT);
            state.intensity[idx] = 255;
        }

        state.drops[x]++;
        if (state.drops[x] >= m_rows || (rand() % 100 > 95)) {
            state.drops[x] = 0;
        }
    }

    if (state.atlasFontSize != m_fontSize) {
        HFONT hFont = CreateFontW(m_fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH | FF_MODERN, L"MS Gothic");
        state.atlas.Build(memDC, hFont, MATRIX_GLYPH_BASE, MATRIX_GLYPH_COUNT);
        DeleteObject(hFont);
        state.atlasFontSize = m_fontSize;
    }

    state.canvas.Begin(width, height);

    for (int y = 0; y < m_rows; y++) {
        for (int x = 0; x < m_cols; x++) {
            int idx = y * m_cols + x;
            int brightness = state.intensity[idx];
            if (brightness == 0) continue;
            // Bright head, green fading tail
            uint32_t color = (brightness > 240) ? 0xC8FFC8 : (static_cast<uint32_t>(brightness) << 8);
            state.atlas.Draw(state.canvas, x * m_fontSize, y * m_fontSize, state.glyphs[idx], color);
        }
    }

    state.canvas.End(memDC, rect.left, rect.top);
}

REGISTER_SCREENSAVER(
    2,
    L"Matrix",
    "matrix",
    { "matrix" },
    WRAP_LEGACY(RenderMatrix),
    {}
);
