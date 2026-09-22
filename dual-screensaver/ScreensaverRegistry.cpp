#include "ScreensaverRegistry.h"
#include "Renderers.h"
#include <algorithm>
#include <cstring>
#include <cwctype>

#include "settings/DonutSettings.h"
#include "settings/GolSettings.h"
#include "settings/EarthSettings.h"
#include "settings/DvdSettings.h"
#include "settings/PongSettings.h"
#include "settings/MazeSettings.h"
#include "settings/ClockSettings.h"
#include "settings/PerlinSettings.h"
#include "settings/AntSettings.h"
#include "settings/CurlSettings.h"

#define WRAP_LEGACY(fn) [](const RenderContext& ctx) { fn(ctx.hdc, ctx.data, ctx.width, ctx.height, ctx.rect); }

extern const WCHAR* g_modeNames[] = {
    L"Donut", L"Game of Life", L"Matrix", L"Earth",
    L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo",
    L"Grid", L"Pong", L"Maze Generator", L"Odometer Clock",
    L"Perlin Flow Field", L"ASCII Fire", L"Hex Memory Dump", L"Sorting Algorithms",
    L"Langton's Ant Symmetrical",
    L"Pipes", L"Brian's Brain",
    L"Mandelbrot Zoom", L"Clifford Attractor", L"Curl Noise Particles",
    L"Harmonograph", L"Bad Apple (ASCII)", L"ASCIIQuarium",
    L"cbonsai (Bonsai Tree)", L"Nyan Cat (ASCII)",
    L"Self-Playing Snake"
};

extern const int g_numScreensavers = (int)(sizeof(g_modeNames) / sizeof(g_modeNames[0]));

namespace ScreensaverRegistry {

static const std::vector<ScreensaverDef>& InitRegistry() {
    static const std::vector<ScreensaverDef> registry = {
        { 0,  L"Donut",                     "donut",        { "donut" },                                          WRAP_LEGACY(RenderDonut),         GetDonutSettings() },
        { 1,  L"Game of Life",              "gol",          { "gol", "life", "gameoflife" },                      WRAP_LEGACY(RenderGoL),           GetGolSettings() },
        { 2,  L"Matrix",                    "matrix",       { "matrix" },                                         WRAP_LEGACY(RenderMatrix),        {} },
        { 3,  L"Earth",                     "earth",        { "earth" },                                          WRAP_LEGACY(RenderEarth),         GetEarthSettings() },
        { 4,  L"Blank",                     "blank",        { "blank", "none" },                                  WRAP_LEGACY(RenderBlank),         {} },
        { 5,  L"Julia Spirals",             "julia",        { "julia", "spirals" },                               WRAP_LEGACY(RenderJulia),         {} },
        { 6,  L"3D Starfield",              "stars",        { "stars", "starfield", "3dstars" },                  WRAP_LEGACY(RenderStars),         {} },
        { 7,  L"Bouncing DVD Logo",         "dvd",          { "dvd" },                                            WRAP_LEGACY(RenderDVD),           GetDvdSettings() },
        { 8,  L"Grid",                      "grid",         { "grid" },                                           WRAP_LEGACY(RenderGrid),          {} },
        { 9,  L"Pong",                      "pong",         { "pong" },                                           WRAP_LEGACY(RenderPong),          GetPongSettings() },
        { 10, L"Maze Generator",             "maze",         { "maze" },                                           WRAP_LEGACY(RenderMaze),          GetMazeSettings() },
        { 11, L"Odometer Clock",            "clock",        { "clock", "odometer" },                              WRAP_LEGACY(RenderClock),         GetClockSettings() },
        { 12, L"Perlin Flow Field",         "perlin",       { "perlin", "flow" },                                 WRAP_LEGACY(RenderPerlin),        GetPerlinSettings() },
        { 13, L"ASCII Fire",                "fire",         { "fire", "flame" },                                  WRAP_LEGACY(RenderFire),          {} },
        { 14, L"Hex Memory Dump",           "memory",       { "memory", "hex", "dump" },                          WRAP_LEGACY(RenderMemoryDump),    {} },
        { 15, L"Sorting Algorithms",        "sort",         { "sort", "sorting" },                                WRAP_LEGACY(RenderRandomSort),    {} },
        { 16, L"Langton's Ant Symmetrical", "ant",          { "ant", "langton" },                                 WRAP_LEGACY(RenderLangton),       GetAntSettings() },
        { 17, L"Pipes",                     "pipes",        { "pipes" },                                          WRAP_LEGACY(RenderPipes),         {} },
        { 18, L"Brian's Brain",             "brain",        { "brain", "brian" },                                 WRAP_LEGACY(RenderBriansBrain),   {} },
        { 19, L"Mandelbrot Zoom",           "mandelbrot",   { "mandelbrot", "mandel" },                           WRAP_LEGACY(RenderMandelbrot),    {} },
        { 20, L"Clifford Attractor",        "clifford",     { "clifford", "attractor" },                          WRAP_LEGACY(RenderClifford),      {} },
        { 21, L"Curl Noise Particles",      "curl",         { "curl", "noise", "particles" },                     WRAP_LEGACY(RenderCurlNoise),     GetCurlSettings() },
        { 22, L"Harmonograph",              "harmonograph", { "harmonograph", "harmo" },                          WRAP_LEGACY(RenderHarmonograph),  {} },
        { 23, L"Bad Apple (ASCII)",         "badapple",     { "badapple", "bad-apple", "apple", "ascii" },        WRAP_LEGACY(RenderBadApple),      {} },
        { 24, L"ASCIIQuarium",              "asciiquarium", { "asciiquarium", "aquarium", "fish" },               WRAP_LEGACY(RenderASCIIQuarium),  {} },
        { 25, L"cbonsai (Bonsai Tree)",     "cbonsai",      { "cbonsai", "bonsai", "tree" },                      WRAP_LEGACY(RenderBonsai),        {} },
        { 26, L"Nyan Cat (ASCII)",          "nyancat",      { "nyancat", "nyan", "cat" },                         WRAP_LEGACY(RenderNyanCat),       {} },
        { 27, L"Self-Playing Snake",        "snake",        { "snake", "ouroboros" },                             WRAP_LEGACY(RenderSnake),         {} }
    };
    return registry;
}

const std::vector<ScreensaverDef>& GetAll() {
    return InitRegistry();
}

int GetCount() {
    return (int)GetAll().size();
}

const ScreensaverDef* GetById(int id) {
    const auto& list = GetAll();
    if (id >= 0 && id < (int)list.size()) {
        return &list[id];
    }
    return nullptr;
}

const ScreensaverDef* FindByAlias(const wchar_t* alias) {
    if (!alias || !alias[0]) return nullptr;
    char buffer[128];
    int len = WideCharToMultiByte(CP_UTF8, 0, alias, -1, buffer, sizeof(buffer), NULL, NULL);
    if (len <= 0) return nullptr;
    return FindByAlias(buffer);
}

const ScreensaverDef* FindByAlias(const char* alias) {
    if (!alias || !alias[0]) return nullptr;
    const auto& list = GetAll();
    for (const auto& item : list) {
        if (_stricmp(alias, item.codeName) == 0) return &item;
        for (const auto& a : item.aliases) {
            if (_stricmp(alias, a.c_str()) == 0) return &item;
        }
    }
    return nullptr;
}

void Execute(int id, const RenderContext& ctx) {
    const auto* def = GetById(id);
    if (def && def->render) {
        def->render(ctx);
    }
}

} // namespace ScreensaverRegistry
