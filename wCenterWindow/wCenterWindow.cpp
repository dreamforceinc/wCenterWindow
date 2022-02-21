// wCenterWindow, v2.3.2
//
// TODO: More verbose logs - partially done.
// TODO: More verbose error messages 
// TODO: Window's title in logs (Impossible?)

#include "framework.h"
#include "wCenterWindow.h"

#define KEY_I 0x49
#define KEY_C 0x43
#define KEY_V 0x56

#define BUF_LEN 4096
#define MAX_LOADSTRING 50
#define WM_WCW	0x8F00

// Global variables:
HINSTANCE			hInst;									// Instance
WCHAR				szTitle[MAX_LOADSTRING];				// Window's title
WCHAR				szClass[MAX_LOADSTRING];				// Window's class
WCHAR				szAbout[MAX_LOADSTRING * 12];			// Description text
WCHAR				szWinTitle[256];
WCHAR				szWinClass[256];
WCHAR				szWinCore[] = TEXT("Windows.UI.Core.CoreWindow");
HANDLE				hHeap = NULL;
HHOOK				hMouseHook = NULL, hKbdHook = NULL;		// Hook's handles
HICON				hIcon = NULL;
HMENU				hMenu = NULL, hPopup = NULL;
HWND				hWnd = NULL, hFgWnd = NULL, hTaskBar = NULL, hDesktop = NULL, hProgman = NULL;
BOOL				bKPressed = FALSE, bMPressed = FALSE, bShowIcon = TRUE, bWorkArea = TRUE;
BOOL				bLCTRL = FALSE, bLWIN = FALSE, bKEYV = FALSE;

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
BOOL				CheckWindow(HWND);
BOOL	CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);
LRESULT CALLBACK	MouseHookProc(int, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



VOID MoveWindowToMonitorCenter(HWND hwnd, BOOL bWorkArea, BOOL bResize)
{
	diag_log(L"Entering MoveWindowToMonitorCenter(): hwnd =", hwnd, L"Title:", (LPWSTR)szBuffer);

	RECT fgwrc = { 0 };
	GetWindowRect(hwnd, &fgwrc);
	LONG nWidth = fgwrc.right - fgwrc.left;
	LONG nHeight = fgwrc.bottom - fgwrc.top;

	diag_log(L"Moving window from x =", fgwrc.left, L"y =", fgwrc.top);

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

	diag_log(L"Moving window to x =", x, L"y =", y);
	diag_log(L"Quiting MoveWindowToMonitorCenter()");
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wcex.lpszClassName = szClass;
	wcex.hIconSm = wcex.hIcon;
	hIcon = wcex.hIcon;
	return RegisterClassExW(&wcex);
}


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OpenLogFile();
	diag_log(L"Entering WinMain()");

	hInst = hInstance;

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
	LoadStringW(hInstance, IDS_CLASSNAME, szClass, _countof(szClass));

	if (FindWindowW(szClass, NULL))
	{
		ShowError(IDS_RUNNING);
		return FALSE;
	}

	MyRegisterClass(hInstance);
	hWnd = CreateWindowExW(0, szClass, szTitle, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		ShowError(IDS_ERR_WND);
		return FALSE;
	}

	int nArgs = 0;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	diag_log(L"Arguments:", nArgs);
	for (int i = 0; i < nArgs; i++)
	{
		diag_log(L"Argument", i, L":", szArglist[i]);
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

	diag_log(L"Quiting WinMain(), msg.wParam =", (int)msg.wParam);
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
		hMenu = LoadMenuW(hInst, MAKEINTRESOURCE(IDR_MENU));
		if (!hMenu)
		{
			diag_log(L"Loading context menu failed!");
			ShowError(IDS_ERR_MENU);
			SendMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log(L"Context menu successfully loaded");

		hPopup = GetSubMenu(hMenu, 0);
		if (!hPopup)
		{
			diag_log(L"Creating popup menu failed!");
			ShowError(IDS_ERR_POPUP);
			SendMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log(L"Popup menu successfully created");

		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE;
		bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
		SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);

		//nid.cbSize = sizeof(NOTIFYICONDATAW);
		nid.cbSize = sizeof(nid);
		nid.hWnd = hWnd;
		//nid.uVersion = NOTIFYICON_VERSION_4;
		nid.uVersion = NOTIFYICON_VERSION;
		nid.uCallbackMessage = WM_WCW;
		nid.hIcon = hIcon;
		nid.uID = IDI_TRAYICON;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.dwInfoFlags = NIIF_NONE;
		nid.dwState = NIS_HIDDEN;
		nid.dwStateMask = NIS_HIDDEN;
		//StringCchCopyW(nid.szTip, _countof(nid.szTip), szTitle);
		StringCchCopyW(nid.szTip, ARRAYSIZE(nid.szTip), szTitle);

		hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, hInst, NULL);
		if (!hMouseHook)
		{
			diag_log(L"Creating mouse hook failed!");
			ShowError(IDS_ERR_HOOK);
			SendMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log(L"Mouse hook was successfully set");

		hKbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
		if (!hKbdHook)
		{
			diag_log(L"Creating keyboard hook failed!");
			ShowError(IDS_ERR_HOOK);
			SendMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
		diag_log(L"Keyboard hook was successfully set");

		LoadStringW(hInst, IDS_ABOUT, szAbout, _countof(szAbout));
	}
	break;

	case WM_WCW:
	{
		if ((lParam == WM_RBUTTONDOWN || lParam == WM_LBUTTONDOWN) && wParam == IDI_TRAYICON)
		{
			SetForegroundWindow(hWnd);
			POINT pt;
			GetCursorPos(&pt);
			int idMenu = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			if (idMenu == ID_POPUPMENU_ICON)
			{
				bShowIcon = FALSE;
				HandlingTrayIcon();
			}
			if (idMenu == ID_POPUPMENU_AREA)
			{
				bWorkArea = ~bWorkArea;
				bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
				SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);
				diag_log(L"Changed 'Use workarea' option to", bWorkArea);
			}
			if (idMenu == ID_POPUPMENU_ABOUT && !bKPressed)
			{
				bKPressed = TRUE;
				diag_log(L"Opening 'About' dialog");
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
				bKPressed = FALSE;
			}
			if (idMenu == ID_POPUPMENU_EXIT) SendMessageW(hWnd, WM_CLOSE, NULL, NULL);
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (wParam == WM_MBUTTONUP) bMPressed = FALSE;
	if (wParam == WM_MBUTTONDOWN && bLCTRL && bLWIN && !bMPressed)
	{
		diag_log(L"Pressed LCTRL + LWIN + MMB");
		bMPressed = TRUE;
		hFgWnd = GetForegroundWindow();
		BOOL bApproved = CheckWindow(hFgWnd);
		if (bApproved) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
		else hFgWnd = NULL;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	pkhs = (KBDLLHOOKSTRUCT*)lParam;

	if (wParam == WM_KEYUP)
	{
		if (pkhs->vkCode == VK_LCONTROL) bLCTRL = FALSE;
		if (pkhs->vkCode == VK_LWIN) bLWIN = FALSE;
		bKPressed = FALSE;
	}

	if (wParam == WM_KEYDOWN)
	{
		if (pkhs->vkCode == VK_LCONTROL) bLCTRL = TRUE;
		if (pkhs->vkCode == VK_LWIN) bLWIN = TRUE;

		if (bLCTRL && bLWIN && pkhs->vkCode == KEY_I && !bKPressed)	// 'I' key
		{
			diag_log(L"Pressed LCTRL + LWIN + I");
			bKPressed = TRUE;
			bShowIcon = ~bShowIcon;
			HandlingTrayIcon();
			return TRUE;
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == KEY_C && !bKPressed && !bKEYV)	// 'C' key
		{
			diag_log(L"Pressed LCTRL + LWIN + C");
			bKPressed = TRUE;
			hFgWnd = GetForegroundWindow();
			BOOL bApproved = CheckWindow(hFgWnd);
			if (bApproved) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
			else hFgWnd = NULL;
			return TRUE;
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == KEY_V && !bKPressed && !bKEYV)	// 'V' key
		{
			diag_log(L"Pressed LCTRL + LWIN + V");
			bKPressed = TRUE; bKEYV = TRUE;
			hFgWnd = GetForegroundWindow();
			BOOL bApproved = CheckWindow(hFgWnd);
			if (bApproved)
			{
				diag_log(L"Opening 'Manual editing' dialog");
				DialogBoxW(hInst, MAKEINTRESOURCE(IDD_MANUAL_EDITING), hWnd, (DLGPROC)DlgProc);
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
			x = GetDlgItemInt(hDlg, IDC_EDIT_X, NULL, TRUE);
			y = GetDlgItemInt(hDlg, IDC_EDIT_Y, NULL, TRUE);
			w = GetDlgItemInt(hDlg, IDC_EDIT_WIDTH, NULL, FALSE);
			h = GetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, NULL, FALSE);
			SendMessageW(hFgWnd, WM_ENTERSIZEMOVE, NULL, NULL);
			MoveWindow(hFgWnd, x, y, w, h, TRUE);
			SendMessageW(hFgWnd, WM_EXITSIZEMOVE, NULL, NULL);
			return TRUE;
			break;
		}
		case IDCANCEL:
		case IDC_BUTTON_CLOSE:
		{
			EndDialog(hDlg, LOWORD(wParam));
			diag_log(L"Closing 'Manual editing' dialog");
			break;
		}
		}
	}
	return FALSE;
}

BOOL CheckWindow(HWND hFW)
{
	diag_log(L"Entering CheckWindow(), hwnd=", hFW);
	GetClassNameW(hFW, szWinClass, _countof(szWinClass));
	if (hFW)
	{
		if (wcscmp(szWinClass, szWinCore) != 0)
		{
			if (hFW != hDesktop && hFW != hTaskBar && hFW != hProgman)
			{
				if (!IsIconic(hFW) && !IsZoomed(hFW))
				{
					GetWindowTextW(hFW, (LPWSTR)szBuffer, BUF_LEN - sizeof(WCHAR));
					diag_log(L"Window with hwnd=", hFW, L"is approved");
					return TRUE;
				}
				else ShowError(IDS_ERR_MAXMIN);
			}
		}
	}
	diag_log(L"Window with hwnd=", hFW, L"is not approved!");
	diag_log(L"Quiting CheckWindow()");
	return FALSE;
}

VOID HandlingTrayIcon()
{
	diag_log(L"Entering HandlingTrayIcon(), bShowIcon =", bShowIcon);
	if (bShowIcon)
	{
		BOOL bResult1 = Shell_NotifyIconW(NIM_ADD, &nid);
		diag_log(L"Shell_NotifyIconW(NIM_ADD):", bResult1);
		BOOL bResult2 = Shell_NotifyIconW(NIM_SETVERSION, &nid);
		diag_log(L"Shell_NotifyIconW(NIM_SETVERSION):", bResult2);
		if (!bResult1 || !bResult2)
		{
			diag_log(L"Error creating trayicon.");
			ShowError(IDS_ERR_ICON);
			bShowIcon = FALSE;
		}
	}
	else
	{
		Shell_NotifyIconW(NIM_DELETE, &nid);
	}
	diag_log(L"Quiting HandlingTrayIcon()");
}

VOID ShowError(UINT uID)
{
	WCHAR szErrorText[MAX_LOADSTRING]; // Error's text
	LoadStringW(hInst, uID, szErrorText, _countof(szErrorText));
	MessageBoxW(hWnd, szErrorText, szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
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
		SetDlgItemTextW(hDlg, IDC_ABOUTHELP, szAbout);
		return (INT_PTR)TRUE;
		break;
	}

	case WM_NOTIFY:
	{
		LPNMHDR pNMHdr = (LPNMHDR)lParam;
		switch (pNMHdr->code)
		{
		case NM_CLICK:
		case NM_RETURN:
			if (pNMHdr->idFrom == IDC_DONATIONLINK)
			{
				PNMLINK pNMLink = (PNMLINK)pNMHdr;
				LITEM item = pNMLink->item;
				ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
				diag_log(L"Pressed donation link");
				return (INT_PTR)TRUE;
			}
			break;
		}
		break;
	}

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			diag_log(L"Closing 'About' dialog");
			return (INT_PTR)TRUE;
			break;
		}
	}
	return (INT_PTR)FALSE;
}
