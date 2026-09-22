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
};

void RenderDonut(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<DonutState>(0);

    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);

    int W = width / tm.tmAveCharWidth;
    int H = height / tm.tmHeight;
    if (W <= 0) W = 1;
    if (H <= 0) H = 1;

    std::vector<float> z(W * H, 0.0f);
    std::vector<char> b(W * H, ' ');

    float K2 = g_DonutSize + g_DonutDistance;
    float proj_scale = (g_DonutSize + 3.0f) / (g_DonutSize + 1.0f);
    float x_mult = W * 0.225f * proj_scale;
    float y_mult = H * 0.409f * proj_scale;

    for (float j = 0; j < 6.28f; j += 0.07f) {
        for (float i = 0; i < 6.28f; i += 0.02f) {
            float c = sin(i), d = cos(j), e = sin(state.A), f = sin(j), g = cos(state.A);
            float h = d + g_DonutSize;
            float D = 1 / (c * h * e + f * g + K2);
            float l = cos(i), m = cos(state.B), n = sin(state.B);
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

    std::string out;
    out.reserve(W * H + H);
    for (int k = 0; k < W * H; k++) {
        out += b[k];
        if ((k + 1) % W == 0) out += '\n';
    }

    SetTextColor(memDC, RGB(0, 255, 0));
    RECT calcRect = rect;
    DrawTextA(memDC, out.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);

    int textHeight = calcRect.bottom - calcRect.top;
    RECT textRect = rect;
    textRect.top = (textRect.bottom - textHeight) / 2;
    DrawTextA(memDC, out.c_str(), -1, &textRect, DT_CENTER);

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
