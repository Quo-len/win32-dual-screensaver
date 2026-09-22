#pragma once
#include <vector>
#include <memory>
#include <windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Texture2D;
struct IDXGISurface1;

struct ScreenData {
    // Window & Display context
    bool isPrimary = false;
    bool isPreview = false;
    ULONGLONG startTime = 0;

    // Shared GDI scratch & frame buffers
    int cols = 0;
    int rows = 0;
    int stride = 0;
    std::vector<unsigned char> grid;
    std::vector<unsigned char> nextGrid;
    std::vector<uint32_t> pixels;
    HFONT hFont = NULL;

    // Type-safe per-screensaver isolated state cache
    std::shared_ptr<void> customState = nullptr;
    int customStateMode = -1;

    template <typename T>
    T& GetCustomState(int mode) {
        if (!customState || customStateMode != mode) {
            customState = std::make_shared<T>();
            customStateMode = mode;
        }
        return *static_cast<T*>(customState.get());
    }

    // Direct3D 11 Interop context
    ID3D11Device* pDevice = nullptr;
    ID3D11DeviceContext* pContext = nullptr;
    IDXGISwapChain* pSwapChain = nullptr;
    ID3D11Texture2D* pBackBuffer = nullptr;
    IDXGISurface1* pSurface = nullptr;

    // Live Diagnostics & Performance HUD
    double lastRenderTimeMs = 0.0;
    double avgRenderTimeMs = 0.0;
    double currentFps = 0.0;
    DWORD lastFpsUpdateTick = 0;
    int frameCounter = 0;
};