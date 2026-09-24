# Dual Screensaver

A high-performance, native Win32 C++20 multi-monitor screensaver engine for Windows featuring 37 procedural and retro arcade visual modes, 4K rendering with zero GPU overhead, per-monitor mode assignment, and customizable randomizer pools.

> 📚 **Navigation**: [Features](#features) • [Visual Modes](#visual-modes-37-built-in) • [Installation](#installation-windows-built-in-screensaver) • [CLI & Hotkeys](#command-line-flags--controls) • [🎬 Video Converter Guide](scripts/README.md)

---

## Features

- **Multi-Monitor Support**: Spans across all detected monitors seamlessly.
- **Per-Monitor Mode Selection**: Assign different animations and seasonal themes to your primary and secondary displays.
- **Randomizer with Custom Pool**: Automatically randomize modes on launch from a custom checklist of your favorite animations.
- **Zero GPU Overhead & 4K Ready**: Lightweight software rasterization engine using Win32 GDI with downscaled 4K presentation support.
- **Command-line Controls**: Full support for Windows screensaver standards (`/s`, `/c`, `/p`), 4K simulation (`just run 4k <mode>`), and direct mode launches (e.g., `dual-screensaver.exe matrix earth`).
- **Zero Heavy Dependencies**: Pure Win32 C++20 and GDI with instant startup and minimal RAM usage (< 25 MB).

---

## Visual Modes (37 Built-in)

| #   | Mode                     | Description                                                                |
| --- | ------------------------ | -------------------------------------------------------------------------- |
| 0   | **Donut**                | Rotating 3D ASCII torus rendered with real-time mathematical illumination  |
| 1   | **Game of Life**         | Conway's Cellular Automata with customizable cell sizes and speed          |
| 2   | **Matrix**               | Iconic digital rain with glowing green glyphs and fading trails            |
| 3   | **Earth**                | Real-time rotating ASCII globe rendered with true spherical projection     |
| 4   | **Blank (Away)**         | Clean black screen showing a retro cowsay message with an away timer       |
| 5   | **Julia Spirals**        | Smooth, dynamic fractal exploration rendered in real-time                  |
| 6   | **3D Starfield**         | Warp-speed retro space travel with depth-projected stars                   |
| 7   | **Bouncing DVD Logo**    | Classic bouncing logo that changes color on every wall collision           |
| 8   | **Grid**                 | Retro synthwave/cyberpunk perspective grid with flowing horizon            |
| 9   | **Pong**                 | Autonomous retro ping pong match with ball physics and paddle AI           |
| 10  | **Maze Generator**       | Procedural labyrinth that generates in real-time and solves itself         |
| 11  | **Odometer Clock**       | Minimalist mechanical flip/odometer-style digital clock                    |
| 12  | **Perlin Flow Field**    | Fluid particle trajectories driven by 2D Perlin noise gradients            |
| 13  | **ASCII Fire**           | Classic Doom-style fire simulation rendered with ASCII density glyphs      |
| 14  | **Hex Memory Dump**      | Cyberpunk-style animated memory buffer scanner                             |
| 15  | **Sorting Algorithms**   | Real-time step-by-step visualizations of classic sorting routines          |
| 16  | **Langton's Ant**        | Multi-agent cellular automata creating intricate tapestries                |
| 17  | **Pipes**                | Classic pipes screensaver with Unicode box-drawing chars and vivid colors  |
| 18  | **Brian's Brain**        | 3-state CA with gliders that never stabilise — white sparks on black       |
| 19  | **Mandelbrot Zoom**      | Infinite zoom into 8 curated targets with smooth coloring palette          |
| 20  | **Clifford Attractor**   | 2D strange attractor orbit density field with dynamic coefficient morphing |
| 21  | **Curl Noise Particles** | Incompressible fluid-like particle flow field with smooth fading trails    |
| 22  | **Harmonograph**         | Triple-pendulum rotary harmonograph with interlaced strands and bloom glow |
| 23  | **Bad Apple (ASCII)**    | High-framerate looped ASCII art video player with RLE compression          |
| 24  | **ASCIIQuarium**         | Vibrant underwater ecosystem with swimming fish, jellyfish, crabs & kelp   |
| 25  | **cbonsai**              | Procedural generative bonsai trees with L-systems and seasonal themes      |
| 26  | **Nyan Cat (ASCII)**     | Animated classic meme cat flying through space with rainbow trail & stars  |
| 27  | **Self-Playing Snake**   | Autonomous snake with spanning-tree Hamiltonian pathing, shortcuts & glow  |
| 28  | **Space Invaders**       | Autonomous arcade space defense with marching alien armada, bunkers & UFO  |
| 29  | **Pac-Man**              | Autonomous arcade maze with authentic ghost AI personalities, fruit & energizer chase |
| 30  | **Tetris**               | Self-playing arcade Tetris with Pierre Dellacherie heuristic AI & retro UI |
| 31  | **Space Invaders (Legacy)** | Original retro raster alien defense with bunkers and flying saucer      |
| 32  | **Pong (Legacy)**           | Minimalist classic 2-player arcade table tennis                         |
| 33  | **Sorting Algorithms (Legacy)** | Traditional vertical bar-chart comparison of sorting algorithms     |
| 34  | **Maze Generator (Legacy)** | Classic depth-first search labyrinth generation & traversal             |
| 35  | **Arkanoid (Breakout Neon)** | Autonomous cyber breakout arcade with physics, neon glows & brick effects |
| 36  | **Asteroids (Vector Arcade)** | Authentic 1979 vector arcade with autonomous ship AI, rock splitting, saucer & thruster particles |

---

## Installation (Windows Built-in Screensaver)

To have **DualSaver** appear in the official Windows Screen Saver dropdown alongside built-in screensavers (Mystify, Ribbons, etc.):

### Option A: Download Pre-built Release

Download `DualScreenSaver.scr` directly from the [GitHub Releases](https://github.com/Quo-len/win32-dual-screensaver/releases) page. Right-click the `.scr` file and click **Install**.

### Option B: Build from Source

You can build using Visual Studio (**Ctrl + Shift + B**), or use [`just`](https://github.com/casey/just) for quick developer tasks:

```powershell
just build               # Build Release (x64) and create DualScreenSaver.scr
just debug               # Build Debug (x64)
just run                 # Launch screensaver in fullscreen (/s)
just run 4k              # Launch screensaver in 4K resolution (/s /4k)
just run <mode>          # Launch specific mode (e.g., just run matrix)
just run 4k <mode>       # Launch specific mode in 4K (e.g., just run 4k bonsai)
just modes               # List all available animation modes
just bench               # Run headless benchmark at 1080p
just bench 4k            # Run headless benchmark in 4K (3840x2160)
just bench-visual        # Run visual on-screen showcase benchmark (1080p)
just bench-visual 4k     # Run visual on-screen showcase benchmark in 4K
just config              # Launch configuration settings dialog (/c)
just clean               # Clean intermediate build files
```

Or via MSBuild directly:

```powershell
msbuild dual-screensaver.slnx /p:Configuration=Release /p:Platform=x64 /v:minimal
```

The build automatically generates both `DualScreenSaver.exe` and `DualScreenSaver.scr` in `dual-screensaver\x64\Release\`.

### Step 2: Copy as `.scr` into `System32` (or Right-Click Install)

You can either:

- **Right-click** `dual-screensaver\x64\Release\DualScreenSaver.scr` and select **Install**.
- Or run PowerShell **as Administrator** to copy into `System32`:

```powershell
Copy-Item ".\dual-screensaver\x64\Release\DualScreenSaver.scr" "C:\Windows\System32\DualScreenSaver.scr" -Force
```

### Step 3: Select in Windows

1. Press `Win + R`, type:
   ```cmd
   control desk.cpl,,@screensaver
   ```
   and press **Enter**.
2. Open the **Screen saver** drop-down menu.
3. Select **DualSaver** (or **DualScreenSaver**).
4. Click **Settings...** to configure your monitors, speeds, and random pool.
5. Click **Apply** &rarr; **OK**.

> [!TIP]
> You can also right-click any `.scr` file in File Explorer and select **Install** to set it as your active screensaver immediately.

---

## Configuration & Settings

Settings are managed via a dedicated graphical window and saved to the registry:
`HKEY_CURRENT_USER\Software\DualSaver`

### Opening Settings

- **From Windows Screen Saver dialog**: Click the **Settings...** button.
- **From Command Line / Run dialog**:
  ```cmd
  DualScreenSaver.exe /c
  ```
- **From File Explorer**: Right-click the `.scr` file and select **Configure**.

### Configurable Options

- **Per-Animation Parameters**:
  - Donut: A rotation speed, B rotation speed, torus radius, distance, text size
  - Game of Life: Cell size (px), tick interval (ms)
  - Earth: Spin velocity
  - Pong & DVD: Velocity multipliers
  - Maze: Generation speed, solver step speed
  - Perlin: Scale, particle velocity
  - Langton's Ant: Ant set count, simulation speed (ops/frame)
  - Tetris: Initial falling speed (with auto-scaling progression rate and maximum speed cap)
- **Display Routing**:
  - Assign any mode independently to **Primary** and **Secondary** monitors.
- **Randomizer & Pool**:
  - Toggle **"Randomize every launch"** for spontaneous variety.
  - Click **"Random Pool..."** (or launch with `/pool`) to check/uncheck exactly which animations are allowed to appear in the rotation. Includes **Select All** and **Deselect All** buttons.

---

## Command-Line Arguments

The screensaver executable supports standard Windows screensaver flags as well as custom switches:

| Flag                      | Purpose                                                            | Example                                     |
| ------------------------- | ------------------------------------------------------------------ | ------------------------------------------- |
| `/s`                      | Runs fullscreen screensaver on all monitors (default behavior)     | `DualScreenSaver.exe /s`                    |
| `/4k`                     | Renders screensaver in full 4K (3840x2160) canvas                  | `DualScreenSaver.exe /s /4k`                |
| `/c`                      | Opens the graphical Settings configuration dialog                  | `DualScreenSaver.exe /c`                    |
| `/pool`                   | Opens the Randomizer Pool checklist dialog directly                | `DualScreenSaver.exe /pool`                 |
| `/p <HWND>`               | Renders preview inside parent window handle                        | `DualScreenSaver.exe /p 123456`             |
| `/debug`                  | Launches with live Performance HUD overlay enabled by default      | `DualScreenSaver.exe /debug`                |
| `--benchmark [4k]`        | Runs headless benchmark & GDI leak test on all modes (1080p or 4K) | `DualScreenSaver.exe --benchmark 4k`        |
| `--benchmark visual [4k]` | Runs on-screen visual benchmark showcase (1080p or 4K)             | `DualScreenSaver.exe --benchmark visual 4k` |
| `<mode1> [mode2]`         | Launch immediately with specific modes by name                     | `DualScreenSaver.exe matrix earth`          |

> [!TIP]
> **Live Diagnostics HUD**: Press **`F5`** at any time while the screensaver is running to toggle the real-time Performance HUD (frame time latency in ms, FPS capacity, GDI handle count, and memory usage).

### Supported Mode Names for CLI Launch

`donut`, `gol`, `matrix`, `earth`, `blank`, `julia`, `stars`, `dvd`, `grid`, `pong`, `maze`, `clock`, `perlin`, `fire`, `memory`, `sort`, `ant`, `pipes`, `brain`, `mandelbrot`, `clifford`, `curl`, `harmonograph`, `badapple`, `asciiquarium`, `cbonsai`, `nyancat`, `snake`, `invaders`, `pacman`, `tetris`.

---

## Building from Source

### Requirements

- Windows 10/11 (x64)
- Visual Studio 2022 / 2026 with **Desktop development with C++** (MSVC toolset, Windows 10/11 SDK).

### Build Instructions

1. Open `dual-screensaver.slnx` (or `dual-screensaver.vcxproj`) in Visual Studio.
2. Select **Configuration** (`Release` or `Debug`) and **Platform** (`x64`).
3. Press **Ctrl + Shift + B** to build.
4. Output binaries will be located under `dual-screensaver\x64\Release\` (or `Debug\`).

---

## Documentation & Related Guides

- 📖 **[Main Project Guide](README.md)** — Core setup, installation, CLI flags, and build instructions.
- 🎬 **[ASCII Video Converter Pipeline](scripts/README.md)** — Step-by-step guide to convert any video into an ASCII screensaver binary stream (`.bin`).

---

## Acknowledgments & Credits

- 🌳 **[PyBonsai](https://github.com/Ben-Edwards44/PyBonsai)** by [Ben Edwards](https://github.com/Ben-Edwards44) — Procedural fractal bonsai generation algorithms and tree branching models (Classic, Fibonacci, Offset Fibonacci, and Random Offset Fibonacci) in the `cbonsai` screensaver.

