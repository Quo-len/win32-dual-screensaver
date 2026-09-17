#include "framework.h"
#include "Renderers.h"
#include "Settings.h"
#include "Perlin.h"
#include <math.h>

void RenderCurlNoise(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    // We reuse flowParticles. Check if we need to initialize or resize.
    if (data->cols != width || data->rows != height || data->pixels.empty() || (int)data->flowParticles.size() != g_CurlCount) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0xFF000000);

        int numParticles = g_CurlCount;
        if (numParticles <= 0) numParticles = 1000;

        data->flowParticles.resize(numParticles);
        for (auto& p : data->flowParticles) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 300 + 50;
        }

        // Initialize perlin permutations
        initPerlin(GetTickCount(), data->perm);
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
    data->flowZOff += 0.001f;

    // We'll use PerlinScale for the spatial frequency
    // If it's too small, the curl will look flat, so we boost it a bit specifically for curl
    float scale = g_PerlinScale * 2.0f;
    float speed = g_PerlinSpeed;

    const float eps = 0.001f; // epsilon for numerical derivative

    int particleCount = (int)data->flowParticles.size();
    for (int i = 0; i < particleCount; i++) {
        auto& p = data->flowParticles[i];

        // Current mapped position
        float nx = p.x * scale;
        float ny = p.y * scale;

        // Compute numerical derivatives of the noise field
        float n_y1 = perlin(nx, ny + eps, data->flowZOff, data->perm);
        float n_y0 = perlin(nx, ny - eps, data->flowZOff, data->perm);
        
        float n_x1 = perlin(nx + eps, ny, data->flowZOff, data->perm);
        float n_x0 = perlin(nx - eps, ny, data->flowZOff, data->perm);

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
