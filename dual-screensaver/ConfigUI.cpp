#include "ConfigUI.h"
#include "Settings.h"
#include "ScreensaverRegistry.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

extern HINSTANCE hInst;

#define IDAPPLY_MAIN 2200
#define IDAPPLY_SUB 2300
#define IDRESET_SUB 2301
#define IDC_SUB_EDIT_BASE 3100
#define IDC_SUB_HELP_BASE 3200
#define IDC_SUB_LBL_BASE 3300

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
		INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES | ICC_BAR_CLASSES };
		InitCommonControlsEx(&icex);

		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		int ss_id = (int)(INT_PTR)pCreate->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ss_id);

		auto* pTipStrings = new std::vector<std::wstring>();
		pTipStrings->reserve(256);
		SetPropW(hWnd, L"TooltipStrings", (HANDLE)pTipStrings);

		HWND hTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			hWnd, NULL, hInst, NULL);
		SetWindowPos(hTooltip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		SendMessageW(hTooltip, TTM_ACTIVATE, TRUE, 0);
		SendMessageW(hTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 100);
		SendMessageW(hTooltip, TTM_SETDELAYTIME, TTDT_RESHOW, 100);
		SendMessageW(hTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000);
		SendMessageW(hTooltip, TTM_SETMAXTIPWIDTH, 0, 280);

		auto addTooltip = [&](HWND hTarget, const WCHAR* text) {
			if (!hTarget || !hTooltip || !text || !text[0]) return;
			pTipStrings->push_back(text);
			TTTOOLINFOW ti = { 0 };
			ti.cbSize = sizeof(TTTOOLINFOW);
			ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
			ti.hwnd = hWnd;
			ti.uId = (UINT_PTR)hTarget;
			ti.lpszText = (LPWSTR)pTipStrings->back().c_str();
			if (!SendMessageW(hTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti)) {
				ti.cbSize = TTTOOLINFOW_V1_SIZE;
				SendMessageW(hTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
			}
		};

		HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		SetPropW(hWnd, L"SubFont", (HANDLE)hFont);

		HFONT hHelpFont = CreateFontW(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
		SetPropW(hWnd, L"SubHelpFont", (HANDLE)hHelpFont);

		int y = 20;
		const int col1X = 20;
		const int lblW = 160;
		const int edtW = 75;
		const int hintX = col1X + lblW + edtW + 10;
		const int hintW = 115;
		const int qX = hintX + hintW + 10;
		const int qW = 24;
		const int qH = 24;
		const int rowH = 36;

		auto createRow = [&](const WCHAR* label, int lblId, int editId, const char* initialVal, const WCHAR* rangeText, const WCHAR* fullTipText, bool hasDesc, int itemIdx) {
			HWND hLbl = CreateWindowW(L"STATIC", label, WS_CHILD | WS_VISIBLE | SS_NOTIFY, col1X, y, lblW, 25, hWnd, (HMENU)(INT_PTR)lblId, hInst, NULL);
			HWND hEdt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, col1X + lblW, y, edtW, 28, hWnd, (HMENU)(INT_PTR)editId, hInst, NULL);
			HWND hHint = CreateWindowW(L"STATIC", rangeText ? rangeText : L"", WS_CHILD | WS_VISIBLE | SS_NOTIFY, hintX, y + 3, hintW, 22, hWnd, NULL, hInst, NULL);
			SendMessage(hLbl, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SendMessage(hEdt, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SendMessage(hHint, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SetWindowTextA(hEdt, initialVal);
			if (fullTipText && fullTipText[0]) {
				addTooltip(hLbl, fullTipText);
				addTooltip(hEdt, fullTipText);
				addTooltip(hHint, fullTipText);
			}
			if (hasDesc) {
				HWND hQ = CreateWindowW(L"BUTTON", L"?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
					qX, y + 2, qW, qH, hWnd, (HMENU)(INT_PTR)(IDC_SUB_HELP_BASE + itemIdx), hInst, NULL);
				SendMessage(hQ, WM_SETFONT, (WPARAM)hHelpFont, MAKELPARAM(TRUE, 0));
				if (fullTipText && fullTipText[0]) {
					addTooltip(hQ, fullTipText);
				}
			}
			y += rowH;
		};

		const auto* def = ScreensaverRegistry::GetById(ss_id);
		if (def) {
			int editId = IDC_SUB_EDIT_BASE;
			int itemIdx = 0;
			char buf[64];
			for (const auto& item : def->settings) {
				WCHAR rangeStr[64] = L"";

				std::wstring desc = (item.description && item.description[0]) ? item.description : L"";
				std::wstring fullTip = desc;
				WCHAR limitsBuf[160];
				if (item.type == SettingType::Int) {
					if (!fullTip.empty()) {
						swprintf_s(limitsBuf, L"\n\nDefault: %d   Allowed: [%d .. %d]", (int)item.defVal, (int)item.minVal, (int)item.maxVal);
					} else {
						swprintf_s(limitsBuf, L"Allowed Min: %d\nAllowed Max: %d\nDefault: %d", (int)item.minVal, (int)item.maxVal, (int)item.defVal);
					}
					swprintf_s(rangeStr, L"[%d .. %d]", (int)item.minVal, (int)item.maxVal);
				} else if (item.type == SettingType::Float) {
					if (!fullTip.empty()) {
						swprintf_s(limitsBuf, L"\n\nDefault: %.*f   Allowed: [%.*f .. %.*f]", item.precision, (float)item.defVal, item.precision, (float)item.minVal, item.precision, (float)item.maxVal);
					} else {
						swprintf_s(limitsBuf, L"Allowed Min: %.*f\nAllowed Max: %.*f\nDefault: %.*f",
							item.precision, (float)item.minVal,
							item.precision, (float)item.maxVal,
							item.precision, (float)item.defVal);
					}
					swprintf_s(rangeStr, L"[%.*f .. %.*f]", item.precision, (float)item.minVal, item.precision, (float)item.maxVal);
				} else if (item.type == SettingType::Bool) {
					if (!fullTip.empty()) {
						swprintf_s(limitsBuf, L"\n\nDefault: %s", (item.defVal != 0.0) ? L"Checked" : L"Unchecked");
					} else {
						swprintf_s(limitsBuf, L"Options: Checked (1) / Unchecked (0)\nDefault: %s",
							(item.defVal != 0.0) ? L"Checked" : L"Unchecked");
					}
				}
				fullTip += limitsBuf;

				bool hasDesc = !desc.empty();

				if (item.type == SettingType::Bool) {
					int chkW = qX - col1X - 8;
					HWND hChk = CreateWindowW(L"BUTTON", item.label, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
						col1X, y, chkW, 25, hWnd, (HMENU)(INT_PTR)editId++, hInst, NULL);
					SendMessage(hChk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
					SendMessage(hChk, BM_SETCHECK, *(bool*)item.valPtr ? BST_CHECKED : BST_UNCHECKED, 0);
					if (!fullTip.empty()) {
						addTooltip(hChk, fullTip.c_str());
					}
					if (hasDesc) {
						HWND hQ = CreateWindowW(L"BUTTON", L"?", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
							qX, y, qW, qH, hWnd, (HMENU)(INT_PTR)(IDC_SUB_HELP_BASE + itemIdx), hInst, NULL);
						SendMessage(hQ, WM_SETFONT, (WPARAM)hHelpFont, MAKELPARAM(TRUE, 0));
						if (!fullTip.empty()) {
							addTooltip(hQ, fullTip.c_str());
						}
					}
					y += 30;
				} else {
					if (item.type == SettingType::Int) {
						sprintf_s(buf, "%d", *(int*)item.valPtr);
					} else if (item.type == SettingType::Float) {
						sprintf_s(buf, "%.*f", item.precision, *(float*)item.valPtr);
					}
					createRow(item.label, IDC_SUB_LBL_BASE + itemIdx, editId++, buf, rangeStr, fullTip.c_str(), hasDesc, itemIdx);
				}
				itemIdx++;
			}
		}

		y += 15;
		int totalW = qX + qW + 20;
		int btnW = 120;
		int gap = 20;
		int btnStartX = (totalW - (btnW * 2 + gap)) / 2;

		HWND hReset = CreateWindowW(L"BUTTON", L"Defaults", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			btnStartX, y, btnW, 35, hWnd, (HMENU)IDRESET_SUB, hInst, NULL);
		HWND hApply = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
			btnStartX + btnW + gap, y, btnW, 35, hWnd, (HMENU)IDAPPLY_SUB, hInst, NULL);
		SendMessage(hReset, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		SendMessage(hApply, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

		RECT rc = { 0, 0, totalW, y + 55 };
		AdjustWindowRectEx(&rc, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
		SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

		break;
	}
	case WM_SETCURSOR:
	{
		HWND hCtrl = (HWND)wParam;
		int id = GetDlgCtrlID(hCtrl);
		if ((id >= IDC_SUB_HELP_BASE && id < IDC_SUB_HELP_BASE + 200) ||
			(id >= IDC_SUB_LBL_BASE && id < IDC_SUB_LBL_BASE + 200)) {
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return TRUE;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_COMMAND:
	{
		WORD cmdId = LOWORD(wParam);
		if (cmdId == IDAPPLY_SUB)
		{
			int ss_id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			const auto* def = ScreensaverRegistry::GetById(ss_id);
			if (def) {
				int editId = IDC_SUB_EDIT_BASE;
				char buf[64];
				for (const auto& item : def->settings) {
					if (item.type == SettingType::Bool) {
						*(bool*)item.valPtr = (SendMessage(GetDlgItem(hWnd, editId++), BM_GETCHECK, 0, 0) == BST_CHECKED);
					} else if (GetDlgItemTextA(hWnd, editId++, buf, sizeof(buf))) {
						double v = atof(buf);
						if (v < item.minVal) v = item.minVal;
						if (v > item.maxVal) v = item.maxVal;
						if (item.type == SettingType::Int) *(int*)item.valPtr = (int)v;
						else if (item.type == SettingType::Float) *(float*)item.valPtr = (float)v;
					}
				}
				SaveSettings();
			}
		}
		else if (cmdId == IDRESET_SUB)
		{
			int ss_id = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			const auto* def = ScreensaverRegistry::GetById(ss_id);
			if (def) {
				int editId = IDC_SUB_EDIT_BASE;
				char buf[64];
				for (const auto& item : def->settings) {
					if (item.type == SettingType::Bool) {
						*(bool*)item.valPtr = (item.defVal != 0.0);
						SendMessage(GetDlgItem(hWnd, editId++), BM_SETCHECK, (item.defVal != 0.0) ? BST_CHECKED : BST_UNCHECKED, 0);
					} else if (item.type == SettingType::Int) {
						*(int*)item.valPtr = (int)item.defVal;
						sprintf_s(buf, "%d", (int)item.defVal);
						SetDlgItemTextA(hWnd, editId++, buf);
					} else if (item.type == SettingType::Float) {
						*(float*)item.valPtr = (float)item.defVal;
						sprintf_s(buf, "%.*f", item.precision, (float)item.defVal);
						SetDlgItemTextA(hWnd, editId++, buf);
					}
				}
				SaveSettings();
			}
		}
		break;
	}
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
	{
		auto* pTipStrings = (std::vector<std::wstring>*)GetPropW(hWnd, L"TooltipStrings");
		if (pTipStrings) {
			delete pTipStrings;
			RemovePropW(hWnd, L"TooltipStrings");
		}
		HFONT hFont = (HFONT)GetPropW(hWnd, L"SubFont");
		if (hFont) {
			DeleteObject(hFont);
			RemovePropW(hWnd, L"SubFont");
		}
		HFONT hHelpFont = (HFONT)GetPropW(hWnd, L"SubHelpFont");
		if (hHelpFont) {
			DeleteObject(hHelpFont);
			RemovePropW(hWnd, L"SubHelpFont");
		}
		break;
	}
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
		int numScreensavers = ScreensaverRegistry::GetCount();
		int half = (numScreensavers + 1) / 2;

		for (int i = 0; i < numScreensavers; i++)
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

		for (int i = 0; i < numScreensavers; i++) {
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
		int numScreensavers = ScreensaverRegistry::GetCount();
		if (id >= IDC_SETTINGS_BASE && id < IDC_SETTINGS_BASE + numScreensavers)
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
			for (int i = 0; i < numScreensavers; i++)
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
