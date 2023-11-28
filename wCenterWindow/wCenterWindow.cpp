// wCenterWindow
// wCenterWindow.cpp
// 
// TODO: Licensing.
// TODO: Change keyboard low-level hook to RegisterHotKey function.
// TODO: Make x64 version.
//
#include "framework.h"
#include "wCenterWindow.h"
//#include "updater.h"

#define NO_DONATION
#define KEY_I 0x49
#define KEY_C 0x43
#define KEY_V 0x56

#define BUF_LEN 1024
#define WM_WCW (WM_APP + 0x0F00)

// Global variables:
HINSTANCE			hInst;									// Instance
WCHAR				szTitle[MAX_LOADSTRING];				// wCenterWindow's title
WCHAR				szClass[MAX_LOADSTRING];				// Window's class
WCHAR				szWinTitle[256];
WCHAR				szWinClass[256];
HANDLE				hHeap = NULL, hUpdater = NULL;
UINT				dwUpdaterID = 0;
HHOOK				hMouseHook = NULL, hKbdHook = NULL;		// Hook's handles
HICON				hIcon = NULL;
HMENU				hMenu = NULL, hPopup = NULL;
HWND				hWnd = NULL, hFgWnd = NULL;
BOOL				bKPressed = FALSE, bMPressed = FALSE, fShowIcon = TRUE, fCheckUpdates = TRUE, bWorkArea = TRUE;
BOOL				bLCTRL = FALSE, bLWIN = FALSE, bKEYV = FALSE;
CLogger				logger(TEXT(PRODUCT_NAME), TEXT(VERSION_STR));

NOTIFYICONDATAW		nid = { 0 };
MENUITEMINFO		mii = { 0 };

LPVOID				szBuffer;

// {2D7B7F30-4B5F-4380-9807-57D7A2E37F6C}
// static const GUID	guid = { 0x2d7b7f30, 0x4b5f, 0x4380, { 0x98, 0x7, 0x57, 0xd7, 0xa2, 0xe3, 0x7f, 0x6c } };

