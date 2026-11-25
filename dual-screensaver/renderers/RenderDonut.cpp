#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/DonutSettings.h"
#include <math.h>
#include <vector>
#include <string>

struct DonutState {
    float A = 0.0f;
    float B = 0.0f;

    // Sin/cos of the fixed torus sweep angles, computed once
    std::vector<float> sinI, cosI, sinJ, cosJ;

    // Per-frame scratch buffers, reused between frames
    std::vector<float> z;
    std::vector<char> b;

    // Cached text-block layout for the current grid and window size
    int layoutW = -1, layoutH = -1, layoutRectW = -1, layoutRectH = -1;
    int textHeight = 0;
    int lineWidth = 0;
};

static void BuildSweepTable(float step, std::vector<float>& sins, std::vector<float>& coss) {
    sins.clear();
    coss.clear();
    for (float a = 0; a < 6.28f; a += step) {
        sins.push_back(static_cast<float>(sin(a)));
        coss.push_back(static_cast<float>(cos(a)));
    }
}

void RenderDonut(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<DonutState>(0);

    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);

    int W = width / tm.tmAveCharWidth;
    int H = height / tm.tmHeight;
    if (W <= 0) W = 1;
    if (H <= 0) H = 1;

    if (state.sinI.empty()) {
        BuildSweepTable(0.02f, state.sinI, state.cosI);
        BuildSweepTable(0.07f, state.sinJ, state.cosJ);
    }

    std::vector<float>& z = state.z;
    std::vector<char>& b = state.b;
    z.assign(W * H, 0.0f);
    b.assign(W * H, ' ');

    float K2 = g_DonutSize + g_DonutDistance;
    float proj_scale = (g_DonutSize + 3.0f) / (g_DonutSize + 1.0f);
    float x_mult = W * 0.225f * proj_scale;
    float y_mult = H * 0.409f * proj_scale;

    const float e = static_cast<float>(sin(state.A)), g = static_cast<float>(cos(state.A));
    const float m = static_cast<float>(cos(state.B)), n = static_cast<float>(sin(state.B));
    const size_t countI = state.sinI.size();
    const size_t countJ = state.sinJ.size();

    for (size_t jj = 0; jj < countJ; ++jj) {
        const float d = state.cosJ[jj], f = state.sinJ[jj];
        const float h = d + g_DonutSize;
        for (size_t ii = 0; ii < countI; ++ii) {
            float c = state.sinI[ii];
            float D = 1 / (c * h * e + f * g + K2);
            float l = state.cosI[ii];
            float t = c * h * g - f * e;

            int x = (W / 2) + (int)(x_mult * D * (l * h * m - t * n));
            int y = (H / 2) + (int)(y_mult * D * (l * h * n + t * m));
            int o = x + W * y;
            int N = (int)(8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n));

            if (y >= 0 && y < H && x >= 0 && x < W && D > z[o]) {
                z[o] = D;
                b[o] = ".,-~:;=!*#$@"[N > 0 ? N : 0];
            }
        }
    }

    SetTextColor(memDC, RGB(0, 255, 0));
    int rectW = rect.right - rect.left;
    int rectH = rect.bottom - rect.top;
    if (state.layoutW != W || state.layoutH != H || state.layoutRectW != rectW || state.layoutRectH != rectH) {
        // Measure the full W x H text block once, exactly as the original DrawText layout did
        std::string block;
        block.reserve(W * H + H);
        for (int row = 0; row < H; row++) {
            block.append(W, ' ');
            block += '\n';
        }
        RECT calcRect = rect;
        DrawTextA(memDC, block.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);
        state.textHeight = calcRect.bottom - calcRect.top;

        SIZE lineSize = {};
        GetTextExtentPoint32A(memDC, block.c_str(), W, &lineSize);
        state.lineWidth = lineSize.cx;

        state.layoutW = W;
        state.layoutH = H;
        state.layoutRectW = rectW;
        state.layoutRectH = rectH;
    }

    // Draw only the non-blank span of each row; blank cells are background either way
    int originX = (rect.left + rect.right - state.lineWidth) / 2;
    int originY = (rect.bottom - state.textHeight) / 2;
    for (int row = 0; row < H; row++) {
        const char* line = &b[row * W];
        int first = 0;
        while (first < W && line[first] == ' ') first++;
        if (first == W) continue;
        int last = W - 1;
        while (line[last] == ' ') last--;
        TextOutA(memDC, originX + first * tm.tmAveCharWidth, originY + row * tm.tmHeight, line + first, last - first + 1);
    }

    state.A += g_ASpeed;
    state.B += g_BSpeed;
}

REGISTER_SCREENSAVER(
    0,
    L"Donut",
    "donut",
    { "donut" },
    WRAP_LEGACY(RenderDonut),
    GetDonutSettings()
);
