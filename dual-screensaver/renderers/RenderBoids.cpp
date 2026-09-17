#include "framework.h"
#include "Renderers.h"
#include <math.h>

static const int   BOID_N      = 150;
static const float MAX_SPD     = 3.0f;
static const float MAX_FORCE   = 0.08f;
static const float SEP_R2      = 35.0f * 35.0f;
static const float ALI_R2      = 70.0f * 70.0f;
static const float COH_R2      = 70.0f * 70.0f;

static void clampVec(float& vx, float& vy, float maxSpd) {
    float spd2 = vx * vx + vy * vy;
    if (spd2 > maxSpd * maxSpd && spd2 > 0.0f) {
        float inv = maxSpd / sqrtf(spd2);
        vx *= inv;
        vy *= inv;
    }
}

void RenderBoids(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    // Init or resize
    if (data->cols != width || data->rows != height || (int)data->flowParticles.size() != BOID_N) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0xFF000000);
        data->flowParticles.resize(BOID_N);
        data->boidVX.resize(BOID_N);
        data->boidVY.resize(BOID_N);
        for (int i = 0; i < BOID_N; i++) {
            data->flowParticles[i].x = (float)(rand() % width);
            data->flowParticles[i].y = (float)(rand() % height);
            float a = (rand() % 6284) * 0.001f;
            data->boidVX[i] = cosf(a) * 2.0f;
            data->boidVY[i] = sinf(a) * 2.0f;
        }
    }

    // Fade trails — asymmetric: faster red fade gives blue/purple tails
    uint32_t* px    = data->pixels.data();
    int       total = width * height;
    for (int i = 0; i < total; i++) {
        uint32_t c = px[i];
        int r = (c >> 16) & 0xFF; r = r > 12 ? r - 12 : 0;
        int g = (c >>  8) & 0xFF; g = g >  6 ? g -  6 : 0;
        int b =  c        & 0xFF; b = b >  4 ? b -  4 : 0;
        px[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    auto& parts = data->flowParticles;
    auto& bvx   = data->boidVX;
    auto& bvy   = data->boidVY;

    for (int i = 0; i < BOID_N; i++) {
        float sx = 0, sy = 0; int sc = 0;  // separation
        float ax = 0, ay = 0; int ac = 0;  // alignment
        float cx = 0, cy = 0; int cc = 0;  // cohesion

        for (int j = 0; j < BOID_N; j++) {
            if (i == j) continue;
            float dx = parts[j].x - parts[i].x;
            float dy = parts[j].y - parts[i].y;
            float d2 = dx * dx + dy * dy;

            if (d2 < SEP_R2 && d2 > 0.01f) {
                float inv = 1.0f / sqrtf(d2);
                sx -= dx * inv; sy -= dy * inv; sc++;
            }
            if (d2 < ALI_R2) { ax += bvx[j]; ay += bvy[j]; ac++; }
            if (d2 < COH_R2) { cx += parts[j].x; cy += parts[j].y; cc++; }
        }

        float fx = 0, fy = 0;

        if (sc > 0) {
            float inv = 1.0f / sc; sx *= inv; sy *= inv;
            clampVec(sx, sy, MAX_SPD); sx -= bvx[i]; sy -= bvy[i];
            clampVec(sx, sy, MAX_FORCE);
            fx += sx * 1.6f; fy += sy * 1.6f;
        }
        if (ac > 0) {
            float inv = 1.0f / ac; ax *= inv; ay *= inv;
            clampVec(ax, ay, MAX_SPD); ax -= bvx[i]; ay -= bvy[i];
            clampVec(ax, ay, MAX_FORCE);
            fx += ax; fy += ay;
        }
        if (cc > 0) {
            float inv = 1.0f / cc;
            cx = cx * inv - parts[i].x;
            cy = cy * inv - parts[i].y;
            clampVec(cx, cy, MAX_SPD); cx -= bvx[i]; cy -= bvy[i];
            clampVec(cx, cy, MAX_FORCE);
            fx += cx; fy += cy;
        }

        bvx[i] += fx; bvy[i] += fy;
        clampVec(bvx[i], bvy[i], MAX_SPD);

        parts[i].x += bvx[i];
        parts[i].y += bvy[i];

        // Toroidal wrap
        if (parts[i].x < 0)       parts[i].x += (float)width;
        if (parts[i].x >= width)   parts[i].x -= (float)width;
        if (parts[i].y < 0)       parts[i].y += (float)height;
        if (parts[i].y >= height)  parts[i].y -= (float)height;

        // Colour from velocity direction — bluish hue, angle-tinted
        float angle = atan2f(bvy[i], bvx[i]);
        int r = (int)((cosf(angle) + 1.0f) * 80.0f) + 95;
        int g = (int)((sinf(angle) + 1.0f) * 55.0f) + 25;
        int b = 235;
        if (r > 255) r = 255;
        if (g > 255) g = 255;

        int ix = (int)parts[i].x;
        int iy = (int)parts[i].y;
        if (ix >= 0 && ix < width && iy >= 0 && iy < height)
            data->pixels[iy * width + ix] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(memDC, 0, 0, width, height,
        0, 0, width, height, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}