// Forward declarations of functions included in this code module:
VOID				HandlingTrayIcon();
VOID				ShowError(UINT);
BOOL				IsWindowApprooved(HWND);
BOOL	CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);
LRESULT CALLBACK	MouseHookProc(int, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

VOID MoveWindowToMonitorCenter(HWND hwnd, BOOL bWorkArea, BOOL bResize)
{
	logger.Out(L"Entering the %s() function", TEXT(__FUNCTION__));

	RECT fgwrc = { 0 };
	GetWindowRect(hwnd, &fgwrc);
	LONG nWidth = fgwrc.right - fgwrc.left;
	LONG nHeight = fgwrc.bottom - fgwrc.top;

	logger.Out(L"%s(%d): Moving the window from %d, %d", TEXT(__FUNCTION__), __LINE__, fgwrc.left, fgwrc.top);

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

	logger.Out(L"%s(%d): Moving the window to %d, %d", TEXT(__FUNCTION__), __LINE__, x, y);

	SendMessageW(hwnd, WM_ENTERSIZEMOVE, NULL, NULL);
	MoveWindow(hwnd, x, y, nWidth, nHeight, TRUE);
	SendMessageW(hwnd, WM_EXITSIZEMOVE, NULL, NULL);

	logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));
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

	logger.Out(L"Entering the %s() function", TEXT(__FUNCTION__));

	int nArgs = 0;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	logger.Out(L"Arguments count: %d", nArgs - 1);

	if (nArgs > 1)
	{
		for (int i = 1; i < nArgs; i++)
		{
			logger.Out(L"Argument %d: %s", i, szArglist[i]);
			if (0 == lstrcmpiW(szArglist[i], L"/hide")) fShowIcon = FALSE;
			if (0 == lstrcmpiW(szArglist[i], L"/noupdate")) fCheckUpdates = FALSE;
		}

	}
	LocalFree(szArglist);

	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	LoadIconMetric(hInst, MAKEINTRESOURCE(IDI_TRAYICON), LIM_LARGE, &(wcex.hIcon));
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

	HandlingTrayIcon();

	hHeap = GetProcessHeap();
	szBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, BUF_LEN);

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
	DestroyIcon(hIcon);

	logger.Out(L"Exit from the %s() function, msg.wParam = %d", TEXT(__FUNCTION__), (int)msg.wParam);

	HeapFree(hHeap, NULL, szBuffer);
	CloseHandle(hUpdater);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			logger.Out(L"%s(%d): Recived WM_CREATE message", TEXT(__FUNCTION__), __LINE__);

			hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDR_MENU));
			if (!hMenu)
			{
				logger.Out(L"%s(%d): Loading context menu failed!", TEXT(__FUNCTION__), __LINE__);
				ShowError(IDS_ERR_MENU);
				PostQuitMessage(0);
				//PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
			}
			logger.Out(L"%s(%d): Context menu successfully loaded", TEXT(__FUNCTION__), __LINE__);

			hPopup = GetSubMenu(hMenu, 0);
			if (!hPopup)
			{
				logger.Out(L"%s(%d): Creating popup menu failed!", TEXT(__FUNCTION__), __LINE__);
				ShowError(IDS_ERR_POPUP);
				PostQuitMessage(0);
				//PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
			}
			logger.Out(L"%s(%d): Popup menu successfully created", TEXT(__FUNCTION__), __LINE__);

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
			StringCchCopyW(nid.szTip, _countof(nid.szTip), szTitle);

			if (fCheckUpdates)
			{
				if (!SetTimer(hWnd, IDT_TIMER, 20000, NULL))	// 20 seconds
				{
					logger.Out(L"%s(%d): Creating timer failed!", TEXT(__FUNCTION__), __LINE__);
					ShowError(IDS_ERR_TIMER);
					fCheckUpdates = FALSE;
				}
				logger.Out(L"%s(%d): Timer successfully created", TEXT(__FUNCTION__), __LINE__);
			}

#ifndef _DEBUG
			hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, hInst, NULL);
			if (!hMouseHook)
			{
				logger.Out(L"%s(%d): Mouse hook creation failed!", TEXT(__FUNCTION__), __LINE__);

				ShowError(IDS_ERR_HOOK);
				PostQuitMessage(0);
				//PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
			}
			logger.Out(L"%s(%d): The mouse hook was successfully installed", TEXT(__FUNCTION__), __LINE__);
