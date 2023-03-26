// wCenterWindow
// wCenterWindow.cpp
//
#include "framework.h"
#include "wCenterWindow.h"

#define KEY_I 0x49
#define KEY_C 0x43
#define KEY_V 0x56

#define BUF_LEN 1024
#define MAX_LOADSTRING 50
#define WM_WCW	0x8F00

// Global variables:
HINSTANCE			hInst;									// Instance
WCHAR				szTitle[MAX_LOADSTRING];				// Window's title
WCHAR				szClass[MAX_LOADSTRING];				// Window's class
WCHAR				szWinTitle[256];
WCHAR				szWinClass[256];
WCHAR				szWinCore[] = L"Windows.UI.Core.CoreWindow";
WCHAR				szWorkerW[] = L"WorkerW";
HANDLE				hHeap = NULL;
HHOOK				hMouseHook = NULL, hKbdHook = NULL;		// Hook's handles
HICON				hIcon = NULL;
HMENU				hMenu = NULL, hPopup = NULL;
HWND				hWnd = NULL, hFgWnd = NULL, hTaskBar = NULL, hDesktop = NULL, hProgman = NULL;
bool				bKPressed = FALSE, bMPressed = FALSE, bShowIcon = TRUE, bWorkArea = TRUE;
bool				bLCTRL = FALSE, bLWIN = FALSE, bKEYV = FALSE;

RECT				rcFW = { 0 };
NOTIFYICONDATAW		nid = { 0 };
LPKBDLLHOOKSTRUCT	pkhs = { 0 };
MENUITEMINFO		mii = { 0 };

LPVOID				szBuffer;

// {2D7B7F30-4B5F-4380-9807-57D7A2E37F6C}
static const GUID	guid = { 0x2d7b7f30, 0x4b5f, 0x4380, { 0x98, 0x7, 0x57, 0xd7, 0xa2, 0xe3, 0x7f, 0x6c } };

