#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>
#include <cstddef>

// Copies a top-down w x h 0xAARRGGBB buffer onto `dc` at (x, y).
void PresentPixels(HDC dc, int x, int y, const uint32_t* pixels, int w, int h);

// Subtracts `amount` from each RGB channel (clamped at 0) and sets alpha to 0xFF.
// Used by the trail-fading screensavers.
void FadePixels(uint32_t* pixels, size_t count, uint8_t amount);

// Reusable 32-bit frame buffer, cleared to black on Begin() and presented in one blit.
class PixelCanvas {
public:
    void Begin(int w, int h);
    void End(HDC dc, int x, int y) const { PresentPixels(dc, x, y, pixels.data(), width, height); }

    int Width() const { return width; }
    int Height() const { return height; }
    uint32_t* Row(int y) { return pixels.data() + static_cast<size_t>(y) * width; }

private:
    std::vector<uint32_t> pixels;
    int width = 0;
    int height = 0;
};
