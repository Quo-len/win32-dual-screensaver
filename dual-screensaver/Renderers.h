#pragma once
#include "framework.h"
#include "ScreenData.h"

void RenderDonut(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderGoL(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderMatrix(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderEarth(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderBlank(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderJulia(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderStars(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderDVD(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderGrid(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderPong(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderMaze(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderClock(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
void RenderPerlin(HDC memDC, ScreenData* data, int width, int height, const RECT& rect);
