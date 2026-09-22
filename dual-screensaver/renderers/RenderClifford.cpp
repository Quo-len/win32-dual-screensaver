#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <math.h>

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

struct CliffordState {
    double t = 0.0;
    double x = 0.1;
    double y = 0.1;
    DWORD lastTick = 0;
};

void RenderClifford(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<CliffordState>(20);

    if (data->cols != width || data->rows != height || data->pixels.empty()) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign(width * height, 0xFF000000);
        state.t = 0.0;
        state.x = 0.1;
        state.y = 0.1;
        state.lastTick = GetTickCount();
    }

    uint32_t* px = data->pixels.data();
    int totalPixels = width * height;
    const int fadeSpeed = 4; // slow fade for trails

    // Fade the buffer
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

    // Time-based parameter animation
    DWORD now = GetTickCount();
    double dtMs = (double)(int)(now - state.lastTick);
    state.lastTick = now;
    if (dtMs < 0.0) dtMs = 0.0;
    if (dtMs > 200.0) dtMs = 200.0;

    state.t += dtMs * 0.0001; // extremely slow change
    double t = state.t;

    // Mutating parameters A, B, C, D
    double a = 1.4 + 0.3 * sin(t * 0.7);
    double b = 1.7 + 0.3 * cos(t * 1.1);
    double c = 1.1 + 0.3 * sin(t * 1.3);
    double d = 0.8 + 0.3 * cos(t * 0.9);

    // Approximate bounding box based on C and D
    double extX = 1.0 + fabs(c) + 0.2;
    double extY = 1.0 + fabs(d) + 0.2;

    double x = state.x;
    double y = state.y;

    int iters = 30000; // Lower iterations to maintain frame rate and prevent blowing out colors
    
    // Scale and center points
    double scale = (width < height ? width : height) / (2.0 * max(extX, extY));
    double cx = width / 2.0;
    double cy = height / 2.0;

    for (int i = 0; i < iters; i++) {
        // Clifford map equation
        double xnew = sin(a * y) + c * cos(a * x);
        double ynew = sin(b * x) + d * cos(b * y);
        x = xnew; y = ynew;

        int sx = (int)(cx + x * scale);
        int sy = (int)(cy + y * scale);

        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
            int idx = sy * width + sx;
            uint32_t col = px[idx];
            int pr = (col >> 16) & 0xFF;
            int pg = (col >> 8) & 0xFF;
            int pb = col & 0xFF;

            // Subtle additive blending to create a soft, glowing, smooth trail
            int addR = (int)((sin(x * 1.5) + 1.0) * 12.0) + 2;
            int addG = (int)((cos(y * 1.5) + 1.0) * 12.0) + 2;
            int addB = (int)((sin(x * y) + 1.0) * 10.0) + 15;

            pr += addR; if (pr > 255) pr = 255;
            pg += addG; if (pg > 255) pg = 255;
            pb += addB; if (pb > 255) pb = 255;

            px[idx] = 0xFF000000 | (pr << 16) | (pg << 8) | pb;
        }
    }
    
    // Save state for next frame to continue orbit
    state.x = x;
    state.y = y;

    // Render buffer to screen
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height,
        0, 0, width, height, px, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

REGISTER_SCREENSAVER(20, L"Clifford Attractor", "clifford", { "clifford", "attractor" }, WRAP_LEGACY(RenderClifford), {});
