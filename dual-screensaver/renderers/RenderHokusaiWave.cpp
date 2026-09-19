// RenderHokusaiWave.cpp
// "The Great Wave off Kanagawa" (Katsushika Hokusai, c.1831)
// Faithful animated ASCII recreation of the iconic woodblock print composition.
//
// Composition (left-to-right, painting layout):
//   - Far left:  The Great Wave, a towering concave wall of water whose crest
//                curls forward in a claw of white foam "fingers" over the trough
//   - Far right: A second smaller wave that mirrors the curve
//   - Center-background: Mount Fuji small white cone, pale and distant
//   - Mid-ocean: Two long oshiokuri-bune boats (mail boats) with rowers
//   - Foreground: Rolling swells and churning foam
//
// Animation phases (loop ~18 sec):
//   Phase 0-0.35 : Wave building / rising
//   Phase 0.35-0.55: Crest at full height, foam claws extend
//   Phase 0.55-0.72: Crash / collapse
//   Phase 0.72-1.0: Recession / regrouping

#include "framework.h"
#include "Renderers.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <cstring>

// ---------------------------------------------------------------------------
// Hokusai Woodblock Color Palette
// The print uses Prussian blue, indigo, off-white cream, and warm grey.
// ---------------------------------------------------------------------------
static const COLORREF C_SKY_PALE      = RGB(185, 210, 232);
static const COLORREF C_SKY_HORIZON   = RGB(140, 180, 215);
static const COLORREF C_FUJI_SNOW     = RGB(238, 240, 245);
static const COLORREF C_FUJI_PEAK     = RGB(160, 185, 210);
static const COLORREF C_FUJI_BASE     = RGB(110, 145, 175);
static const COLORREF C_WAVE_DEEP     = RGB(14,  52, 108);
static const COLORREF C_WAVE_MID      = RGB(24,  80, 145);
static const COLORREF C_WAVE_LIGHT    = RGB(40, 120, 185);
static const COLORREF C_WAVE_SURF     = RGB(65, 150, 205);
static const COLORREF C_FOAM_WHITE    = RGB(252, 252, 255);
static const COLORREF C_FOAM_CREAM    = RGB(215, 235, 250);
static const COLORREF C_TROUGH_DARK   = RGB(8,   30,  72);
static const COLORREF C_BOAT_BROWN    = RGB(90,  65,  35);
static const COLORREF C_BOAT_DARK     = RGB(50,  35,  18);
static const COLORREF C_ROWER_TAN     = RGB(170, 140,  90);

// ---------------------------------------------------------------------------
struct HkCell {
    char ch = ' ';
    COLORREF color = RGB(185, 210, 232);
};

static void hkPut(std::vector<HkCell>& grid, int cols, int rows,
                  int x, int y, char ch, COLORREF col)
{
    if (x >= 0 && x < cols && y >= 0 && y < rows) {
        grid[y * cols + x].ch    = ch;
        grid[y * cols + x].color = col;
    }
}

static void hkStr(std::vector<HkCell>& grid, int cols, int rows,
                  int x, int y, const char* str, COLORREF col)
{
    for (int i = 0; str[i]; ++i)
        if (str[i] != ' ')
            hkPut(grid, cols, rows, x + i, y, str[i], col);
}

static float smoothstep(float e0, float e1, float x) {
    float t = (x - e0) / (e1 - e0);
    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
    return t * t * (3.f - 2.f * t);
}

// ---------------------------------------------------------------------------
// Mount Fuji
// ---------------------------------------------------------------------------
static void DrawFuji(std::vector<HkCell>& grid, int cols, int rows)
{
    int cx  = (int)(cols * 0.63f);
    int ty  = (int)(rows * 0.09f);
    int fH  = (int)(rows * 0.13f);
    if (fH < 5)  fH = 5;
    if (fH > 12) fH = 12;

    for (int r = 0; r < fH; ++r) {
        int y    = ty + r;
        int x0   = cx - r;
        int x1   = cx + r;
        for (int x = x0; x <= x1; ++x) {
            char ch; COLORREF col;
            if (r == 0)               { ch = '^';  col = C_FUJI_SNOW; }
            else if (r <= 2)          { ch = (x==x0)?'/':((x==x1)?'\\':'_'); col = C_FUJI_SNOW; }
            else if (r < fH - 1)      {
                if (x == x0)          { ch = '/';  col = C_FUJI_PEAK; }
                else if (x == x1)     { ch = '\\'; col = C_FUJI_PEAK; }
                else                  { ch = '.';  col = C_FUJI_BASE; }
            } else                    { ch = '_';  col = C_FUJI_BASE; }
            hkPut(grid, cols, rows, x, y, ch, col);
        }
    }
}

