#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include <math.h>

// ---------------------------------------------------------------------------
// Eight curated deep-zoom targets.  minScale is where we stop and cycle to
// the next one (double precision reliable to ~1e-14; 1e-10/1e-9 is safe).
// ---------------------------------------------------------------------------
struct MandTarget { double cx, cy, minScale; };
static const MandTarget TARGETS[] = {
    { -0.74364990000,  0.13182590000, 1e-10 }, // Seahorse Valley
    { -0.74529380000,  0.11300900000, 1e-10 }, // Double Spiral
    {  0.29294000000,  0.01394000000, 1e-10 }, // Elephant Valley
    { -0.77568377000,  0.13646737000, 1e-10 }, // Swirl
    { -0.10109000000,  0.95629000000, 1e-9  }, // Rabbit
    { -1.25066000000,  0.02012000000, 1e-9  }, // Lightning
    {  0.00164372100,  0.82246763300, 1e-9  }, // Tip Spiral
    { -0.72690000000,  0.18890000000, 1e-10 }, // Mini-brot Vortex
};
static const int NUM_TARGETS = 8;

// ---------------------------------------------------------------------------
// Ultrafractal "Electric" palette → COLORREF (for SetTextColor).
// Five colour stops, linearly interpolated, wraps at 1.0 → 0.0.
// ---------------------------------------------------------------------------
static COLORREF mandColor(double t) {
    t = fmod(t, 1.0);
    if (t < 0.0) t += 1.0;
    struct { float pos, r, g, b; } S[] = {
        { 0.0000f, 0.000f, 0.027f, 0.392f }, // deep blue
        { 0.1600f, 0.125f, 0.420f, 0.796f }, // sky blue
        { 0.4200f, 0.929f, 1.000f, 1.000f }, // white-cyan
        { 0.6425f, 1.000f, 0.667f, 0.000f }, // gold
        { 0.8575f, 0.000f, 0.008f, 0.000f }, // near-black green
        { 1.0000f, 0.000f, 0.027f, 0.392f }, // wrap: deep blue
    };
    float ft = (float)t;
    for (int i = 0; i < 5; i++) {
        if (ft >= S[i].pos && ft <= S[i + 1].pos) {
            float f = (ft - S[i].pos) / (S[i+1].pos - S[i].pos);
            int r = (int)((S[i].r + f*(S[i+1].r - S[i].r)) * 255.0f + 0.5f);
            int g = (int)((S[i].g + f*(S[i+1].g - S[i].g)) * 255.0f + 0.5f);
            int b = (int)((S[i].b + f*(S[i+1].b - S[i].b)) * 255.0f + 0.5f);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            return RGB(r, g, b);
        }
    }
    return RGB(0, 0, 7);
}

// ---------------------------------------------------------------------------
// ASCII Mandelbrot zoom.
//
// Rendering strategy (matches the app's ASCII aesthetic):
//   - Uses the shared hFont (Consolas) to get character cell dimensions.
//   - Grid is cols×rows character cells — typically ~192×54 on 1080p.
//     That is only ~10 K cells vs ~500 K pixels, so each frame renders in
//     ~5 ms → genuine smooth 30 fps without any incremental tricks.
//   - Character density ramp maps smooth-iteration → ASCII glyph:
//       space (outer, escaped fast) → dense chars (boundary, escaped slow).
//   - Colour from the Electric palette + slow palette rotation.
//   - Time-based (GetTickCount delta) zoom: exactly the same real-world zoom
//     speed regardless of frame rate.
//   - Cardioid + period-2 bulb pre-rejection skips most interior cells.
//   - Complex-plane step accounts for character aspect ratio so the fractal
//     is geometrically correct (not stretched/squashed).
// ---------------------------------------------------------------------------
struct MandelbrotState {
    double mandCX = -0.74364990000;
    double mandCY = 0.13182590000;
    double mandScale = 3.5;
    double mandPalOff = 0.0;
    int mandTargetIdx = 0;
    DWORD mandLastTick = 0;
};

