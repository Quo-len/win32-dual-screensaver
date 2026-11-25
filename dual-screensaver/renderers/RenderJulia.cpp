#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "GlyphAtlas.h"
#include <math.h>
#include <vector>

struct JuliaState {
    float A = 0.0f;
    std::vector<char> b;
    GlyphAtlas atlas;
    HFONT atlasFont = NULL;
    CenteredTextBlock layout;
    PixelCanvas canvas;
};

void RenderJulia(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<JuliaState>(5);

    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);

    int W = width / tm.tmAveCharWidth;
    int H = height / tm.tmHeight;
    if (W <= 0) W = 1;
    if (H <= 0) H = 1;

    std::vector<char>& b = state.b;
    b.assign(W * H, ' ');

    double zoom = 1.0 + 0.15 * sin(state.A * 0.3);
    double widthInComplex = 3.5 / zoom;
    double heightInComplex = widthInComplex * ((double)H / W) * 2.0;

    double minX = -widthInComplex / 2.0;
    double minY = -heightInComplex / 2.0;

    double dx = widthInComplex / W;
    double dy = heightInComplex / H;

    double cx = 0.7885 * cos(state.A * 0.5);
    double cy = 0.7885 * sin(state.A * 0.5);

    const char* charset = " .,-~:;=!*#$@";
    const char* insideChars = "WM#0@&8Q";
    int maxIter = 80;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            double zx = minX + x * dx;
            double zy = minY + y * dy;
            int iter = 0;
            while (zx * zx + zy * zy < 4.0 && iter < maxIter) {
                double tmp = zx * zx - zy * zy + cx;
                zy = 2.0 * zx * zy + cy;
                zx = tmp;
                iter++;
            }
            if (iter == maxIter) {
                b[y * W + x] = insideChars[(x * 17 + y * 31) % 8];
            }
            else {
                b[y * W + x] = iter < 3 ? ' ' : charset[iter % 13];
            }
        }
    }

    if (state.atlasFont != data->hFont) {
        state.atlas.Build(memDC, data->hFont, L' ', 95);
        state.atlasFont = data->hFont;
    }
    state.layout.Update(memDC, rect, W, H);
    state.canvas.Begin(width, height);

    const int charW = tm.tmAveCharWidth;
    const int charH = tm.tmHeight;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            char c = b[y * W + x];
            if (c == ' ') continue;
            state.atlas.Draw(state.canvas, state.layout.x + x * charW, state.layout.y + y * charH, c - ' ', 0xFFFFFF);
        }
    }
    state.canvas.End(memDC, rect.left, rect.top);

    state.A += g_ASpeed * 0.5f;
}

REGISTER_SCREENSAVER(
    5,
    L"Julia Spirals",
    "julia",
    { "julia", "spirals" },
    WRAP_LEGACY(RenderJulia),
    {}
);
