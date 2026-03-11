#include "framework.h"
#include "Renderers.h"
#include "Settings.h"
#include "Perlin.h"
#include <math.h>

void RenderPerlin(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int numParticles = 6000;

    if (data->cols != width || data->rows != height || (int)data->pixels.size() != width * height) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0);

        data->flowParticles.resize(numParticles);
        for (auto& p : data->flowParticles) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 400 + 50;
        }
    }

    uint32_t* px = data->pixels.data();
    std::vector<int> faded;
    faded.reserve(numParticles * 2);

    for (auto& p : data->flowParticles) {
        float angle = perlin(p.x * g_PerlinScale, p.y * g_PerlinScale, data->flowZOff, data->perm) * 3.14159f * 4.0f;
        float vx = cos(angle) * 1.5f;
        float vy = sin(angle) * 1.5f;

        p.prev_x = p.x;
        p.prev_y = p.y;
        p.x += vx;
        p.y += vy;
        p.life--;

        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height || p.life <= 0) {
            p.x = (float)(rand() % width);
            p.y = (float)(rand() % height);
            p.prev_x = p.x;
            p.prev_y = p.y;
            p.life = rand() % 400 + 50;
        }

        float dx = p.x - p.prev_x;
        float dy = p.y - p.prev_y;
        int steps = (int)(max(fabs(dx), fabs(dy))) + 1;
        float xInc = dx / steps;
        float yInc = dy / steps;
        float cx = p.prev_x;
        float cy = p.prev_y;

        for (int i = 0; i <= steps; i++) {
            int px_x = (int)cx;
            int py_y = (int)cy;
            if (px_x >= 0 && px_x < width && py_y >= 0 && py_y < height) {
                int idx = py_y * width + px_x;
                uint32_t c = px[idx];
                uint32_t r = ((c >> 16) & 0xFF) + 35;
                uint32_t g = ((c >> 8) & 0xFF) + 35;
                uint32_t b = (c & 0xFF) + 40;
                if (r > 255) r = 255;
                if (g > 255) g = 255;
                if (b > 255) b = 255;
                px[idx] = (r << 16) | (g << 8) | b;
                faded.push_back(idx);
            }
            cx += xInc;
            cy += yInc;
        }
    }

    for (int idx : faded) {
        uint32_t c = px[idx];
        uint32_t r = (c >> 16) & 0xFF;
        uint32_t g = (c >> 8) & 0xFF;
        uint32_t b = c & 0xFF;
        if (r > 0) r--;
        if (g > 0) g--;
        if (b > 0) b--;
        px[idx] = (r << 16) | (g << 8) | b;
    }
    data->flowZOff += 0.0008f;

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height, 0, 0, width, height,
        data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}