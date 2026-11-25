#include "ConfigUI.h"
#include "Settings.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Resource.h"
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "delayimp.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern HINSTANCE hInst;

namespace {

// ---------------------------------------------------------------------------
// Screensaver catalog: sidebar grouping and descriptions
// ---------------------------------------------------------------------------

enum Category { CatSimulations, CatMath, CatArcade, CatRetro, CatLegacy, CatCount };

const wchar_t* const kCategoryNames[CatCount] = {
    L"SIMULATIONS", L"MATH & ALGORITHMS", L"ARCADE GAMES", L"RETRO & FUN", L"LEGACY"
};

struct SaverInfo {
    const char* code;
    Category category;
    const wchar_t* description;
};

const SaverInfo kSaverInfo[] = {
    { "gol",             CatSimulations, L"Conway's Game of Life cellular automaton with adjustable cell size and speed." },
    { "matrix",          CatSimulations, L"Iconic digital rain with glowing green glyphs and fading trails." },
    { "perlin",          CatSimulations, L"Fluid particle trajectories driven by Perlin noise gradients." },
    { "fire",            CatSimulations, L"Classic Doom-style fire simulation drawn with ASCII density glyphs." },
    { "ant",             CatSimulations, L"Multi-agent Langton's Ant automaton weaving intricate tapestries." },
    { "brain",           CatSimulations, L"Brian's Brain: a 3-state automaton whose gliders never settle down." },
    { "curl",            CatSimulations, L"Incompressible, fluid-like particle flow with smooth fading trails." },
    { "donut",           CatMath,        L"Rotating 3D ASCII torus lit with real-time mathematical illumination." },
    { "earth",           CatMath,        L"Rotating ASCII globe rendered with true spherical projection." },
    { "julia",           CatMath,        L"Smooth, ever-changing Julia set fractal rendered in ASCII." },
    { "mandelbrot",      CatMath,        L"Endless zoom into curated Mandelbrot targets with a smooth palette." },
    { "clifford",        CatMath,        L"Clifford strange attractor with slowly morphing coefficients." },
    { "harmonograph",    CatMath,        L"Rotary harmonograph with interlaced glowing strands." },
    { "grid",            CatMath,        L"Synthwave perspective grid flowing toward the horizon." },
    { "sort",            CatMath,        L"Step-by-step visualizations of classic sorting algorithms." },
    { "maze",            CatMath,        L"Procedural labyrinth that generates in real time and solves itself." },
    { "pong",            CatArcade,      L"Autonomous ping pong match with ball physics and paddle AI." },
    { "snake",           CatArcade,      L"Self-playing snake following a Hamiltonian path with shortcuts." },
    { "invaders",        CatArcade,      L"Autonomous space defense against a marching alien armada." },
    { "pacman",          CatArcade,      L"Autonomous maze chase with distinct ghost personalities and fruit." },
    { "tetris",          CatArcade,      L"Self-playing Tetris driven by a classic placement heuristic." },
    { "arkanoid",        CatArcade,      L"Autonomous neon brick breaker with physics and glow effects." },
    { "asteroids",       CatArcade,      L"Vector-style asteroid field with an autonomous ship, saucer and particles." },
    { "dungeon",         CatArcade,      L"First-person walk through a procedural stone dungeon by torchlight." },
    { "blank",           CatRetro,       L"Clean black screen with a retro cowsay message and an away timer." },
    { "stars",           CatRetro,       L"Warp-speed travel through depth-projected stars." },
    { "dvd",             CatRetro,       L"The classic bouncing logo that changes color on every wall hit." },
    { "clock",           CatRetro,       L"Minimalist mechanical odometer-style digital clock." },
    { "memory",          CatRetro,       L"Cyberpunk-style animated memory buffer scanner." },
    { "pipes",           CatRetro,       L"Classic 3D-pipes look drawn with box-drawing characters." },
    { "badapple",        CatRetro,       L"Looped ASCII-art video player with RLE-compressed frames." },
    { "asciiquarium",    CatRetro,       L"Underwater scene with swimming fish, jellyfish, crabs and kelp." },
    { "cbonsai",         CatRetro,       L"Procedurally grown bonsai trees with several branching styles." },
    { "nyancat",         CatRetro,       L"A flying pixel cat with a rainbow trail and twinkling stars." },
    { "invaders-legacy", CatLegacy,      L"Original raster version of the alien defense game." },
    { "pong-legacy",     CatLegacy,      L"Minimalist classic two-player table tennis." },
    { "sort-legacy",     CatLegacy,      L"Traditional bar-chart comparison of sorting algorithms." },
    { "maze-legacy",     CatLegacy,      L"Classic depth-first labyrinth generation and traversal." },
};

const SaverInfo* FindSaverInfo(const ScreensaverDef& def) {
    for (const auto& info : kSaverInfo) {
        if (strcmp(info.code, def.codeName) == 0) return &info;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Theme: follows the Windows app light/dark mode and accent color
// ---------------------------------------------------------------------------

struct Theme {
    bool dark = false;
    COLORREF bg, sidebar, card, hover, selected, border, text, muted, subtle, rail, accent, accentHover, onAccent;
};

bool ReadDword(const wchar_t* key, const wchar_t* value, DWORD& out) {
    DWORD size = sizeof(out);
    return RegGetValueW(HKEY_CURRENT_USER, key, value, RRF_RT_REG_DWORD, nullptr, &out, &size) == ERROR_SUCCESS;
}

COLORREF Mix(COLORREF a, COLORREF b, float t) {
    auto lerp = [t](int x, int y) { return static_cast<BYTE>(x + (y - x) * t + 0.5f); };
    return RGB(lerp(GetRValue(a), GetRValue(b)), lerp(GetGValue(a), GetGValue(b)), lerp(GetBValue(a), GetBValue(b)));
}

Theme LoadTheme() {
    Theme t;
    DWORD light = 1;
    ReadDword(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", light);
    t.dark = (light == 0);

    // AccentPalette holds 8 RGBA shades; Windows uses a lighter shade on dark and a darker one on light
    BYTE palette[32];
    DWORD size = sizeof(palette);
    bool havePalette = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent",
        L"AccentPalette", RRF_RT_REG_BINARY, nullptr, palette, &size) == ERROR_SUCCESS && size == sizeof(palette);
    if (havePalette) {
        int shade = t.dark ? 1 : 4;
        t.accent = RGB(palette[shade * 4], palette[shade * 4 + 1], palette[shade * 4 + 2]);
    }
    else {
        t.accent = t.dark ? RGB(96, 205, 255) : RGB(0, 95, 184);
    }

    if (t.dark) {
        t.bg = RGB(32, 32, 32);
        t.sidebar = RGB(26, 26, 26);
        t.card = RGB(43, 43, 43);
        t.hover = RGB(50, 50, 50);
        t.selected = RGB(45, 45, 45);
        t.border = RGB(56, 56, 56);
        t.text = RGB(255, 255, 255);
        t.muted = RGB(200, 200, 200);
        t.subtle = RGB(150, 150, 150);
        t.rail = RGB(110, 110, 110);
        t.accentHover = Mix(t.accent, RGB(0, 0, 0), 0.1f);
    }
    else {
        t.bg = RGB(249, 249, 249);
        t.sidebar = RGB(240, 240, 240);
        t.card = RGB(255, 255, 255);
        t.hover = RGB(234, 234, 234);
        t.selected = RGB(230, 230, 230);
        t.border = RGB(224, 224, 224);
        t.text = RGB(26, 26, 26);
        t.muted = RGB(90, 90, 90);
        t.subtle = RGB(135, 135, 135);
        t.rail = RGB(160, 160, 160);
        t.accentHover = Mix(t.accent, RGB(255, 255, 255), 0.1f);
    }
    float lum = (0.299f * GetRValue(t.accent) + 0.587f * GetGValue(t.accent) + 0.114f * GetBValue(t.accent)) / 255.0f;
    t.onAccent = lum > 0.55f ? RGB(0, 0, 0) : RGB(255, 255, 255);
    return t;
}

// ---------------------------------------------------------------------------
// Setting value helpers and the snapshot used for Cancel
// ---------------------------------------------------------------------------

double GetValue(const SettingItem& item) {
    switch (item.type) {
    case SettingType::Int: return *static_cast<int*>(item.valPtr);
    case SettingType::Float: return *static_cast<float*>(item.valPtr);
    case SettingType::Bool: return *static_cast<bool*>(item.valPtr) ? 1.0 : 0.0;
    }
    return 0.0;
}

void SetValue(const SettingItem& item, double v) {
    if (v < item.minVal) v = item.minVal;
    if (v > item.maxVal) v = item.maxVal;
    switch (item.type) {
    case SettingType::Int:
        *static_cast<int*>(item.valPtr) = static_cast<int>(std::lround(v));
        break;
    case SettingType::Float: {
        double scale = std::pow(10.0, item.precision);
        *static_cast<float*>(item.valPtr) = static_cast<float>(std::round(v * scale) / scale);
        break;
    }
    case SettingType::Bool:
        *static_cast<bool*>(item.valPtr) = (v != 0.0);
        break;
    }
}

std::wstring FormatValue(const SettingItem& item) {
    wchar_t buf[64];
    if (item.type == SettingType::Float) swprintf_s(buf, L"%.*f", item.precision, *static_cast<float*>(item.valPtr));
    else swprintf_s(buf, L"%d", *static_cast<int*>(item.valPtr));
    return buf;
}

std::wstring DisplayLabel(const SettingItem& item) {
    std::wstring s = item.label ? item.label : L"";
    while (!s.empty() && (s.back() == L':' || s.back() == L' ')) s.pop_back();
    return s;
}

double StepOf(const SettingItem& item) {
    return item.type == SettingType::Float ? std::pow(10.0, -item.precision) : 1.0;
}

struct Snapshot {
    int primary = 0;
    int secondary = 0;
    int randomMode = 0;
    uint64_t pool = 0;
    std::vector<double> values;

    static Snapshot Capture() {
        Snapshot s;
        s.primary = g_ModePrimary;
        s.secondary = g_ModeSecondary;
        s.randomMode = g_RandomMode;
        s.pool = g_RandomPool;
        for (const auto& def : ScreensaverRegistry::GetAll())
            for (const auto& item : def.settings) s.values.push_back(GetValue(item));
        return s;
    }

    void Restore() const {
        g_ModePrimary = primary;
        g_ModeSecondary = secondary;
        g_RandomMode = randomMode;
        g_RandomPool = pool;
        size_t i = 0;
        for (const auto& def : ScreensaverRegistry::GetAll())
            for (const auto& item : def.settings) SetValue(item, values[i++]);
    }

    bool Matches(const Snapshot& o) const {
        return primary == o.primary && secondary == o.secondary && randomMode == o.randomMode &&
            pool == o.pool && values == o.values;
    }
};

// ---------------------------------------------------------------------------
// Window state
// ---------------------------------------------------------------------------

enum class HitKind {
    None, ListRow, ListCheck, PoolAll, PoolNone, PoolSwitch, AssignPrimary, AssignSecondary,
    ResetDefaults, Slider, ValueBox, BoolSwitch, Randomize, Cancel, Save
};

struct Hit {
    RECT rc = {};
    HitKind kind = HitKind::None;
    int index = -1;
    bool operator==(const Hit& o) const { return kind == o.kind && index == o.index; }
};

struct ListRow {
    bool isHeader = false;
    int category = 0;
    int id = -1;
    int y = 0;
    int h = 0;
};

struct Fonts {
    HFONT title = nullptr, section = nullptr, body = nullptr, bodySemi = nullptr, note = nullptr, caps = nullptr;
    void Destroy() {
        for (HFONT f : { title, section, body, bodySemi, note, caps }) if (f) DeleteObject(f);
        *this = Fonts();
    }
};

constexpr UINT_PTR kPreviewTimer = 1;
constexpr UINT kMsgEditDone = WM_APP + 1;

constexpr int kClientW = 1020;
constexpr int kClientH = 700;
constexpr int kSidebarW = 300;
constexpr int kFooterH = 68;
constexpr int kListTop = 76;
constexpr int kRowH = 36;
constexpr int kCategoryH = 34;
constexpr int kPad = 32;

struct ConfigState {
    HWND hwnd = nullptr;
    float scale = 1.0f;
    Theme theme;
    Fonts fonts;
    HBRUSH editBrush = nullptr;

    std::vector<ListRow> rows;
    int listContentH = 0;
    int listScroll = 0;
    int selected = 0;

    int detailScroll = 0;
    int detailContentH = 0;

    std::vector<Hit> hits;
    Hit hover;
    Hit pressed;
    int dragItem = -1;
    int activeSlider = -1;
    bool trackingLeave = false;

    HWND edit = nullptr;
    int editItem = -1;
    bool closingEdit = false;

    Snapshot saved;

    // Back buffer for flicker-free painting
    HDC backDC = nullptr;
    HBITMAP backBmp = nullptr;
    HGDIOBJ backOld = nullptr;
    int backW = 0, backH = 0;

    // Live preview of the selected screensaver
    ScreenData* preview = nullptr;
    int previewMode = -1;
    HDC pvDC = nullptr;
    HBITMAP pvBmp = nullptr;
    HGDIOBJ pvOld = nullptr;
    int pvW = 0, pvH = 0;
    RECT pvRect = {};
    LARGE_INTEGER pvLast = {};

    int S(float v) const { return static_cast<int>(std::lround(v * scale)); }

    Gdiplus::Graphics* g = nullptr;
};

ConfigState* GetState(HWND hwnd) {
    return reinterpret_cast<ConfigState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

int ClientWidth(HWND hwnd) { RECT rc; GetClientRect(hwnd, &rc); return rc.right; }
int ClientHeight(HWND hwnd) { RECT rc; GetClientRect(hwnd, &rc); return rc.bottom; }

// ---------------------------------------------------------------------------
// Fonts, theme application and layout data
// ---------------------------------------------------------------------------

int CALLBACK FontFoundProc(const LOGFONTW*, const TEXTMETRICW*, DWORD, LPARAM found) {
    *reinterpret_cast<bool*>(found) = true;
    return 0;
}

bool FontInstalled(const wchar_t* face) {
    LOGFONTW lf = {};
    lf.lfCharSet = DEFAULT_CHARSET;
    wcscpy_s(lf.lfFaceName, face);
    bool found = false;
    HDC dc = GetDC(nullptr);
    EnumFontFamiliesExW(dc, &lf, FontFoundProc, reinterpret_cast<LPARAM>(&found), 0);
    ReleaseDC(nullptr, dc);
    return found;
}

HFONT MakeFont(const wchar_t* face, int px, int weight) {
    return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
}

void CreateFonts(ConfigState& st) {
    st.fonts.Destroy();
    static const bool variable = FontInstalled(L"Segoe UI Variable Text");
    const wchar_t* text = variable ? L"Segoe UI Variable Text" : L"Segoe UI";
    const wchar_t* display = variable ? L"Segoe UI Variable Display" : L"Segoe UI";
    st.fonts.title = MakeFont(display, st.S(28), FW_SEMIBOLD);
    st.fonts.section = MakeFont(display, st.S(18), FW_SEMIBOLD);
    st.fonts.body = MakeFont(text, st.S(14), FW_NORMAL);
    st.fonts.bodySemi = MakeFont(text, st.S(14), FW_SEMIBOLD);
    st.fonts.note = MakeFont(text, st.S(12), FW_NORMAL);
    st.fonts.caps = MakeFont(text, st.S(11), FW_SEMIBOLD);
}

void ApplyTheme(ConfigState& st) {
    st.theme = LoadTheme();
    BOOL dark = st.theme.dark ? TRUE : FALSE;
    DwmSetWindowAttribute(st.hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
    COLORREF caption = st.theme.sidebar;
    DwmSetWindowAttribute(st.hwnd, 35 /* DWMWA_CAPTION_COLOR */, &caption, sizeof(caption));
    if (st.editBrush) DeleteObject(st.editBrush);
    st.editBrush = CreateSolidBrush(st.theme.bg);
}

void BuildRows(ConfigState& st) {
    st.rows.clear();
    int y = 0;
    const int count = ScreensaverRegistry::GetCount();
    for (int cat = 0; cat < CatCount; ++cat) {
        bool headerAdded = false;
        for (int id = 0; id < count; ++id) {
            const auto* def = ScreensaverRegistry::GetById(id);
            if (!def) continue;
            const SaverInfo* info = FindSaverInfo(*def);
            int itemCat = info ? info->category : CatRetro;
            if (itemCat != cat) continue;
            if (!headerAdded) {
                st.rows.push_back({ true, cat, -1, y, st.S(kCategoryH) });
                y += st.S(kCategoryH);
                headerAdded = true;
            }
            st.rows.push_back({ false, cat, id, y, st.S(kRowH) });
            y += st.S(kRowH);
        }
    }
    st.listContentH = y + st.S(12);
}

int ListViewportH(const ConfigState& st) {
    return ClientHeight(st.hwnd) - st.S(kFooterH) - st.S(kListTop);
}

int DetailViewportH(const ConfigState& st) {
    return ClientHeight(st.hwnd) - st.S(kFooterH);
}

void ClampScroll(ConfigState& st) {
    int maxList = st.listContentH - ListViewportH(st);
    if (st.listScroll > maxList) st.listScroll = maxList;
    if (st.listScroll < 0) st.listScroll = 0;
    int maxDetail = st.detailContentH - DetailViewportH(st);
    if (st.detailScroll > maxDetail) st.detailScroll = maxDetail;
    if (st.detailScroll < 0) st.detailScroll = 0;
}

// ---------------------------------------------------------------------------
// Live preview
// ---------------------------------------------------------------------------

void DestroyPreviewSurface(ConfigState& st) {
    if (st.pvDC) {
        SelectObject(st.pvDC, st.pvOld);
        DeleteDC(st.pvDC);
        st.pvDC = nullptr;
    }
    if (st.pvBmp) {
        DeleteObject(st.pvBmp);
        st.pvBmp = nullptr;
    }
    st.pvW = st.pvH = 0;
}

void DestroyPreviewData(ConfigState& st) {
    if (st.preview) {
        if (st.preview->hFont) DeleteObject(st.preview->hFont);
        st.preview->ReleaseBackBuffer();
        delete st.preview;
        st.preview = nullptr;
    }
}

void RestartPreview(ConfigState& st) {
    DestroyPreviewData(st);
    st.preview = new ScreenData();
    st.preview->isPrimary = true;
    st.preview->isPreview = true;
    st.preview->hFont = CreateFontW(st.S(10), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    st.preview->startTime = GetTickCount64();
    st.previewMode = st.selected;
    st.pvLast.QuadPart = 0;
}

void RenderPreviewFrame(ConfigState& st) {
    int w = st.pvRect.right - st.pvRect.left;
    int h = st.pvRect.bottom - st.pvRect.top;
    if (w <= 0 || h <= 0 || !st.preview) return;

    if (!st.pvDC || st.pvW != w || st.pvH != h) {
        DestroyPreviewSurface(st);
        HDC wdc = GetDC(st.hwnd);
        st.pvDC = CreateCompatibleDC(wdc);
        st.pvBmp = CreateCompatibleBitmap(wdc, w, h);
        ReleaseDC(st.hwnd, wdc);
        st.pvOld = SelectObject(st.pvDC, st.pvBmp);
        st.pvW = w;
        st.pvH = h;
    }

    RECT rc = { 0, 0, w, h };
    FillRect(st.pvDC, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
    int saved = SaveDC(st.pvDC);
    SetBkMode(st.pvDC, OPAQUE);
    SetBkColor(st.pvDC, RGB(0, 0, 0));

    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    float dt = st.pvLast.QuadPart ? static_cast<float>(now.QuadPart - st.pvLast.QuadPart) / freq.QuadPart : 0.033f;
    st.pvLast = now;

    RenderContext ctx;
    ctx.hdc = st.pvDC;
    ctx.data = st.preview;
    ctx.width = w;
    ctx.height = h;
    ctx.rect = rc;
    TEXTMETRICW tm;
    HGDIOBJ oldFont = SelectObject(st.pvDC, st.preview->hFont);
    if (GetTextMetricsW(st.pvDC, &tm) && tm.tmAveCharWidth > 0 && tm.tmHeight > 0) {
        ctx.charWidth = tm.tmAveCharWidth;
        ctx.charHeight = tm.tmHeight;
        ctx.termCols = w / tm.tmAveCharWidth;
        ctx.termRows = h / tm.tmHeight;
    }
    SelectObject(st.pvDC, oldFont);
    ctx.deltaTime = dt;
    ctx.totalTime = static_cast<double>(GetTickCount64() - st.preview->startTime) / 1000.0;
    ctx.frameIndex = st.preview->frameCounter++;

    ScreensaverRegistry::Execute(st.previewMode, ctx);
    RestoreDC(st.pvDC, saved);
}

void PresentPreview(ConfigState& st) {
    if (IsIconic(st.hwnd) || !st.pvDC) return;
    RECT viewport = { st.S(kSidebarW), 0, ClientWidth(st.hwnd), DetailViewportH(st) };
    RECT visible;
    if (!IntersectRect(&visible, &viewport, &st.pvRect)) return;
    HDC dc = GetDC(st.hwnd);
    IntersectClipRect(dc, viewport.left, viewport.top, viewport.right, viewport.bottom);
    BitBlt(dc, st.pvRect.left, st.pvRect.top, st.pvW, st.pvH, st.pvDC, 0, 0, SRCCOPY);
    ReleaseDC(st.hwnd, dc);
}

// ---------------------------------------------------------------------------
// Drawing primitives
// ---------------------------------------------------------------------------

Gdiplus::Color GC(COLORREF c, BYTE a = 255) {
    return Gdiplus::Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

void RoundPath(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float r) {
    float d = r * 2.0f;
    if (d > w) d = w;
    if (d > h) d = h;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void FillRound(ConfigState& st, const RECT& rc, float r, COLORREF c, BYTE alpha = 255) {
    Gdiplus::GraphicsPath path;
    RoundPath(path, static_cast<float>(rc.left), static_cast<float>(rc.top),
        static_cast<float>(rc.right - rc.left), static_cast<float>(rc.bottom - rc.top), r);
    Gdiplus::SolidBrush brush(GC(c, alpha));
    st.g->FillPath(&brush, &path);
}

void StrokeRound(ConfigState& st, const RECT& rc, float r, COLORREF c, float width = 1.0f) {
    Gdiplus::GraphicsPath path;
    float inset = width / 2.0f;
    RoundPath(path, rc.left + inset, rc.top + inset, rc.right - rc.left - width, rc.bottom - rc.top - width, r);
    Gdiplus::Pen pen(GC(c), width);
    st.g->DrawPath(&pen, &path);
}

void FillCircle(ConfigState& st, float cx, float cy, float radius, COLORREF c) {
    Gdiplus::SolidBrush brush(GC(c));
    st.g->FillEllipse(&brush, cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
}

void FillRectSolid(HDC dc, const RECT& rc, COLORREF c) {
    SetDCBrushColor(dc, c);
    FillRect(dc, &rc, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
}

void DrawTextIn(ConfigState& st, HDC dc, const std::wstring& s, RECT rc, HFONT font, COLORREF color, UINT format) {
    st.g->Flush(Gdiplus::FlushIntentionSync);
    SelectObject(dc, font);
    SetTextColor(dc, color);
    DrawTextW(dc, s.c_str(), -1, &rc, format | DT_NOPREFIX);
}

int MeasureWrapped(HDC dc, const std::wstring& s, int width, HFONT font) {
    SelectObject(dc, font);
    RECT rc = { 0, 0, width, 0 };
    DrawTextW(dc, s.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    return rc.bottom;
}

int MeasureWidth(HDC dc, const std::wstring& s, HFONT font) {
    SelectObject(dc, font);
    SIZE sz = {};
    GetTextExtentPoint32W(dc, s.c_str(), static_cast<int>(s.size()), &sz);
    return sz.cx;
}

void AddHit(ConfigState& st, const RECT& rc, HitKind kind, int index, const RECT& clip) {
    RECT visible;
    if (IntersectRect(&visible, &rc, &clip)) st.hits.push_back({ visible, kind, index });
}

bool IsHover(const ConfigState& st, HitKind kind, int index = -1) {
    return st.hover.kind == kind && st.hover.index == index;
}

void DrawSwitch(ConfigState& st, const RECT& rc, bool on, bool hover, bool enabled) {
    const Theme& t = st.theme;
    float h = static_cast<float>(rc.bottom - rc.top);
    float radius = h / 2.0f;
    if (on) {
        FillRound(st, rc, radius, enabled ? (hover ? t.accentHover : t.accent) : t.subtle);
        float knob = h * (hover ? 0.36f : 0.32f);
        FillCircle(st, rc.right - radius, rc.top + radius, knob, enabled ? t.onAccent : t.bg);
    }
    else {
        if (hover && enabled) FillRound(st, rc, radius, t.hover);
        StrokeRound(st, rc, radius, enabled ? t.muted : t.subtle, (std::max)(1.0f, st.scale));
        float knob = h * (hover ? 0.28f : 0.24f);
        FillCircle(st, rc.left + radius, rc.top + radius, knob, enabled ? t.muted : t.subtle);
    }
}

void DrawCheckbox(ConfigState& st, const RECT& rc, bool on, bool hover) {
    const Theme& t = st.theme;
    float r = 4.0f * st.scale;
    if (on) {
        FillRound(st, rc, r, hover ? t.accentHover : t.accent);
        float w = static_cast<float>(rc.right - rc.left);
        Gdiplus::Pen pen(GC(t.onAccent), 1.8f * st.scale);
        pen.SetStartCap(Gdiplus::LineCapRound);
        pen.SetEndCap(Gdiplus::LineCapRound);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        Gdiplus::PointF pts[3] = {
            { rc.left + w * 0.26f, rc.top + w * 0.52f },
            { rc.left + w * 0.43f, rc.top + w * 0.68f },
            { rc.left + w * 0.75f, rc.top + w * 0.34f },
        };
        st.g->DrawLines(&pen, pts, 3);
    }
    else {
        FillRound(st, rc, r, hover ? t.hover : t.card);
        StrokeRound(st, rc, r, t.subtle, (std::max)(1.0f, st.scale));
    }
}

void DrawButton(ConfigState& st, HDC dc, const RECT& rc, const std::wstring& label, bool accent, bool hover, bool enabled) {
    const Theme& t = st.theme;
    float r = 5.0f * st.scale;
    COLORREF textColor;
    if (!enabled) {
        FillRound(st, rc, r, t.card);
        StrokeRound(st, rc, r, t.border);
        textColor = t.subtle;
    }
    else if (accent) {
        FillRound(st, rc, r, hover ? t.accentHover : t.accent);
        textColor = t.onAccent;
    }
    else {
        FillRound(st, rc, r, hover ? t.hover : t.card);
        StrokeRound(st, rc, r, t.border);
        textColor = t.text;
    }
    DrawTextIn(st, dc, label, rc, st.fonts.body, textColor, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawSlider(ConfigState& st, const RECT& track, double fraction, bool hover, bool dragging) {
    const Theme& t = st.theme;
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    float cy = (track.top + track.bottom) / 2.0f;
    float railH = 4.0f * st.scale;
    float thumbX = track.left + static_cast<float>(fraction) * (track.right - track.left);

    RECT rail = { track.left, static_cast<LONG>(cy - railH / 2), track.right, static_cast<LONG>(cy + railH / 2) };
    FillRound(st, rail, railH / 2, t.rail);
    RECT filled = rail;
    filled.right = static_cast<LONG>(thumbX);
    if (filled.right > filled.left) FillRound(st, filled, railH / 2, t.accent);

    float outer = 10.0f * st.scale;
    FillCircle(st, thumbX, cy, outer, t.card);
    Gdiplus::Pen ring(GC(t.border), (std::max)(1.0f, st.scale));
    st.g->DrawEllipse(&ring, thumbX - outer, cy - outer, outer * 2, outer * 2);
    float inner = (dragging ? 4.5f : hover ? 7.0f : 6.0f) * st.scale;
    FillCircle(st, thumbX, cy, inner, hover || dragging ? t.accentHover : t.accent);
}

// ---------------------------------------------------------------------------
// Painting: each paint also records clickable regions for hit testing
// ---------------------------------------------------------------------------

void PaintSidebar(ConfigState& st, HDC dc) {
    const Theme& t = st.theme;
    const int sw = st.S(kSidebarW);
    const int footerTop = ClientHeight(st.hwnd) - st.S(kFooterH);
    RECT side = { 0, 0, sw, footerTop };
    FillRectSolid(dc, side, t.sidebar);

    const int count = ScreensaverRegistry::GetCount();
    const uint64_t allMask = count >= 64 ? ~0ULL : ((1ULL << count) - 1);
    int inPool = 0;
    for (int i = 0; i < count; ++i) if (g_RandomPool & (1ULL << i)) inPool++;

    RECT title = { st.S(20), st.S(18), sw - st.S(110), st.S(40) };
    DrawTextIn(st, dc, L"Screensavers", title, st.fonts.section, t.text, DT_SINGLELINE | DT_VCENTER);

    wchar_t sub[96];
    if ((g_RandomPool & allMask) == 0) swprintf_s(sub, L"Random pool empty: all %d are used", count);
    else swprintf_s(sub, L"%d of %d in the random pool", inPool, count);
    RECT subRc = { st.S(20), st.S(42), sw - st.S(16), st.S(60) };
    DrawTextIn(st, dc, sub, subRc, st.fonts.note, t.muted, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    RECT full = { 0, 0, sw, footerTop };
    int noneW = MeasureWidth(dc, L"None", st.fonts.bodySemi) + st.S(16);
    int allW = MeasureWidth(dc, L"All", st.fonts.bodySemi) + st.S(16);
    RECT none = { sw - st.S(12) - noneW, st.S(16), sw - st.S(12), st.S(42) };
    RECT all = { none.left - allW, none.top, none.left, none.bottom };
    if (IsHover(st, HitKind::PoolAll)) FillRound(st, all, 4.0f * st.scale, t.hover);
    if (IsHover(st, HitKind::PoolNone)) FillRound(st, none, 4.0f * st.scale, t.hover);
    DrawTextIn(st, dc, L"All", all, st.fonts.bodySemi, t.accent, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawTextIn(st, dc, L"None", none, st.fonts.bodySemi, t.accent, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddHit(st, all, HitKind::PoolAll, -1, full);
    AddHit(st, none, HitKind::PoolNone, -1, full);

    // Scrollable list
    RECT viewport = { 0, st.S(kListTop), sw, footerTop };
    int saved = SaveDC(dc);
    IntersectClipRect(dc, viewport.left, viewport.top, viewport.right, viewport.bottom);
    st.g->SetClip(Gdiplus::Rect(viewport.left, viewport.top, viewport.right - viewport.left, viewport.bottom - viewport.top));

    for (const auto& row : st.rows) {
        int y = viewport.top + row.y - st.listScroll;
        if (y + row.h < viewport.top || y > viewport.bottom) continue;
        if (row.isHeader) {
            RECT rc = { st.S(20), y + st.S(10), sw - st.S(16), y + row.h };
            DrawTextIn(st, dc, kCategoryNames[row.category], rc, st.fonts.caps, t.subtle, DT_SINGLELINE | DT_VCENTER);
            continue;
        }
        const auto* def = ScreensaverRegistry::GetById(row.id);
        bool isSelected = row.id == st.selected;
        RECT item = { st.S(8), y + st.S(2), sw - st.S(10), y + row.h - st.S(2) };
        if (isSelected) {
            FillRound(st, item, 5.0f * st.scale, t.selected);
            int barH = st.S(16);
            int barTop = (item.top + item.bottom - barH) / 2;
            RECT bar = { item.left, barTop, item.left + st.S(3), barTop + barH };
            FillRound(st, bar, 1.5f * st.scale, t.accent);
        }
        else if (IsHover(st, HitKind::ListRow, row.id) || IsHover(st, HitKind::ListCheck, row.id)) {
            FillRound(st, item, 5.0f * st.scale, t.hover);
        }

        int box = st.S(18);
        int boxTop = (item.top + item.bottom - box) / 2;
        RECT check = { st.S(20), boxTop, st.S(20) + box, boxTop + box };
        bool inPoolItem = (g_RandomPool & (1ULL << row.id)) != 0;
        DrawCheckbox(st, check, inPoolItem, IsHover(st, HitKind::ListCheck, row.id));

        int right = item.right - st.S(8);
        if (!g_RandomMode) {
            auto badge = [&](const wchar_t* text) {
                int bw = st.S(20);
                RECT b = { right - bw, (item.top + item.bottom) / 2 - st.S(10), right, (item.top + item.bottom) / 2 + st.S(10) };
                FillRound(st, b, 4.0f * st.scale, t.accent);
                DrawTextIn(st, dc, text, b, st.fonts.caps, t.onAccent, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                right = b.left - st.S(4);
            };
            if (g_ModeSecondary == row.id) badge(L"2");
            if (g_ModePrimary == row.id) badge(L"1");
        }

        RECT name = { check.right + st.S(12), item.top, right - st.S(4), item.bottom };
        DrawTextIn(st, dc, def ? def->name : L"", name, isSelected ? st.fonts.bodySemi : st.fonts.body, t.text,
            DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        RECT checkHit = { st.S(12), item.top, check.right + st.S(6), item.bottom };
        AddHit(st, checkHit, HitKind::ListCheck, row.id, viewport);
        AddHit(st, item, HitKind::ListRow, row.id, viewport);
    }

    int viewH = viewport.bottom - viewport.top;
    if (st.listContentH > viewH) {
        int thumbH = (std::max)(st.S(32), viewH * viewH / st.listContentH);
        int thumbY = viewport.top + (viewH - thumbH) * st.listScroll / (st.listContentH - viewH);
        RECT thumb = { sw - st.S(6), thumbY + st.S(2), sw - st.S(3), thumbY + thumbH - st.S(2) };
        FillRound(st, thumb, 1.5f * st.scale, t.subtle, 160);
    }

    st.g->ResetClip();
    RestoreDC(dc, saved);

    RECT divider = { sw - 1, 0, sw, footerTop };
    FillRectSolid(dc, divider, t.border);
}

void PaintDetail(ConfigState& st, HDC dc) {
    const Theme& t = st.theme;
    const int sw = st.S(kSidebarW);
    const int cw = ClientWidth(st.hwnd);
    const int footerTop = DetailViewportH(st);
    RECT viewport = { sw, 0, cw, footerTop };
    FillRectSolid(dc, viewport, t.bg);

    const auto* def = ScreensaverRegistry::GetById(st.selected);
    if (!def) return;
    const SaverInfo* info = FindSaverInfo(*def);

    int saved = SaveDC(dc);
    IntersectClipRect(dc, viewport.left, viewport.top, viewport.right, viewport.bottom);
    st.g->SetClip(Gdiplus::Rect(viewport.left, viewport.top, viewport.right - viewport.left, viewport.bottom - viewport.top));

    const int x0 = sw + st.S(kPad);
    const int contentW = cw - x0 - st.S(kPad);
    int y = st.S(28) - st.detailScroll;

    // Header
    RECT cat = { x0, y, x0 + contentW, y + st.S(16) };
    DrawTextIn(st, dc, kCategoryNames[info ? info->category : CatRetro], cat, st.fonts.caps, t.accent, DT_SINGLELINE | DT_VCENTER);
    y += st.S(20);
    RECT title = { x0, y, x0 + contentW, y + st.S(38) };
    DrawTextIn(st, dc, def->name, title, st.fonts.title, t.text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    y += st.S(42);
    if (info) {
        int h = MeasureWrapped(dc, info->description, contentW, st.fonts.body);
        RECT desc = { x0, y, x0 + contentW, y + h };
        DrawTextIn(st, dc, info->description, desc, st.fonts.body, t.muted, DT_WORDBREAK);
        y += h;
    }
    y += st.S(22);

    // Preview and the options column beside it (stacked when the window is narrow)
    int pvW = (std::min)(st.S(400), contentW * 62 / 100);
    int colW = contentW - pvW - st.S(24);
    bool stacked = colW < st.S(200);
    if (stacked) {
        pvW = (std::min)(contentW, st.S(400));
        colW = contentW;
    }
    int pvH = pvW * 9 / 16;
    RECT frame = { x0, y, x0 + pvW, y + pvH };
    FillRound(st, frame, 8.0f * st.scale, RGB(0, 0, 0));
    StrokeRound(st, frame, 8.0f * st.scale, t.border);
    int inset = st.S(6);
    st.pvRect = { frame.left + inset, frame.top + inset, frame.right - inset, frame.bottom - inset };
    if (st.pvDC && st.pvW == st.pvRect.right - st.pvRect.left && st.pvH == st.pvRect.bottom - st.pvRect.top) {
        st.g->Flush(Gdiplus::FlushIntentionSync);
        BitBlt(dc, st.pvRect.left, st.pvRect.top, st.pvW, st.pvH, st.pvDC, 0, 0, SRCCOPY);
    }

    int cx = stacked ? x0 : frame.right + st.S(24);
    int cy = stacked ? frame.bottom + st.S(20) : frame.top;

    // Random pool card
    bool inPool = (g_RandomPool & (1ULL << st.selected)) != 0;
    RECT poolCard = { cx, cy, cx + colW, cy + st.S(64) };
    FillRound(st, poolCard, 8.0f * st.scale, t.card);
    StrokeRound(st, poolCard, 8.0f * st.scale, t.border);
    RECT poolLabel = { cx + st.S(16), cy + st.S(12), poolCard.right - st.S(64), cy + st.S(32) };
    DrawTextIn(st, dc, L"Random pool", poolLabel, st.fonts.bodySemi, t.text, DT_SINGLELINE | DT_VCENTER);
    RECT poolHint = { cx + st.S(16), cy + st.S(32), poolCard.right - st.S(64), cy + st.S(52) };
    DrawTextIn(st, dc, inPool ? L"Can be picked at random" : L"Never picked at random", poolHint, st.fonts.note, t.muted,
        DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
    RECT poolSwitch = { poolCard.right - st.S(56), cy + st.S(22), poolCard.right - st.S(16), cy + st.S(42) };
    DrawSwitch(st, poolSwitch, inPool, IsHover(st, HitKind::PoolSwitch), true);
    AddHit(st, poolCard, HitKind::PoolSwitch, -1, viewport);
    cy = poolCard.bottom + st.S(20);

    // Display assignment
    RECT showOn = { cx, cy, cx + colW, cy + st.S(16) };
    DrawTextIn(st, dc, L"SHOW ON", showOn, st.fonts.caps, t.subtle, DT_SINGLELINE | DT_VCENTER);
    cy += st.S(22);
    bool assignEnabled = !g_RandomMode;
    auto assignButton = [&](HitKind kind, const wchar_t* label, bool assigned) {
        RECT b = { cx, cy, cx + colW, cy + st.S(36) };
        std::wstring text = assigned ? std::wstring(L"\u2713  ") + label : std::wstring(label);
        DrawButton(st, dc, b, text, assigned && assignEnabled, IsHover(st, kind), assignEnabled);
        if (assignEnabled) AddHit(st, b, kind, -1, viewport);
        cy = b.bottom + st.S(8);
    };
    assignButton(HitKind::AssignPrimary, L"Primary display", g_ModePrimary == st.selected);
    assignButton(HitKind::AssignSecondary, L"Secondary display", g_ModeSecondary == st.selected);
    if (!assignEnabled) {
        std::wstring note = L"Randomize is on, so each display picks from the random pool.";
        int h = MeasureWrapped(dc, note, colW, st.fonts.note);
        RECT n = { cx, cy, cx + colW, cy + h };
        DrawTextIn(st, dc, note, n, st.fonts.note, t.muted, DT_WORDBREAK);
        cy += h;
    }

    y = (std::max)(static_cast<int>(frame.bottom), cy) + st.S(32);

    // Settings
    RECT section = { x0, y, x0 + contentW, y + st.S(28) };
    DrawTextIn(st, dc, L"Settings", section, st.fonts.section, t.text, DT_SINGLELINE | DT_VCENTER);
    if (!def->settings.empty()) {
        int resetW = MeasureWidth(dc, L"Reset to defaults", st.fonts.bodySemi) + st.S(16);
        RECT reset = { x0 + contentW - resetW, y, x0 + contentW, y + st.S(28) };
        if (IsHover(st, HitKind::ResetDefaults)) FillRound(st, reset, 4.0f * st.scale, t.hover);
        DrawTextIn(st, dc, L"Reset to defaults", reset, st.fonts.bodySemi, t.accent, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        AddHit(st, reset, HitKind::ResetDefaults, -1, viewport);
    }
    y += st.S(40);

    const int rowPadX = st.S(18);
    const int innerW = contentW - rowPadX * 2;
    std::vector<int> rowHeights;
    int cardH = 0;
    for (const auto& item : def->settings) {
        int h = st.S(44);
        if (item.description && item.description[0]) h += MeasureWrapped(dc, item.description, innerW, st.fonts.note) + st.S(2);
        rowHeights.push_back(h);
        cardH += h;
    }
    if (def->settings.empty()) cardH = st.S(56);

    RECT card = { x0, y, x0 + contentW, y + cardH + st.S(8) };
    FillRound(st, card, 8.0f * st.scale, t.card);
    StrokeRound(st, card, 8.0f * st.scale, t.border);

    if (def->settings.empty()) {
        RECT msg = { x0 + rowPadX, y, card.right - rowPadX, card.bottom };
        DrawTextIn(st, dc, L"This screensaver has no adjustable settings.", msg, st.fonts.body, t.muted, DT_SINGLELINE | DT_VCENTER);
    }

    int ry = y + st.S(4);
    for (size_t i = 0; i < def->settings.size(); ++i) {
        const auto& item = def->settings[i];
        int idx = static_cast<int>(i);
        int rowTop = ry;
        if (i > 0) {
            RECT line = { x0 + rowPadX, rowTop, card.right - rowPadX, rowTop + 1 };
            FillRectSolid(dc, line, t.border);
        }
        int lineMid = rowTop + st.S(22);
        int labelW = (std::min)(st.S(220), innerW * 38 / 100);
        RECT label = { x0 + rowPadX, rowTop, x0 + rowPadX + labelW, rowTop + st.S(44) };
        DrawTextIn(st, dc, DisplayLabel(item), label, st.fonts.body, t.text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

        if (item.type == SettingType::Bool) {
            bool on = *static_cast<bool*>(item.valPtr);
            RECT sw2 = { card.right - rowPadX - st.S(40), lineMid - st.S(10), card.right - rowPadX, lineMid + st.S(10) };
            std::wstring state = on ? L"On" : L"Off";
            RECT stateRc = { sw2.left - st.S(60), rowTop, sw2.left - st.S(12), rowTop + st.S(44) };
            DrawTextIn(st, dc, state, stateRc, st.fonts.body, t.muted, DT_SINGLELINE | DT_VCENTER | DT_RIGHT);
            DrawSwitch(st, sw2, on, IsHover(st, HitKind::BoolSwitch, idx), true);
            RECT hit = { label.left, rowTop, card.right - rowPadX, rowTop + st.S(44) };
            AddHit(st, hit, HitKind::BoolSwitch, idx, viewport);
        }
        else {
            RECT box = { card.right - rowPadX - st.S(76), lineMid - st.S(15), card.right - rowPadX, lineMid + st.S(15) };
            bool editing = st.editItem == idx && st.edit;
            bool boxHover = IsHover(st, HitKind::ValueBox, idx);
            FillRound(st, box, 4.0f * st.scale, boxHover ? t.hover : t.bg);
            StrokeRound(st, box, 4.0f * st.scale, editing ? t.accent : (boxHover ? t.subtle : t.border));
            if (!editing) DrawTextIn(st, dc, FormatValue(item), box, st.fonts.body, t.text, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            AddHit(st, box, HitKind::ValueBox, idx, viewport);

            RECT track = { label.right + st.S(16), lineMid - st.S(12), box.left - st.S(22), lineMid + st.S(12) };
            if (track.right - track.left > st.S(40)) {
                double range = item.maxVal - item.minVal;
                double fraction = range > 0 ? (GetValue(item) - item.minVal) / range : 0.0;
                bool sliderHover = IsHover(st, HitKind::Slider, idx) || st.activeSlider == idx;
                DrawSlider(st, track, fraction, sliderHover, st.dragItem == idx);
                RECT hit = { track.left - st.S(10), track.top, track.right + st.S(10), track.bottom };
                AddHit(st, hit, HitKind::Slider, idx, viewport);
            }
        }

        if (item.description && item.description[0]) {
            int h = rowHeights[i] - st.S(44) - st.S(2);
            RECT desc = { x0 + rowPadX, rowTop + st.S(40), card.right - rowPadX, rowTop + st.S(40) + h };
            DrawTextIn(st, dc, item.description, desc, st.fonts.note, t.subtle, DT_WORDBREAK);
        }
        ry += rowHeights[i];
    }

    y = card.bottom + st.S(kPad);
    st.detailContentH = y + st.detailScroll;

    int viewH = footerTop;
    if (st.detailContentH > viewH) {
        int thumbH = (std::max)(st.S(32), viewH * viewH / st.detailContentH);
        int thumbY = (viewH - thumbH) * st.detailScroll / (st.detailContentH - viewH);
        RECT thumb = { cw - st.S(6), thumbY + st.S(2), cw - st.S(3), thumbY + thumbH - st.S(2) };
        FillRound(st, thumb, 1.5f * st.scale, t.subtle, 160);
    }

    st.g->ResetClip();
    RestoreDC(dc, saved);
}

void PaintFooter(ConfigState& st, HDC dc) {
    const Theme& t = st.theme;
    const int cw = ClientWidth(st.hwnd);
    const int ch = ClientHeight(st.hwnd);
    RECT footer = { 0, ch - st.S(kFooterH), cw, ch };
    FillRectSolid(dc, footer, t.sidebar);
    RECT line = { 0, footer.top, cw, footer.top + 1 };
    FillRectSolid(dc, line, t.border);

    int mid = (footer.top + footer.bottom) / 2;
    RECT sw = { st.S(20), mid - st.S(10), st.S(60), mid + st.S(10) };
    DrawSwitch(st, sw, g_RandomMode != 0, IsHover(st, HitKind::Randomize), true);
    std::wstring label = L"Randomize on every launch";
    int labelW = MeasureWidth(dc, label, st.fonts.body);
    RECT labelRc = { sw.right + st.S(12), footer.top, sw.right + st.S(12) + labelW, footer.bottom };
    DrawTextIn(st, dc, label, labelRc, st.fonts.body, t.text, DT_SINGLELINE | DT_VCENTER);
    RECT randomHit = { sw.left - st.S(6), mid - st.S(16), labelRc.right + st.S(6), mid + st.S(16) };
    AddHit(st, randomHit, HitKind::Randomize, -1, footer);

    int bw = st.S(104), bh = st.S(34);
    RECT save = { cw - st.S(20) - bw, mid - bh / 2, cw - st.S(20), mid + bh / 2 };
    RECT cancel = { save.left - st.S(8) - bw, save.top, save.left - st.S(8), save.bottom };
    DrawButton(st, dc, cancel, L"Cancel", false, IsHover(st, HitKind::Cancel), true);
    DrawButton(st, dc, save, L"Save", true, IsHover(st, HitKind::Save), true);
    AddHit(st, cancel, HitKind::Cancel, -1, footer);
    AddHit(st, save, HitKind::Save, -1, footer);

    if (!Snapshot::Capture().Matches(st.saved)) {
        RECT note = { labelRc.right + st.S(16), footer.top, cancel.left - st.S(16), footer.bottom };
        DrawTextIn(st, dc, L"Unsaved changes", note, st.fonts.note, t.muted, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS);
    }
}

void Paint(ConfigState& st, HDC target) {
    int cw = ClientWidth(st.hwnd);
    int ch = ClientHeight(st.hwnd);
    if (cw <= 0 || ch <= 0) return;

    if (!st.backDC || st.backW != cw || st.backH != ch) {
        if (st.backDC) {
            SelectObject(st.backDC, st.backOld);
            DeleteDC(st.backDC);
            DeleteObject(st.backBmp);
        }
        st.backDC = CreateCompatibleDC(target);
        st.backBmp = CreateCompatibleBitmap(target, cw, ch);
        st.backOld = SelectObject(st.backDC, st.backBmp);
        st.backW = cw;
        st.backH = ch;
    }

    HDC dc = st.backDC;
    int saved = SaveDC(dc);
    SetBkMode(dc, TRANSPARENT);
    ClampScroll(st);
    st.hits.clear();

    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    st.g = &g;
    PaintSidebar(st, dc);
    PaintDetail(st, dc);
    PaintFooter(st, dc);
    g.Flush(Gdiplus::FlushIntentionSync);
    st.g = nullptr;

    RestoreDC(dc, saved);
    BitBlt(target, 0, 0, cw, ch, dc, 0, 0, SRCCOPY);
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

Hit HitTest(const ConfigState& st, POINT pt) {
    for (const auto& h : st.hits) {
        if (PtInRect(&h.rc, pt)) return h;
    }
    return {};
}

void Redraw(ConfigState& st) {
    InvalidateRect(st.hwnd, nullptr, FALSE);
}

const SettingItem* SelectedItem(const ConfigState& st, int index) {
    const auto* def = ScreensaverRegistry::GetById(st.selected);
    if (!def || index < 0 || index >= static_cast<int>(def->settings.size())) return nullptr;
    return &def->settings[index];
}

void EndEdit(ConfigState& st, bool commit) {
    if (!st.edit || st.closingEdit) return;
    st.closingEdit = true;
    if (commit) {
        if (const SettingItem* item = SelectedItem(st, st.editItem)) {
            wchar_t buf[64] = {};
            GetWindowTextW(st.edit, buf, 64);
            wchar_t* end = nullptr;
            double v = wcstod(buf, &end);
            if (end != buf) {
                SetValue(*item, v);
                RestartPreview(st);
            }
        }
    }
    DestroyWindow(st.edit);
    st.edit = nullptr;
    st.editItem = -1;
    st.closingEdit = false;
    SetFocus(st.hwnd);
    Redraw(st);
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    switch (msg) {
    case WM_GETDLGCODE:
        return DLGC_WANTALLKEYS | DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_KEYDOWN:
        if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
            PostMessageW(GetParent(hwnd), kMsgEditDone, wParam == VK_RETURN ? 1 : 0, 0);
            return 0;
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN || wParam == VK_ESCAPE) return 0;
        break;
    case WM_KILLFOCUS:
        PostMessageW(GetParent(hwnd), kMsgEditDone, 1, 0);
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditSubclassProc, 0);
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void BeginEdit(ConfigState& st, const Hit& hit) {
    const SettingItem* item = SelectedItem(st, hit.index);
    if (!item) return;
    EndEdit(st, true);
    RECT rc = hit.rc;
    int textH = st.S(20);
    int top = (rc.top + rc.bottom - textH) / 2;
    st.edit = CreateWindowExW(0, L"EDIT", FormatValue(*item).c_str(), WS_CHILD | WS_VISIBLE | ES_CENTER | ES_AUTOHSCROLL,
        rc.left + st.S(6), top, rc.right - rc.left - st.S(12), textH, st.hwnd, nullptr, hInst, nullptr);
    st.editItem = hit.index;
    SendMessageW(st.edit, WM_SETFONT, reinterpret_cast<WPARAM>(st.fonts.body), TRUE);
    SetWindowSubclass(st.edit, EditSubclassProc, 0, 0);
    SetFocus(st.edit);
    SendMessageW(st.edit, EM_SETSEL, 0, -1);
    Redraw(st);
}

void SetSliderFromX(ConfigState& st, int index, int x) {
    const SettingItem* item = SelectedItem(st, index);
    if (!item) return;
    for (const auto& h : st.hits) {
        if (h.kind != HitKind::Slider || h.index != index) continue;
        int left = h.rc.left + st.S(10);
        int right = h.rc.right - st.S(10);
        if (right <= left) return;
        double fraction = static_cast<double>(x - left) / (right - left);
        if (fraction < 0) fraction = 0;
        if (fraction > 1) fraction = 1;
        SetValue(*item, item->minVal + fraction * (item->maxVal - item->minVal));
        Redraw(st);
        return;
    }
}

void Select(ConfigState& st, int id) {
    if (id == st.selected || id < 0) return;
    EndEdit(st, true);
    st.selected = id;
    st.detailScroll = 0;
    st.activeSlider = -1;
    RestartPreview(st);
    Redraw(st);
}

void EnsureSelectedVisible(ConfigState& st) {
    for (const auto& row : st.rows) {
        if (row.isHeader || row.id != st.selected) continue;
        int view = ListViewportH(st);
        if (row.y < st.listScroll || row.y + row.h > st.listScroll + view) st.listScroll = row.y - (view - row.h) / 2;
        ClampScroll(st);
        return;
    }
}

void MoveSelection(ConfigState& st, int direction) {
    int current = -1;
    for (int i = 0; i < static_cast<int>(st.rows.size()); ++i)
        if (!st.rows[i].isHeader && st.rows[i].id == st.selected) current = i;
    for (int i = current + direction; i >= 0 && i < static_cast<int>(st.rows.size()); i += direction) {
        if (!st.rows[i].isHeader) {
            Select(st, st.rows[i].id);
            EnsureSelectedVisible(st);
            return;
        }
    }
}

void ResetSelectedToDefaults(ConfigState& st) {
    const auto* def = ScreensaverRegistry::GetById(st.selected);
    if (!def) return;
    for (const auto& item : def->settings) SetValue(item, item.defVal);
    RestartPreview(st);
}

void FinishAndClose(ConfigState& st, bool save) {
    EndEdit(st, true);
    if (save) SaveSettings();
    else st.saved.Restore();
    st.saved = Snapshot::Capture();
    DestroyWindow(st.hwnd);
}

void Activate(ConfigState& st, const Hit& hit) {
    const int count = ScreensaverRegistry::GetCount();
    switch (hit.kind) {
    case HitKind::PoolAll:
        g_RandomPool = count >= 64 ? ~0ULL : ((1ULL << count) - 1);
        break;
    case HitKind::PoolNone:
        g_RandomPool = 0;
        break;
    case HitKind::AssignPrimary:
        g_ModePrimary = st.selected;
        break;
    case HitKind::AssignSecondary:
        g_ModeSecondary = st.selected;
        break;
    case HitKind::ResetDefaults:
        ResetSelectedToDefaults(st);
        break;
    case HitKind::Cancel:
        FinishAndClose(st, false);
        return;
    case HitKind::Save:
        FinishAndClose(st, true);
        return;
    default:
        return;
    }
    Redraw(st);
}

void OnMouseDown(ConfigState& st, POINT pt) {
    if (st.edit) EndEdit(st, true);
    SetFocus(st.hwnd);
    Hit hit = HitTest(st, pt);
    st.pressed = hit;
    switch (hit.kind) {
    case HitKind::ListRow:
        Select(st, hit.index);
        break;
    case HitKind::ListCheck:
        g_RandomPool ^= (1ULL << hit.index);
        Redraw(st);
        break;
    case HitKind::PoolSwitch:
        g_RandomPool ^= (1ULL << st.selected);
        Redraw(st);
        break;
    case HitKind::Randomize:
        g_RandomMode = g_RandomMode ? 0 : 1;
        Redraw(st);
        break;
    case HitKind::BoolSwitch:
        if (const SettingItem* item = SelectedItem(st, hit.index)) {
            SetValue(*item, GetValue(*item) != 0.0 ? 0.0 : 1.0);
            RestartPreview(st);
            Redraw(st);
        }
        break;
    case HitKind::Slider:
        st.dragItem = hit.index;
        st.activeSlider = hit.index;
        SetCapture(st.hwnd);
        SetSliderFromX(st, hit.index, pt.x);
        break;
    case HitKind::ValueBox:
        st.activeSlider = hit.index;
        BeginEdit(st, hit);
        break;
    default:
        break;
    }
}

void OnMouseUp(ConfigState& st, POINT pt) {
    if (st.dragItem >= 0) {
        st.dragItem = -1;
        ReleaseCapture();
        RestartPreview(st);
        Redraw(st);
        return;
    }
    Hit hit = HitTest(st, pt);
    if (hit.kind != HitKind::None && hit == st.pressed) Activate(st, hit);
    st.pressed = {};
}

void OnMouseMove(ConfigState& st, POINT pt) {
    if (st.dragItem >= 0) {
        SetSliderFromX(st, st.dragItem, pt.x);
        return;
    }
    if (!st.trackingLeave) {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, st.hwnd, 0 };
        TrackMouseEvent(&tme);
        st.trackingLeave = true;
    }
    Hit hit = HitTest(st, pt);
    if (!(hit == st.hover)) {
        st.hover = hit;
        Redraw(st);
    }
}

void OnWheel(ConfigState& st, POINT screenPt, int delta) {
    POINT pt = screenPt;
    ScreenToClient(st.hwnd, &pt);
    EndEdit(st, true);
    int notches = delta / WHEEL_DELTA;
    if (notches == 0) notches = delta > 0 ? 1 : -1;
    if (pt.x < st.S(kSidebarW)) st.listScroll -= notches * st.S(kRowH) * 3;
    else st.detailScroll -= notches * st.S(80);
    ClampScroll(st);
    Redraw(st);
}

void AdjustActiveSlider(ConfigState& st, int direction, bool coarse) {
    const SettingItem* item = SelectedItem(st, st.activeSlider);
    if (!item || item->type == SettingType::Bool) return;
    double step = StepOf(*item) * (coarse ? 10.0 : 1.0);
    SetValue(*item, GetValue(*item) + direction * step);
    Redraw(st);
}

LRESULT CALLBACK ConfigWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ConfigState* st = GetState(hwnd);
    switch (msg) {
    case WM_NCCREATE: {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* state = static_cast<ConfigState*>(cs->lpCreateParams);
        state->hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        break;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (st) Paint(*st, dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SIZE:
        if (st) {
            if (wParam != SIZE_MINIMIZED) EndEdit(*st, true);
            Redraw(*st);
        }
        return 0;
    case WM_GETMINMAXINFO:
        if (st) {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            RECT rc = { 0, 0, st->S(760), st->S(540) };
            AdjustWindowRectExForDpi(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0, GetDpiForWindow(hwnd));
            mmi->ptMinTrackSize = { rc.right - rc.left, rc.bottom - rc.top };
        }
        return 0;
    case WM_DPICHANGED:
        if (st) {
            EndEdit(*st, true);
            st->scale = HIWORD(wParam) / 96.0f;
            CreateFonts(*st);
            BuildRows(*st);
            RestartPreview(*st);
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            Redraw(*st);
        }
        return 0;
    case WM_SETTINGCHANGE:
        if (st && lParam && wcscmp(reinterpret_cast<const wchar_t*>(lParam), L"ImmersiveColorSet") == 0) {
            ApplyTheme(*st);
            Redraw(*st);
        }
        break;
    case WM_TIMER:
        if (st && wParam == kPreviewTimer && !IsIconic(hwnd)) {
            RenderPreviewFrame(*st);
            PresentPreview(*st);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (st) OnMouseDown(*st, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;
    case WM_LBUTTONUP:
        if (st) OnMouseUp(*st, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;
    case WM_MOUSEMOVE:
        if (st) OnMouseMove(*st, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;
    case WM_MOUSELEAVE:
        if (st) {
            st->trackingLeave = false;
            if (st->hover.kind != HitKind::None) {
                st->hover = {};
                Redraw(*st);
            }
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (st) OnWheel(*st, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_CAPTURECHANGED:
        if (st && st->dragItem >= 0 && reinterpret_cast<HWND>(lParam) != hwnd) {
            st->dragItem = -1;
            RestartPreview(*st);
            Redraw(*st);
        }
        return 0;
    case WM_KEYDOWN:
        if (!st) break;
        switch (wParam) {
        case VK_UP: MoveSelection(*st, -1); return 0;
        case VK_DOWN: MoveSelection(*st, 1); return 0;
        case VK_SPACE:
            g_RandomPool ^= (1ULL << st->selected);
            Redraw(*st);
            return 0;
        case VK_LEFT:
        case VK_RIGHT:
            AdjustActiveSlider(*st, wParam == VK_RIGHT ? 1 : -1, GetKeyState(VK_SHIFT) < 0);
            return 0;
        case VK_RETURN: FinishAndClose(*st, true); return 0;
        case VK_ESCAPE: FinishAndClose(*st, false); return 0;
        }
        break;
    case WM_KEYUP:
        if (st && (wParam == VK_LEFT || wParam == VK_RIGHT) && st->activeSlider >= 0) RestartPreview(*st);
        break;
    case kMsgEditDone:
        if (st) EndEdit(*st, wParam != 0);
        return 0;
    case WM_CTLCOLOREDIT:
        if (st) {
            HDC edc = reinterpret_cast<HDC>(wParam);
            SetTextColor(edc, st->theme.text);
            SetBkColor(edc, st->theme.bg);
            return reinterpret_cast<LRESULT>(st->editBrush);
        }
        break;
    case WM_CLOSE:
        if (st) {
            EndEdit(*st, true);
            if (!Snapshot::Capture().Matches(st->saved)) {
                int answer = MessageBoxW(hwnd, L"Save your changes before closing?", L"DualSaver",
                    MB_YESNOCANCEL | MB_ICONQUESTION);
                if (answer == IDCANCEL) return 0;
                FinishAndClose(*st, answer == IDYES);
                return 0;
            }
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ReleaseResources(ConfigState& st) {
    DestroyPreviewData(st);
    DestroyPreviewSurface(st);
    if (st.backDC) {
        SelectObject(st.backDC, st.backOld);
        DeleteDC(st.backDC);
        DeleteObject(st.backBmp);
        st.backDC = nullptr;
    }
    st.fonts.Destroy();
    if (st.editBrush) DeleteObject(st.editBrush);
    st.editBrush = nullptr;
}

}

void ShowSettingsWindow(HINSTANCE hInstance)
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = ConfigWindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DUALSCREENSAVER));
    wc.lpszClassName = L"DualSaverSettingsClass";
    RegisterClassExW(&wc);

    ConfigState st;
    st.saved = Snapshot::Capture();
    if (g_ModePrimary >= 0 && g_ModePrimary < ScreensaverRegistry::GetCount()) st.selected = g_ModePrimary;

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"DualSaver Settings", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, nullptr, nullptr, hInstance, &st);
    if (!hwnd) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return;
    }

    st.scale = GetDpiForWindow(hwnd) / 96.0f;
    ApplyTheme(st);
    CreateFonts(st);
    BuildRows(st);
    RestartPreview(st);

    RECT rc = { 0, 0, st.S(kClientW), st.S(kClientH) };
    AdjustWindowRectExForDpi(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0, GetDpiForWindow(hwnd));
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
    int workW = mi.rcWork.right - mi.rcWork.left;
    int workH = mi.rcWork.bottom - mi.rcWork.top;
    if (winW > workW) winW = workW;
    if (winH > workH) winH = workH;
    SetWindowPos(hwnd, nullptr, mi.rcWork.left + (workW - winW) / 2, mi.rcWork.top + (workH - winH) / 2,
        winW, winH, SWP_NOZORDER);
    EnsureSelectedVisible(st);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetTimer(hwnd, kPreviewTimer, 33, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseResources(st);
    Gdiplus::GdiplusShutdown(gdiplusToken);
}
