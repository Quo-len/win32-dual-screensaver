#include "framework.h"
#include "ScreensaverRegistry.h"
#include "ScreenData.h"

struct StarState {
    struct Star { float x, y, z; };
    std::vector<Star> stars;
};

void RenderStars(HDC memDC, ScreenData* data, int width, int height, const RECT& rect) {
    auto& state = data->GetCustomState<StarState>(6);

    if (state.stars.empty()) {
        state.stars.resize(800);
        for (auto& s : state.stars) {
            s.x = (rand() % 4000) - 2000.0f;
            s.y = (rand() % 4000) - 2000.0f;
            s.z = (rand() % 2000) + 1.0f;
        }
    }

    for (auto& s : state.stars) {
        s.z -= 8.0f;
        if (s.z <= 1.0f) {
            s.x = (rand() % 4000) - 2000.0f;
            s.y = (rand() % 4000) - 2000.0f;
            s.z = 2000.0f;
        }
        int px = (int)((s.x / s.z) * 150) + width / 2;
        int py = (int)((s.y / s.z) * 150) + height / 2;

        if (px >= 0 && px < width && py >= 0 && py < height) {
            int brightness = 255 - (int)((s.z / 2000.0f) * 255);
            SetPixel(memDC, px, py, RGB(brightness, brightness, brightness));

            if (s.z < 500.0f) {
                SetPixel(memDC, px + 1, py, RGB(brightness, brightness, brightness));
                SetPixel(memDC, px, py + 1, RGB(brightness, brightness, brightness));
                SetPixel(memDC, px + 1, py + 1, RGB(brightness, brightness, brightness));
            }
        }
    }
}

REGISTER_SCREENSAVER(
    6,
    L"3D Starfield",
    "stars",
    { "stars", "starfield", "3dstars" },
    WRAP_LEGACY(RenderStars),
    {}
);

