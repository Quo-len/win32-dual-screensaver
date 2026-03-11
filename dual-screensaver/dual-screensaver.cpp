#include "framework.h"
#include "dual-screensaver.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>

#define MAX_LOADSTRING 100

#define IDC_EDIT_ASPEED       1001
#define IDC_EDIT_BSPEED       1002
#define IDC_EDIT_SIZE         1003
#define IDC_EDIT_TEXTSIZE     1004
#define IDC_EDIT_GOL_SIZE     1005
#define IDC_EDIT_GOL_SPEED    1006
#define IDC_COMBO_PRIMARY     1007
#define IDC_COMBO_SECONDARY   1008
#define IDOK_BTN              1009
#define IDCANCEL_BTN          1010
#define IDRESET_BTN           1011
#define IDC_EDIT_EARTH_SPEED  1012
#define IDC_CHECK_RANDOM      1013
#define IDC_EDIT_PONG_SPEED   1014
#define IDC_EDIT_MAZE_BUILD_SPEED 1015
#define IDC_EDIT_MAZE_SOLVE_SPEED 1016

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING] = L"DualSaver";
WCHAR szWindowClass[MAX_LOADSTRING] = L"DualSaverClass";

float g_ASpeed = 0.04f;
float g_BSpeed = 0.02f;
float g_DonutSize = 2.0f;
int g_TextSize = 20;
int g_GolCellSize = 2;
int g_GolSpeed = 33;
float g_EarthSpeed = 0.05f;
float g_PongSpeed = 15.0f;
float g_MazeBuildSpeed = 10.0f;
float g_MazeSolveSpeed = 10.0f;

// 0=Donut, 1=GoL, 2=Matrix, 3=Earth, 4=Blank, 5=Julia, 6=Stars, 7=DVD, 8=Grid, 9=Pong, 10=Maze, 11=Clock
int g_ModePrimary = 11;
int g_ModeSecondary = 1;
int g_RandomMode = 0;

const WCHAR* REG_PATH = L"Software\\DualSaver";

struct MazeCell {
	bool visited;
	bool wallTop, wallRight, wallBottom, wallLeft;
	bool solveVisited;
	bool inPath;
};

struct ScreenData {
	bool isPrimary;
	bool isPreview;
	float A = 0;
	float B = 0;
	int cols = 0;
	int rows = 0;
	int stride = 0;
	std::vector<unsigned char> grid;
	std::vector<unsigned char> nextGrid;
	std::vector<uint32_t> pixels;
	DWORD lastGolUpdate = 0;
	HFONT hFont = NULL;

	std::vector<int> matrixDrops;
	std::vector<wchar_t> matrixChars;
	std::vector<unsigned char> matrixIntensity;
	HFONT hMatrixFont = NULL;

	DWORD startTime = 0;

	struct Star { float x, y, z; };
	std::vector<Star> stars;

	float logoX = 0, logoY = 0, logoDX = 3.0f, logoDY = 2.5f;
	int logoColorIndex = 0;

	std::vector<std::string> hexGrid;
	int hexCols = 0;
	int hexRows = 0;
	int activeRow = 0;
	int activeCol = 0;
	bool isRowActive = true;
	DWORD lastHexUpdate = 0;

	float pBallX = -1.0f;
	float pBallY = -1.0f;
	float pBallDX = 0.0f;
	float pBallDY = 0.0f;
	float pPadLeftY = 0.0f;
	float pPadRightY = 0.0f;

	std::vector<MazeCell> mazeGrid;
	int mazeCols = 0;
	int mazeRows = 0;
	int mazeState = 0;
	int mazeStart = 0;
	int mazeEnd = 0;
	std::vector<int> mazeStack;
	std::vector<int> solveStack;
	DWORD lastMazeUpdate = 0;
	int mazeWaitTimer = 0;

	// For Mode 11: Odometer Clock
	float digitOffset[8] = { 0 };
	char currentStr[16] = { 0 };
	char targetStr[16] = { 0 };
};

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK       MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
LRESULT CALLBACK    ConfigWindowProc(HWND, UINT, WPARAM, LPARAM);
void                ShowSettingsWindow(HINSTANCE);

void LoadSettings()
{
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		DWORD size = sizeof(float);
		RegQueryValueExW(hKey, L"ASpeed", NULL, NULL, (LPBYTE)&g_ASpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"BSpeed", NULL, NULL, (LPBYTE)&g_BSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"DonutSize", NULL, NULL, (LPBYTE)&g_DonutSize, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"TextSize", NULL, NULL, (LPBYTE)&g_TextSize, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"GolCellSize", NULL, NULL, (LPBYTE)&g_GolCellSize, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"GolSpeed", NULL, NULL, (LPBYTE)&g_GolSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"EarthSpeed", NULL, NULL, (LPBYTE)&g_EarthSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"PongSpeed", NULL, NULL, (LPBYTE)&g_PongSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"MazeBuildSpeed", NULL, NULL, (LPBYTE)&g_MazeBuildSpeed, &size);
		size = sizeof(float);
		RegQueryValueExW(hKey, L"MazeSolveSpeed", NULL, NULL, (LPBYTE)&g_MazeSolveSpeed, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModePrimary", NULL, NULL, (LPBYTE)&g_ModePrimary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"ModeSecondary", NULL, NULL, (LPBYTE)&g_ModeSecondary, &size);
		size = sizeof(int);
		RegQueryValueExW(hKey, L"RandomMode", NULL, NULL, (LPBYTE)&g_RandomMode, &size);
		RegCloseKey(hKey);
	}
}