// Forward declarations of functions included in this code module:
VOID				HandlingTrayIcon();
VOID				ShowError(UINT);
bool				IsWindowApprooved(HWND);
BOOL	CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);
LRESULT CALLBACK	MouseHookProc(int, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
std::string			ConvertWideToUtf8(const std::wstring&);



VOID MoveWindowToMonitorCenter(HWND hwnd, BOOL bWorkArea, BOOL bResize)
{
	diag_log("Entering MoveWindowToMonitorCenter(), handle = 0x", hwnd);

	RECT fgwrc = { 0 };
	GetWindowRect(hwnd, &fgwrc);
	LONG nWidth = fgwrc.right - fgwrc.left;
	LONG nHeight = fgwrc.bottom - fgwrc.top;

	diag_log("Moving window from x = ", fgwrc.left, ", y = ", fgwrc.top);

	MONITORINFO mi = { 0 };
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
	RECT area = { 0 };
	if (bWorkArea)
	{
		area.bottom = mi.rcWork.bottom;
		area.left = mi.rcWork.left;
		area.right = mi.rcWork.right;
		area.top = mi.rcWork.top;
	}
	else
	{
		area.bottom = mi.rcMonitor.bottom;
		area.left = mi.rcMonitor.left;
		area.right = mi.rcMonitor.right;
		area.top = mi.rcMonitor.top;
	}

	int aw = area.right - area.left;
	int ah = area.bottom - area.top;
	if (nWidth > aw && bResize) nWidth = aw;
	if (nHeight > ah && bResize) nHeight = ah;
	if (area.left < 0) aw = -aw;
	if (area.top < 0) ah = -ah;
	int x = (aw - nWidth) / 2;
	int y = (ah - nHeight) / 2;

	SendMessageW(hwnd, WM_ENTERSIZEMOVE, NULL, NULL);
	MoveWindow(hwnd, x, y, nWidth, nHeight, TRUE);
	SendMessageW(hwnd, WM_EXITSIZEMOVE, NULL, NULL);

	diag_log("Moving window to x = ", x, ", y = ", y);
	diag_log("Quiting MoveWindowToMonitorCenter()");
}



int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	hInst = hInstance;

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
	LoadStringW(hInstance, IDS_CLASSNAME, szClass, _countof(szClass));

	if (FindWindowW(szClass, NULL))
	{
		ShowError(IDS_RUNNING);
		return FALSE;
	}

	OpenLogFile();
	diag_log("Entering WinMain()");

	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.lpszClassName = szClass;
	wcex.hIconSm = wcex.hIcon;
	hIcon = wcex.hIcon;
	if (!RegisterClassExW(&wcex))
	{
		ShowError(IDS_ERR_CLASS);
		return FALSE;
	}

	hWnd = CreateWindowExW(0, szClass, szTitle, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		ShowError(IDS_ERR_WND);
		return FALSE;
	}

	int nArgs = 0;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	std::string arg;

	diag_log("Arguments: ", nArgs - 1);
	for (int i = 1; i < nArgs; i++)
	{
		arg = ConvertWideToUtf8(szArglist[i]);
		diag_log("Argument #", i, ": ", arg);
	}

	(nArgs >= 2 && 0 == lstrcmpiW(szArglist[1], L"/hide")) ? bShowIcon = FALSE : bShowIcon = TRUE;
	LocalFree(szArglist);
	HandlingTrayIcon();

	hHeap = GetProcessHeap();
	szBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUF_LEN);

	hTaskBar = FindWindowW(L"Shell_TrayWnd", NULL);
	hProgman = FindWindowW(L"Progman", NULL);
	hDesktop = GetDesktopWindow();

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			ShowError(IDS_ERR_MAIN);
			return -1;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	if (hKbdHook) UnhookWindowsHookEx(hKbdHook);
	if (hMenu) DestroyMenu(hMenu);
	Shell_NotifyIconW(NIM_DELETE, &nid);

	diag_log("Quiting WinMain(), msg.wParam = ", (int)msg.wParam);
	CloseLogFile();
	HeapFree(hHeap, NULL, szBuffer);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		diag_log("Recived WM_CREATE message");
		hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDR_MENU));
		if (!hMenu)
		{
			diag_log("Loading context menu failed!");
			ShowError(IDS_ERR_MENU);
			PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log("Context menu successfully loaded");

		hPopup = GetSubMenu(hMenu, 0);
		if (!hPopup)
		{
			diag_log("Creating popup menu failed!");
			ShowError(IDS_ERR_POPUP);
			PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log("Popup menu successfully created");

		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
		SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);

		nid.cbSize = sizeof(NOTIFYICONDATAW);
		nid.hWnd = hWnd;
		nid.uVersion = NOTIFYICON_VERSION;
		nid.uCallbackMessage = WM_WCW;
		nid.hIcon = hIcon;
		nid.uID = IDI_TRAYICON;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.dwInfoFlags = NIIF_NONE;
		nid.dwState = NIS_HIDDEN;
		nid.dwStateMask = NIS_HIDDEN;
		StringCchCopyW(nid.szTip, _countof(nid.szTip), szTitle);

#ifndef _DEBUG
		hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, hInst, NULL);
		if (!hMouseHook)
		{
			diag_log("Creating mouse hook failed!");
			ShowError(IDS_ERR_HOOK);
			PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log("Mouse hook was successfully set");
#endif // !_DEBUG

		hKbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
		if (!hKbdHook)
		{
			diag_log("Creating keyboard hook failed!");
			ShowError(IDS_ERR_HOOK);
			PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log("Keyboard hook was successfully set");
		break;
	}

	case WM_WCW:
	{
		if (IDI_TRAYICON == wParam && (WM_RBUTTONDOWN == lParam || WM_LBUTTONDOWN == lParam))
		{
			diag_log("Entering menu handler");
			SetForegroundWindow(hWnd);
			POINT pt;
			GetCursorPos(&pt);
			int idMenu = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			if (ID_POPUPMENU_ICON == idMenu)
			{
				diag_log("Pressed 'Hide icon' menuitem");
				bShowIcon = FALSE;
				HandlingTrayIcon();
			}
			if (ID_POPUPMENU_AREA == idMenu)
			{
				diag_log("Pressed 'Use workarea' menuitem");
				bWorkArea = !bWorkArea;
				bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
				SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);
				diag_log("Changed 'Use workarea' option to ", bWorkArea);
			}
			if (ID_POPUPMENU_ABOUT == idMenu && !bKPressed)
			{
				diag_log("Pressed 'About' menuitem");
				bKPressed = TRUE;
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
				bKPressed = FALSE;
			}
			if (ID_POPUPMENU_EXIT == idMenu)
			{
				diag_log("Pressed 'Exit' menuitem");
				PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
			}
			diag_log("Quiting menu handler");
		}
		break;
	}

	case WM_QUERYENDSESSION:
	{
		diag_log("Recieved WM_QUERYENDSESSION message, lParam = ", lParam);
		CloseLogFile();
		return TRUE;
		break;
	}

	case WM_DESTROY:
	{
		diag_log("Recived WM_DESTROY message");
		PostQuitMessage(0);
		break;
	}

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (WM_MBUTTONUP == wParam) bMPressed = FALSE;
	if (WM_MBUTTONDOWN == wParam && bLCTRL && bLWIN && !bMPressed)
	{
		diag_log("Pressed LCTRL + LWIN + MMB");
		bMPressed = TRUE;
		hFgWnd = GetForegroundWindow();
		if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
		else hFgWnd = NULL;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	pkhs = (KBDLLHOOKSTRUCT*)lParam;

	if (WM_KEYUP == wParam)
	{
		if (VK_LCONTROL == pkhs->vkCode) bLCTRL = FALSE;
		if (VK_LWIN == pkhs->vkCode) bLWIN = FALSE;
		bKPressed = FALSE;
	}

	if (WM_KEYDOWN == wParam)
	{
		if (VK_LCONTROL == pkhs->vkCode) bLCTRL = TRUE;
		if (VK_LWIN == pkhs->vkCode) bLWIN = TRUE;

		if (KEY_I == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed)	// 'I' key
		{
			diag_log("Pressed LCTRL + LWIN + I");
			bKPressed = TRUE;
			bShowIcon = !bShowIcon;
			HandlingTrayIcon();
			return TRUE;
		}

		if (KEY_C == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV)	// 'C' key
		{
			diag_log("Pressed LCTRL + LWIN + C");
			bKPressed = TRUE;
			hFgWnd = GetForegroundWindow();
			if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
			else hFgWnd = NULL;
			return TRUE;
		}

		if (KEY_V == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV)	// 'V' key
		{
			diag_log("Pressed LCTRL + LWIN + V");
			bKPressed = TRUE; bKEYV = TRUE;
			hFgWnd = GetForegroundWindow();
			if (IsWindowApprooved(hFgWnd))
			{
				diag_log("Opening 'Manual editing' dialog");
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_MANUAL_EDITING), hFgWnd, (DLGPROC)DlgProc);
				SetForegroundWindow(hFgWnd);
			}
			else hFgWnd = NULL;
			bKEYV = FALSE;
			return TRUE;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT dlgmsg, WPARAM wParam, LPARAM lParam)
{
	int x, y, w, h;
	switch (dlgmsg)
	{
	case WM_INITDIALOG:
	{
		diag_log("Initializing 'Manual editing' dialog");
		SetWindowTextW(hDlg, szTitle);
		GetWindowTextW(hFgWnd, szWinTitle, _countof(szWinTitle));
		GetClassNameW(hFgWnd, szWinClass, _countof(szWinClass));
		GetWindowRect(hFgWnd, &rcFW);
		x = rcFW.left;
		y = rcFW.top;
		w = rcFW.right - rcFW.left;
		h = rcFW.bottom - rcFW.top;
		SetDlgItemInt(hDlg, IDC_EDIT_X, x, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_Y, y, TRUE);
		SetDlgItemInt(hDlg, IDC_EDIT_WIDTH, w, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, h, FALSE);
		SetDlgItemTextW(hDlg, IDC_EDIT_TITLE, szWinTitle);
		SetDlgItemTextW(hDlg, IDC_EDIT_CLASS, szWinClass);
		UpdateWindow(hDlg);
		break;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON_SET:
		{
			diag_log("Pressed 'Set' button");
			x = GetDlgItemInt(hDlg, IDC_EDIT_X, NULL, TRUE);
			y = GetDlgItemInt(hDlg, IDC_EDIT_Y, NULL, TRUE);
			w = GetDlgItemInt(hDlg, IDC_EDIT_WIDTH, NULL, FALSE);
			h = GetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, NULL, FALSE);
			SendMessageW(hFgWnd, WM_ENTERSIZEMOVE, NULL, NULL);
			MoveWindow(hFgWnd, x, y, w, h, TRUE);
			SendMessageW(hFgWnd, WM_EXITSIZEMOVE, NULL, NULL);
			diag_log("Window with handle 0x", hFgWnd, " was moved to x = ", x, ", y = ", y);
			return TRUE;
			break;
		}
		case IDCANCEL:
		case IDC_BUTTON_CLOSE:
		{
			diag_log("Closing 'Manual editing' dialog");
			EndDialog(hDlg, LOWORD(wParam));
			break;
		}
		}
	}
	return FALSE;
}