#endif // !_DEBUG

			hKbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
			if (!hKbdHook)
			{
				logger.Out(L"%s(%d): Keyboard hook creation failed!", TEXT(__FUNCTION__), __LINE__);

				ShowError(IDS_ERR_HOOK);
				PostQuitMessage(0);
				//PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
			}
			logger.Out(L"%s(%d): The keyboard hook was successfully installed", TEXT(__FUNCTION__), __LINE__);
			break;
		}

		case WM_TIMER:
		{
			if (fCheckUpdates)
			{
				Sleep(10000);	// 10 seconds

				logger.Out(L"%s(%d): Checking for updates is enabled, fCheckUpdates = %s", TEXT(__FUNCTION__), __LINE__, fCheckUpdates ? L"True" : L"False");

				//hUpdater = CreateThread(NULL, 0, &Updater, nullptr, 0, nullptr);
				//hUpdater = (HANDLE)_beginthreadex(NULL, 0, &Updater, NULL, 0, &dwUpdaterID);
				if (NULL == hUpdater)
				{
					DWORD dwLastError = GetLastError();
					logger.Out(L"%s(%d): Creating Updater thread failed! Error: %d", TEXT(__FUNCTION__), __LINE__, dwLastError);
				}
				else
				{
					if (!SetTimer(hWnd, IDT_TIMER, 86390000, NULL))	// 1 day - 10 seconds
					{
						logger.Out(L"%s(%d): Creating timer failed!", TEXT(__FUNCTION__), __LINE__);
						ShowError(IDS_ERR_TIMER);
						fCheckUpdates = FALSE;
					}
					logger.Out(L"%s(%d): Timer successfully created", TEXT(__FUNCTION__), __LINE__);
				}
			}
			else
			{
				logger.Out(L"%s(%d): Checking for updates is disabled, fCheckUpdates = %s", TEXT(__FUNCTION__), __LINE__, fCheckUpdates ? L"True" : L"False");
			}
			break;
		}

		case WM_WCW:			// Popup menu handler
		{
			if (IDI_TRAYICON == wParam && (WM_RBUTTONDOWN == lParam || WM_LBUTTONDOWN == lParam))
			{
				logger.Out(L"%s(%d): Entering the WM_WCW message handler", TEXT(__FUNCTION__), __LINE__);

				SetForegroundWindow(hWnd);
				POINT pt;
				GetCursorPos(&pt);
				int idMenu = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
				if (ID_POPUPMENU_ICON == idMenu)
				{
					logger.Out(L"%s(%d): Pressed the 'Hide icon' menuitem", TEXT(__FUNCTION__), __LINE__);

					fShowIcon = FALSE;
					HandlingTrayIcon();
				}
				if (ID_POPUPMENU_AREA == idMenu)
				{
					logger.Out(L"%s(%d): Pressed the 'Use workarea' menuitem", TEXT(__FUNCTION__), __LINE__);

					bWorkArea = !bWorkArea;
					bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
					SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);
					logger.Out(L"%s(%d): Changed 'Use workarea' option to %s", TEXT(__FUNCTION__), __LINE__, bWorkArea ? L"True" : L"False");
				}
				if (ID_POPUPMENU_ABOUT == idMenu && !bKPressed)
				{
					logger.Out(L"%s(%d): Pressed the 'About' menuitem", TEXT(__FUNCTION__), __LINE__);

					bKPressed = TRUE;
					DialogBoxW(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
					bKPressed = FALSE;
				}
				if (ID_POPUPMENU_EXIT == idMenu)
				{
					logger.Out(L"%s(%d): Pressed the 'Exit' menuitem", TEXT(__FUNCTION__), __LINE__);

					PostQuitMessage(0);
					//PostMessageW(hWnd, WM_CLOSE, NULL, NULL);
				}

				logger.Out(L"%s(%d): Exit from the WM_WCW message handler", TEXT(__FUNCTION__), __LINE__);
			}
			break;
		}

		case WM_QUERYENDSESSION:
		{
			logger.Out(L"%s(%d): Recieved the WM_QUERYENDSESSION message, lParam = 0x%08X", TEXT(__FUNCTION__), __LINE__, (long)lParam);

			PostQuitMessage(0);
			return TRUE;
			break;
		}

		case WM_DESTROY:
		{
			logger.Out(L"%s(%d): Recieved the WM_DESTROY message", TEXT(__FUNCTION__), __LINE__);

			PostQuitMessage(0);
			break;
		}

		default: return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (WM_MBUTTONUP == wParam) bMPressed = FALSE;
	if (WM_MBUTTONDOWN == wParam && bLCTRL && bLWIN && !bMPressed)
	{
		logger.Out(L"%s(%d): Pressed LCTRL + LWIN + MMB", TEXT(__FUNCTION__), __LINE__);

		bMPressed = TRUE;
		hFgWnd = GetForegroundWindow();
		if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
		else hFgWnd = NULL;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPKBDLLHOOKSTRUCT pkhs = { 0 };
	pkhs = (LPKBDLLHOOKSTRUCT)lParam;
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
			logger.Out(L"%s(%d): Pressed LCTRL + LWIN + I", TEXT(__FUNCTION__), __LINE__);

			bKPressed = TRUE;
			fShowIcon = !fShowIcon;
			HandlingTrayIcon();
			return TRUE;
		}

		if (KEY_C == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV)	// 'C' key
		{
			logger.Out(L"%s(%d): Pressed LCTRL + LWIN + C", TEXT(__FUNCTION__), __LINE__);

			bKPressed = TRUE;
			hFgWnd = GetForegroundWindow();
			if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
			else hFgWnd = NULL;
			return TRUE;
		}

		if (KEY_V == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV)	// 'V' key
		{
			logger.Out(L"%s(%d): Pressed LCTRL + LWIN + V", TEXT(__FUNCTION__), __LINE__);

			bKPressed = TRUE; bKEYV = TRUE;
			hFgWnd = GetForegroundWindow();
			if (IsWindowApprooved(hFgWnd))
			{
				logger.Out(L"%s(%d): Opening the 'Manual editing' dialog", TEXT(__FUNCTION__), __LINE__);

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
	RECT rcFW = { 0 };
	int x, y, w, h;
	switch (dlgmsg)
	{
		case WM_INITDIALOG:
		{
			logger.Out(L"%s(%d): Initializing the 'Manual editing' dialog", TEXT(__FUNCTION__), __LINE__);

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
		{
			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_SET:
				{
					logger.Out(L"%s(%d): Pressed the 'Set' button", TEXT(__FUNCTION__), __LINE__);

					x = GetDlgItemInt(hDlg, IDC_EDIT_X, NULL, TRUE);
					y = GetDlgItemInt(hDlg, IDC_EDIT_Y, NULL, TRUE);
					w = GetDlgItemInt(hDlg, IDC_EDIT_WIDTH, NULL, FALSE);
					h = GetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, NULL, FALSE);
					SendMessageW(hFgWnd, WM_ENTERSIZEMOVE, NULL, NULL);
					MoveWindow(hFgWnd, x, y, w, h, TRUE);
					SendMessageW(hFgWnd, WM_EXITSIZEMOVE, NULL, NULL);

					logger.Out(L"%s(%d): Window with handle 0x%08X was moved to %d, %d", TEXT(__FUNCTION__), __LINE__, hFgWnd, x, y);

					return TRUE;
					break;
				}
				case IDCANCEL:
				case IDC_BUTTON_CLOSE:
				{
					logger.Out(L"%s(%d): Closing the 'Manual editing' dialog", TEXT(__FUNCTION__), __LINE__);

					EndDialog(hDlg, LOWORD(wParam));
					break;
				}
			}
			break;
		}
	}
	return FALSE;
}

