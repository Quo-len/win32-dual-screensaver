#include "framework.h"
#include "Renderers.h"
#include "Settings.h"

void RenderMatrix(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int m_fontSize = data->isPreview ? 10 : g_TextSize;
    if (m_fontSize < 5) m_fontSize = 5;

    int m_cols = width / m_fontSize;
    int m_rows = height / m_fontSize;

    if (m_cols <= 0) m_cols = 1;
    if (m_rows <= 0) m_rows = 1;

    if (data->matrixDrops.size() != (size_t)m_cols || data->matrixChars.size() != (size_t)(m_cols * m_rows)) {
        data->matrixDrops.assign(m_cols, 0);
        data->matrixChars.assign(m_cols * m_rows, L' ');
        data->matrixIntensity.assign(m_cols * m_rows, 0);
        for (int i = 0; i < m_cols; ++i) data->matrixDrops[i] = rand() % m_rows;
    }

    for (int x = 0; x < m_cols; x++) {
        for (int y = 0; y < m_rows; y++) {
            int idx = y * m_cols + x;
            if (data->matrixIntensity[idx] > 0) {
                int v = data->matrixIntensity[idx] - (rand() % 25 + 5);
                data->matrixIntensity[idx] = v < 0 ? 0 : v;
            }
        }

        int headY = data->matrixDrops[x];
        if (headY >= 0 && headY < m_rows) {
            int idx = headY * m_cols + x;
            data->matrixChars[idx] = 0x30A0 + (rand() % 96);
            data->matrixIntensity[idx] = 255;
        }

        data->matrixDrops[x]++;
        if (data->matrixDrops[x] >= m_rows || (rand() % 100 > 95)) {
            data->matrixDrops[x] = 0;
        }
    }

    SelectObject(memDC, data->hMatrixFont);
    SetBkMode(memDC, TRANSPARENT);

    for (int y = 0; y < m_rows; y++) {
        for (int x = 0; x < m_cols; x++) {
            int idx = y * m_cols + x;
            int brightness = data->matrixIntensity[idx];
            if (brightness > 0) {
                COLORREF color = (brightness > 240) ? RGB(200, 255, 200) : RGB(0, brightness, 0);
                SetTextColor(memDC, color);
                TextOutW(memDC, x * m_fontSize, y * m_fontSize, &data->matrixChars[idx], 1);
            }
        }
    }
}
