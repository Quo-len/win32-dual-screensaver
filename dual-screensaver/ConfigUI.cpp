#include "ConfigUI.h"
#include "Settings.h"
#include "Defaults.h"
#include "ScreensaverRegistry.h"
#include <stdio.h>
#include <stdlib.h>

extern HINSTANCE hInst;

#define NUM_SCREENSAVERS 28  

#define IDAPPLY_MAIN 2200
#define IDAPPLY_SUB 2300
#define IDRESET_SUB 2301
#define IDC_SUB_EDIT_BASE 3100

bool HasSettings(int id) {
	const auto* def = ScreensaverRegistry::GetById(id);
	return def && !def->settings.empty();
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

		const auto* def = ScreensaverRegistry::GetById(ss_id);
		if (def) {
			int editId = IDC_SUB_EDIT_BASE;
			char buf[64];
			for (const auto& item : def->settings) {
				if (item.type == SettingType::Int) {
					sprintf_s(buf, "%d", *(int*)item.valPtr);
				} else if (item.type == SettingType::Float) {
					sprintf_s(buf, "%.*f", item.precision, *(float*)item.valPtr);
				} else if (item.type == SettingType::Bool) {
					sprintf_s(buf, "%d", *(bool*)item.valPtr ? 1 : 0);
				}
				createRow(item.label, editId++, buf);
			}
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
			const auto* def = ScreensaverRegistry::GetById(ss_id);
			if (def) {
				int editId = IDC_SUB_EDIT_BASE;
				char buf[64];
				for (const auto& item : def->settings) {
					if (GetDlgItemTextA(hWnd, editId++, buf, sizeof(buf))) {
						double v = atof(buf);
						if (v < item.minVal) v = item.minVal;
						if (v > item.maxVal) v = item.maxVal;
						if (item.type == SettingType::Int) *(int*)item.valPtr = (int)v;
						else if (item.type == SettingType::Float) *(float*)item.valPtr = (float)v;
						else if (item.type == SettingType::Bool) *(bool*)item.valPtr = (v != 0.0);
					}
				}
				SaveSettings();
			}
		}
		else if (LOWORD(wParam) == IDRESET_SUB)
		{
			int ss_id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			const auto* def = ScreensaverRegistry::GetById(ss_id);
			if (def) {
				int editId = IDC_SUB_EDIT_BASE;
				char buf[64];
				for (const auto& item : def->settings) {
					if (item.type == SettingType::Int) {
						*(int*)item.valPtr = (int)item.defVal;
						sprintf_s(buf, "%d", (int)item.defVal);
					} else if (item.type == SettingType::Float) {
						*(float*)item.valPtr = (float)item.defVal;
						sprintf_s(buf, "%.*f", item.precision, (float)item.defVal);
					} else if (item.type == SettingType::Bool) {
						*(bool*)item.valPtr = (item.defVal != 0.0);
						sprintf_s(buf, "%d", (item.defVal != 0.0) ? 1 : 0);
					}
					SetDlgItemTextA(hWnd, editId++, buf);
				}
				SaveSettings();
			}
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

			if (g_RandomPool & (1ULL << i))
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
			uint64_t newMask = 0;
			for (int i = 0; i < NUM_SCREENSAVERS; i++)
			{
				if (IsDlgButtonChecked(hWnd, IDC_POOL_CHECK_BASE + i) == BST_CHECKED)
				{
					newMask |= (1ULL << i);
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
