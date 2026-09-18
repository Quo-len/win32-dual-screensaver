// RenderHarmonograph.cpp  — Triple-Pendulum Harmonograph
//
// Rendering features:
//  • Smooth sub-pixel line segments between consecutive samples
//  • 4 interlaced strands (phase offsets 0, π/4, π/2, 3π/4)
//    → woven, braided figure that glows where strands cross
//  • Velocity-modulated brightness: slow cusps glow bright,
//    fast passages are dimmer (authentic paper-plotter feel)
//  • Per-dot bloom halo (3-px soft kernel)
//  • Frequencies chosen from simple integer / half-integer ratios
//    → proper closed rose / petal Lissajous figures
//  • Additive glow accumulates; buffer fades at 1 unit/frame
//  • Re-randomises every 50 s with a fresh random parameter set

#include "framework.h"
#include "Renderers.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------
namespace {

// Additive blend one pixel (no bounds check — caller guarantees valid idx)
inline void BlendAt(uint32_t* px, int idx, int r, int g, int b)
{
    uint32_t c = px[idx];
    int pr = (int)((c >> 16) & 0xFF) + r; if (pr > 255) pr = 255;
    int pg = (int)((c >>  8) & 0xFF) + g; if (pg > 255) pg = 255;
    int pb = (int)( c        & 0xFF) + b; if (pb > 255) pb = 255;
    px[idx] = 0xFF000000u | ((uint32_t)pr << 16) | ((uint32_t)pg << 8) | (uint32_t)pb;
}

// Sub-pixel bilinear dot  + 1-px soft bloom halo
inline void GlowDot(uint32_t* px, int W, int H,
                    float fx, float fy,
                    int r, int g, int b, float bright)
{
    int ix = (int)fx;
    int iy = (int)fy;
    if (ix < 1 || ix >= W - 1 || iy < 1 || iy >= H - 1) return;

    // --- bilinear core ---
    float rx = fx - (float)ix;
    float ry = fy - (float)iy;
    float w00 = (1.0f - rx) * (1.0f - ry) * bright;
    float w10 =          rx  * (1.0f - ry) * bright;
    float w01 = (1.0f - rx) *          ry  * bright;
    float w11 =          rx  *          ry  * bright;

    auto addW = [&](int base, float w) {
        int ri = (int)(r * w); int gi = (int)(g * w); int bi2 = (int)(b * w);
        if (ri == 0 && gi == 0 && bi2 == 0) return;
        BlendAt(px, base, ri, gi, bi2);
    };
    addW(iy * W + ix,         w00);
    addW(iy * W + ix + 1,     w10);
    addW((iy + 1) * W + ix,   w01);
    addW((iy + 1) * W + ix + 1, w11);

    // --- soft 1-px halo (1/4 of core) ---
    float hb = bright * 0.28f;
    int hr = (int)(r * hb), hg = (int)(g * hb), hb2 = (int)(b * hb);
    if (hr | hg | hb2) {
        BlendAt(px, iy * W + ix - 1,       hr, hg, hb2);
        BlendAt(px, iy * W + ix + 1,       hr, hg, hb2);
        BlendAt(px, (iy - 1) * W + ix,     hr, hg, hb2);
        BlendAt(px, (iy + 1) * W + ix,     hr, hg, hb2);
    }
}

// Draw a glowing line segment by marching at 1-px density
void DrawGlowLine(uint32_t* px, int W, int H,
                  float x0, float y0, float x1, float y1,
                  int r, int g, int b, float bright)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.3f) {
        GlowDot(px, W, H, x0, y0, r, g, b, bright);
        return;
    }
    int steps = (int)ceilf(len * 1.6f);
    if (steps > 1200) steps = 1200;
    float inv = 1.0f / (float)steps;
    for (int i = 0; i <= steps; i++) {
        float ft = (float)i * inv;
        GlowDot(px, W, H, x0 + dx * ft, y0 + dy * ft, r, g, b, bright);
    }
}

