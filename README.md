# Dual Screensaver

A Win32 dual-monitor screensaver for Windows with configurable visual modes and per-monitor selection.

## Features

- Fullscreen screensaver windows on all detected monitors
- Preview (`/p`) and settings (`/c`) command-line modes
- Primary/secondary monitor mode selection
- Optional random mode selection on each launch
- Built-in modes:
  - Donut (ASCII torus)
  - Game of Life
  - Matrix rain
  - ASCII Earth
  - Blank mode with away-time message
  - Julia fractal
  - Stars
  - DVD logo bounce
  - Grid
  - Pong
  - Maze (build/solve)
  - Clock
  - Perlin noise
  - Fire
  - Memory Dump
  - RandomSort
  - Langton's Ant

## Settings

The settings window allows configuring (per mode):

- Donut: A speed, B speed, donut size, text size
- Game of Life: cell size, update speed
- Earth: spin speed
- Pong: speed
- DVD: speed
- Maze: build speed, solve speed
- Perlin: scale
- Langton's Ant: ant count, ant speed
- Monitor mode mapping (primary/secondary)
- Randomize modes every launch

Settings are stored in the registry:

`HKEY_CURRENT_USER\Software\DualSaver`

## Build

1. Open `dual-screensaver.sln` in Visual Studio.
2. Build the `dual-screensaver` project.

## Run

- Configure settings: run with `/c`
- Preview mode: run with `/p <HWND>`
- Start screensaver mode: run without `/c`

## Notes

- This is a native Win32 C++ project.
- Font-dependent ASCII rendering may vary slightly by system/font availability.
