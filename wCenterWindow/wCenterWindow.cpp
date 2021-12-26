// wCenterWindow, v2.3
//
// TODO: More verbose logs
// TODO: More verbose error messages
// TODO: Window's title in logs

#include "framework.h"
#include "wCenterWindow.h"
#include "Logger.h"

#define KEY_I 0x49
#define KEY_C 0x43
#define KEY_V 0x56

#define MAX_LOADSTRING 50
#define WM_WCW	0x8F00

// Global variables:
HINSTANCE			hInst;									// Instance
TCHAR				szTitle[MAX_LOADSTRING];				// Window's title
TCHAR				szClass[MAX_LOADSTRING];				// Window's class
TCHAR				szAbout[MAX_LOADSTRING * 12];			// Description text
TCHAR				szWinTitle[256];
TCHAR				szWinClass[256];
TCHAR				szWinCore[] = TEXT("Windows.UI.Core.CoreWindow");
HHOOK				hMouseHook = NULL, hKbdHook = NULL;		// Hook's handles
HICON				hIcon = NULL;
HMENU				hMenu = NULL, hPopup = NULL;
HWND				hWnd = NULL, hFgWnd = NULL, hTaskBar = NULL, hDesktop = NULL, hProgman = NULL;
BOOL				bKPressed = FALSE, bMPressed = FALSE, bShowIcon = TRUE, bWorkArea = TRUE;
BOOL				bLCTRL = FALSE, bLWIN = FALSE, bKEYV = FALSE;

RECT				rcFW = { 0 };
NOTIFYICONDATA		nid = { 0 };
LPKBDLLHOOKSTRUCT	pkhs;
MENUITEMINFO		mii;

// Forward declarations of functions included in this code module:
VOID				HandlingTrayIcon();
VOID				ShowError(UINT);
BOOL				CheckWindow(HWND);
BOOL	CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);
LRESULT CALLBACK	MouseHookProc(int, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAYICON));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.lpszClassName = szClass;
	wcex.hIconSm = wcex.hIcon;
	hIcon = wcex.hIcon;
	return RegisterClassEx(&wcex);
}

VOID MoveWindowToMonitorCenter(HWND hwnd, BOOL bWorkArea, BOOL bResize)
{
	diag_log(L"Entering MoveWindowToMonitorCenter(): hwnd =", hwnd);

	RECT fgwrc;
	GetWindowRect(hwnd, &fgwrc);
	LONG nWidth = fgwrc.right - fgwrc.left;
	LONG nHeight = fgwrc.bottom - fgwrc.top;

	diag_log(L"Moving window from x =", fgwrc.left, L"y =", fgwrc.top);

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
	RECT area;
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

	SendMessage(hwnd, WM_ENTERSIZEMOVE, NULL, NULL);
	MoveWindow(hwnd, x, y, nWidth, nHeight, TRUE);
	SendMessage(hwnd, WM_EXITSIZEMOVE, NULL, NULL);

	diag_log(L"Moving window to x =", x, L"y =", y);
	diag_log(L"Quiting MoveWindowToMonitorCenter()");
}