// Compact HSV → RGB  (h in [0,1])
inline void HSV(float h, float s, float v, int& r, int& g, int& b)
{
    h -= floorf(h);
    float hi = floorf(h * 6.0f);
    float f  = h * 6.0f - hi;
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - f * s);
    float t2 = v * (1.0f - (1.0f - f) * s);
    float rv, gv, bv;
    switch ((int)hi % 6) {
        case 0: rv = v;  gv = t2; bv = p;  break;
        case 1: rv = q;  gv = v;  bv = p;  break;
        case 2: rv = p;  gv = v;  bv = t2; break;
        case 3: rv = p;  gv = q;  bv = v;  break;
        case 4: rv = t2; gv = p;  bv = v;  break;
        default:rv = v;  gv = p;  bv = q;  break;
    }
    r = (int)(rv * 255.0f);
    g = (int)(gv * 255.0f);
    b = (int)(bv * 255.0f);
}

// Integer and half-integer bases → proper closed Lissajous / rose patterns
static const double kBases[] = {
    1.0, 2.0, 3.0, 4.0, 5.0,          // integers
    1.5, 2.5, 3.5,                      // half-integers
    1.333, 1.667, 2.333, 2.667          // third-integers
};
static const int kNBases = (int)(sizeof(kBases) / sizeof(kBases[0]));

void InitHarmo(ScreenData* data)
{
    for (int i = 0; i < 3; i++) {
        double base  = kBases[rand() % kNBases];
        // Tiny irrational nudge: figure slowly precesses instead of closing
        double drift = ((rand() % 401) - 200) * 0.000035;
        data->harmoP[i].freq  = base + drift;
        data->harmoP[i].amp   = 0.40 + (rand() % 20) * 0.01;   // 0.40..0.59
        data->harmoP[i].phase = (rand() % 6284) * 0.001;
        data->harmoP[i].damp  = 0.0;
    }
    data->harmoT           = 0.0;
    data->harmoInitialized = true;
}

} // namespace