void SaveSettings()
{
	HKEY hKey;
	if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_PATH, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		RegSetValueExW(hKey, L"ASpeed", 0, REG_DWORD, (const BYTE*)&g_ASpeed, sizeof(float));
		RegSetValueExW(hKey, L"BSpeed", 0, REG_DWORD, (const BYTE*)&g_BSpeed, sizeof(float));
		RegSetValueExW(hKey, L"DonutSize", 0, REG_DWORD, (const BYTE*)&g_DonutSize, sizeof(float));
		RegSetValueExW(hKey, L"TextSize", 0, REG_DWORD, (const BYTE*)&g_TextSize, sizeof(int));
		RegSetValueExW(hKey, L"GolCellSize", 0, REG_DWORD, (const BYTE*)&g_GolCellSize, sizeof(int));
		RegSetValueExW(hKey, L"GolSpeed", 0, REG_DWORD, (const BYTE*)&g_GolSpeed, sizeof(int));
		RegSetValueExW(hKey, L"EarthSpeed", 0, REG_DWORD, (const BYTE*)&g_EarthSpeed, sizeof(float));
		RegSetValueExW(hKey, L"PongSpeed", 0, REG_DWORD, (const BYTE*)&g_PongSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeBuildSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeBuildSpeed, sizeof(float));
		RegSetValueExW(hKey, L"MazeSolveSpeed", 0, REG_DWORD, (const BYTE*)&g_MazeSolveSpeed, sizeof(float));
		RegSetValueExW(hKey, L"ModePrimary", 0, REG_DWORD, (const BYTE*)&g_ModePrimary, sizeof(int));
		RegSetValueExW(hKey, L"ModeSecondary", 0, REG_DWORD, (const BYTE*)&g_ModeSecondary, sizeof(int));
		RegSetValueExW(hKey, L"RandomMode", 0, REG_DWORD, (const BYTE*)&g_RandomMode, sizeof(int));
		RegCloseKey(hKey);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	srand((unsigned int)time(NULL));
	LoadSettings();

	if (wcsstr(lpCmdLine, L"/c") || wcsstr(lpCmdLine, L"/C"))
	{
		ShowSettingsWindow(hInstance);
		return 0;
	}

	if (g_RandomMode) {
		g_ModePrimary = rand() % 12;
		g_ModeSecondary = rand() % 12;
	}

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DUALSCREENSAVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (wcsstr(lpCmdLine, L"/p") || wcsstr(lpCmdLine, L"/P"))
	{
		WCHAR* spacePos = wcschr(lpCmdLine, L' ');
		if (spacePos)
		{
			HWND hParent = (HWND)_wcstoui64(spacePos + 1, NULL, 10);
			RECT rc;
			GetClientRect(hParent, &rc);
			CreateWindowExW(0, szWindowClass, szTitle, WS_CHILD | WS_VISIBLE,
				0, 0, rc.right, rc.bottom, hParent, NULL, hInstance, (LPVOID)TRUE);

			MSG msg;
			while (GetMessage(&msg, nullptr, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return 0;
	}

	if (!InitInstance(hInstance, nCmdShow)) return FALSE;

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DUALSCREENSAVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
	return TRUE;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MONITORINFO mi = { sizeof(mi) };
	GetMonitorInfo(hMonitor, &mi);

	CreateWindowExW(WS_EX_TOPMOST, szWindowClass, szTitle,
		WS_POPUP | WS_VISIBLE,
		mi.rcMonitor.left, mi.rcMonitor.top,
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top,
		nullptr, nullptr, hInst, (LPVOID)(mi.dwFlags & MONITORINFOF_PRIMARY));

	ShowCursor(FALSE);
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static POINT initialMousePos;
	static bool mouseInitialized = false;

	ScreenData* data = (ScreenData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (message)
	{
	case WM_NCCREATE:
	{
		CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
		ScreenData* newData = new ScreenData();
		newData->isPrimary = (bool)cs->lpCreateParams;
		newData->isPreview = (cs->style & WS_CHILD) != 0;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)newData);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_CREATE:
		if (data) {
			data->hFont = CreateFontA(data->isPreview ? 10 : g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FIXED_PITCH | FF_MODERN, "Consolas");

			data->hMatrixFont = CreateFontW(data->isPreview ? 10 : g_TextSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				FIXED_PITCH | FF_MODERN, L"MS Gothic");

			data->startTime = GetTickCount();
		}
		SetTimer(hWnd, 1, 33, NULL);
		break;
	case WM_TIMER:
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		HDC memDC = CreateCompatibleDC(hdc);
		HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, width, height);
		HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hMemBmp);

		FillRect(memDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
		SetBkMode(memDC, OPAQUE);
		SetBkColor(memDC, RGB(0, 0, 0));

		int mode = data->isPreview ? 0 : (data->isPrimary ? g_ModePrimary : g_ModeSecondary);

		if (mode == 0) {
			SelectObject(memDC, data->hFont);
			TEXTMETRICA tm;
			GetTextMetricsA(memDC, &tm);

			int W = width / tm.tmAveCharWidth;
			int H = height / tm.tmHeight;
			if (W <= 0) W = 1;
			if (H <= 0) H = 1;

			std::vector<float> z(W * H, 0.0f);
			std::vector<char> b(W * H, ' ');

			float K2 = g_DonutSize + 3.0f;
			float proj_scale = K2 / (g_DonutSize + 1.0f);
			float x_mult = W * 0.225f * proj_scale;
			float y_mult = H * 0.409f * proj_scale;

			for (float j = 0; j < 6.28f; j += 0.07f) {
				for (float i = 0; i < 6.28f; i += 0.02f) {
					float c = sin(i), d = cos(j), e = sin(data->A), f = sin(j), g = cos(data->A);
					float h = d + g_DonutSize;
					float D = 1 / (c * h * e + f * g + K2);
					float l = cos(i), m = cos(data->B), n = sin(data->B);
					float t = c * h * g - f * e;

					int x = (W / 2) + x_mult * D * (l * h * m - t * n);
					int y = (H / 2) + y_mult * D * (l * h * n + t * m);
					int o = x + W * y;
					int N = 8 * ((f * e - c * d * g) * m - c * d * e - f * g - l * d * n);

					if (y >= 0 && y < H && x >= 0 && x < W && D > z[o]) {
						z[o] = D;
						b[o] = ".,-~:;=!*#$@"[N > 0 ? N : 0];
					}
				}
			}

			std::string out;
			out.reserve(W * H + H);
			for (int k = 0; k < W * H; k++) {
				out += b[k];
				if ((k + 1) % W == 0) out += '\n';
			}

			SetTextColor(memDC, RGB(0, 255, 0));
			RECT calcRect = rect;
			DrawTextA(memDC, out.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);

			int textHeight = calcRect.bottom - calcRect.top;
			RECT textRect = rect;
			textRect.top = (textRect.bottom - textHeight) / 2;
			DrawTextA(memDC, out.c_str(), -1, &textRect, DT_CENTER);

			data->A += g_ASpeed;
			data->B += g_BSpeed;
		}
		else if (mode == 1) {
			int targetCols = width / g_GolCellSize;
			int targetRows = height / g_GolCellSize;

			if (targetCols <= 0) targetCols = 1;
			if (targetRows <= 0) targetRows = 1;

			if (data->cols != targetCols || data->rows != targetRows || data->grid.empty()) {
				data->cols = targetCols;
				data->rows = targetRows;
				data->stride = targetCols + 2;

				int totalSize = data->stride * (targetRows + 2);
				data->grid.assign(totalSize, 0);
				data->nextGrid.assign(totalSize, 0);
				data->pixels.assign(targetCols * targetRows, 0);

				for (int y = 1; y <= data->rows; y++) {
					for (int x = 1; x <= data->cols; x++) {
						data->grid[y * data->stride + x] = ((rand() % 100) > 70) ? 1 : 0;
					}
				}
			}

			DWORD now = GetTickCount();
			if (now - data->lastGolUpdate >= (DWORD)g_GolSpeed) {
				unsigned char* grid = data->grid.data();
				unsigned char* next = data->nextGrid.data();
				int stride = data->stride;
				int rows = data->rows;
				int cols = data->cols;

				for (int x = 1; x <= cols; x++) {
					grid[x] = grid[rows * stride + x];
					grid[(rows + 1) * stride + x] = grid[stride + x];
				}
				for (int y = 0; y <= rows + 1; y++) {
					grid[y * stride] = grid[y * stride + cols];
					grid[y * stride + cols + 1] = grid[y * stride + 1];
				}

				bool changed = false;
				int aliveCount = 0;

				for (int y = 1; y <= rows; y++) {
					int idx = y * stride + 1;
					for (int x = 1; x <= cols; x++, idx++) {

						int n = grid[idx - stride - 1] + grid[idx - stride] + grid[idx - stride + 1] +
							grid[idx - 1] + grid[idx + 1] +
							grid[idx + stride - 1] + grid[idx + stride] + grid[idx + stride + 1];

						unsigned char state = grid[idx];
						unsigned char nextState = (n == 3 || (n == 2 && state)) ? 1 : 0;

						next[idx] = nextState;

						if (nextState) aliveCount++;
						if (state != nextState) changed = true;
					}
				}

				if (!changed || aliveCount == 0) {
					for (int y = 1; y <= rows; y++) {
						for (int x = 1; x <= cols; x++) {
							next[y * stride + x] = ((rand() % 100) > 70) ? 1 : 0;
						}
					}
				}

				data->grid.swap(data->nextGrid);
				data->lastGolUpdate = now;
			}

			uint32_t* px = data->pixels.data();
			unsigned char* grid = data->grid.data();
			int stride = data->stride;

			for (int y = 1; y <= data->rows; y++) {
				int rowOffset = y * stride;
				for (int x = 1; x <= data->cols; x++) {
					*px++ = grid[rowOffset + x] ? 0x0000FF00 : 0x00000000;
				}
			}

			BITMAPINFO bmi = { 0 };
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = data->cols;
			bmi.bmiHeader.biHeight = -data->rows;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			StretchDIBits(
				memDC,
				0, 0, width, height,
				0, 0, data->cols, data->rows,
				data->pixels.data(), &bmi,
				DIB_RGB_COLORS, SRCCOPY
			);
		}
		else if (mode == 2) {
			int m_fontSize = data->isPreview ? 10 : g_TextSize;
			if (m_fontSize < 5) m_fontSize = 5;

			int m_cols = width / m_fontSize;
			int m_rows = height / m_fontSize;

			if (m_cols <= 0) m_cols = 1;
			if (m_rows <= 0) m_rows = 1;

			if (data->matrixDrops.size() != m_cols || data->matrixChars.size() != m_cols * m_rows) {
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
		else if (mode == 3) {
			SelectObject(memDC, data->hFont);
			TEXTMETRICA tm;
			GetTextMetricsA(memDC, &tm);

			int W = width / tm.tmAveCharWidth;
			int H = height / tm.tmHeight;
			if (W <= 0) W = 1;
			if (H <= 0) H = 1;

			std::vector<char> b(W * H, ' ');

			float R_x = min(W / 2.0f, H * 1.0f) * 0.9f;
			float R_y = R_x * 0.5f;

			const char* earth_map[34] = {
				"............................................................................................................................................",
				"............................................................................................................................................",
				"..................................+++++++..+++++++++++++++++++..............................................................................",
				"......................+.+++++..+.+.+++++........++++++++++++++.............................+..........++++++++++++++..+.....++..............",
				"......++++++++++++++++++++++++++++++++..++++.....++++++++++.................++++++++.....+++++++++++++++++++++++++++++++++++++++++++++++++++",
				"......+++++++++++++++++++++++++++++.....++++......++++...................++++.+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++.",
				"........++.......+++++++++++++++++......++++++......................+....++++..++++++++++++++++++++++++++++++++++++++++++++++......++.......",
				"....................++++++++++++++++++.+++++++++....................++..++++++++++++++++++++++++++++++++++++++++++++++++++++.......+........",
				"......................+++++++++++++++++++++++........................+++++++++++++++++++++++++++++++++++++++++++++++++++++++................",
				"......................+++++++++++++++++++++........................++++...+.+++++....++++.+++++++++++++++++++++++++++++++...................",
				"......................+++++++++++++++++++..........................+++........+..++++++++..+++++++++++++++++++++++++...+....+...............",
				".........................++++++++++++++............................+++++++++..+....++++++++++++++++++++++++++++++++++.......................",
				"..........................++++++......+..........................++++++++++++++++++++++++++++++++++++++++++++++++++++.......................",
				".............................+++................................++++++++++++++++++++.++++++++....+++++++++++++++++..........................",
				"...............................++.++............................+++++++++++++++++++++.+++++.......++++.....+++++............................",
				"....................................++..........................+++++++++++++++++++++++++..........++.......+.++............................",
				"........................................+++++++..................++++++++++++++++++++++++...................................................",
				".......................................+++++++++++........................++++++++++++++.....................+...+++........................",
				".......................................++++++++++++++++...................++++++++++++.......................++..++........++...............",
				".......................................+++++++++++++++++...................++++++++++.......................................++..............",
				"........................................+++++++++++++++....................+++++++++++..................................+++..+..............",
				"..........................................+++++++++++++....................+++++++++...++............................++++++++++.............",
				"...........................................+++++++++........................++++++++...+..........................+++++++++++++++...........",
				"..........................................+++++++++.........................++++++................................+++++++++++++++...........",
				"..........................................+++++++............................+++...................................+++....+++++++...........",
				"..........................................++++................................................................................+.............",
				".........................................++++...............................................................................................",
				".........................................+++................................................................................................",
				"............................................................................................................................................",
				"............................................................................................................................................",
				"............................................................................................................................................",
				"............................................++.....................................+.++++++++++++..+++++++++++++++++++++++++++++++++........",
				"....................+++++++...++++++++++++++++.................+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++......",
				"..........+++++++++++++++++++++++++++++++..........+....+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++......."
			};

			for (int y = 0; y < H; y++) {
				for (int x = 0; x < W; x++) {
					float cx = (x - W / 2.0f);
					float cy = (y - H / 2.0f);

					float nx = cx / R_x;
					float ny = cy / R_y;
					float d2 = nx * nx + ny * ny;

					if (d2 <= 1.0f) {
						float nz = sqrt(1.0f - d2);
						float lat = asin(ny);
						float lon = atan2(nx, nz) + data->A;

						float u = (lon + 3.14159f) / (2.0f * 3.14159f);
						float v = (lat + 3.14159f / 2.0f) / 3.14159f;

						u = u - floor(u);
						if (v < 0.0f) v = 0.0f;
						if (v > 0.999f) v = 0.999f;

						int map_x = (int)(u * 140) % 140;
						int map_y = (int)(v * 34) % 34;

						b[y * W + x] = earth_map[map_y][map_x];
					}
				}
			}

			std::string out;
			out.reserve(W * H + H);
			for (int k = 0; k < W * H; k++) {
				out += b[k];
				if ((k + 1) % W == 0) out += '\n';
			}

			SetTextColor(memDC, RGB(0, 255, 0));
			RECT calcRect = rect;
			DrawTextA(memDC, out.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);

			int textHeight = calcRect.bottom - calcRect.top;
			RECT textRect = rect;
			textRect.top = (textRect.bottom - textHeight) / 2;
			DrawTextA(memDC, out.c_str(), -1, &textRect, DT_CENTER);

			data->A += g_EarthSpeed;
		}
		else if (mode == 4) {
			DWORD elapsed = GetTickCount() - data->startTime;
			int seconds = (elapsed / 1000) % 60;
			int minutes = (elapsed / 60000) % 60;
			int hours = (elapsed / 3600000);

			char msg[128];
			sprintf_s(msg, "< Away from PC for %02d hours, %02d minutes, %02d seconds >", hours, minutes, seconds);
			int msgLen = (int)strlen(msg);

			std::string topDashes(msgLen - 2, '_');
			std::string bottomDashes(msgLen - 2, '-');

			std::string cow = " " + topDashes + "\n" +
				msg + "\n" +
				" " + bottomDashes + "\n" +
				"        \\   ^__^\n" +
				"         \\  (oo)\\_______\n" +
				"            (__)\\       )\\/\\\n" +
				"                ||----w |\n" +
				"                ||     ||";

			SelectObject(memDC, data->hFont);
			SetTextColor(memDC, RGB(200, 200, 200));
			SetBkMode(memDC, TRANSPARENT);

			RECT calcRect = { 0, 0, 0, 0 };
			DrawTextA(memDC, cow.c_str(), -1, &calcRect, DT_CALCRECT | DT_LEFT);

			int cowWidth = calcRect.right - calcRect.left;
			int cowHeight = calcRect.bottom - calcRect.top;

			RECT drawRect;
			drawRect.left = (width / 2) + (int)(width * 0.05f);
			if (drawRect.left + cowWidth > width) drawRect.left = width - cowWidth - 20;

			drawRect.right = drawRect.left + cowWidth;
			drawRect.bottom = height - 50;
			drawRect.top = drawRect.bottom - cowHeight;

			DrawTextA(memDC, cow.c_str(), -1, &drawRect, DT_LEFT);
		}
		else if (mode == 5) {
			SelectObject(memDC, data->hFont);
			TEXTMETRICA tm;
			GetTextMetricsA(memDC, &tm);

			int W = width / tm.tmAveCharWidth;
			int H = height / tm.tmHeight;
			if (W <= 0) W = 1;
			if (H <= 0) H = 1;

			std::vector<char> b(W * H, ' ');

			double zoom = 1.0 + 0.15 * sin(data->A * 0.3);
			double widthInComplex = 3.5 / zoom;
			double heightInComplex = widthInComplex * ((double)H / W) * 2.0;

			double minX = -widthInComplex / 2.0;
			double minY = -heightInComplex / 2.0;

			double dx = widthInComplex / W;
			double dy = heightInComplex / H;

			double cx = 0.7885 * cos(data->A * 0.5);
			double cy = 0.7885 * sin(data->A * 0.5);

			const char* charset = " .,-~:;=!*#$@";
			const char* insideChars = "WM#0@&8Q";
			int maxIter = 80;

			for (int y = 0; y < H; y++) {
				for (int x = 0; x < W; x++) {
					double zx = minX + x * dx;
					double zy = minY + y * dy;
					int iter = 0;
					while (zx * zx + zy * zy < 4.0 && iter < maxIter) {
						double tmp = zx * zx - zy * zy + cx;
						zy = 2.0 * zx * zy + cy;
						zx = tmp;
						iter++;
					}
					if (iter == maxIter) {
						b[y * W + x] = insideChars[(x * 17 + y * 31) % 8];
					}
					else {
						if (iter < 3) {
							b[y * W + x] = ' ';
						}
						else {
							b[y * W + x] = charset[iter % 13];
						}
					}
				}
			}

			std::string out;
			out.reserve(W * H + H);
			for (int k = 0; k < W * H; k++) {
				out += b[k];
				if ((k + 1) % W == 0) out += '\n';
			}

			SetTextColor(memDC, RGB(255, 255, 255));
			SetBkMode(memDC, TRANSPARENT);

			RECT calcRect = rect;
			DrawTextA(memDC, out.c_str(), -1, &calcRect, DT_CALCRECT | DT_CENTER);

			int textHeight = calcRect.bottom - calcRect.top;
			RECT textRect = rect;
			textRect.top = (textRect.bottom - textHeight) / 2;

			DrawTextA(memDC, out.c_str(), -1, &textRect, DT_CENTER);

			data->A += g_ASpeed * 0.5f;
		}
		else if (mode == 6) { // 3D Starfield
			if (data->stars.empty()) {
				data->stars.resize(800);
				for (auto& s : data->stars) {
					s.x = (rand() % 4000) - 2000.0f;
					s.y = (rand() % 4000) - 2000.0f;
					s.z = (rand() % 2000) + 1.0f;
				}
			}
			for (auto& s : data->stars) {
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
		else if (mode == 7) { // Classic Bouncing Logo
			const char* logoText = "DVD";

			int dvdFontSize = max(50, height / 6);
			HFONT hDvdFont = CreateFontA(
				dvdFontSize,
				0, 0, 0,
				FW_HEAVY,
				FALSE, // italic off for precise bounds
				FALSE, FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				ANTIALIASED_QUALITY,
				DEFAULT_PITCH | FF_SWISS,
				"Impact"
			);

			HFONT hOldFont = (HFONT)SelectObject(memDC, hDvdFont);

			SIZE textSize;
			GetTextExtentPoint32A(memDC, logoText, (int)strlen(logoText), &textSize);

			int tw = textSize.cx;
			int th = textSize.cy;

			if (data->logoX == 0 && data->logoY == 0) {
				data->logoX = (float)(rand() % max(1, width - tw));
				data->logoY = (float)(rand() % max(1, height - th));
			}

			data->logoX += data->logoDX;
			data->logoY += data->logoDY;

			bool bounced = false;

			if (data->logoX <= 0) {
				data->logoX = 0;
				data->logoDX *= -1;
				bounced = true;
			}
			else if (data->logoX + tw >= width) {
				data->logoX = (float)(width - tw);
				data->logoDX *= -1;
				bounced = true;
			}

			if (data->logoY <= 0) {
				data->logoY = 0;
				data->logoDY *= -1;
				bounced = true;
			}
			else if (data->logoY + th >= height) {
				data->logoY = (float)(height - th);
				data->logoDY *= -1;
				bounced = true;
			}

			if (bounced) {
				data->logoColorIndex = (data->logoColorIndex + 1) % 6;
			}

			COLORREF colors[6] = {
				RGB(255,50,50),
				RGB(50,255,50),
				RGB(100,100,255),
				RGB(255,255,50),
				RGB(255,50,255),
				RGB(50,255,255)
			};

			SetBkMode(memDC, TRANSPARENT);
			SetTextColor(memDC, colors[data->logoColorIndex]);

			TextOutA(memDC, (int)data->logoX, (int)data->logoY, logoText, (int)strlen(logoText));

			SelectObject(memDC, hOldFont);
			DeleteObject(hDvdFont);
		}
		else if (mode == 8) {
			int padding = 40;
			int cellSize = g_TextSize * 2;
			if (cellSize < 10) cellSize = 10;

			int targetCols = (width - padding * 2) / cellSize;
			int targetRows = (height - padding * 2) / cellSize;

			if (targetCols <= 0) targetCols = 1;
			if (targetRows <= 0) targetRows = 1;

			if (data->hexCols != targetCols || data->hexRows != targetRows || data->hexGrid.empty()) {
				data->hexCols = targetCols;
				data->hexRows = targetRows;
				data->hexGrid.assign(targetCols * targetRows, "00");
				const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF" };
				for (int i = 0; i < targetCols * targetRows; i++) {
					data->hexGrid[i] = codes[rand() % 6];
				}
				data->activeRow = rand() % targetRows;
				data->activeCol = rand() % targetCols;
				data->isRowActive = true;
				data->lastHexUpdate = GetTickCount();
			}

			DWORD now = GetTickCount();
			if (now - data->lastHexUpdate > 1000) {
				if (data->isRowActive) {
					data->activeCol = rand() % data->hexCols;
					data->isRowActive = false;
				}
				else {
					data->activeRow = rand() % data->hexRows;
					data->isRowActive = true;
				}
				data->lastHexUpdate = now;
			}

			SelectObject(memDC, data->hFont);

			int startX = (width - (data->hexCols * cellSize)) / 2;
			int startY = (height - (data->hexRows * cellSize)) / 2;

			for (int r = 0; r < data->hexRows; r++) {
				for (int c = 0; c < data->hexCols; c++) {
					int idx = r * data->hexCols + c;

					COLORREF color = RGB(40, 200, 40);

					if (r == data->activeRow && c == data->activeCol) {
						color = RGB(0, 0, 0);
						SetBkMode(memDC, OPAQUE);
						SetBkColor(memDC, RGB(255, 0, 85));
					}
					else if ((data->isRowActive && r == data->activeRow) || (!data->isRowActive && c == data->activeCol)) {
						color = RGB(250, 250, 50);
						SetBkMode(memDC, TRANSPARENT);
					}
					else {
						SetBkMode(memDC, TRANSPARENT);
					}

					if (rand() % 1000 > 985) {
						const char* codes[] = { "55", "BD", "1C", "E9", "7A", "FF", "4B", "00" };
						data->hexGrid[idx] = codes[rand() % 8];
					}

					SetTextColor(memDC, color);
					TextOutA(memDC, startX + c * cellSize, startY + r * cellSize, data->hexGrid[idx].c_str(), 2);
				}
			}
		}
		else if (mode == 9) {
			int padWidth = max(5, width / 70);
			int padHeight = max(20, height / 6);
			int ballSize = padWidth;

			float speedX = g_PongSpeed;
			float speedY = g_PongSpeed;

			if (data->pBallX < 0.0f) {
				data->pBallX = (float)(width / 2);
				data->pBallY = (float)(height / 2);
				data->pBallDX = speedX;
				data->pBallDY = speedY;
				data->pPadLeftY = (float)(height / 2 - padHeight / 2);
				data->pPadRightY = (float)(height / 2 - padHeight / 2);
			}

			data->pBallDX = (data->pBallDX > 0) ? speedX : -speedX;
			data->pBallDY = (data->pBallDY > 0) ? speedY : -speedY;

			data->pBallX += data->pBallDX;
			data->pBallY += data->pBallDY;

			if (data->pBallY <= 0.0f) {
				data->pBallY = 0.0f;
				data->pBallDY *= -1.0f;
			}
			else if (data->pBallY + ballSize >= height) {
				data->pBallY = height - ballSize;
				data->pBallDY *= -1.0f;
			}

			if (data->pBallX <= padWidth) {
				data->pBallX = padWidth;
				data->pBallDX *= -1.0f;
			}
			else if (data->pBallX + ballSize >= width - padWidth) {
				data->pBallX = width - padWidth - ballSize;
				data->pBallDX *= -1.0f;
			}

			data->pPadLeftY = data->pBallY + (ballSize / 2.0f) - (padHeight / 2.0f);
			data->pPadRightY = data->pBallY + (ballSize / 2.0f) - (padHeight / 2.0f);

			if (data->pPadLeftY < 0.0f) data->pPadLeftY = 0.0f;
			if (data->pPadLeftY > height - padHeight) data->pPadLeftY = (float)(height - padHeight);

			if (data->pPadRightY < 0.0f) data->pPadRightY = 0.0f;
			if (data->pPadRightY > height - padHeight) data->pPadRightY = (float)(height - padHeight);

			HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));

			RECT rectLeft = { 0, (int)data->pPadLeftY, padWidth, (int)data->pPadLeftY + padHeight };
			FillRect(memDC, &rectLeft, hBrush);

			RECT rectRight = { width - padWidth, (int)data->pPadRightY, width, (int)data->pPadRightY + padHeight };
			FillRect(memDC, &rectRight, hBrush);

			RECT rectBall = { (int)data->pBallX, (int)data->pBallY, (int)data->pBallX + ballSize, (int)data->pBallY + ballSize };
			FillRect(memDC, &rectBall, hBrush);

			for (int i = 0; i < height; i += padHeight) {
				RECT dash = { width / 2 - padWidth / 4, i + padHeight / 4, width / 2 + padWidth / 4, i + padHeight * 3 / 4 };
				FillRect(memDC, &dash, hBrush);
			}

			DeleteObject(hBrush);
		}
		else if (mode == 10) {
			int cellSize = max(10, g_TextSize);
			int padding = 40;
			int mCols = (width - padding * 2) / cellSize;
			int mRows = (height - padding * 2) / cellSize;

			if (mCols <= 2) mCols = 3;
			if (mRows <= 2) mRows = 3;

			if (data->mazeCols != mCols || data->mazeRows != mRows || data->mazeGrid.empty() || data->mazeState == 0) {
				data->mazeCols = mCols;
				data->mazeRows = mRows;

				data->mazeGrid.clear();
				data->mazeGrid.assign(mCols * mRows, { false, true, true, true, true, false, false });
				data->mazeState = 1;
				data->mazeStack.clear();
				data->solveStack.clear();

				data->mazeStart = rand() % (mCols * mRows);
				data->mazeEnd = rand() % (mCols * mRows);
				while (data->mazeEnd == data->mazeStart) {
					data->mazeEnd = rand() % (mCols * mRows);
				}

				data->mazeStack.push_back(data->mazeStart);
				data->mazeGrid[data->mazeStart].visited = true;
				data->lastMazeUpdate = GetTickCount();
				data->mazeWaitTimer = 0;
			}

			DWORD now = GetTickCount();
			int stepsPerFrame = max(1, (int)(data->mazeState == 1 ? g_MazeBuildSpeed : g_MazeSolveSpeed));

			if (now - data->lastMazeUpdate > 16) {
				data->lastMazeUpdate = now;

				if (data->mazeState == 1) {
					for (int step = 0; step < stepsPerFrame * 5 && !data->mazeStack.empty(); ++step) {
						int current = data->mazeStack.back();
						int cx = current % data->mazeCols;
						int cy = current / data->mazeCols;

						std::vector<int> neighbors;
						std::vector<int> dirs;
						if (cy > 0 && !data->mazeGrid[current - data->mazeCols].visited) { neighbors.push_back(current - data->mazeCols); dirs.push_back(0); }
						if (cx < data->mazeCols - 1 && !data->mazeGrid[current + 1].visited) { neighbors.push_back(current + 1); dirs.push_back(1); }
						if (cy < data->mazeRows - 1 && !data->mazeGrid[current + data->mazeCols].visited) { neighbors.push_back(current + data->mazeCols); dirs.push_back(2); }
						if (cx > 0 && !data->mazeGrid[current - 1].visited) { neighbors.push_back(current - 1); dirs.push_back(3); }

						if (!neighbors.empty()) {
							int r = rand() % neighbors.size();
							int next = neighbors[r];
							int dir = dirs[r];

							if (dir == 0) { data->mazeGrid[current].wallTop = false; data->mazeGrid[next].wallBottom = false; }
							else if (dir == 1) { data->mazeGrid[current].wallRight = false; data->mazeGrid[next].wallLeft = false; }
							else if (dir == 2) { data->mazeGrid[current].wallBottom = false; data->mazeGrid[next].wallTop = false; }
							else if (dir == 3) { data->mazeGrid[current].wallLeft = false; data->mazeGrid[next].wallRight = false; }

							data->mazeGrid[next].visited = true;
							data->mazeStack.push_back(next);
						}
						else {
							data->mazeStack.pop_back();
						}
					}
					if (data->mazeStack.empty()) {
						data->mazeState = 2;
						data->solveStack.push_back(data->mazeStart);
						data->mazeGrid[data->mazeStart].solveVisited = true;
						data->mazeGrid[data->mazeStart].inPath = true;
					}
				}
				else if (data->mazeState == 2) {
					for (int step = 0; step < stepsPerFrame && !data->solveStack.empty(); ++step) {
						int current = data->solveStack.back();

						if (current == data->mazeEnd) {
							data->mazeState = 3;
							data->mazeWaitTimer = 45;
							break;
						}

						int cx = current % data->mazeCols;
						int cy = current / data->mazeCols;

						std::vector<int> nextDirs;
						if (!data->mazeGrid[current].wallTop && !data->mazeGrid[current - data->mazeCols].solveVisited) nextDirs.push_back(current - data->mazeCols);
						if (!data->mazeGrid[current].wallRight && !data->mazeGrid[current + 1].solveVisited) nextDirs.push_back(current + 1);
						if (!data->mazeGrid[current].wallBottom && !data->mazeGrid[current + data->mazeCols].solveVisited) nextDirs.push_back(current + data->mazeCols);
						if (!data->mazeGrid[current].wallLeft && !data->mazeGrid[current - 1].solveVisited) nextDirs.push_back(current - 1);

						if (!nextDirs.empty()) {
							int next = nextDirs[0];
							data->mazeGrid[next].solveVisited = true;
							data->mazeGrid[next].inPath = true;
							data->solveStack.push_back(next);
						}
						else {
							data->mazeGrid[current].inPath = false;
							data->solveStack.pop_back();
						}
					}
				}
				else if (data->mazeState == 3) {
					data->mazeWaitTimer--;
					if (data->mazeWaitTimer <= 0) {
						data->mazeState = 0;
					}
				}
			}

			int offsetX = (width - (data->mazeCols * cellSize)) / 2;
			int offsetY = (height - (data->mazeRows * cellSize)) / 2;

			HPEN hWallPen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
			HPEN hOldPen = (HPEN)SelectObject(memDC, hWallPen);
			HBRUSH hPathBrush = CreateSolidBrush(RGB(255, 0, 0));
			HBRUSH hSolveBrush = CreateSolidBrush(RGB(50, 50, 150));
			HBRUSH hHeadBrush = CreateSolidBrush(RGB(255, 255, 255));
			HBRUSH hStartBrush = CreateSolidBrush(RGB(255, 255, 0));
			HBRUSH hEndBrush = CreateSolidBrush(RGB(0, 255, 255));
			HBRUSH hOldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));

			for (int cy = 0; cy < data->mazeRows; cy++) {
				for (int cx = 0; cx < data->mazeCols; cx++) {
					int i = cy * data->mazeCols + cx;
					int px = offsetX + cx * cellSize;
					int py = offsetY + cy * cellSize;

					if (data->mazeGrid[i].inPath) {
						RECT rc = { px + cellSize / 4, py + cellSize / 4, px + cellSize - cellSize / 4, py + cellSize - cellSize / 4 };
						FillRect(memDC, &rc, hPathBrush);
					}
					else if (data->mazeGrid[i].solveVisited) {
						RECT rc = { px + cellSize / 3, py + cellSize / 3, px + cellSize - cellSize / 3, py + cellSize - cellSize / 3 };
						FillRect(memDC, &rc, hSolveBrush);
					}

					if (data->mazeGrid[i].visited) {
						if (data->mazeGrid[i].wallTop) { MoveToEx(memDC, px, py, NULL); LineTo(memDC, px + cellSize, py); }
						if (data->mazeGrid[i].wallBottom) { MoveToEx(memDC, px, py + cellSize, NULL); LineTo(memDC, px + cellSize, py + cellSize); }
						if (data->mazeGrid[i].wallLeft) { MoveToEx(memDC, px, py, NULL); LineTo(memDC, px, py + cellSize); }
						if (data->mazeGrid[i].wallRight) { MoveToEx(memDC, px + cellSize, py, NULL); LineTo(memDC, px + cellSize, py + cellSize); }
					}
				}
			}

			if (data->mazeState > 0) {
				int sx = (data->mazeStart % data->mazeCols) * cellSize + offsetX;
				int sy = (data->mazeStart / data->mazeCols) * cellSize + offsetY;
				RECT sr = { sx + 2, sy + 2, sx + cellSize - 2, sy + cellSize - 2 };
				FillRect(memDC, &sr, hStartBrush);

				int ex = (data->mazeEnd % data->mazeCols) * cellSize + offsetX;
				int ey = (data->mazeEnd / data->mazeCols) * cellSize + offsetY;
				RECT er = { ex + 2, ey + 2, ex + cellSize - 2, ey + cellSize - 2 };
				FillRect(memDC, &er, hEndBrush);
			}

			if (data->mazeState == 1 && !data->mazeStack.empty()) {
				int current = data->mazeStack.back();
				int cx = current % data->mazeCols;
				int cy = current / data->mazeCols;
				RECT rc = { offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, offsetX + (cx + 1) * cellSize - 2, offsetY + (cy + 1) * cellSize - 2 };
				FillRect(memDC, &rc, hHeadBrush);
			}
			else if (data->mazeState == 2 && !data->solveStack.empty()) {
				int current = data->solveStack.back();
				int cx = current % data->mazeCols;
				int cy = current / data->mazeCols;
				RECT rc = { offsetX + cx * cellSize + 2, offsetY + cy * cellSize + 2, offsetX + (cx + 1) * cellSize - 2, offsetY + (cy + 1) * cellSize - 2 };
				FillRect(memDC, &rc, hHeadBrush);
			}

			SelectObject(memDC, hOldPen);
			SelectObject(memDC, hOldBrush);
			DeleteObject(hWallPen);
			DeleteObject(hPathBrush);
			DeleteObject(hSolveBrush);
			DeleteObject(hHeadBrush);
			DeleteObject(hStartBrush);
			DeleteObject(hEndBrush);
		}
		else if (mode == 11) { // Odometer Clock
			int clockFontSize = max(30, height / 3);
			HFONT hClockFont = CreateFontA(clockFontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
				FIXED_PITCH | FF_MODERN, "Consolas");

			HFONT hOldFont = (HFONT)SelectObject(memDC, hClockFont);

			SYSTEMTIME st;
			GetLocalTime(&st);

			char newTime[16];
			sprintf_s(newTime, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

			if (data->currentStr[0] == '\0') {
				strcpy_s(data->currentStr, newTime);
				strcpy_s(data->targetStr, newTime);
				for (int i = 0; i < 8; i++) data->digitOffset[i] = 0.0f;
			}

			for (int i = 0; i < 8; i++) {
				if (data->targetStr[i] != newTime[i]) {
					data->targetStr[i] = newTime[i];
				}
			}

			SetTextColor(memDC, RGB(0, 255, 150));
			SetBkMode(memDC, TRANSPARENT);

			SIZE sz;
			GetTextExtentPoint32A(memDC, "0", 1, &sz);
			int charW = sz.cx;
			int charH = sz.cy;

			int totalW = 8 * charW;
			int startX = (width - totalW) / 2;
			int startY = (height - charH) / 2;

			for (int i = 0; i < 8; i++) {
				if (data->currentStr[i] != data->targetStr[i]) {
					data->digitOffset[i] += 0.12f;
					if (data->digitOffset[i] >= 1.0f) {
						data->currentStr[i] = data->targetStr[i];
						data->digitOffset[i] = 0.0f;
					}
				}

				int x = startX + i * charW;
				int y = startY;

				HRGN hRgn = CreateRectRgn(x, y, x + charW, y + charH);
				SelectClipRgn(memDC, hRgn);

				if (data->currentStr[i] == data->targetStr[i]) {
					char str[2] = { data->currentStr[i], 0 };
					TextOutA(memDC, x, y, str, 1);
				}
				else {
					char strOld[2] = { data->currentStr[i], 0 };
					char targetOld[2] = { data->targetStr[i], 0 };

					int yOffset = (int)(data->digitOffset[i] * charH);

					TextOutA(memDC, x, y - yOffset, strOld, 1);
					TextOutA(memDC, x, y + charH - yOffset, targetOld, 1);
				}

				SelectClipRgn(memDC, NULL);
				DeleteObject(hRgn);
			}

			SelectObject(memDC, hOldFont);
			DeleteObject(hClockFont);
		}

		BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

		SelectObject(memDC, hOldBmp);
		DeleteObject(hMemBmp);
		DeleteDC(memDC);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_MOUSEMOVE:
	{
		if (data && data->isPreview) break;
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		if (!mouseInitialized) {
			initialMousePos = pt;
			mouseInitialized = true;
		}
		else if (abs(pt.x - initialMousePos.x) > 5 || abs(pt.y - initialMousePos.y) > 5) {
			PostQuitMessage(0);
		}
	}
	break;
	case WM_KEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if (data && !data->isPreview) PostQuitMessage(0);
		break;
	case WM_DESTROY:
		if (data) {
			if (data->hFont) DeleteObject(data->hFont);
			if (data->hMatrixFont) DeleteObject(data->hMatrixFont);
			delete data;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ShowSettingsWindow(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ConfigWindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcex.lpszClassName = L"SaverSettingsClass";

	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SaverSettingsClass", L"Screensaver Settings",
		WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		CW_USEDEFAULT, CW_USEDEFAULT, 310, 680,
		nullptr, nullptr, hInstance, nullptr);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT CALLBACK ConfigWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		HFONT hBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		int y = 10;
		HWND hL1 = CreateWindowW(L"STATIC", L"Donut Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL1, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h1 = CreateWindowW(L"STATIC", L"A Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hA = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_ASPEED, hInst, NULL); y += 30;

		HWND h2 = CreateWindowW(L"STATIC", L"B Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_BSPEED, hInst, NULL); y += 30;

		HWND h3 = CreateWindowW(L"STATIC", L"Donut Size:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_SIZE, hInst, NULL); y += 30;

		HWND h4 = CreateWindowW(L"STATIC", L"Text Size:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hT = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_TEXTSIZE, hInst, NULL); y += 35;

		HWND hL2 = CreateWindowW(L"STATIC", L"Game of Life Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL2, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h5 = CreateWindowW(L"STATIC", L"Cell Size (px):", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hG1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_GOL_SIZE, hInst, NULL); y += 30;

		HWND h6 = CreateWindowW(L"STATIC", L"Speed (ms):", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hG2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_GOL_SPEED, hInst, NULL); y += 35;

		HWND hL4 = CreateWindowW(L"STATIC", L"Earth Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL4, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h9 = CreateWindowW(L"STATIC", L"Spin Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hES = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_EARTH_SPEED, hInst, NULL); y += 35;

		HWND hL5 = CreateWindowW(L"STATIC", L"Ping Pong Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL5, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h10 = CreateWindowW(L"STATIC", L"Game Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hPS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_PONG_SPEED, hInst, NULL); y += 35;

		HWND hL6 = CreateWindowW(L"STATIC", L"Maze Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL6, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h11 = CreateWindowW(L"STATIC", L"Build Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hMB = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_MAZE_BUILD_SPEED, hInst, NULL); y += 30;

		HWND h12 = CreateWindowW(L"STATIC", L"Solve Speed:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hMS = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 130, y, 100, 20, hWnd, (HMENU)IDC_EDIT_MAZE_SOLVE_SPEED, hInst, NULL); y += 35;

		HWND hL3 = CreateWindowW(L"STATIC", L"Monitor Settings", WS_CHILD | WS_VISIBLE, 10, y, 200, 20, hWnd, NULL, hInst, NULL);
		SendMessage(hL3, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0)); y += 25;

		HWND h7 = CreateWindowW(L"STATIC", L"Primary:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hC1 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, y, 100, 100, hWnd, (HMENU)IDC_COMBO_PRIMARY, hInst, NULL); y += 30;

		HWND h8 = CreateWindowW(L"STATIC", L"Secondary:", WS_CHILD | WS_VISIBLE, 20, y, 100, 20, hWnd, NULL, hInst, NULL);
		HWND hC2 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 130, y, 100, 100, hWnd, (HMENU)IDC_COMBO_SECONDARY, hInst, NULL); y += 35;

		HWND hRand = CreateWindowW(L"BUTTON", L"Randomize every launch", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 20, y, 250, 20, hWnd, (HMENU)IDC_CHECK_RANDOM, hInst, NULL); y += 35;

		HWND hOk = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 30, y, 70, 25, hWnd, (HMENU)IDOK_BTN, hInst, NULL);
		HWND hReset = CreateWindowW(L"BUTTON", L"Reset", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 110, y, 70, 25, hWnd, (HMENU)IDRESET_BTN, hInst, NULL);
		HWND hCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 190, y, 70, 25, hWnd, (HMENU)IDCANCEL_BTN, hInst, NULL);

		const WCHAR* options[] = { L"Donut", L"Game of Life", L"Matrix", L"Earth", L"Blank", L"Julia Spirals", L"3D Starfield", L"Bouncing DVD Logo", L"Grid" , L"Pong", L"Maze Generator", L"Odometer Clock" };
		for (int i = 0; i < 12; i++) {
			SendMessage(hC1, CB_ADDSTRING, 0, (LPARAM)options[i]);
			SendMessage(hC2, CB_ADDSTRING, 0, (LPARAM)options[i]);
		}

		SendMessage(h1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hA, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hB, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h3, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hS, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h4, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hT, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h5, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hG1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h6, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hG2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h9, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hES, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h10, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hPS, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h11, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hMB, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h12, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hMS, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h7, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hC1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(h8, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hC2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hRand, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hReset, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		char buf[32];
		sprintf_s(buf, "%.3f", g_ASpeed); SetWindowTextA(hA, buf);
		sprintf_s(buf, "%.3f", g_BSpeed); SetWindowTextA(hB, buf);
		sprintf_s(buf, "%.1f", g_DonutSize); SetWindowTextA(hS, buf);
		sprintf_s(buf, "%d", g_TextSize); SetWindowTextA(hT, buf);
		sprintf_s(buf, "%d", g_GolCellSize); SetWindowTextA(hG1, buf);
		sprintf_s(buf, "%d", g_GolSpeed); SetWindowTextA(hG2, buf);
		sprintf_s(buf, "%.3f", g_EarthSpeed); SetWindowTextA(hES, buf);
		sprintf_s(buf, "%.1f", g_PongSpeed); SetWindowTextA(hPS, buf);
		sprintf_s(buf, "%.1f", g_MazeBuildSpeed); SetWindowTextA(hMB, buf);
		sprintf_s(buf, "%.1f", g_MazeSolveSpeed); SetWindowTextA(hMS, buf);
		SendMessage(hC1, CB_SETCURSEL, g_ModePrimary, 0);
		SendMessage(hC2, CB_SETCURSEL, g_ModeSecondary, 0);

		SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_SETCHECK, g_RandomMode ? BST_CHECKED : BST_UNCHECKED, 0);

		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK_BTN)
		{
			char buf[32];
			GetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf, 32); g_ASpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf, 32); g_BSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf, 32); g_DonutSize = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf, 32); g_TextSize = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf, 32); g_GolCellSize = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf, 32); g_GolSpeed = atoi(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf, 32); g_EarthSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf, 32); g_PongSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf, 32); g_MazeBuildSpeed = (float)atof(buf);
			GetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf, 32); g_MazeSolveSpeed = (float)atof(buf);

			g_ModePrimary = SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY), CB_GETCURSEL, 0, 0);
			g_ModeSecondary = SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_GETCURSEL, 0, 0);

			g_RandomMode = SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;

			if (g_GolCellSize < 1) g_GolCellSize = 1;
			if (g_GolSpeed < 10) g_GolSpeed = 10;
			if (g_MazeBuildSpeed < 1) g_MazeBuildSpeed = 1.0f;
			if (g_MazeSolveSpeed < 1) g_MazeSolveSpeed = 1.0f;

			SaveSettings();
			PostQuitMessage(0);
		}
		else if (LOWORD(wParam) == IDRESET_BTN)
		{
			char buf[32];
			sprintf_s(buf, "%.3f", 0.04f); SetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf);
			sprintf_s(buf, "%.3f", 0.02f); SetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf);
			sprintf_s(buf, "%.1f", 2.0f); SetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf);
			sprintf_s(buf, "%d", 20); SetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf);
			sprintf_s(buf, "%d", 2); SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf);
			sprintf_s(buf, "%d", 33); SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf);
			sprintf_s(buf, "%.3f", 0.05f); SetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf);
			sprintf_s(buf, "%.1f", 15.0f); SetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf);
			sprintf_s(buf, "%.1f", 10.0f); SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf);
			sprintf_s(buf, "%.1f", 10.0f); SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf);

			SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY), CB_SETCURSEL, 11, 0);
			SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_SETCURSEL, 1, 0);
			SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_SETCHECK, BST_UNCHECKED, 0);
		}
		else if (LOWORD(wParam) == IDCANCEL_BTN)
		{
			PostQuitMessage(0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}