# AGENTS.md — Developer & AI Agent Guide for DualSaver

This document provides complete instructions for developers and AI agents working on **DualSaver** (`win32-dual-screensaver`). It details the repository architecture, build system, coding conventions, and a step-by-step guide on how and where to write code to add or modify screensavers.

---

## 1. Repository & Tech Stack Overview

- **Language:** C++20 (`stdcpp20`)
- **Platform:** Windows x64 (Win32 API, GDI / GDI+, optional Direct3D 11 interop)
- **IDE / Build:** Visual Studio 2022+ (`dual-screensaver.slnx` / `dual-screensaver.vcxproj`, platform toolset `v145` or `v143`)
- **Task Runner:** [`just`](https://github.com/casey/just) via PowerShell (`justfile`)
- **Key Targets:**
  - `dual-screensaver/x64/Release/DualScreenSaver.exe`
  - `dual-screensaver/x64/Release/DualScreenSaver.scr` (automatically copied via post-build event)

---

## 2. Directory Layout

```text
dual-screensaver/
├── assets/                  # Embedded headers, binary assets & icons
├── renderers/               # Screensaver render implementations (Render*.cpp)
├── settings/                # Screensaver settings definitions (*Settings.h/cpp)
├── utils/                   # Shared utilities (Perlin noise, SystemMetrics, etc.)
├── dual-screensaver.cpp     # Win32 entry point, message loops, monitor dispatcher
├── dual-screensaver.h       # Global headers, prototypes, extern declarations
├── ScreensaverRegistry.h    # Auto-registration macro, registry structures
├── ScreensaverRegistry.cpp  # Registry lookup, alias resolution & mode array
├── ScreenData.h             # Per-monitor runtime state & isolated custom state cache
├── Settings.h / .cpp        # Registry persistence (HKCU\Software\DualScreenSaver)
├── ConfigUI.h / .cpp        # Win32 screensaver settings dialog UI
├── Benchmark.h / .cpp       # Headless benchmark harness & GDI leak detector
├── dual-screensaver.rc      # Win32 resource script (dialogs, icons)
├── dual-screensaver.vcxproj # MSBuild project with wildcard compilation
└── x64/                     # Build output (Release/Debug)
justfile                     # Project automation commands (build, bench, run)
README.md                    # User documentation & screensaver table
AGENTS.md                    # This reference guide
```

> **Note on MSBuild Wildcards:**
> `dual-screensaver.vcxproj` uses `<ClCompile Include="renderers\*.cpp" />` and `<ClCompile Include="settings\*.cpp" />`. Any new `.cpp` added to `renderers/` or `settings/` is **automatically compiled and linked without editing the vcxproj file**.

---

## 3. Core Architectural Concepts

### 3.1 Self-Registration Pattern (`ScreensaverRegistry.h`)
Screensavers do not need to be manually registered in `main()` or `dual-screensaver.cpp`. They self-register at static initialization time using the `REGISTER_SCREENSAVER` macro:

```cpp
REGISTER_SCREENSAVER(
    id,                              // Unique integer ID (0 to 63)
    L"Display Name",                 // wchar_t* display name for UI
    "codename",                      // Primary ASCII code name
    { "alias1", "alias2" },          // std::vector<std::string> CLI aliases
    WRAP_LEGACY(RenderFunctionName), // Render function callback
    GetCustomSettings()              // std::vector<SettingItem> or {}
);
```

### 3.2 State Isolation (`ScreenData.h`)
The application supports multi-monitor setups where different screensavers can run on different screens independently, or the same screensaver can run on multiple monitors simultaneously.

To prevent state collisions across monitors:
- **Never use mutable global state** for screensaver logic.
- Store all simulation/render variables inside a custom state struct.
- Access state through `ScreenData`:
  ```cpp
  auto& s = data->GetCustomState<MyStateStruct>(SCREENSAVER_ID);
  ```
- `GetCustomState<T>(id)` automatically handles allocation, reset on mode switch, and per-monitor isolation.

### 3.3 Two Common Rendering Paradigms
1. **Virtual Framebuffer + `StretchDIBits` (Fast Pixel / 3D Simulations):**
   - Allocate `std::vector<uint32_t> fb` in the custom state struct.
   - Compute virtual width/height adapting to screen aspect ratio (e.g. `vH = 270`, `vW = 270 * width / height`).
   - Draw pixels into `s.fb` (`0xFF000000 | (R << 16) | (G << 8) | B`).
   - Blit to screen with `StretchDIBits(memDC, 0, 0, width, height, 0, 0, vW, vH, s.fb.data(), &bmi, DIB_RGB_COLORS, SRCCOPY)`.
   - Examples: `RenderMaze.cpp`, `RenderTetris.cpp`, `RenderAsteroids.cpp`, `RenderDungeon.cpp`.

2. **GDI / Consolas Text Grid (ASCII Art & Text Simulations):**
   - Select `data->hFont` into `memDC`.
   - Query `GetTextMetrics(memDC, &tm)`.
   - Calculate columns and rows (`width / tm.tmAveCharWidth`, `height / tm.tmHeight`).
   - Render characters with `TextOutA` or `DrawTextA`.
   - Examples: `RenderMatrix.cpp`, `RenderASCIIQuarium.cpp`, `RenderMandelbrot.cpp`, `RenderFire.cpp`.

---

## 4. Step-by-Step: Adding a New Screensaver

### Step 1: Check Available ID
Check existing screensavers in `dual-screensaver/renderers/` or search for `REGISTER_SCREENSAVER`. IDs are 0-indexed and must be `< 64`. Pick the next sequential unused ID (e.g. `38`).

### Step 2: Implement the Renderer
Create `dual-screensaver/renderers/Render<Name>.cpp`:

```cpp
#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"
#include "Settings.h"
#include <vector>
#include <cstdint>

struct MyScreensaverState {
    int virtualW = 0;
    int virtualH = 0;
    std::vector<uint32_t> fb;
    uint64_t lastTick = 0;
    bool initialized = false;
};

void RenderMyScreensaver(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    if (width <= 0 || height <= 0) return;

    auto& s = data->GetCustomState<MyScreensaverState>(38); // Match screensaver ID

    // Aspect-ratio aware virtual resolution
    int vH = 270;
    int vW = static_cast<int>(270.0f * static_cast<float>(width) / static_cast<float>(height));
    if (vW % 2 != 0) vW++;

    if (!s.initialized || s.virtualW != vW || s.virtualH != vH) {
        s.virtualW = vW;
        s.virtualH = vH;
        s.fb.assign(vW * vH, 0xFF000000); // 0xFFRRGGBB
        s.lastTick = GetTickCount64();
        s.initialized = true;
    }

    // Delta time calculation
    uint64_t now = GetTickCount64();
    uint64_t elapsed = now - s.lastTick;
    if (elapsed > 200) elapsed = 200;
    s.lastTick = now;
    float dt = elapsed > 0 ? (elapsed / 1000.0f) : 0.016f;

    // TODO: Update simulation logic using dt
    // TODO: Render to s.fb

    // Blit to screen
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = s.virtualW;
    bmi.bmiHeader.biHeight = -s.virtualH; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetStretchBltMode(memDC, COLORONCOLOR);
    StretchDIBits(
        memDC,
        0, 0, width, height,
        0, 0, s.virtualW, s.virtualH,
        s.fb.data(),
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

REGISTER_SCREENSAVER(
    38,
    L"My Screensaver",
    "mysaver",
    { "mysaver", "alias" },
    WRAP_LEGACY(RenderMyScreensaver),
    {} // Or GetMySaverSettings()
);
```

### Step 3: (Optional) Add Settings
If your screensaver needs configurable options in the `/c` config dialog:
1. Create `dual-screensaver/settings/<Name>Settings.h`:
   ```cpp
   #pragma once
   #include "SettingItem.h"

   extern float g_MySaverSpeed;
   std::vector<SettingItem> GetMySaverSettings();
   ```
2. Create `dual-screensaver/settings/<Name>Settings.cpp`:
   ```cpp
   #include "MySaverSettings.h"
   constexpr float DEFAULT_SPEED = 1.0f;
   float g_MySaverSpeed = DEFAULT_SPEED;

   std::vector<SettingItem> GetMySaverSettings() {
       return {
           { L"MySaverSpeed", L"Speed:", SettingType::Float, &g_MySaverSpeed, DEFAULT_SPEED, 0.5, 5.0, 1 }
       };
   }
   ```
3. Pass `GetMySaverSettings()` in `REGISTER_SCREENSAVER`.

### Step 4: Update Documentation & Task Runner
1. In `justfile`: Add the new alias to the `modes:` command list.
2. In `README.md`: Add a row with ID, bold name, and description in the screensaver table.

---

## 5. Build, Test & Benchmark Commands

### Build Commands
Using `just`:
```powershell
just build     # Release x64 build
just debug     # Debug x64 build
just clean     # Clean build artifacts
```

Using PowerShell / MSBuild directly:
```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -prerelease -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
& $msbuild dual-screensaver.slnx /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
```

### Running the Screensaver
```powershell
# Run a specific mode directly
.\dual-screensaver\x64\Release\DualScreenSaver.exe <alias>

# Run in fullscreen screensaver mode
.\dual-screensaver\x64\Release\DualScreenSaver.exe /s

# Open settings configuration dialog
.\dual-screensaver\x64\Release\DualScreenSaver.exe /c
```

### Verification & Performance Benchmarking
Always run the headless benchmark after adding or modifying a renderer:
```powershell
# Run 1080p headless benchmark suite across all screensavers
.\dual-screensaver\x64\Release\DualScreenSaver.exe --benchmark

# Run 4K benchmark
.\dual-screensaver\x64\Release\DualScreenSaver.exe --benchmark 4k
```
Check `benchmark_report.txt` to verify:
- **GDI Leaks:** Must be `0` (any leak fails the test).
- **Avg Frame Time:** Target `< 10 ms` (> 100 FPS).
- **RAM Usage:** Reasonable (typically < 25 MB).
- **Status:** Must be `PASS`.

---

## 6. Code Style & Quality Guidelines

- **Commit Messages:** Use conventional commit format (`feat:`, `fix:`, `docs:`, `refactor:`, `perf:`, `test:`, `chore:`) followed by a short imperative message. Never add scope in parentheses or brackets (e.g. `feat: add stone dungeon screensaver`, not `feat(dungeon): ...`).
- **GDI Resource Management:** Win32 GDI handles are limited. Always delete created fonts, brushes, and pens (`DeleteObject`), restore previous DC objects after `SelectObject`, or prefer Direct3D/software framebuffers to avoid GDI handle creation.
- **Aspect Ratio Independence:** Never hardcode fixed pixel dimensions that assume 16:9 1080p. Always compute layout from runtime `width` and `height` to properly support 16:10, 4:3, 21:9 ultrawide, and multi-monitor virtual desktops.
- **No Dead Code:** Do not commit unused includes, commented-out blocks, or debug prints.
