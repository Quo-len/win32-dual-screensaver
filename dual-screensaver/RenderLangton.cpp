#include "framework.h"
#include "Renderers.h"
#include "Settings.h"
#include <vector>

void RenderLangton(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    int cellSize = 2;
    int cols = width / cellSize;
    int rows = height / cellSize;

    if (cols <= 0 || rows <= 0) return;

    if (data->antCols != cols || data->antRows != rows) {
        data->antCols = cols;
        data->antRows = rows;
        data->antGrid.assign(cols * rows, 0);
        data->pixels.assign(cols * rows, 0xFF000000);
        data->ants.clear();

        int symType = rand() % 3;

        for (int i = 0; i < g_AntCount; i++) {
            int cx = cols / 2;
            int cy = rows / 2;

            int rx = rand() % (cx / 4);
            int ry = rand() % (cy / 4);
            int rdir = rand() % 4;

            if (symType == 0) {
                data->ants.push_back({ cx + rx, cy + ry, rdir });
                data->ants.push_back({ cx - rx, cy - ry, (rdir + 2) % 4 });
            }
            else if (symType == 1) {
                data->ants.push_back({ cx + rx, cy + ry, rdir });

                int dirX = (rdir == 1) ? 3 : ((rdir == 3) ? 1 : rdir);
                data->ants.push_back({ cx - rx, cy + ry, dirX });

                int dirY = (rdir == 0) ? 2 : ((rdir == 2) ? 0 : rdir);
                data->ants.push_back({ cx + rx, cy - ry, dirY });

                int dirXY = (dirX == 0) ? 2 : ((dirX == 2) ? 0 : dirX);
                data->ants.push_back({ cx - rx, cy - ry, dirXY });
            }
            else {
                data->ants.push_back({ cx + rx, cy + ry, rdir });
                data->ants.push_back({ cx + ry, cy - rx, (rdir + 1) % 4 });
                data->ants.push_back({ cx - rx, cy - ry, (rdir + 2) % 4 });
                data->ants.push_back({ cx - ry, cy + rx, (rdir + 3) % 4 });
            }
        }
    }

    for (int step = 0; step < g_AntSpeed; step++) {
        for (auto& ant : data->ants) {
            if (ant.x < 0) ant.x += cols;
            if (ant.x >= cols) ant.x -= cols;
            if (ant.y < 0) ant.y += rows;
            if (ant.y >= rows) ant.y -= rows;

            int idx = ant.y * cols + ant.x;
            unsigned char state = data->antGrid[idx];

            if (state == 0) {
                ant.dir = (ant.dir + 1) % 4;
                data->antGrid[idx] = 1;
                data->pixels[idx] = 0xFFFFFFFF;
            }
            else {
                ant.dir = (ant.dir + 3) % 4;
                data->antGrid[idx] = 0;
                data->pixels[idx] = 0xFF000000;
            }

            if (ant.dir == 0) ant.y--;
            else if (ant.dir == 1) ant.x++;
            else if (ant.dir == 2) ant.y++;
            else if (ant.dir == 3) ant.x--;
        }
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cols;
    bmi.bmiHeader.biHeight = -rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(memDC, 0, 0, cols * cellSize, rows * cellSize,
        0, 0, cols, rows, data->pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}