void RenderMandelbrot(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& state = data->GetCustomState<MandelbrotState>(19);

    // --- Character grid setup ---
    SelectObject(memDC, data->hFont);
    TEXTMETRICA tm;
    GetTextMetricsA(memDC, &tm);
    int cw = tm.tmAveCharWidth;
    int ch = tm.tmHeight;
    if (cw <= 0 || ch <= 0) return;

    int cols = width  / cw;
    int rows = height / ch;
    if (cols <= 0 || rows <= 0) return;

    // --- Init / reinit on resize or first use ---
    if (data->cols != cols || data->rows != rows || state.mandLastTick == 0) {
        data->cols          = cols;
        data->rows          = rows;
        state.mandCX        = TARGETS[0].cx;
        state.mandCY        = TARGETS[0].cy;
        state.mandScale     = 3.5;
        state.mandTargetIdx = 0;
        state.mandLastTick  = GetTickCount();
        state.mandPalOff    = 0.0;
    }

    // --- Frame-rate-independent timing ---
    DWORD  now   = GetTickCount();
    double dtMs  = (double)(int)(now - state.mandLastTick);
    state.mandLastTick = now;
    if (dtMs < 0.0)   dtMs = 0.0;
    if (dtMs > 200.0) dtMs = 200.0; // clamp for pauses / debugger

    // Zoom 1.5 % per 33-ms frame: scale *= exp(-ln(1/0.985) * dt/33.333)
    state.mandScale  *= exp(-0.015114 * dtMs / 33.333);

    // Palette rotates one full cycle in ~40 s at 30 fps
    state.mandPalOff += dtMs * 0.000025;

    // --- Complex-plane mapping ---
    // dx/dy are the complex-plane step per character column / row.
    // We map to screen pixels via: step = scale * charPixelSize / height
    // This keeps the mapping isotropic regardless of font aspect ratio.
    const double ESC2    = 65536.0;             // |z| bailout = 256, ESC2 = 256²
    const double LOG2    = 0.69314718055994530941;
    const double LOG_8L2 = log(8.0 * LOG2);     // pre-computed for smooth colouring
    const int    MXITER  = 200;

    double sc  = state.mandScale;
    double cx  = state.mandCX;
    double cy  = state.mandCY;
    double dx  = sc * cw / height;  // complex units per char-column
    double dy  = sc * ch / height;  // complex units per char-row
    double x0  = cx - 0.5 * cols * dx;
    double y0  = cy - 0.5 * rows * dy;
    double pal = state.mandPalOff;

    // ASCII density ramp: space (fast-escape, outer) → dense (slow-escape, boundary)
    static const char DENS[] = " .,:;+=*#%@W";
    static const int  NDENS  = 12; // number of non-null chars above

    SetBkMode(memDC, TRANSPARENT);

    for (int row = 0; row < rows; row++) {
        double im  = y0 + row * dy;
        double im2 = im * im;

        for (int col = 0; col < cols; col++) {
            double re = x0 + col * dx;

            // Cardioid: q(q + re - 0.25) < im²/4  → interior, skip
            {
                double re4 = re - 0.25;
                double q   = re4 * re4 + im2;
                if (q * (q + re4) < 0.25 * im2) continue;
            }
            // Period-2 bulb: (re+1)² + im² < 0.0625 → interior, skip
            {
                double rp1 = re + 1.0;
                if (rp1 * rp1 + im2 < 0.0625)   continue;
            }

            // Core loop: z = z² + c, z₀ = 0
            double zr = 0.0, zi = 0.0, zr2 = 0.0, zi2 = 0.0;
            int    iter = 0;
            while (zr2 + zi2 < ESC2 && iter < MXITER) {
                zi  = 2.0 * zr * zi + im;
                zr  = zr2 - zi2 + re;
                zr2 = zr * zr;
                zi2 = zi * zi;
                ++iter;
            }
            if (iter == MXITER) continue; // interior → leave black

            // Smooth (continuous) iteration count — eliminates colour banding
            // smooth = iter + 1 - log₂(log₂|z|)
            double log_zn = 0.5 * log(zr2 + zi2);          // log(|z|)
            double nu     = (log(log_zn) - LOG_8L2) / LOG2; // log₂(log₂|z| / log₂(256))
            double smooth = (double)iter + 1.0 - nu;

            // ASCII character: denser glyph = closer to set boundary
            int ci = (int)(smooth / (double)MXITER * (NDENS - 1) + 0.5);
            if (ci < 0) ci = 0;
            if (ci >= NDENS) ci = NDENS - 1;
            char c = DENS[ci];
            if (c == ' ') continue; // outermost region stays black

            // Colour: electric palette + slow phase rotation (~3 cycles / MAX_ITER)
            double t = fmod(smooth * (1.0 / 85.0) + pal, 1.0);
            SetTextColor(memDC, mandColor(t));
            TextOutA(memDC, col * cw, row * ch, &c, 1);
        }
    }

    // Switch to next target when double precision gives out
    if (state.mandScale < TARGETS[state.mandTargetIdx].minScale) {
        state.mandTargetIdx = (state.mandTargetIdx + 1) % NUM_TARGETS;
        state.mandCX    = TARGETS[state.mandTargetIdx].cx;
        state.mandCY    = TARGETS[state.mandTargetIdx].cy;
        state.mandScale = 3.5;
    }
}

REGISTER_SCREENSAVER(19, L"Mandelbrot Zoom", "mandelbrot", { "mandelbrot", "mandel" }, WRAP_LEGACY(RenderMandelbrot), {});