int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	OpenLogFile();
	diag_log(L"Entering WinMain()");

	hInst = hInstance;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
	LoadString(hInstance, IDS_CLASSNAME, szClass, _countof(szClass));

	if (FindWindow(szClass, NULL))
	{
		ShowError(IDS_RUNNING);
		return FALSE;
	}

	MyRegisterClass(hInstance);
	hWnd = CreateWindowEx(0, szClass, szTitle, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		ShowError(IDS_ERR_WND);
		return FALSE;
	}

	int nArgs = 0;
	LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	diag_log(L"nArgs =", nArgs, L", Args: ", *szArglist);

	(nArgs >= 2 && 0 == lstrcmpiW(szArglist[1], TEXT("/hide"))) ? bShowIcon = FALSE : bShowIcon = TRUE;
	LocalFree(szArglist);
	HandlingTrayIcon();

	hTaskBar = FindWindow(TEXT("Shell_TrayWnd"), NULL);
	hProgman = FindWindow(TEXT("Progman"), NULL);
	hDesktop = GetDesktopWindow();

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			ShowError(IDS_ERR_MAIN);
			return -1;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
	if (hKbdHook) UnhookWindowsHookEx(hKbdHook);
	if (hMenu) DestroyMenu(hMenu);
	Shell_NotifyIcon(NIM_DELETE, &nid);

	diag_log(L"Quiting WinMain(). msg.wParam =", (int)msg.wParam);
	CloseLogFile();

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU));
			if (!hMenu)
			{
				ShowError(IDS_ERR_MENU);
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}

			hPopup = GetSubMenu(hMenu, 0);
			if (!hPopup)
			{
				ShowError(IDS_ERR_POPUP);
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}

			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;
			bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
			SetMenuItemInfo(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);

			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hWnd;
			nid.uVersion = NOTIFYICON_VERSION;
			nid.uCallbackMessage = WM_WCW;
			nid.hIcon = hIcon;
			nid.uID = IDI_TRAYICON;
			nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
			nid.dwInfoFlags = NIIF_INFO;
			StringCchCopy(nid.szTip, _countof(nid.szTip), szTitle);

			hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInst, NULL);
			if (!hMouseHook)
			{
				ShowError(IDS_ERR_HOOK);
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}

			hKbdHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
			if (!hKbdHook)
			{
				ShowError(IDS_ERR_HOOK);
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}

			LoadString(hInst, IDS_ABOUT, szAbout, _countof(szAbout));
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
					bWorkArea = !bWorkArea;
					bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
					SetMenuItemInfo(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);
				}
				if (idMenu == ID_POPUPMENU_ABOUT && !bKPressed)
				{
					bKPressed = TRUE;
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
					bKPressed = FALSE;
				}
				if (idMenu == ID_POPUPMENU_EXIT) SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}
		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (wParam == WM_MBUTTONUP) bMPressed = FALSE;
	if (wParam == WM_MBUTTONDOWN && bLCTRL && bLWIN && !bMPressed)
	{
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
			bKPressed = TRUE;
			bShowIcon = !bShowIcon;
			HandlingTrayIcon();
			return TRUE;
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == KEY_C && !bKPressed && !bKEYV)	// 'C' key
		{
			bKPressed = TRUE;
			hFgWnd = GetForegroundWindow();
			BOOL bApproved = CheckWindow(hFgWnd);
			if (bApproved) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
			else hFgWnd = NULL;
			return TRUE;
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == KEY_V && !bKPressed && !bKEYV)	// 'V' key
		{
			bKPressed = TRUE; bKEYV = TRUE;
			hFgWnd = GetForegroundWindow();
			BOOL bApproved = CheckWindow(hFgWnd);
			if (bApproved) DialogBox(hInst, MAKEINTRESOURCE(IDD_MANUAL_EDITING), hWnd, (DLGPROC)DlgProc);
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
			SetWindowText(hDlg, szTitle);
			GetWindowText(hFgWnd, szWinTitle, _countof(szWinTitle));
			GetClassName(hFgWnd, szWinClass, _countof(szWinClass));
			GetWindowRect(hFgWnd, &rcFW);
			x = rcFW.left;
			y = rcFW.top;
			w = rcFW.right - rcFW.left;
			h = rcFW.bottom - rcFW.top;
			SetDlgItemInt(hDlg, IDC_EDIT_X, x, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_Y, y, TRUE);
			SetDlgItemInt(hDlg, IDC_EDIT_WIDTH, w, FALSE);
			SetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, h, FALSE);
			SetDlgItemText(hDlg, IDC_EDIT_TITLE, szWinTitle);
			SetDlgItemText(hDlg, IDC_EDIT_CLASS, szWinClass);
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
					SendMessage(hFgWnd, WM_ENTERSIZEMOVE, NULL, NULL);
					MoveWindow(hFgWnd, x, y, w, h, TRUE);
					SendMessage(hFgWnd, WM_EXITSIZEMOVE, NULL, NULL);
					return TRUE;
					break;
				}
				case IDCANCEL:
				case IDC_BUTTON_CLOSE:
				{
					EndDialog(hDlg, LOWORD(wParam));
					break;
				}
			}
	}
	return FALSE;
}

BOOL CheckWindow(HWND hFW)
{
	GetClassName(hFW, szWinClass, _countof(szWinClass));
	if (hFW)
	{
		if (_tcscmp(szWinClass, szWinCore) != 0)
		{
			if (hFW != hDesktop && hFW != hTaskBar && hFW != hProgman)
			{
				if (!IsIconic(hFW) && !IsZoomed(hFW)) return TRUE;
				else ShowError(IDS_ERR_MAXMIN);
			}
		}
	}
	return FALSE;
}

VOID HandlingTrayIcon()
{
	diag_log(L"Entering HandlingTrayIcon(). bShowIcon =", bShowIcon);
	if (bShowIcon)
	{
		if (!Shell_NotifyIcon(NIM_ADD, &nid))
		{
			diag_log(L"GetLastError():", GetLastError());
			ShowError(IDS_ERR_ICON);
			bShowIcon = FALSE;
		}
	}
	else
	{
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
	diag_log(L"Quiting HandlingTrayIcon()");
}

VOID ShowError(UINT uID)
{
	TCHAR szErrorText[MAX_LOADSTRING]; // Error's text
	LoadString(hInst, uID, szErrorText, _countof(szErrorText));
	MessageBox(hWnd, szErrorText, szTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LINK_CLASS;
	InitCommonControlsEx(&icex);

	switch (message)
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText(hDlg, IDC_ABOUTHELP, szAbout);
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
						ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
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
				return (INT_PTR)TRUE;
				break;
			}
	}
	return (INT_PTR)FALSE;
}