bool IsWindowApprooved(HWND hFW)
{
	diag_log("Entering IsWindowApprooved(), handle = 0x", hFW);
	bool bApprooved = FALSE;
	if (hFW)
	{
		GetClassNameW(hFW, szWinClass, _countof(szWinClass));
		if (GetWindowTextW(hFW, (LPWSTR)szBuffer, BUF_LEN - sizeof(WCHAR))) diag_log("Title: '", ConvertWideToUtf8((LPWSTR)szBuffer), "'");
		if (IsIconic(hFW)) diag_log("Window is iconic");
		if (IsZoomed(hFW)) diag_log("Window is maximized");
		if ((wcscmp(szWinClass, szWinCore) != 0) &&
			(wcscmp(szWinClass, szWorkerW) != 0) &&
			(hFW != hDesktop && hFW != hTaskBar && hFW != hProgman))
		{
			if (!IsIconic(hFW) && !IsZoomed(hFW))
			{
				diag_log("Window is approved");
				bApprooved = TRUE;
			}
			else ShowError(IDS_ERR_MAXMIN);
		}
		else diag_log("The window belongs to the Windows environment");
	}
	if (!bApprooved) diag_log("Window is not approved!");
	diag_log("Quiting IsWindowApprooved()");
	return bApprooved;
}