BOOL IsWindowApprooved(HWND hFW)
{
	logger.Out(L"Entering the %s() function, handle = 0x%08X", TEXT(__FUNCTION__), hFW);

	bool bApprooved = FALSE;
	if (hFW)
	{
		if (GetWindowTextW(hFW, (LPWSTR)szBuffer, BUF_LEN - sizeof(WCHAR)))
		{
			logger.Out(L"%s(%d): Window title: '%s'", TEXT(__FUNCTION__), __LINE__, (LPWSTR)szBuffer);
		}

		if (IsIconic(hFW))
		{
			logger.Out(L"%s(%d): The window is iconified", TEXT(__FUNCTION__), __LINE__);
		}

		if (IsZoomed(hFW))
		{
			logger.Out(L"%s(%d): The window is maximized", TEXT(__FUNCTION__), __LINE__);
		}

		LONG_PTR wlp = GetWindowLongPtrW(hFW, GWL_STYLE);
		if (wlp & WS_CAPTION)
		{
			if (!IsIconic(hFW) && !IsZoomed(hFW))
			{
				logger.Out(L"%s(%d): The window is approved!", TEXT(__FUNCTION__), __LINE__);

				bApprooved = TRUE;
			}
			else ShowError(IDS_ERR_MAXMIN);
		}
		else
		{
			logger.Out(L"%s(%d): The window has no caption!", TEXT(__FUNCTION__), __LINE__);
		}
	}

	if (!bApprooved)
	{
		logger.Out(L"%s(%d): The window is not approved!", TEXT(__FUNCTION__), __LINE__);
	}

	logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));

	return bApprooved;
}

