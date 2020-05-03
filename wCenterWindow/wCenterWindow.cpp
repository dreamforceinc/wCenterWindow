// wCenterWindow, v2.1

#include <windows.h>
#include <strsafe.h>
#include "resource.h"
#include "S:\MyFunctions\MoveWindowToMonitorCenter.h"

#define MAX_LOADSTRING 32
#define WM_WCW	0x8F00

// Global variables:
HINSTANCE			hInst;							// Instance
CHAR				szTitle[MAX_LOADSTRING];		// Window's title
CHAR				szClass[MAX_LOADSTRING];		// Window's class
CHAR				szAbout[MAX_LOADSTRING * 14];	// Description text
HHOOK				KeyboardHook;
HICON				hIcon;
HMENU				hMenu, hPopup;
HWND				hWnd, hTaskBar, hDesktop, hProgman;
BOOL				bPressed = FALSE, bShowIcon = TRUE, bWorkArea = TRUE;
BOOL				bLCTRL = FALSE, bLWIN = FALSE, bKEYI = FALSE, bKEYC = FALSE;

NOTIFYICONDATA		nid = { 0 };
LPKBDLLHOOKSTRUCT	pkhs;
MENUITEMINFO		mii;
POINT				ptMousePos;

// Function's prototypes
ATOM				MyRegisterClass(HINSTANCE);
VOID				HandlingTrayIcon();
VOID				ShowError(UINT);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

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
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	(nArgs >= 2 && 0 == lstrcmpiW(szArglist[1], L"/hide")) ? bShowIcon = FALSE : bShowIcon = TRUE;
	LocalFree(szArglist);
	HandlingTrayIcon();

	hTaskBar = FindWindow("Shell_TrayWnd", NULL);
	hProgman = FindWindow("Progman", NULL);
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
	return (int)msg.wParam;
}

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

			KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
			if (!KeyboardHook)
			{
				ShowError(IDS_ERR_HOOK);
				SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}

			LoadString(hInst, IDS_ABOUT, szAbout, _countof(szAbout));
		}
		break;

		case WM_WCW:
		{
			if (lParam == WM_RBUTTONDOWN && wParam == IDI_TRAYICON)
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
				if (idMenu == ID_POPUPMENU_ABOUT && !bPressed)
				{
					bPressed = TRUE;
					if (MessageBox(hWnd, szAbout, szTitle, MB_OK | MB_TOPMOST | MB_ICONINFORMATION) == IDOK) bPressed = FALSE;
				}
				if (idMenu == ID_POPUPMENU_EXIT) SendMessage(hWnd, WM_CLOSE, NULL, NULL);
			}
		}
		break;

		case WM_DESTROY:
		{
			if (KeyboardHook) UnhookWindowsHookEx(KeyboardHook);
			if (hMenu) DestroyMenu(hMenu);
			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
		}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	pkhs = (KBDLLHOOKSTRUCT*)lParam;

	if (wParam == WM_KEYUP)
	{
		if (pkhs->vkCode == VK_LCONTROL) bLCTRL = FALSE;
		if (pkhs->vkCode == VK_LWIN) bLWIN = FALSE;
		bPressed = FALSE;
	}

	if (wParam == WM_KEYDOWN)
	{
		if (pkhs->vkCode == VK_LCONTROL) bLCTRL = TRUE;
		if (pkhs->vkCode == VK_LWIN) bLWIN = TRUE;

		if (bLCTRL && bLWIN && pkhs->vkCode == 0x49 && !bPressed)	// 'I' key
		{
			bPressed = TRUE;
			bShowIcon = !bShowIcon;
			HandlingTrayIcon();
			return TRUE;
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == 0x43 && !bPressed)	// 'C' key
		{
			bPressed = TRUE;
			HWND hFgWnd = GetForegroundWindow();

			if (hFgWnd)
			{
				if (hFgWnd != hDesktop && hFgWnd != hTaskBar && hFgWnd != hProgman)
				{
					if (!IsIconic(hFgWnd) && !IsZoomed(hFgWnd))
					{
						RECT fgwrc;
						GetWindowRect(hFgWnd, &fgwrc);
						LONG fgWidth = fgwrc.right - fgwrc.left;
						LONG fgHeight = fgwrc.bottom - fgwrc.top;
						MoveWindowToMonitorCenter(hFgWnd, fgWidth, fgHeight, bWorkArea, FALSE);
					}
				}
			}
			return TRUE;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

VOID HandlingTrayIcon()
{
	if (bShowIcon)
	{
		if (!Shell_NotifyIcon(NIM_ADD, &nid))
		{
			ShowError(IDS_ERR_ICON);
			bShowIcon = FALSE;
		}
	}
	else
	{
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}
}

VOID ShowError(UINT uID)
{
	CHAR szErrorText[MAX_LOADSTRING]; // Error's text
	LoadString(hInst, uID, szErrorText, _countof(szErrorText));
	MessageBox(hWnd, szErrorText, szTitle, MB_OK | MB_ICONERROR);
}
