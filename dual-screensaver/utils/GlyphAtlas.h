#pragma once
#include "PixelCanvas.h"
#include <windows.h>
#include <vector>
#include <cstdint>

// Glyphs of one font pre-rendered once with GDI, then stamped into a PixelCanvas
// in any color. Replaces per-character TextOut/SetTextColor calls, which are slow.
class GlyphAtlas {
public:
    // Renders glyphs [first, first + count) of the given font (font is not taken over).
    void Build(HDC refDC, HFONT font, wchar_t first, int count);

    // Draws glyph `index` as TextOut would with its origin at (x, y), in color 0xRRGGBB,
    // over a black background. Overlapping glyphs keep the brighter value per channel.
    void Draw(PixelCanvas& canvas, int x, int y, int index, uint32_t rgb) const;

private:
    std::vector<uint32_t> coverage;
    int glyphCount = 0;
    int cellW = 0;
    int cellH = 0;
    int pad = 0;
};

// Origin of a cols x rows character block centered in `rect`, matching the layout
// DrawText(DT_CENTER) produced for rows joined with '\n'. Measured once per size.
struct CenteredTextBlock {
    int x = 0;
    int y = 0;

    void Update(HDC dc, const RECT& rect, int cols, int rows);

private:
    int cols = -1, rows = -1, rectW = -1, rectH = -1;
};