VOID HandlingTrayIcon()
{
	logger.Out(L"Entering the %s() function, fShowIcon = %s", TEXT(__FUNCTION__), fShowIcon ? L"True" : L"False");

	if (fShowIcon)
	{
		bool bResult1 = Shell_NotifyIconW(NIM_ADD, &nid);
		logger.Out(L"%s(%d): Shell_NotifyIconW(NIM_ADD): %s", TEXT(__FUNCTION__), __LINE__, bResult1 ? L"True" : L"False");

		bool bResult2 = Shell_NotifyIconW(NIM_SETVERSION, &nid);
		logger.Out(L"%s(%d): Shell_NotifyIconW(NIM_SETVERSION): %s", TEXT(__FUNCTION__), __LINE__, bResult2 ? L"True" : L"False");

		if (!bResult1 || !bResult2)
		{
			logger.Out(L"%s(%d): Error creating trayicon!", TEXT(__FUNCTION__), __LINE__);

			ShowError(IDS_ERR_ICON);
			Shell_NotifyIconW(NIM_DELETE, &nid);
			fShowIcon = FALSE;
		}
		else
		{
			Shell_NotifyIconW(NIM_MODIFY, &nid);
		}
	}
	else
	{
		Shell_NotifyIconW(NIM_DELETE, &nid);
	}

	logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));
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
			logger.Out(L"%s(%d): Initializing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

			WCHAR szAboutProgName[MAX_LOADSTRING];
			WCHAR szAboutCopyright[MAX_LOADSTRING];
			WCHAR szAboutBuildTime[MAX_LOADSTRING];
			WCHAR szAboutHelp[MAX_LOADSTRING * 12];
			MultiByteToWideChar(1251, 0, PRODUCT_NAME_FULL, _countof(PRODUCT_NAME_FULL), szAboutProgName, MAX_LOADSTRING);
			MultiByteToWideChar(1251, 0, PRODUCT_COPYRIGHT, _countof(PRODUCT_COPYRIGHT), szAboutCopyright, MAX_LOADSTRING);
			MultiByteToWideChar(1251, 0, ABOUT_BUILD, _countof(ABOUT_BUILD), szAboutBuildTime, MAX_LOADSTRING);
			LoadStringW(hInst, IDS_ABOUT, szAboutHelp, _countof(szAboutHelp));
			SetDlgItemTextW(hDlg, IDC_ABOUT_PROGNAME, szAboutProgName);
			SetDlgItemTextW(hDlg, IDC_ABOUT_COPYRIGHT, szAboutCopyright);
			SetDlgItemTextW(hDlg, IDC_ABOUT_BUILDTIME, szAboutBuildTime);
			SetDlgItemTextW(hDlg, IDC_ABOUTEDIT, szAboutHelp);
#ifdef NO_DONATION
			HWND hLink = GetDlgItem(hDlg, IDC_DONATIONLINK);
			if (hLink) DestroyWindow(hLink);
			HWND hText = GetDlgItem(hDlg, IDC_DONATIONTEXT);
			if (hText) DestroyWindow(hText);
#endif // !NO_DONATION

			logger.Out(L"%s(%d): End of initializing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

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

				logger.Out(L"%s(%d): Pressed the donation link! :-)", TEXT(__FUNCTION__), __LINE__);

				return (INT_PTR)TRUE;
			}
			break;
		}

		case WM_COMMAND:
		{
			if (IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam))
			{
				EndDialog(hDlg, LOWORD(wParam));

				logger.Out(L"%s(%d): Closing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

				return (INT_PTR)TRUE;
			}
			break;
		}
	}
	return (INT_PTR)FALSE;
}
