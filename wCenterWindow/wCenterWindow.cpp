// wCenterWindow.cpp : Определяет точку входа для приложения.

#include "framework.h"
#include "wCenterWindow.h"

#define MAX_LOADSTRING 32
#define WM_WCW	WM_USER + 0x7F00

// Глобальные переменные:
HINSTANCE		hInst;							// Текущий экземпляр
WCHAR			szTitle[MAX_LOADSTRING];		// Текст строки заголовка
WCHAR			szClass[MAX_LOADSTRING];		// Имя класса главного окна
WCHAR			szAbout[MAX_LOADSTRING * 9];	// Текст описания
HHOOK			KeyboardHook;
HICON			hIcon;
HMENU			hMenu, hPopup;
HWND			hWnd;
NOTIFYICONDATA	nid = { 0 };
KBDLLHOOKSTRUCT	*pkhs;
BOOL			pressed = FALSE, showIcon = TRUE;
BOOL			bLCTRL = FALSE, bLWIN = FALSE, bKEYI = FALSE, bKEYC = FALSE;
int				dtCenterX, dtCenterY;

// Прототипы функций
ATOM				MyRegisterClass(HINSTANCE);
VOID				HandlingTrayIcon();
VOID				ShowError(UINT);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	KeyboardHookProc(int, WPARAM, LPARAM);

// Точка входа
int APIENTRY wWinMain(_In_		HINSTANCE	hInstance,
					  _In_opt_	HINSTANCE	hPrevInstance,
					  _In_		LPWSTR		lpCmdLine,
					  _In_		int			nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
	LoadStringW(hInstance, IDS_CLASSNAME, szClass, _countof(szClass));

	if (FindWindow(szClass, NULL))
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

	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uVersion = NOTIFYICON_VERSION;
	nid.uCallbackMessage = WM_WCW;
	nid.hIcon = hIcon;
	nid.uID = IDI_TRAYICON;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.dwInfoFlags = NIIF_INFO;
	StringCchCopy(nid.szTip, _countof(nid.szTip), szTitle);

	int nArgs = 0;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	(nArgs >= 2 && 0 == lstrcmpiW(szArglist[1], L"/hide")) ? showIcon = FALSE : showIcon = TRUE;
	LocalFree(szArglist);
	HandlingTrayIcon();

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

		KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInst, NULL);
		if (!KeyboardHook)
		{
			ShowError(IDS_ERR_HOOK);
			SendMessage(hWnd, WM_CLOSE, NULL, NULL);
		}

		LoadStringW(hInst, IDS_ABOUT, szAbout, _countof(szAbout));
		RECT dtrc = { 0 };
		SystemParametersInfo(SPI_GETWORKAREA, NULL, &dtrc, FALSE);
		dtCenterX = dtrc.right / 2, dtCenterY = dtrc.bottom / 2;
		break;
	}
	case WM_WCW:
		if (lParam == WM_RBUTTONDOWN && wParam == IDI_TRAYICON)
		{
			SetForegroundWindow(hWnd);
			POINT pt;
			GetCursorPos(&pt);
			int idMenu = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			if (idMenu == ID_POPUPMENU_ICON)
			{
				showIcon = FALSE;
				HandlingTrayIcon();
			}
			if (idMenu == ID_POPUPMENU_ABOUT && !pressed)
			{
				pressed = TRUE;
				if (MessageBox(hWnd, szAbout, szTitle, MB_OK | MB_TOPMOST) == IDOK) pressed = FALSE;
			}
			if (idMenu == ID_POPUPMENU_EXIT) SendMessage(hWnd, WM_CLOSE, NULL, NULL);
		}
		break;

	case WM_DESTROY:
		if (KeyboardHook) UnhookWindowsHookEx(KeyboardHook);
		if (hMenu) DestroyMenu(hMenu);
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
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
		pressed = FALSE;
	}

	if (wParam == WM_KEYDOWN)
	{
		if (pkhs->vkCode == VK_LCONTROL) bLCTRL = TRUE;
		if (pkhs->vkCode == VK_LWIN) bLWIN = TRUE;

		if (bLCTRL && bLWIN && pkhs->vkCode == 0x49 && !pressed)	// 'I' key
		{
			pressed = TRUE;
			showIcon = !showIcon;
			HandlingTrayIcon();
		}

		if (bLCTRL && bLWIN && pkhs->vkCode == 0x43 && !pressed)	// 'C' key
		{
			pressed = TRUE;
			HWND fgWindow = GetForegroundWindow();
			if (fgWindow)
			{
				HWND parentWindow = fgWindow;
				while (TRUE)
				{
					parentWindow = GetParent(fgWindow);
					if (parentWindow) fgWindow = parentWindow;
					else break;
				}
				WINDOWPLACEMENT wp = { 0 };
				wp.length = sizeof(WINDOWPLACEMENT);
				GetWindowPlacement(fgWindow, &wp);
				if (wp.showCmd == SW_SHOWNORMAL)
				{
					int fgW = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
					int fgH = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
					int fgX = dtCenterX - (fgW / 2);
					int fgY = dtCenterY - (fgH / 2);
					wp.rcNormalPosition.left = fgX;
					wp.rcNormalPosition.top = fgY;
					wp.rcNormalPosition.right = fgX + fgW;
					wp.rcNormalPosition.bottom = fgY + fgH;
					SendMessage(fgWindow, WM_ENTERSIZEMOVE, NULL, NULL);
					SetWindowPlacement(fgWindow, &wp);
					SendMessage(fgWindow, WM_EXITSIZEMOVE, NULL, NULL);
				}
			}
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

VOID HandlingTrayIcon()
{
	if (showIcon)
	{
		if (!Shell_NotifyIcon(NIM_ADD, &nid))
		{
			ShowError(IDS_ERR_ICON);
			showIcon = FALSE;
		}
	}
	else
	{
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

}

VOID ShowError(UINT uID)
{
	WCHAR szErrorText[MAX_LOADSTRING]; // Текст ошибки
	LoadStringW(hInst, uID, szErrorText, _countof(szErrorText));
	MessageBox(hWnd, szErrorText, szTitle, MB_OK | MB_ICONERROR);
}
