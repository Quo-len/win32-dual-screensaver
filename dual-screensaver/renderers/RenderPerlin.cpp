#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/PerlinSettings.h"
#include "../utils/Perlin.h"
#include <math.h>

struct FlowParticle {
    float x, y;
    float prev_x, prev_y;
    int life;
};

struct PerlinState {
    std::vector<FlowParticle> flowParticles;
    float flowZOff = 0.0f;
    int perm[512] = { 0 };
    bool initialized = false;
};

void RenderPerlin(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<PerlinState>(12);
    if (!state.initialized) {
        initPerlin((unsigned int)GetTickCount64(), state.perm);
        state.initialized = true;
    }

    if (data->cols != width || data->rows != height || data->pixels.empty()) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0xFF000000);

        int numParticles = (width * height) / 500;
        if (numParticles > 8000) numParticles = 8000;

        state.flowParticles.resize(numParticles);
        for (auto& p : state.flowParticles) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 200 + 50;
        }
    }

    uint32_t* px = data->pixels.data();
    int totalPixels = width * height;
    const int fadeSpeed = 6;

    for (int i = 0; i < totalPixels; i++) {
        uint32_t c = px[i];

        int r = (c >> 16) & 0xFF;
        int g = (c >> 8) & 0xFF;
        int b = c & 0xFF;

        r = (r > fadeSpeed) ? r - fadeSpeed : 0;
        g = (g > fadeSpeed) ? g - fadeSpeed : 0;
        b = (b > fadeSpeed) ? b - fadeSpeed : 0;

        px[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    state.flowZOff += 0.003f;
    const float PI = 3.14159265358979323846f;
    const int speedMultiplier = g_PerlinSpeed;

    int particleCount = (int)state.flowParticles.size();

    int steps = (int)ceil(g_PerlinSpeed);
    float stepFraction = g_PerlinSpeed / (float)steps;

    for (int i = 0; i < particleCount; i++) {
        auto& p = state.flowParticles[i];

        float noiseVal = perlin(p.x * g_PerlinScale, p.y * g_PerlinScale, state.flowZOff, state.perm);
        float angle = noiseVal * PI * 4.0f;

        float vx = cos(angle);
        float vy = sin(angle);

        for (int step = 0; step < steps; ++step) {
            p.x += vx * 2.5f * stepFraction;
            p.y += vy * 2.5f * stepFraction;

            int ix = (int)p.x;
            int iy = (int)p.y;

            if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                int r = (int)((cos(angle) + 1.0f) * 55.0f) + 130;
                int g = (int)((sin(angle) + 1.0f) * 20.0f) + 10;
                int b = 250;

                if (r > 255) r = 255;
                if (g > 255) g = 255;

                data->pixels[iy * width + ix] = 0xFF000000 | (r << 16) | (g << 8) | b;
            }
        }

        p.life -= steps;

        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height || p.life <= 0) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 200 + 50;
        }
        else {
            p.prev_x = p.x;
            p.prev_y = p.y;
        }
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height,
        0, 0, width, height, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

REGISTER_SCREENSAVER(
    12,
    L"Perlin Flow Field",
    "perlin",
    { "perlin", "flow" },
    WRAP_LEGACY(RenderPerlin),
    GetPerlinSettings()
);