# ASCII Video Converter & Bad Apple Screensaver Pipeline

This directory contains reusable tools to convert any MP4 / MKV / AVI / WebM video into an ultra-fast, compressed binary stream (`.bin`) designed for the **Dual-Screensaver ASCII Video Player** mode.

---

## Files in this Directory

- `convert_video.py` — The core Python converter script.
- `convert_video.ps1` — Convenient PowerShell wrapper.
- `bad-apple.mp4` — The source Bad Apple video.
- `bad_apple.bin` — The generated compressed ASCII video stream (automatically deployed to the screensaver build directories).

---

## How to Convert ANY Other Video in the Future

Whenever you want to use a different video:

### Option A: Automatic Auto-Detect
1. Put your new video file (`my_video.mp4`) in this folder.
2. Run in terminal:
   ```powershell
   python convert_video.py
   ```
   *(Or right-click `convert_video.ps1` -> Run with PowerShell)*
3. The script will automatically pick up your video, process it, compress it, generate `bad_apple.bin`, and deploy it directly to the screensaver folders.

### Option B: Command-Line Options
You can specify exact parameters for custom dimensions, custom framerates, or brightness inversion:

```powershell
python convert_video.py -i "C:\path\to\another_video.mp4" -o "bad_apple.bin" --width 160 --height 60 --fps 30
```

#### Available Flags:
| Argument | Description | Default |
|---|---|---|
| `-i`, `--input` | Path to video file (MP4, MKV, AVI, etc.) | Auto-detects video in folder |
| `-o`, `--output` | Output binary filename | `bad_apple.bin` |
| `-w`, `--width` | Horizontal resolution in characters | `160` |
| `-H`, `--height` | Vertical resolution in characters | `60` |
| `--fps` | Target playback framerate | `30.0` |
| `--levels` | Grayscale quantization levels (2 to 256) | `16` |
| `--invert` | Invert black and white levels | Off |

---

## How it Works

1. **Resolution & Monospace Ratio**: Characters in standard monospace fonts (like Consolas) have a ~1:2 width-to-height pixel ratio. A 160x60 character grid precisely matches a 4:3 video aspect ratio without distortion.
2. **RLE Compression**: Bad Apple and high-contrast animations compress down to ~7 MB for a full 3.6-minute 30 FPS video (~6,572 frames).
3. **Seamless Looping**: The C++ screensaver engine uses time-based modular indexing (`frame = (elapsed_ms * fps / 1000) % total_frames`), seamlessly restarting at frame 0 when the video ends.
4. **Instant GDI Line Blitting**: Uses line-buffered `TextOutA` calls (~60 calls per frame), delivering silky-smooth 60+ FPS playback with minimal CPU usage.
