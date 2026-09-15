# Dual Screensaver

A high-performance, native Win32 multi-monitor screensaver for Windows with 22 procedural and retro visual modes, per-monitor customization, and customizable randomizer pools.

---

## Features

- **Multi-Monitor Support**: Spans across all detected monitors seamlessly.
- **Per-Monitor Mode Selection**: Assign different animations to your primary and secondary displays.
- **Randomizer with Custom Pool**: Automatically randomize modes on launch from a custom checklist of your favorite animations.
- **Command-line Controls**: Full support for Windows screensaver standards (`/s`, `/c`, `/p`) and direct mode launches (e.g., `dual-screensaver.exe matrix earth`).
- **Zero Heavy Dependencies**: Pure Win32 C++ and GDI with lightweight resource usage.

---

## Visual Modes (22 Built-in)

| #   | Mode                   | Description                                                               |
| --- | ---------------------- | ------------------------------------------------------------------------- |
| 0   | **Donut**              | Rotating 3D ASCII torus rendered with real-time mathematical illumination |
| 1   | **Game of Life**       | Conway's Cellular Automata with customizable cell sizes and speed         |
| 2   | **Matrix**             | Iconic digital rain with glowing green glyphs and fading trails           |
| 3   | **Earth**              | Real-time rotating ASCII globe rendered with true spherical projection    |
| 4   | **Blank (Away)**       | Clean black screen showing a retro cowsay message with an away timer      |
| 5   | **Julia Spirals**      | Smooth, dynamic fractal exploration rendered in real-time                 |
| 6   | **3D Starfield**       | Warp-speed retro space travel with depth-projected stars                  |
| 7   | **Bouncing DVD Logo**  | Classic bouncing logo that changes color on every wall collision          |
| 8   | **Grid**               | Retro synthwave/cyberpunk perspective grid with flowing horizon           |
| 9   | **Pong**               | Autonomous retro ping pong match with ball physics and paddle AI          |
| 10  | **Maze Generator**     | Procedural labyrinth that generates in real-time and solves itself        |
| 11  | **Odometer Clock**     | Minimalist mechanical flip/odometer-style digital clock                   |
| 12  | **Perlin Flow Field**  | Fluid particle trajectories driven by 2D Perlin noise gradients           |
| 13  | **ASCII Fire**         | Classic Doom-style fire simulation rendered with ASCII density glyphs     |
| 14  | **Hex Memory Dump**    | Cyberpunk-style animated memory buffer scanner                            |
| 15  | **Sorting Algorithms** | Real-time step-by-step visualizations of classic sorting routines         |
| 16  | **Langton's Ant**      | Multi-agent symmetrical cellular automata creating intricate tapestries   |
| 17  | **Boids Flocking**     | 150 agents following separation, alignment & cohesion rules with trails   |
| 18  | **Cyclic CA**          | 16-state cyclic cellular automaton producing perpetual spinning spirals   |
| 19  | **Pipes**              | Classic pipes screensaver with Unicode box-drawing chars and vivid colors |
| 20  | **Brian's Brain**      | 3-state CA with gliders that never stabilise — white sparks on black      |
| 21  | **Mandelbrot Zoom**    | Infinite zoom into 8 curated targets with smooth coloring palette         |

---

## Installation (Windows Built-in Screensaver)

To have **DualSaver** appear in the official Windows Screen Saver dropdown alongside built-in screensavers (Mystify, Ribbons, etc.):

### Step 1: Build the Project

Build the `Release` (or `Debug`) configuration:

- In Visual Studio: Select `Release` & `x64`, then press **Ctrl + Shift + B**.
- Or via PowerShell:
  ```powershell
  & "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\amd64\MSBuild.exe" "dual-screensaver\dual-screensaver.vcxproj" /p:Configuration=Release /p:Platform=x64
  ```

### Step 2: Copy as `.scr` into `System32`

Windows detects screensavers by looking for `.scr` files inside `C:\Windows\System32`.

Run PowerShell **as Administrator** and execute:

```powershell
# Copy and rename the executable to .scr in System32
Copy-Item ".\x64\Release\DualScreenSaver.exe" "C:\Windows\System32\DualScreenSaver.scr" -Force
```

_(If building Debug, use `.\x64\Debug\DualScreenSaver-Debug.exe`)_.

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
- **Display Routing**:
  - Assign any mode independently to **Primary** and **Secondary** monitors.
- **Randomizer & Pool**:
  - Toggle **"Randomize every launch"** for spontaneous variety.
  - Click **"Random Pool..."** (or launch with `/pool`) to check/uncheck exactly which animations are allowed to appear in the rotation. Includes **Select All** and **Deselect All** buttons.

---

## Command-Line Arguments

The screensaver executable supports standard Windows screensaver flags as well as custom switches:

| Flag              | Purpose                                                        | Example                            |
| ----------------- | -------------------------------------------------------------- | ---------------------------------- |
| `/s`              | Runs fullscreen screensaver on all monitors (default behavior) | `DualScreenSaver.exe /s`           |
| `/c`              | Opens the graphical Settings configuration dialog              | `DualScreenSaver.exe /c`           |
| `/pool`           | Opens the Randomizer Pool checklist dialog directly            | `DualScreenSaver.exe /pool`        |
| `/p <HWND>`       | Renders preview inside parent window handle                    | `DualScreenSaver.exe /p 123456`    |
| `<mode1> [mode2]` | Launch immediately with specific modes by name                 | `DualScreenSaver.exe matrix earth` |

### Supported Mode Names for CLI Launch

`donut`, `gol`, `matrix`, `earth`, `blank`, `julia`, `stars`, `dvd`, `grid`, `pong`, `maze`, `clock`, `perlin`, `fire`, `memory`, `sort`, `ant`, `boids`, `cyclic`, `pipes`, `brain`, `mandelbrot`.

---

## Building from Source

### Requirements

- Windows 10/11 (x64)
- Visual Studio 2022 / 2026 with **Desktop development with C++** (MSVC toolset, Windows 10/11 SDK).

### Build Instructions

1. Open `dual-screensaver.slnx` (or `dual-screensaver.vcxproj`) in Visual Studio.
2. Select **Configuration** (`Release` or `Debug`) and **Platform** (`x64`).
3. Press **Ctrl + Shift + B** to build.
4. Output binaries will be located under `x64\Release\` (or `x64\Debug\`).
