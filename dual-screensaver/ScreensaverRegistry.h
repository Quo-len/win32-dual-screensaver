#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>

struct ScreenData;

// Unified render context passed to screensavers
struct RenderContext {
    HDC hdc = NULL;
    ScreenData* data = nullptr;
    int width = 0;
    int height = 0;
    RECT rect = { 0, 0, 0, 0 };

    // Pre-calculated terminal font metrics (Consolas)
    int charWidth = 0;
    int charHeight = 0;
    int termCols = 0;
    int termRows = 0;

    // Smooth frame timing
    float deltaTime = 0.033f;
    double totalTime = 0.0;
    uint64_t frameIndex = 0;
};

// Render function pointer taking unified RenderContext
typedef void (*RenderCtxFn)(const RenderContext& ctx);

// Legacy render function signature for backward compatibility
typedef void (*RenderLegacyFn)(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);

#include "settings/SettingItem.h"

// Metadata & execution descriptor for a screensaver
struct ScreensaverDef {
    int id;
    const wchar_t* name;
    const char* codeName;
    std::vector<std::string> aliases;
    RenderCtxFn render;
    std::vector<SettingItem> settings;
};

namespace ScreensaverRegistry {
    const std::vector<ScreensaverDef>& GetAll();
    int GetCount();
    const ScreensaverDef* GetById(int id);
    const ScreensaverDef* FindByAlias(const wchar_t* alias);
    const ScreensaverDef* FindByAlias(const char* alias);
    void Execute(int id, const RenderContext& ctx);
}
