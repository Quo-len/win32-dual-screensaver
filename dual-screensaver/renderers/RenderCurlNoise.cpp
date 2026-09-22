#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include "../settings/CurlSettings.h"
#include "../utils/Perlin.h"
#include <math.h>

struct FlowParticle {
    float x, y;
    float prev_x, prev_y;
    int life;
};

struct CurlState {
    std::vector<FlowParticle> flowParticles;
    float flowZOff = 0.0f;
    int perm[512] = { 0 };
    bool initialized = false;
};

void RenderCurlNoise(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<CurlState>(21);

    if (data->cols != width || data->rows != height || data->pixels.empty() || (int)state.flowParticles.size() != g_CurlCount || !state.initialized) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0xFF000000);

        int numParticles = g_CurlCount;
        if (numParticles <= 0) numParticles = 1000;

        state.flowParticles.resize(numParticles);
        for (auto& p : state.flowParticles) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 300 + 50;
        }

        // Initialize perlin permutations
        initPerlin(GetTickCount(), state.perm);
        state.initialized = true;
    }

    uint32_t* px = data->pixels.data();
    int totalPixels = width * height;
    
    // Fade the background slightly each frame for the trailing effect
    const int fadeSpeed = 4;
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

    // Slowly move forward in the Z axis of the noise field
    state.flowZOff += 0.001f;

    // We'll use PerlinScale for the spatial frequency
    // If it's too small, the curl will look flat, so we boost it a bit specifically for curl
    float scale = g_PerlinScale * 2.0f;
    float speed = g_PerlinSpeed;

    const float eps = 0.001f; // epsilon for numerical derivative

    int particleCount = (int)state.flowParticles.size();
    for (int i = 0; i < particleCount; i++) {
        auto& p = state.flowParticles[i];

        // Current mapped position
        float nx = p.x * scale;
        float ny = p.y * scale;

        // Compute numerical derivatives of the noise field
        float n_y1 = perlin(nx, ny + eps, state.flowZOff, state.perm);
        float n_y0 = perlin(nx, ny - eps, state.flowZOff, state.perm);
        
        float n_x1 = perlin(nx + eps, ny, state.flowZOff, state.perm);
        float n_x0 = perlin(nx - eps, ny, state.flowZOff, state.perm);

        // Curl is (dy, -dx)
        float dy = (n_y1 - n_y0) / (2.0f * eps);
        float dx = (n_x1 - n_x0) / (2.0f * eps);

        float vx = dy;
        float vy = -dx;

        // Normalize velocity to keep particle speed consistent
        float len = sqrt(vx * vx + vy * vy);
        if (len > 0.0001f) {
            vx /= len;
            vy /= len;
        }

        // Apply velocity
        p.x += vx * speed * 3.0f;
        p.y += vy * speed * 3.0f;

        // Decrease life
        p.life--;
        
        // Out of bounds or dead
        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height || p.life <= 0) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 300 + 50;
        }

        int ix = (int)p.x;
        int iy = (int)p.y;

        if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
            // Map the velocity direction to a hue-like color
            // angle goes from -PI to PI
            float angle = atan2(vy, vx);
            // shift angle to 0..1
            float normAngle = (angle + 3.14159265f) / (2.0f * 3.14159265f);

            // Simple HSL to RGB approximation for nice colors
            int r = (int)((sin(normAngle * 6.28318f) * 0.5f + 0.5f) * 200) + 55;
            int g = (int)((sin(normAngle * 6.28318f + 2.09439f) * 0.5f + 0.5f) * 200) + 55;
            int b = (int)((sin(normAngle * 6.28318f + 4.18879f) * 0.5f + 0.5f) * 200) + 55;

            px[iy * width + ix] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }

        p.prev_x = p.x;
        p.prev_y = p.y;
    }

    // Blit the pixel buffer to the device context
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(memDC, rect.left, rect.top, width, height,
        0, 0, 0, height, data->pixels.data(), &bmi, DIB_RGB_COLORS);
}

REGISTER_SCREENSAVER(21, L"Curl Noise Particles", "curl", { "curl", "noise", "particles" }, WRAP_LEGACY(RenderCurlNoise), GetCurlSettings());
