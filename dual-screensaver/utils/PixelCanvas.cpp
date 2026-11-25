#include "PixelCanvas.h"
#include <cstring>
#include <emmintrin.h>

void PresentPixels(HDC dc, int x, int y, const uint32_t* pixels, int w, int h) {
    if (w <= 0 || h <= 0) return;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    SetDIBitsToDevice(dc, x, y, w, h, 0, 0, 0, h, pixels, &bmi, DIB_RGB_COLORS);
}

void FadePixels(uint32_t* pixels, size_t count, uint8_t amount) {
    const uint32_t sub = (static_cast<uint32_t>(amount) << 16) | (static_cast<uint32_t>(amount) << 8) | amount;
    const __m128i subV = _mm_set1_epi32(static_cast<int>(sub));
    const __m128i alphaV = _mm_set1_epi32(static_cast<int>(0xFF000000u));

    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixels + i));
        v = _mm_or_si128(_mm_subs_epu8(v, subV), alphaV);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(pixels + i), v);
    }
    for (; i < count; ++i) {
        uint32_t c = pixels[i];
        uint32_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
        r = r > amount ? r - amount : 0;
        g = g > amount ? g - amount : 0;
        b = b > amount ? b - amount : 0;
        pixels[i] = 0xFF000000u | (r << 16) | (g << 8) | b;
    }
}

void PixelCanvas::Begin(int w, int h) {
    size_t size = static_cast<size_t>(w) * h;
    if (w != width || h != height || pixels.size() != size) {
        width = w;
        height = h;
        pixels.assign(size, 0);
    }
    else {
        std::memset(pixels.data(), 0, size * sizeof(uint32_t));
    }
}
