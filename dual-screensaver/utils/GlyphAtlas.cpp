#include "GlyphAtlas.h"
#include <string>
#include <algorithm>

void GlyphAtlas::Build(HDC refDC, HFONT font, wchar_t first, int count) {
    HDC dc = CreateCompatibleDC(refDC);
    HGDIOBJ oldFont = SelectObject(dc, font);

    TEXTMETRICW tm;
    GetTextMetricsW(dc, &tm);
    int maxW = 0;
    for (int g = 0; g < count; ++g) {
        wchar_t ch = static_cast<wchar_t>(first + g);
        SIZE sz;
        if (GetTextExtentPoint32W(dc, &ch, 1, &sz) && sz.cx > maxW) maxW = sz.cx;
    }

    pad = tm.tmHeight / 4 + 1;
    cellW = maxW + tm.tmOverhang + pad * 2;
    cellH = tm.tmHeight + pad * 2;
    glyphCount = count;
    int atlasW = cellW * count;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = atlasW;
    bmi.bmiHeader.biHeight = -cellH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ oldBmp = SelectObject(dc, dib);

    RECT r = { 0, 0, atlasW, cellH };
    FillRect(dc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    for (int g = 0; g < count; ++g) {
        wchar_t ch = static_cast<wchar_t>(first + g);
        TextOutW(dc, g * cellW + pad, pad, &ch, 1);
    }
    GdiFlush();

    const uint32_t* px = static_cast<const uint32_t*>(bits);
    coverage.assign(px, px + static_cast<size_t>(atlasW) * cellH);

    SelectObject(dc, oldBmp);
    SelectObject(dc, oldFont);
    DeleteObject(dib);
    DeleteDC(dc);
}

void GlyphAtlas::Draw(PixelCanvas& canvas, int x, int y, int index, uint32_t rgb) const {
    if (index < 0 || index >= glyphCount) return;

    const uint32_t cr = (rgb >> 16) & 0xFF;
    const uint32_t cg = (rgb >> 8) & 0xFF;
    const uint32_t cb = rgb & 0xFF;

    const int originX = x - pad;
    const int originY = y - pad;
    const int sx0 = (std::max)(0, -originX);
    const int sy0 = (std::max)(0, -originY);
    const int sx1 = (std::min)(cellW, canvas.Width() - originX);
    const int sy1 = (std::min)(cellH, canvas.Height() - originY);
    const int stride = cellW * glyphCount;
    const uint32_t* glyph = coverage.data() + index * cellW;

    for (int sy = sy0; sy < sy1; ++sy) {
        const uint32_t* src = glyph + static_cast<size_t>(sy) * stride;
        uint32_t* dst = canvas.Row(originY + sy) + originX;
        for (int sx = sx0; sx < sx1; ++sx) {
            const uint32_t cov = src[sx];
            if ((cov & 0x00FFFFFF) == 0) continue;
            uint32_t r = (((cov >> 16) & 0xFF) * cr + 127) / 255;
            uint32_t g = (((cov >> 8) & 0xFF) * cg + 127) / 255;
            uint32_t b = ((cov & 0xFF) * cb + 127) / 255;
            const uint32_t d = dst[sx];
            r = (std::max)(r, (d >> 16) & 0xFF);
            g = (std::max)(g, (d >> 8) & 0xFF);
            b = (std::max)(b, d & 0xFF);
            dst[sx] = (r << 16) | (g << 8) | b;
        }
    }
}

void CenteredTextBlock::Update(HDC dc, const RECT& rect, int newCols, int newRows) {
    int newRectW = rect.right - rect.left;
    int newRectH = rect.bottom - rect.top;
    if (newCols == cols && newRows == rows && newRectW == rectW && newRectH == rectH) return;

    std::string block;
    block.reserve(static_cast<size_t>(newCols + 1) * newRows);
    for (int row = 0; row < newRows; ++row) {
        block.append(newCols, ' ');
        block += '\n';
    }
    RECT calcRect = rect;
    DrawTextA(dc, block.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);
    SIZE lineSize = {};
    GetTextExtentPoint32A(dc, block.c_str(), newCols, &lineSize);

    x = (rect.left + rect.right - lineSize.cx) / 2;
    y = (rect.bottom - (calcRect.bottom - calcRect.top)) / 2;
    cols = newCols;
    rows = newRows;
    rectW = newRectW;
    rectH = newRectH;
}