// ---------------------------------------------------------------------------
// Single boat
// ---------------------------------------------------------------------------
static void DrawBoat(std::vector<HkCell>& grid, int cols, int rows,
                     int bx, int by, float rockPhase)
{
    by += (int)(sinf(rockPhase) * 0.7f);
    hkStr(grid, cols, rows, bx,      by-2, "___o___",    C_BOAT_DARK);
    hkStr(grid, cols, rows, bx-1,    by-1, "/       \\",  C_BOAT_BROWN);
    hkStr(grid, cols, rows, bx-2,    by,   "/_________\\", C_BOAT_DARK);
    hkPut(grid, cols, rows, bx+1, by-2, 'i', C_ROWER_TAN);
    hkPut(grid, cols, rows, bx+3, by-2, 'i', C_ROWER_TAN);
    hkPut(grid, cols, rows, bx+5, by-2, 'i', C_ROWER_TAN);
}

// ---------------------------------------------------------------------------
// Great Wave profile Y at column x
// Returns surface row (may be above seaY)
// ---------------------------------------------------------------------------
static float GWProfile(float x, float xC, float H, float seaY, float cols)
{
    float d       = x - xC;
    float backW   = cols * 0.30f;
    float frontW  = cols * 0.13f;

    if (d < -backW)        return seaY;
    if (d < 0.f) {
        float t = (d + backW) / backW;
        return seaY - powf(t, 2.1f) * H;
    }
    if (d < frontW) {
        float t = d / frontW;
        return seaY - H * powf(1.f - t, 0.55f);
    }
    // Trough dip
    float t = (d - frontW) / (cols * 0.08f);
    if (t > 1.f) t = 1.f;
    return seaY + sinf(t * 3.14159f) * H * 0.10f;
}