VOID HandlingTrayIcon()
{
	diag_log("Entering HandlingTrayIcon(), bShowIcon = ", bShowIcon);
	if (bShowIcon)
	{
		bool bResult1 = Shell_NotifyIconW(NIM_ADD, &nid);
		diag_log("Shell_NotifyIconW(NIM_ADD): ", bResult1);
		bool bResult2 = Shell_NotifyIconW(NIM_SETVERSION, &nid);
		diag_log("Shell_NotifyIconW(NIM_SETVERSION): ", bResult2);
		if (!bResult1 || !bResult2)
		{
			diag_log("Error creating trayicon!");
			ShowError(IDS_ERR_ICON);
			bShowIcon = FALSE;
		}
	}
	else
	{
		Shell_NotifyIconW(NIM_DELETE, &nid);
	}
	diag_log("Quiting HandlingTrayIcon()");
}

VOID ShowError(UINT uID)
{
	WCHAR szErrorText[MAX_LOADSTRING]; // Error's text
	LoadStringW(hInst, uID, szErrorText, _countof(szErrorText));
	MessageBoxW(NULL, szErrorText, szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INITCOMMONCONTROLSEX icex = { 0 };
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS;
	InitCommonControlsEx(&icex);

	switch (message)
	{
	case WM_INITDIALOG:
	{
		diag_log("Initializing 'About' dialog");

		WCHAR szAboutProgName[MAX_LOADSTRING];
		WCHAR szAboutCopyright[MAX_LOADSTRING];
		WCHAR szAboutBuildTime[MAX_LOADSTRING];
		WCHAR szAboutHelp[MAX_LOADSTRING * 12];
		MultiByteToWideChar(1251, 0, PRODUCT_NAME_FULL, _countof(PRODUCT_NAME_FULL), szAboutProgName, MAX_LOADSTRING);
		MultiByteToWideChar(1251, 0, PRODUCT_COPYRIGHT, _countof(PRODUCT_COPYRIGHT), szAboutCopyright, MAX_LOADSTRING);
		MultiByteToWideChar(1251, 0, BUILD_DATETIME, _countof(BUILD_DATETIME), szAboutBuildTime, MAX_LOADSTRING);
		LoadStringW(hInst, IDS_ABOUT, szAboutHelp, _countof(szAboutHelp));
		SetDlgItemTextW(hDlg, IDC_ABOUT_PROGNAME, szAboutProgName);
		SetDlgItemTextW(hDlg, IDC_ABOUT_COPYRIGHT, szAboutCopyright);
		SetDlgItemTextW(hDlg, IDC_ABOUT_BUILDTIME, szAboutBuildTime);
		SetDlgItemTextW(hDlg, IDC_ABOUTHELP, szAboutHelp);
		return (INT_PTR)TRUE;
		break;
	}

	case WM_NOTIFY:
	{
		LPNMHDR pNMHdr = (LPNMHDR)lParam;
		if ((NM_CLICK == pNMHdr->code || NM_RETURN == pNMHdr->code) && IDC_DONATIONLINK == pNMHdr->idFrom)
		{
			PNMLINK pNMLink = (PNMLINK)pNMHdr;
			LITEM item = pNMLink->item;
			ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
			diag_log("Pressed donation link");
			return (INT_PTR)TRUE;
		}
		break;
	}

	case WM_COMMAND:
	{
		if (IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam))
		{
			EndDialog(hDlg, LOWORD(wParam));
			diag_log("Closing 'About' dialog");
			return (INT_PTR)TRUE;
		}
		break;
	}
	}
	return (INT_PTR)FALSE;
}

std::string ConvertWideToUtf8(const std::wstring& wstr)
{
	int count = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
	std::string str(count, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], count, NULL, NULL);
	return str;
}