// ---------------------------------------------------------------------------
void RenderHarmonograph(HDC memDC, ScreenData* data, int width, int height, const RECT& rect)
{
    if (width <= 0 || height <= 0) return;

    if (data->cols != width || data->rows != height || data->pixels.empty()) {
        data->cols = width;
        data->rows = height;
        data->pixels.assign((size_t)width * height, 0xFF000000u);
        data->harmoLastTick  = GetTickCount();
        data->harmoGlobalT   = 0.0;
        data->harmoColorT    = 0.0;
        InitHarmo(data);
    }
    if (!data->harmoInitialized) InitHarmo(data);

    uint32_t* px    = data->pixels.data();
    int        total = width * height;

    // -------------------------------------------------------------------
    // Very slow fade — persistence ~5 s, long luminous trails
    // -------------------------------------------------------------------
    for (int i = 0; i < total; i++) {
        uint32_t c = px[i];
        int r = (int)((c >> 16) & 0xFF);
        int g = (int)((c >>  8) & 0xFF);
        int b = (int)( c        & 0xFF);
        r = (r > 1) ? r - 1 : 0;
        g = (g > 1) ? g - 1 : 0;
        b = (b > 1) ? b - 1 : 0;
        px[i] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }

    // -------------------------------------------------------------------
    // Delta time
    // -------------------------------------------------------------------
    DWORD  now  = GetTickCount();
    double dtMs = (double)(int)(now - data->harmoLastTick);
    data->harmoLastTick = now;
    if (dtMs < 0.0)   dtMs = 0.0;
    if (dtMs > 100.0) dtMs = 100.0;
    double dt = dtMs * 0.001;

    data->harmoGlobalT += dt;
    data->harmoColorT  += dt * 0.02;   // very slow global hue drift

    // -------------------------------------------------------------------
    // Periodic reset — new random figure
    // -------------------------------------------------------------------
    if (data->harmoGlobalT >= 50.0) {
        data->harmoGlobalT = 0.0;
        data->harmoT       = 0.0;
        data->pixels.assign((size_t)width * height, 0xFF000000u);
        InitHarmo(data);
        px = data->pixels.data();
    }

    // -------------------------------------------------------------------
    // Draw 4 interlaced strands
    //   Each strand is the same figure but phase-shifted by k*π/4.
    //   They weave together and glow extra-bright where they overlap.
    // -------------------------------------------------------------------
    const int    kStrands   = 4;
    const double kAdvance   = dt * (2.0 * M_PI) / 5.5;   // ~2π every 5.5 s
    const int    kTotalSteps = 1200;
    const int    kStepsPerStrand = kTotalSteps / kStrands;
    const double subStep    = kAdvance / (double)kTotalSteps;

    double scale = (double)(width < height ? width : height) * 0.42;
    double cxd   = (double)width  * 0.5;
    double cyd   = (double)height * 0.5;

    auto& P = data->harmoP;

    // Subtle amplitude breathe — organic pulsing
    double breathe = 1.0 + 0.10 * sin(data->harmoGlobalT * 0.5);

    double t = data->harmoT;

    for (int strand = 0; strand < kStrands; strand++) {
        double phaseOff = strand * (M_PI * 0.5);   // 0, π/2, π, 3π/2

        // Hue: each strand is 90° apart on the colour wheel,
        // plus a slow global drift so the palette slowly rotates
        float hue0 = (float)data->harmoColorT + (float)strand * 0.25f;

        // Track previous point for line drawing
        {
            double x0 = P[0].amp * breathe * sin(P[0].freq * t + P[0].phase + phaseOff)
                       + P[1].amp           * sin(P[1].freq * t + P[1].phase);
            double y0 = P[1].amp           * sin(P[1].freq * t + P[1].phase + M_PI * 0.5)
                       + P[2].amp * breathe * sin(P[2].freq * t + P[2].phase + phaseOff);

            float prevSx = (float)(cxd + x0 * scale);
            float prevSy = (float)(cyd + y0 * scale);

            double localT = t;

            for (int s = 0; s < kStepsPerStrand; s++) {
                localT += subStep * kStrands;   // each strand advances by its own slice

                double x = P[0].amp * breathe * sin(P[0].freq * localT + P[0].phase + phaseOff)
                          + P[1].amp           * sin(P[1].freq * localT + P[1].phase);
                double y = P[1].amp           * sin(P[1].freq * localT + P[1].phase + M_PI * 0.5)
                          + P[2].amp * breathe * sin(P[2].freq * localT + P[2].phase + phaseOff);

                float sx = (float)(cxd + x * scale);
                float sy = (float)(cyd + y * scale);

                // Velocity → brightness: slow near cusps → very bright;
                // fast through centre → dimmer (authentic harmonograph feel)
                float ddx = sx - prevSx;
                float ddy = sy - prevSy;
                float spd = sqrtf(ddx * ddx + ddy * ddy);
                float bright = 4.5f / (1.0f + spd * 0.18f);
                if (bright > 6.0f) bright = 6.0f;

                // Hue slowly shifts along the curve too
                float hue = hue0 + (float)(localT * 0.025);
                int cr, cg, cb;
                HSV(hue, 0.88f, 1.0f, cr, cg, cb);

                DrawGlowLine(px, width, height, prevSx, prevSy, sx, sy,
                             cr, cg, cb, bright);

                prevSx = sx;
                prevSy = sy;
            }
        }
    }

    // Advance the global t pointer by one full advance step
    t += kAdvance;
    data->harmoT = t;

    // -------------------------------------------------------------------
    // Blit to screen
    // -------------------------------------------------------------------
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = width;
    bmi.bmiHeader.biHeight      = -height;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, width, height,
                  0, 0, width, height,
                  px, &bmi, DIB_RGB_COLORS, SRCCOPY);
}
