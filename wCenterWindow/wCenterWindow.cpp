// wCenterWindow.cpp : Определяет точку входа для приложения.

#include "framework.h"
#include "wCenterWindow.h"

#define MAX_LOADSTRING 80
#define WM_WCW	WM_USER + 0x7F00

// Глобальные переменные:
HINSTANCE		hInst;						// Текущий экземпляр
WCHAR			szTitle[MAX_LOADSTRING];	// Текст строки заголовка
WCHAR			szClass[MAX_LOADSTRING];	// Имя класса главного окна
WCHAR			szHelp[MAX_LOADSTRING];		// Текст описания
HHOOK			KeyboardHook;
HICON			hIcon;
HMENU			hMenu, hPopup;
HWND			hWnd;
NOTIFYICONDATA	nid = { 0 };
KBDLLHOOKSTRUCT	*pkhs;
int				dtCenterX, dtCenterY;
bool			pressed = FALSE;

// Прототипы функций
ATOM				MyRegisterClass(HINSTANCE);
VOID				ShowError(HINSTANCE, UINT);
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

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDS_CLASSNAME, szClass, MAX_LOADSTRING);
	if (FindWindow(szClass, NULL))
	{
		ShowError(hInstance, IDS_RUNNING);
		return FALSE;
	}
	MyRegisterClass(hInstance);
	hWnd = CreateWindowExW(0, szClass, szTitle, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{
		ShowError(hInstance, IDS_ERR_WND);
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
	StringCchCopy(nid.szTip, sizeof(nid.szTip), szTitle);
	if (!Shell_NotifyIcon(NIM_ADD, &nid))
	{
		ShowError(hInstance, IDS_ERR_ICON);
		SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	}

	hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU));
	if (!hMenu)
	{
		ShowError(hInstance, IDS_ERR_MENU);
		SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	}

	hPopup = GetSubMenu(hMenu, 0);
	if (!hPopup)
	{
		ShowError(hInstance, IDS_ERR_POPUP);
		SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	}

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, NULL);
	if (!KeyboardHook)
	{
		ShowError(hInstance, IDS_ERR_HOOK);
		SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	}

	LoadStringW(hInstance, IDS_HELPTEXT, szHelp, MAX_LOADSTRING);
	RECT dtrc = { 0 };
	SystemParametersInfo(SPI_GETWORKAREA, NULL, &dtrc, FALSE);
	dtCenterX = dtrc.right / 2, dtCenterY = dtrc.bottom / 2;

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			ShowError(hInstance, IDS_ERR_MAIN);
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
	case WM_WCW:
		if (lParam == WM_RBUTTONDOWN && wParam == IDI_TRAYICON)
		{
			SetForegroundWindow(hWnd);
			POINT pt;
			GetCursorPos(&pt);
			int idMenu = TrackPopupMenu(hPopup, TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
			if (idMenu == ID_POPUPMENU_HELP && !pressed)
			{
				pressed = TRUE;
				if (MessageBox(hWnd, szHelp, szTitle, MB_OK | MB_TOPMOST) == IDOK) pressed = FALSE;
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
	bool bLCTRL, bLWIN;
	GetAsyncKeyState(VK_LCONTROL) < 0 ? bLCTRL = TRUE : bLCTRL = FALSE;
	GetAsyncKeyState(VK_LWIN) < 0 ? bLWIN = TRUE : bLWIN = FALSE;

	if ((bLCTRL && bLWIN && pkhs->vkCode == 0x43) && !pressed)
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
		pressed = FALSE;
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

VOID ShowError(HINSTANCE hInstance, UINT uID)
{
	WCHAR szErrorText[MAX_LOADSTRING]; // Текст ошибки
	LoadStringW(hInstance, uID, szErrorText, MAX_LOADSTRING);
	MessageBox(hWnd, szErrorText, szTitle, MB_OK | MB_ICONERROR);
}