// ---------------------------------------------------------------------------
// Main renderer
// ---------------------------------------------------------------------------
void RenderHokusaiWave(HDC memDC, ScreenData* data, int width, int height, const RECT& rect)
{
    SelectObject(memDC, data->hFont);

    TEXTMETRIC tm;
    GetTextMetrics(memDC, &tm);
    int cW = tm.tmAveCharWidth; if (cW <= 0) cW = 8;
    int cH = tm.tmHeight;       if (cH <= 0) cH = 16;

    int cols = width  / cW; if (cols < 40) cols = 40;
    int rows = height / cH; if (rows < 20) rows = 20;

    // Init
    if (!data->waveInitialized ||
        data->waveWidthInChars  != cols ||
        data->waveHeightInChars != rows)
    {
        data->waveWidthInChars  = cols;
        data->waveHeightInChars = rows;
        data->waveSimTime       = 0.0f;
        data->lighthouseAngle   = 0.0f;
        data->waveParticles.clear();
        data->waveStars.clear();
        data->waveLastTick   = GetTickCount();
        data->waveInitialized = true;
    }

    DWORD now = GetTickCount();
    float dt  = (now - data->waveLastTick) / 1000.0f;
    if (dt <= 0.f || dt > 0.15f) dt = 0.033f;
    data->waveLastTick = now;
    data->waveSimTime += dt;
    float T = data->waveSimTime;

    // ---- Clear background (sky colour) ----
    {
        HBRUSH sky = CreateSolidBrush(C_SKY_PALE);
        FillRect(memDC, &rect, sky);
        DeleteObject(sky);
    }

    // ---- Grid ----
    std::vector<HkCell> grid(cols * rows);
    for (auto& c : grid) { c.ch = ' '; c.color = C_SKY_PALE; }

    // ---- Wave cycle ----
    float period    = 18.0f;
    float wp        = fmodf(T, period) / period;  // 0..1

    // Great wave crest X: sweeps from off-left into view
    float xC_frac   = -0.08f + wp * 0.52f;
    float xC        = xC_frac * (float)cols;

    float heightMax = (float)rows * 0.60f;
    float hNorm;
    if      (wp < 0.35f)  hNorm = smoothstep(0.f, 0.35f, wp);
    else if (wp < 0.55f)  hNorm = 1.0f;
    else if (wp < 0.72f)  hNorm = 1.0f - smoothstep(0.55f, 0.72f, wp);
    else                  hNorm = 0.0f;
    float wH = heightMax * hNorm;

    float breakProg = 0.f;
    if (wp > 0.42f && wp < 0.72f) breakProg = smoothstep(0.42f, 0.72f, wp);

    int seaY = (int)(rows * 0.70f);

    // ---- 1. Background sea fill (below sea level) ----
    for (int y = seaY; y < rows; ++y) {
        float dy = (float)(y - seaY) / (float)(rows - seaY);
        COLORREF oc = (dy < 0.3f) ? C_WAVE_SURF : (dy < 0.6f) ? C_WAVE_MID : C_WAVE_DEEP;
        for (int x = 0; x < cols; ++x) {
            char wch = (x % 2 == 0) ? '~' : '-';
            grid[y * cols + x].ch    = wch;
            grid[y * cols + x].color = oc;
        }
    }

    // ---- 2. Mount Fuji ----
    DrawFuji(grid, cols, rows);

    // ---- 3. Background rolling swells (behind great wave) ----
    for (int x = 0; x < cols; ++x) {
        float sw = sinf(x * 0.11f - T * 0.9f) * 2.2f + cosf(x * 0.18f + T * 0.5f) * 1.1f;
        int sy   = seaY - 4 + (int)sw;
        if (sy >= 0 && sy < rows) {
            char sc = (x % 3 == 0) ? '~' : '-';
            grid[sy * cols + x].ch    = sc;
            grid[sy * cols + x].color = C_WAVE_SURF;
        }
        for (int y = sy + 1; y < seaY; ++y) {
            float dd = (float)(y - sy) / (float)(seaY - sy);
            COLORREF wc = (dd < 0.3f) ? C_WAVE_LIGHT : (dd < 0.6f) ? C_WAVE_MID : C_WAVE_DEEP;
            char wch    = (y % 2 == 0) ? '=' : '-';
            grid[y * cols + x].ch    = wch;
            grid[y * cols + x].color = wc;
        }
    }

    // ---- 4. Boats ----
    if (wp < 0.60f || wp > 0.82f) {
        float rk = T * 2.3f;
        DrawBoat(grid, cols, rows,
                 (int)(cols * 0.52f), seaY - 3 + (int)(sinf(T * 1.1f) * 1.5f), rk);
        DrawBoat(grid, cols, rows,
                 (int)(cols * 0.67f), seaY - 2 + (int)(sinf(T * 1.3f + 1.5f) * 1.5f), rk + 1.7f);
    }

    // ---- 5. Great Wave body ----
    if (wH > 1.f) {
        float backW  = (float)cols * 0.30f;
        float frontW = (float)cols * 0.13f;

        for (int x = (int)(xC - backW); x < (int)(xC + frontW * 2.5f); ++x) {
            if (x < 0 || x >= cols) continue;
            float surfY = GWProfile((float)x, xC, wH, (float)seaY, (float)cols);
            int   surfR = (int)roundf(surfY);
            if (surfR < 0)    surfR = 0;
            if (surfR >= rows) continue;

            float d = (float)x - xC;

            for (int y = surfR; y < rows; ++y) {
                int depth = y - surfR;
                char wch; COLORREF wcol;

                // Inside barrel / trough (front face, shallow depth)
                bool inBarrel = (d > 0.f && d < frontW * 0.65f && depth > 1 && depth < 9);

                if (inBarrel) {
                    wch  = ((x + y) % 4 == 0) ? ':' : ' ';
                    wcol = C_TROUGH_DARK;
                } else if (depth == 0) {
                    // Crest surface
                    if (d >= -3.f && d < frontW && wH > rows * 0.18f) {
                        wch  = (x % 2 == 0) ? '~' : '^';
                        wcol = (breakProg > 0.15f) ? C_FOAM_WHITE : C_WAVE_SURF;
                    } else {
                        wch  = (x % 3 == 0) ? '~' : '-';
                        wcol = C_WAVE_SURF;
                    }
                } else if (depth <= 2)  { wch = (x%2==0)?'=':'~'; wcol = C_WAVE_LIGHT; }
                else if (depth <= 5)    { wch = (x%2==0)?'=':'-'; wcol = C_WAVE_MID;   }
                else                    {
                    wch  = (x%3==0)?'-':((x%5==0)?'~':'=');
                    wcol = C_WAVE_DEEP;
                }
                hkPut(grid, cols, rows, x, y, wch, wcol);
            }

            // Diagonal wave-face striations (Hokusai's curved parallel lines)
            if (d > 2.f && d < frontW) {
                for (int k = 0; k < 5; ++k) {
                    int sx = x - k;
                    int sy = surfR + 2 + k;
                    hkPut(grid, cols, rows, sx, sy, '/', C_WAVE_LIGHT);
                }
            }
        }
    }

    // ---- 6. Claw foam fingers ----
    if (wH > rows * 0.22f && breakProg < 0.88f) {
        int crestRow = (int)roundf(GWProfile(xC, xC, wH, (float)seaY, (float)cols));
        float frontW = (float)cols * 0.13f;

        // Foamy crest band across lip
        for (int fx = (int)(xC - 3); fx <= (int)(xC + frontW * 0.8f); ++fx) {
            float fy = GWProfile((float)fx, xC, wH, (float)seaY, (float)cols);
            int   fr = (int)roundf(fy) - 1;
            char  fc = (fx % 3 == 0) ? '*' : ((fx % 2 == 0) ? '^' : '~');
            COLORREF fcol = (fx % 4 == 0) ? C_FOAM_CREAM : C_FOAM_WHITE;
            hkPut(grid, cols, rows, fx, fr,   fc,  fcol);
            hkPut(grid, cols, rows, fx, fr+1, '~', C_FOAM_WHITE);
        }

        // The iconic "claws": 5 curved tendrils pointing down-right from crest lip
        int numClaws = 5;
        for (int ci = 0; ci < numClaws; ++ci) {
            float clawFrac = (float)ci / (float)(numClaws - 1);
            float clawX    = xC + clawFrac * frontW * 0.55f;
            int   baseRow  = crestRow - 1;
            int   clawLen  = (int)(3.f + breakProg * 9.f);
            if (clawLen > 13) clawLen = 13;

            for (int ck = 0; ck < clawLen; ++ck) {
                float curveFrac = (float)ck / (float)clawLen;
                int cx2 = (int)(clawX + curveFrac * 4.f);
                int cy2 = baseRow + ck;
                char clawCh;
                COLORREF clawCol;
                if      (ck == 0)            { clawCh = '(';  clawCol = C_FOAM_WHITE; }
                else if (ck < clawLen - 1)   { clawCh = (ck%2==0)?'(':'\u007c'; clawCol = (ck<3)?C_FOAM_WHITE:C_FOAM_CREAM; }
                else                         { clawCh = '.';  clawCol = C_FOAM_CREAM; }
                hkPut(grid, cols, rows, cx2, cy2, clawCh, clawCol);
            }
            hkPut(grid, cols, rows, (int)clawX-1, baseRow, '~', C_FOAM_WHITE);
            hkPut(grid, cols, rows, (int)clawX,   baseRow-1, '^', C_FOAM_WHITE);
        }
    }

    // ---- 7. Breaking foam burst ----
    if (breakProg > 0.25f) {
        int cX  = (int)xC;
        int cR  = (int)roundf(GWProfile(xC, xC, wH, (float)seaY, (float)cols));
        int bst = (int)(breakProg * (float)cols * 0.12f);
        for (int bk = 0; bk < bst; ++bk) {
            int bx2 = cX + bk;
            int by2 = cR + (int)(sinf((float)bk * 0.5f + T * 5.f) * 2.f);
            char bc = (bk%4==0)?'*':((bk%3==0)?'o':((bk%2==0)?'.':'%'));
            hkPut(grid, cols, rows, bx2, by2, bc, C_FOAM_WHITE);
        }
    }

    // ---- 8. Spray particles ----
    if (breakProg > 0.08f && breakProg < 0.92f && (rand() % 3 == 0)) {
        int spx = (int)xC + rand() % (int)((float)cols * 0.10f + 1.f);
        int spy = (int)roundf(GWProfile((float)spx, xC, wH, (float)seaY, (float)cols)) - 1;
        ScreenData::WaveParticle p;
        p.x = (float)spx; p.y = (float)spy;
        p.vx = 0.6f + (float)(rand() % 25) / 15.f;
        p.vy = -0.8f - (float)(rand() % 18) / 12.f;
        p.life = 0.f; p.maxLife = 0.5f + (float)(rand() % 12) / 14.f;
        int rt = rand() % 4;
        p.ch    = (rt==0)?'*':(rt==1?'.':'o');
        p.color = (rt < 2) ? C_FOAM_WHITE : C_FOAM_CREAM;
        data->waveParticles.push_back(p);
    }
    for (size_t i = 0; i < data->waveParticles.size(); ) {
        auto& p = data->waveParticles[i];
        p.life += dt;
        p.x    += p.vx * (dt * 25.f);
        p.y    += p.vy * (dt * 25.f);
        p.vy   += 0.10f * (dt * 25.f);
        if (p.life >= p.maxLife || p.x < 0.f || p.x >= (float)cols || p.y < 0.f || p.y >= (float)rows) {
            data->waveParticles.erase(data->waveParticles.begin() + i);
        } else {
            hkPut(grid, cols, rows, (int)roundf(p.x), (int)roundf(p.y), p.ch, p.color);
            ++i;
        }
    }

    // ---- 9. Right mirror wave ----
    {
        float rxC = (float)cols * 0.78f + sinf(T * 0.3f) * (float)cols * 0.02f;
        float rH  = wH * 0.42f;
        float rFW = (float)cols * 0.08f;
        float rBW = (float)cols * 0.17f;

        for (int x = (int)(rxC - rBW); x <= (int)(rxC + rFW * 2.f); ++x) {
            if (x < 0 || x >= cols) continue;
            float d2 = (float)x - rxC;
            float ry;
            if      (d2 < 0.f)  { float t=(d2+rBW)/rBW; ry=(float)seaY-powf(t>0.f?t:0.f,1.8f)*rH; }
            else if (d2 < rFW)  { float t=d2/rFW;        ry=(float)seaY-rH*powf(1.f-t,0.6f);       }
            else                { ry=(float)seaY; }
            int rs = (int)roundf(ry);
            if (rs < 0 || rs >= rows) continue;
            char rc = (x%2==0)?'^':'~';
            COLORREF rcol = (d2 > -2.f && d2 < rFW) ? C_FOAM_CREAM : C_WAVE_SURF;
            hkPut(grid, cols, rows, x, rs, rc, rcol);
            int bdy = (int)(rH * 0.5f);
            for (int yd = 1; yd < bdy && rs+yd < rows; ++yd) {
                hkPut(grid, cols, rows, x, rs+yd, (x%2==0)?'=':'-',
                      yd<3 ? C_WAVE_LIGHT : C_WAVE_MID);
            }
        }
        if (rH > rows * 0.06f) {
            int rcR = (int)((float)seaY - rH);
            for (int fx = (int)(rxC-2); fx <= (int)(rxC+rFW*0.5f); ++fx) {
                hkPut(grid, cols, rows, fx, rcR-1, '^', C_FOAM_WHITE);
                hkPut(grid, cols, rows, fx, rcR,   '~', C_FOAM_CREAM);
            }
        }
    }

    // ---- 10. Foreground chop ----
    for (int x = 0; x < cols; ++x) {
        float chop = sinf(x * 0.22f - T * 2.1f) * 1.5f + sinf(x * 0.35f + T * 1.6f) * 0.8f;
        int cy = seaY + (int)chop + 2;
        if (cy >= 0 && cy < rows)
            hkPut(grid, cols, rows, x, cy, (x%2==0)?'~':'-', C_WAVE_SURF);
    }

    // ---- 11. GDI Span-Batched Rendering ----
    SetBkMode(memDC, OPAQUE);

    for (int y = 0; y < rows; ++y) {
        // Determine background colour for this row band
        COLORREF rowBk;
        if (y < seaY) {
            float skyT = (float)y / (float)seaY;
            rowBk = (skyT < 0.5f) ? C_SKY_PALE : C_SKY_HORIZON;
        } else {
            float dy = (float)(y - seaY) / (float)(rows - seaY);
            rowBk = (dy < 0.3f) ? C_WAVE_SURF : (dy < 0.6f) ? C_WAVE_MID : C_WAVE_DEEP;
        }

        int x = 0;
        while (x < cols) {
            if (grid[y * cols + x].ch == ' ') { ++x; continue; }

            int startX = x;
            COLORREF curCol = grid[y * cols + x].color;
            char span[512]; int spanLen = 0;

            while (x < cols &&
                   grid[y * cols + x].ch != ' ' &&
                   grid[y * cols + x].color == curCol &&
                   spanLen < 500)
            {
                span[spanLen++] = grid[y * cols + x].ch;
                ++x;
            }
            span[spanLen] = '\0';

            SetBkColor(memDC, rowBk);
            SetTextColor(memDC, curCol);
            TextOutA(memDC, startX * cW, y * cH, span, spanLen);
        }
    }
}
