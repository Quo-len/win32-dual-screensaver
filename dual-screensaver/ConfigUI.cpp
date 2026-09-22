#include "ConfigUI.h"
#include "Settings.h"
#include "Defaults.h"
#include <stdio.h>
#include <stdlib.h>

extern HINSTANCE hInst;

#define NUM_SCREENSAVERS 27  

#define IDAPPLY_MAIN 2200
#define IDAPPLY_SUB 2300
#define IDRESET_SUB 2301

bool HasSettings(int id) {
	switch (id) {
	case 0: return true; // Donut
	case 1: return true; // GoL
	case 3: return true; // Earth
	case 7: return true; // DVD
	case 9: return true; // Pong
	case 10: return true; // Maze
	case 11: return true; // Clock
	case 12: return true; // Perlin
	case 16: return true; // Langton's Ant
	case 21: return true; // Curl Noise Particles
	default: return false;
	}
}

LRESULT CALLBACK SubSettingsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		int ss_id = (int)(INT_PTR)pCreate->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ss_id);

		HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		int y = 20;
		const int col1X = 20;
		const int lblW = 120;
		const int edtW = 140;
		const int rowH = 40;

		auto createRow = [&](const WCHAR* label, int editId, const char* initialVal) {
			HWND hLbl = CreateWindowW(L"STATIC", label, WS_CHILD | WS_VISIBLE, col1X, y, lblW, 25, hWnd, NULL, hInst, NULL);
			HWND hEdt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + lblW, y, edtW, 30, hWnd, (HMENU)(INT_PTR)editId, hInst, NULL);
			SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SendMessage(hEdt, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SetWindowTextA(hEdt, initialVal);
			y += rowH;
		};

		char buf[32];
		switch (ss_id) {
		case 0:
			sprintf_s(buf, "%.3f", g_ASpeed); createRow(L"A Speed:", IDC_EDIT_ASPEED, buf);
			sprintf_s(buf, "%.3f", g_BSpeed); createRow(L"B Speed:", IDC_EDIT_BSPEED, buf);
			sprintf_s(buf, "%.1f", g_DonutSize); createRow(L"Donut Size:", IDC_EDIT_SIZE, buf);
			sprintf_s(buf, "%.1f", g_DonutDistance); createRow(L"Distance:", IDC_EDIT_DONUT_DISTANCE, buf);
			break;
		case 1:
			sprintf_s(buf, "%d", g_GolCellSize); createRow(L"Cell (px):", IDC_EDIT_GOL_SIZE, buf);
			sprintf_s(buf, "%d", g_GolSpeed); createRow(L"Speed (ms):", IDC_EDIT_GOL_SPEED, buf);
			break;
		case 3:
			sprintf_s(buf, "%.3f", g_EarthSpeed); createRow(L"Spin:", IDC_EDIT_EARTH_SPEED, buf);
			break;
		case 7:
			sprintf_s(buf, "%.1f", g_DvdSpeed); createRow(L"Speed:", IDC_EDIT_DVD_SPEED, buf);
			break;
		case 9:
			sprintf_s(buf, "%.1f", g_PongSpeed); createRow(L"Speed:", IDC_EDIT_PONG_SPEED, buf);
			break;
		case 10:
			sprintf_s(buf, "%.1f", g_MazeBuildSpeed); createRow(L"Build Spd:", IDC_EDIT_MAZE_BUILD_SPEED, buf);
			sprintf_s(buf, "%.1f", g_MazeSolveSpeed); createRow(L"Solve Spd:", IDC_EDIT_MAZE_SOLVE_SPEED, buf);
			break;
		case 11:
			sprintf_s(buf, "%d", g_TextSize); createRow(L"Text Size:", IDC_EDIT_TEXTSIZE, buf);
			break;
		case 12:
			sprintf_s(buf, "%.4f", g_PerlinScale); createRow(L"Scale:", IDC_EDIT_PERLIN_SCALE, buf);
			sprintf_s(buf, "%.2f", g_PerlinSpeed); createRow(L"Speed:", IDC_EDIT_PERLIN_SPEED, buf);
			break;
		case 16:
			sprintf_s(buf, "%d", g_AntCount); createRow(L"Sets (Sym):", IDC_EDIT_ANT_COUNT, buf);
			sprintf_s(buf, "%d", g_AntSpeed); createRow(L"Speed:", IDC_EDIT_ANT_SPEED, buf);
			break;
		case 21:
			sprintf_s(buf, "%d", g_CurlCount); createRow(L"Particles:", IDC_EDIT_CURL_COUNT, buf);
			break;
		}

		y += 10;
		HWND hReset = CreateWindowW(L"BUTTON", L"Defaults", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			20, y, 120, 35, hWnd, (HMENU)IDRESET_SUB, hInst, NULL);
		HWND hApply = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			155, y, 125, 35, hWnd, (HMENU)IDAPPLY_SUB, hInst, NULL);
		SendMessage(hReset, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hApply, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		RECT rc = { 0, 0, col1X + lblW + edtW + 40, y + 60 };
		AdjustWindowRectEx(&rc, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
		SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

		break;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDAPPLY_SUB)
		{
			int ss_id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			char buf[32];
			auto getFloat = [&](int id, float& val) { if (GetDlgItemTextA(hWnd, id, buf, 32)) val = (float)atof(buf); };
			auto getInt = [&](int id, int& val) { if (GetDlgItemTextA(hWnd, id, buf, 32)) val = atoi(buf); };

			switch (ss_id) {
			case 0:
				getFloat(IDC_EDIT_ASPEED, g_ASpeed); getFloat(IDC_EDIT_BSPEED, g_BSpeed);
				getFloat(IDC_EDIT_SIZE, g_DonutSize); getFloat(IDC_EDIT_DONUT_DISTANCE, g_DonutDistance);
				break;
			case 1:
				getInt(IDC_EDIT_GOL_SIZE, g_GolCellSize); getInt(IDC_EDIT_GOL_SPEED, g_GolSpeed);
				if (g_GolCellSize < 1) g_GolCellSize = 1;
				if (g_GolSpeed < 10) g_GolSpeed = 10;
				break;
			case 3: getFloat(IDC_EDIT_EARTH_SPEED, g_EarthSpeed); break;
			case 7: getFloat(IDC_EDIT_DVD_SPEED, g_DvdSpeed); break;
			case 9: getFloat(IDC_EDIT_PONG_SPEED, g_PongSpeed); break;
			case 10:
				getFloat(IDC_EDIT_MAZE_BUILD_SPEED, g_MazeBuildSpeed); getFloat(IDC_EDIT_MAZE_SOLVE_SPEED, g_MazeSolveSpeed);
				if (g_MazeBuildSpeed < 1.0f) g_MazeBuildSpeed = 1.0f;
				if (g_MazeSolveSpeed < 1.0f) g_MazeSolveSpeed = 1.0f;
				break;
			case 11: getInt(IDC_EDIT_TEXTSIZE, g_TextSize); break;
			case 12:
				getFloat(IDC_EDIT_PERLIN_SCALE, g_PerlinScale); getFloat(IDC_EDIT_PERLIN_SPEED, g_PerlinSpeed);
				if (g_PerlinSpeed < 0.0f) g_PerlinSpeed = 0.1f;
				break;
			case 16:
				getInt(IDC_EDIT_ANT_COUNT, g_AntCount); getInt(IDC_EDIT_ANT_SPEED, g_AntSpeed);
				if (g_AntCount < 1) g_AntCount = 1;
				if (g_AntSpeed < 1) g_AntSpeed = 1;
				break;
			case 21:
				getInt(IDC_EDIT_CURL_COUNT, g_CurlCount);
				if (g_CurlCount < 100) g_CurlCount = 100;
				if (g_CurlCount > 500000) g_CurlCount = 500000;
				break;
			}
			SaveSettings();
		}
		else if (LOWORD(wParam) == IDRESET_SUB)
		{
			int ss_id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			char buf[32];
			switch (ss_id) {
			case 0:
				g_ASpeed = DEFAULT_ASPEED; sprintf_s(buf, "%.3f", g_ASpeed); SetDlgItemTextA(hWnd, IDC_EDIT_ASPEED, buf);
				g_BSpeed = DEFAULT_BSPEED; sprintf_s(buf, "%.3f", g_BSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_BSPEED, buf);
				g_DonutSize = DEFAULT_DONUTSIZE; sprintf_s(buf, "%.1f", g_DonutSize); SetDlgItemTextA(hWnd, IDC_EDIT_SIZE, buf);
				g_DonutDistance = DEFAULT_DONUTDISTANCE; sprintf_s(buf, "%.1f", g_DonutDistance); SetDlgItemTextA(hWnd, IDC_EDIT_DONUT_DISTANCE, buf);
				break;
			case 1:
				g_GolCellSize = DEFAULT_GOLCELLSIZE; sprintf_s(buf, "%d", g_GolCellSize); SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SIZE, buf);
				g_GolSpeed = DEFAULT_GOLSPEED; sprintf_s(buf, "%d", g_GolSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_GOL_SPEED, buf);
				break;
			case 3:
				g_EarthSpeed = DEFAULT_EARTHSPEED; sprintf_s(buf, "%.3f", g_EarthSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_EARTH_SPEED, buf);
				break;
			case 7:
				g_DvdSpeed = DEFAULT_DVDSPEED; sprintf_s(buf, "%.1f", g_DvdSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_DVD_SPEED, buf);
				break;
			case 9:
				g_PongSpeed = DEFAULT_PONGSPEED; sprintf_s(buf, "%.1f", g_PongSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_PONG_SPEED, buf);
				break;
			case 10:
				g_MazeBuildSpeed = DEFAULT_MAZEBUILDSPEED; sprintf_s(buf, "%.1f", g_MazeBuildSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_BUILD_SPEED, buf);
				g_MazeSolveSpeed = DEFAULT_MAZESOLVESPEED; sprintf_s(buf, "%.1f", g_MazeSolveSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_MAZE_SOLVE_SPEED, buf);
				break;
			case 11:
				g_TextSize = DEFAULT_TEXTSIZE; sprintf_s(buf, "%d", g_TextSize); SetDlgItemTextA(hWnd, IDC_EDIT_TEXTSIZE, buf);
				break;
			case 12:
				g_PerlinScale = DEFAULT_PERLINSCALE; sprintf_s(buf, "%.4f", g_PerlinScale); SetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SCALE, buf);
				g_PerlinSpeed = DEFAULT_PERLINSPEED; sprintf_s(buf, "%.2f", g_PerlinSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_PERLIN_SPEED, buf);
				break;
			case 16:
				g_AntCount = DEFAULT_ANT_COUNT; sprintf_s(buf, "%d", g_AntCount); SetDlgItemTextA(hWnd, IDC_EDIT_ANT_COUNT, buf);
				g_AntSpeed = DEFAULT_ANT_SPEED; sprintf_s(buf, "%d", g_AntSpeed); SetDlgItemTextA(hWnd, IDC_EDIT_ANT_SPEED, buf);
				break;
			case 21:
				g_CurlCount = DEFAULT_CURL_COUNT; sprintf_s(buf, "%d", g_CurlCount); SetDlgItemTextA(hWnd, IDC_EDIT_CURL_COUNT, buf);
				break;
			}
			SaveSettings();
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK ConfigWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		HFONT hBold = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

		HWND hTitle = CreateWindowW(L"STATIC", L"Screensavers in Random Pool:",
			WS_CHILD | WS_VISIBLE, 20, 15, 380, 25, hWnd, NULL, hInst, NULL);
		SendMessage(hTitle, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0));

		int startY = 50;
		int rowH = 35;
		int col1X = 25;
		int col2X = 350;
		int chkW = 200;
		int btnW = 90;
		int half = (NUM_SCREENSAVERS + 1) / 2;

		for (int i = 0; i < NUM_SCREENSAVERS; i++)
		{
			int x = (i < half) ? col1X : col2X;
			int y = startY + ((i < half) ? i : (i - half)) * rowH;

			HWND hChk = CreateWindowW(L"BUTTON", g_modeNames[i],
				WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
				x, y, chkW, 26, hWnd, (HMENU)(INT_PTR)(IDC_POOL_CHECK_BASE + i), hInst, NULL);
			SendMessage(hChk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

			if (g_RandomPool & (1u << i))
				SendMessage(hChk, BM_SETCHECK, BST_CHECKED, 0);

			if (HasSettings(i)) {
				HWND hSetBtn = CreateWindowW(L"BUTTON", L"Settings...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					x + chkW, y, btnW, 26, hWnd, (HMENU)(INT_PTR)(IDC_SETTINGS_BASE + i), hInst, NULL);
				SendMessage(hSetBtn, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			}
		}

		int optionsY = startY + half * rowH + 20;

		HWND hLblRand = CreateWindowW(L"STATIC", L"Mode Options:", WS_CHILD | WS_VISIBLE, 20, optionsY, 200, 25, hWnd, NULL, hInst, NULL);
		SendMessage(hLblRand, WM_SETFONT, (WPARAM)hBold, MAKELPARAM(TRUE, 0));
		optionsY += 30;

		HWND hRand = CreateWindowW(L"BUTTON", L"Randomize every launch", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
			25, optionsY, 250, 30, hWnd, (HMENU)IDC_CHECK_RANDOM, hInst, NULL);
		SendMessage(hRand, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hRand, BM_SETCHECK, g_RandomMode ? BST_CHECKED : BST_UNCHECKED, 0);
		optionsY += 40;

		HWND hLblPrim = CreateWindowW(L"STATIC", L"Primary:", WS_CHILD | WS_VISIBLE, 25, optionsY, 80, 25, hWnd, NULL, hInst, NULL);
		HWND hC1 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 110, optionsY, 200, 300, hWnd, (HMENU)IDC_COMBO_PRIMARY, hInst, NULL);
		SendMessage(hLblPrim, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hC1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		
		HWND hLblSec = CreateWindowW(L"STATIC", L"Secondary:", WS_CHILD | WS_VISIBLE, 350, optionsY, 100, 25, hWnd, NULL, hInst, NULL);
		HWND hC2 = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 450, optionsY, 200, 300, hWnd, (HMENU)IDC_COMBO_SECONDARY, hInst, NULL);
		SendMessage(hLblSec, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hC2, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		for (int i = 0; i < NUM_SCREENSAVERS; i++) {
			SendMessage(hC1, CB_ADDSTRING, 0, (LPARAM)g_modeNames[i]);
			SendMessage(hC2, CB_ADDSTRING, 0, (LPARAM)g_modeNames[i]);
		}
		SendMessage(hC1, CB_SETCURSEL, g_ModePrimary, 0);
		SendMessage(hC2, CB_SETCURSEL, g_ModeSecondary, 0);

		optionsY += 50;

		const int clientW = 700;
		const int btnApplyW = 160;
		const int btnApplyH = 40;
		int btnApplyX = (clientW - btnApplyW) / 2;

		HWND hApply = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			btnApplyX, optionsY, btnApplyW, btnApplyH, hWnd, (HMENU)IDAPPLY_MAIN, hInst, NULL);
		SendMessage(hApply, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		int clientH = optionsY + btnApplyH + 25;
		RECT rc = { 0, 0, clientW, clientH };
		AdjustWindowRectEx(&rc, WS_VISIBLE | WS_SYSMENU | WS_CAPTION, FALSE, WS_EX_DLGMODALFRAME);
		int winW = rc.right - rc.left;
		int winH = rc.bottom - rc.top;
		int screenW = GetSystemMetrics(SM_CXSCREEN);
		int screenH = GetSystemMetrics(SM_CYSCREEN);
		int posX = (screenW - winW) / 2;
		int posY = (screenH - winH) / 2;
		SetWindowPos(hWnd, NULL, (posX > 0 ? posX : 0), (posY > 0 ? posY : 0), winW, winH, SWP_NOZORDER);

		break;
	}
	case WM_COMMAND:
	{
		WORD id = LOWORD(wParam);
		if (id >= IDC_SETTINGS_BASE && id < IDC_SETTINGS_BASE + NUM_SCREENSAVERS)
		{
			int ss_id = id - IDC_SETTINGS_BASE;
			
			static bool s_subClassRegistered = false;
			if (!s_subClassRegistered) {
				WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
				wcex.style = CS_HREDRAW | CS_VREDRAW;
				wcex.lpfnWndProc = SubSettingsProc;
				wcex.hInstance = hInst;
				wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
				wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
				wcex.lpszClassName = L"SaverSubSettingsClass";
				RegisterClassExW(&wcex);
				s_subClassRegistered = true;
			}

			WCHAR title[128];
			swprintf(title, 128, L"%s Settings", g_modeNames[ss_id]);
			
			HWND hSub = CreateWindowExW(WS_EX_TOOLWINDOW, L"SaverSubSettingsClass", title,
				WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
				CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
				hWnd, nullptr, hInst, (LPVOID)(INT_PTR)ss_id);
		}
		else if (id == IDAPPLY_MAIN)
		{
			unsigned int newMask = 0;
			for (int i = 0; i < NUM_SCREENSAVERS; i++)
			{
				if (IsDlgButtonChecked(hWnd, IDC_POOL_CHECK_BASE + i) == BST_CHECKED)
				{
					newMask |= (1u << i);
				}
			}
			g_RandomPool = newMask;
			g_ModePrimary = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_PRIMARY), CB_GETCURSEL, 0, 0);
			g_ModeSecondary = (int)SendMessage(GetDlgItem(hWnd, IDC_COMBO_SECONDARY), CB_GETCURSEL, 0, 0);
			g_RandomMode = SendMessage(GetDlgItem(hWnd, IDC_CHECK_RANDOM), BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0;
			
			SaveSettings();
		}
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void ShowSettingsWindow(HINSTANCE hInstance)
{
	static bool s_classRegistered = false;
	if (!s_classRegistered)
	{
		WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = ConfigWindowProc;
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wcex.lpszClassName = L"SaverSettingsClass";
		RegisterClassExW(&wcex);
		s_classRegistered = true;
	}

	HWND hWnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"SaverSettingsClass", L"DualSaver Configuration",
		WS_VISIBLE | WS_SYSMENU | WS_CAPTION,
		CW_USEDEFAULT, CW_USEDEFAULT, 720, 680,
		nullptr, nullptr, hInstance, nullptr);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
