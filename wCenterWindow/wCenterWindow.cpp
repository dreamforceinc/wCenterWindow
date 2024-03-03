// wCenterWindow
// wCenterWindow.cpp

// TODO: Move the InitCommonControlsEx() function to the WinMain().
// TODO: Split main cpp-file to separate files.
// TODO: Make the automatic updater (download, unzip and replace executable).
// TODO: Change keyboard low-level hook to RegisterHotKey function. (Is it really needed?)

#include "framework.h"
#include "wCenterWindow.h"
#include "updater.h"

//#define NO_DONATION
#define KEY_I 0x49
#define KEY_C 0x43
#define KEY_V 0x56

#define MAX_WINTITLE_BUFFER_LENGTH 1024
#define WM_WCW (WM_APP + 0x0F00)

// Global variables:
WCHAR szTitle[MAX_LOADSTRING]{ 0 }; // wCenterWindow's title
HICON            hIconSmall = NULL, hIconLarge = NULL;
HMENU            hPopup = NULL;
HWND             hFgWnd = NULL;
BOOL             bKPressed = FALSE, bMPressed = FALSE, fShowIcon = TRUE, fCheckUpdates = TRUE, bWorkArea = TRUE;
BOOL             bLCTRL = FALSE, bLWIN = FALSE, bKEYV = FALSE;
UINT             uMsgRestore = 0;
CLogger          logger(TEXT(PRODUCT_NAME_FULL));

NOTIFYICONDATAW  nid = { 0 };
MENUITEMINFOW    mii = { 0 };

LPVOID           szWinTitleBuffer = nullptr;
LPVOID           szWinClassBuffer = nullptr;

// Forward declarations of functions included in this code module:
VOID             HandlingTrayIcon();
VOID             ShowError(UINT, LPCWSTR);
VOID             ShowPopupMenu(HWND, POINT);
BOOL             IsWindowApprooved(HWND);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardHookProc(int, WPARAM, LPARAM);
LRESULT CALLBACK MouseHookProc(int, WPARAM, LPARAM);
INT_PTR	CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

static VOID MoveWindowToMonitorCenter(HWND hwnd, BOOL bWorkArea, BOOL bResize) {
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
    if (bWorkArea) {
        area.bottom = mi.rcWork.bottom;
        area.left = mi.rcWork.left;
        area.right = mi.rcWork.right;
        area.top = mi.rcWork.top;
    }
    else {
        area.bottom = mi.rcMonitor.bottom;
        area.left = mi.rcMonitor.left;
        area.right = mi.rcMonitor.right;
        area.top = mi.rcMonitor.top;
    }

    LONG aw = area.right - area.left;
    LONG ah = area.bottom - area.top;
    if ((nWidth > aw) && bResize) nWidth = aw;
    if ((nHeight > ah) && bResize) nHeight = ah;
    if (area.left < 0) {
        aw = -aw;
        area.left = 0;
    }
    if (area.top < 0) {
        ah = -ah;
        area.top = 0;
    }
    int x = area.left + (aw - nWidth) / 2;
    int y = area.top + (ah - nHeight) / 2;

    logger.Out(L"%s(%d): Moving the window to %d, %d", TEXT(__FUNCTION__), __LINE__, x, y);

    SendMessageW(hwnd, WM_ENTERSIZEMOVE, NULL, NULL);
    MoveWindow(hwnd, x, y, nWidth, nHeight, TRUE);
    SendMessageW(hwnd, WM_EXITSIZEMOVE, NULL, NULL);

    logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WCHAR szClass[MAX_LOADSTRING]{ 0 }; // Window's class

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, _countof(szTitle));
    LoadStringW(hInstance, IDS_CLASSNAME, szClass, _countof(szClass));

    if (FindWindowW(szClass, NULL)) {
        ShowError(IDS_RUNNING, szTitle);
        return -10;
    }

    logger.Out(L"Entering the %s() function", TEXT(__FUNCTION__));

    int nArgs = 0;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

    logger.Out(L"Arguments count: %d", nArgs - 1);

    if (nArgs > 1) {
        for (int i = 1; i < nArgs; i++) {
            logger.Out(L"Argument %d: %s", i, szArglist[i]);
            if (0 == lstrcmpiW(szArglist[i], L"/hide")) fShowIcon = FALSE;
            if (0 == lstrcmpiW(szArglist[i], L"/noupdate")) fCheckUpdates = FALSE;
        }

    }
    LocalFree(szArglist);

    LoadIconMetric(hInstance, MAKEINTRESOURCEW(IDI_TRAYICON), LIM_LARGE, &hIconLarge);
    LoadIconMetric(hInstance, MAKEINTRESOURCEW(IDI_TRAYICON), LIM_SMALL, &hIconSmall);

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = hIconLarge;
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.lpszClassName = szClass;
    wcex.hIconSm = hIconSmall;
    if (!RegisterClassExW(&wcex)) {
        ShowError(IDS_ERR_CLASS, szTitle);
        return -9;
    }

    HWND hMainWnd = CreateWindowExW(0, szClass, szTitle, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hMainWnd) {
        ShowError(IDS_ERR_WND, szTitle);
        return -8;
    }

#ifndef _DEBUG
    HHOOK hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandleW(NULL), NULL);
    if (!hMouseHook) {
        logger.Out(L"%s(%d): Mouse hook creation failed!", TEXT(__FUNCTION__), __LINE__);

        ShowError(IDS_ERR_HOOK, szTitle);
        return -7;
    }
    logger.Out(L"%s(%d): The mouse hook was successfully installed", TEXT(__FUNCTION__), __LINE__);
#endif // !_DEBUG

    HHOOK hKbdHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandleW(NULL), NULL);
    if (!hKbdHook) {
        logger.Out(L"%s(%d): Keyboard hook creation failed!", TEXT(__FUNCTION__), __LINE__);

        ShowError(IDS_ERR_HOOK, szTitle);
        return -6;
    }
    logger.Out(L"%s(%d): The keyboard hook was successfully installed", TEXT(__FUNCTION__), __LINE__);

    HMENU hMenu = LoadMenuW(hInstance, MAKEINTRESOURCE(IDR_MENU));
    if (!hMenu) {
        logger.Out(L"%s(%d): Loading context menu failed!", TEXT(__FUNCTION__), __LINE__);
        ShowError(IDS_ERR_MENU, szTitle);
        return -5;
    }
    logger.Out(L"%s(%d): Context menu successfully loaded", TEXT(__FUNCTION__), __LINE__);

    hPopup = GetSubMenu(hMenu, 0);
    if (!hPopup) {
        logger.Out(L"%s(%d): Creating popup menu failed!", TEXT(__FUNCTION__), __LINE__);
        ShowError(IDS_ERR_POPUP, szTitle);
        return -4;
    }
    logger.Out(L"%s(%d): Popup menu successfully created", TEXT(__FUNCTION__), __LINE__);

    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_STATE;
    bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
    SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);

    HandlingTrayIcon();

    HANDLE hHeap = GetProcessHeap();

    szWinTitleBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_WINTITLE_BUFFER_LENGTH);
    if (nullptr == szWinTitleBuffer) {
        ShowError(IDS_ERR_HEAP, szTitle);
        return -3;
    }

    szWinClassBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_WINTITLE_BUFFER_LENGTH);
    if (nullptr == szWinClassBuffer) {
        ShowError(IDS_ERR_HEAP, szTitle);
        return -2;
    }

    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
            ShowError(IDS_ERR_MAIN, szTitle);
            return -1;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

#ifndef _DEBUG
    if (hMouseHook) UnhookWindowsHookEx(hMouseHook);
#endif // !_DEBUG
    if (hKbdHook) UnhookWindowsHookEx(hKbdHook);
    if (hMenu) DestroyMenu(hMenu);
    Shell_NotifyIconW(NIM_DELETE, &nid);
    KillTimer(hMainWnd, IDT_TIMER);
    DestroyIcon(hIconSmall);
    DestroyIcon(hIconLarge);
    HeapFree(hHeap, NULL, szWinClassBuffer);
    HeapFree(hHeap, NULL, szWinTitleBuffer);

    logger.Out(L"Exit from the %s() function, msg.wParam = 0x%0*tX", TEXT(__FUNCTION__), (sizeof(UINT_PTR) * 2), static_cast<UINT_PTR>(msg.wParam));

    return static_cast<UINT_PTR>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hMainWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
        {
            logger.Out(L"%s(%d): Recived WM_CREATE message", TEXT(__FUNCTION__), __LINE__);

            nid.cbSize = sizeof(NOTIFYICONDATAW);
            nid.hWnd = hMainWnd;
            nid.uVersion = NOTIFYICON_VERSION_4;
            nid.uCallbackMessage = WM_WCW;
            nid.hIcon = hIconSmall;
            nid.uID = IDI_TRAYICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
            StringCchCopyW(nid.szTip, _countof(nid.szTip), szTitle);

            uMsgRestore = RegisterWindowMessageW(L"TaskbarCreated");
            if (!uMsgRestore) {
                logger.Out(L"%s(%d): Registering 'TaskbarCreated' message failed!", TEXT(__FUNCTION__), __LINE__);
            }
            logger.Out(L"%s(%d): The 'TaskbarCreated' message successfully registered", TEXT(__FUNCTION__), __LINE__);

            if (fCheckUpdates) {
                if (!SetTimer(hMainWnd, IDT_TIMER, (T1 * 1000 - T0 * 1000), NULL)) // 50 seconds
                {
                    logger.Out(L"%s(%d): Creating timer failed!", TEXT(__FUNCTION__), __LINE__);
                    ShowError(IDS_ERR_TIMER, szTitle);
                    fCheckUpdates = FALSE;
                }
                logger.Out(L"%s(%d): Timer successfully created (%d sec)", TEXT(__FUNCTION__), __LINE__, (T1 - T0));
            }
            break;
        }

        case WM_TIMER:
        {
            if (fCheckUpdates) {
                logger.Out(L"%s(%d): Checking for updates is enabled", TEXT(__FUNCTION__), __LINE__);

                HANDLE hUpdater = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &Updater, NULL, 0, NULL));
                if (NULL == hUpdater) {
                    DWORD dwLastError = GetLastError();
                    logger.Out(L"%s(%d): Creating Updater thread failed! Error: %d", TEXT(__FUNCTION__), __LINE__, dwLastError);
                }
                else {
                    if (!SetTimer(hMainWnd, IDT_TIMER, (T2 * 1000), NULL)) // 1 day
                    {
                        logger.Out(L"%s(%d): Creating timer failed!", TEXT(__FUNCTION__), __LINE__);
                        ShowError(IDS_ERR_TIMER, szTitle);
                        fCheckUpdates = FALSE;
                    }
                    logger.Out(L"%s(%d): Timer successfully created (%d sec)", TEXT(__FUNCTION__), __LINE__, T2);
                    CloseHandle(hUpdater);
                }
            }
            else {
                logger.Out(L"%s(%d): Checking for updates is disabled", TEXT(__FUNCTION__), __LINE__);
            }
            break;
        }

        // Popup menu handler
        case WM_WCW:
        {
            if (WM_CONTEXTMENU == LOWORD(lParam)) {
                logger.Out(L"%s(%d): Recived WM_CONTEXTMENU message", TEXT(__FUNCTION__), __LINE__);

                POINT pt{ GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam) };
                ShowPopupMenu(hMainWnd, pt);
            }
            break;
        }

        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);

            // Parse the menu selections:
            switch (wmId) {
                case ID_POPUPMENU_ICON:
                {
                    logger.Out(L"%s(%d): Pressed the 'Hide icon' menuitem", TEXT(__FUNCTION__), __LINE__);

                    fShowIcon = FALSE;
                    HandlingTrayIcon();
                    break;
                }
                case ID_POPUPMENU_AREA:
                {
                    logger.Out(L"%s(%d): Pressed the 'Use workarea' menuitem", TEXT(__FUNCTION__), __LINE__);

                    bWorkArea = !bWorkArea;
                    bWorkArea ? mii.fState = MFS_CHECKED : mii.fState = MFS_UNCHECKED;
                    SetMenuItemInfoW(hPopup, ID_POPUPMENU_AREA, FALSE, &mii);
                    logger.Out(L"%s(%d): Changed 'Use workarea' option to %s", TEXT(__FUNCTION__), __LINE__, bWorkArea ? L"True" : L"False");
                    break;
                }
                case ID_POPUPMENU_HELP:
                {
                    if (!bKPressed) {
                        logger.Out(L"%s(%d): Pressed the 'Help' menuitem", TEXT(__FUNCTION__), __LINE__);

                        bKPressed = TRUE;
                        WCHAR szHelp[MAX_LOADSTRING * 15];
                        LoadStringW(GetModuleHandleW(NULL), IDS_HELP, szHelp, _countof(szHelp));
                        MessageBoxW(hMainWnd, szHelp, szTitle, MB_OK | MB_ICONINFORMATION);
                        bKPressed = FALSE;
                    }
                    break;
                }
                case ID_POPUPMENU_ABOUT:
                {
                    if (!bKPressed) {
                        logger.Out(L"%s(%d): Pressed the 'About' menuitem", TEXT(__FUNCTION__), __LINE__);

                        bKPressed = TRUE;
                        DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_ABOUTBOX), hMainWnd, static_cast<DLGPROC>(About), 0L);
                        bKPressed = FALSE;
                    }
                    break;
                }
                case ID_POPUPMENU_EXIT:
                {
                    logger.Out(L"%s(%d): Pressed the 'Exit' menuitem", TEXT(__FUNCTION__), __LINE__);

                    DestroyWindow(hMainWnd);
                    break;
                }
            }
            break;
        }

        case WM_QUERYENDSESSION:
        {
            logger.Out(L"%s(%d): Recieved the WM_QUERYENDSESSION message, lParam = 0x%p", TEXT(__FUNCTION__), __LINE__, lParam);

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

        default:
        {
            if (message == uMsgRestore) {
                logger.Out(L"%s(%d): Recieved the 'TaskbarCreated' message", TEXT(__FUNCTION__), __LINE__);

                HandlingTrayIcon();
                break;
            }
            return DefWindowProcW(hMainWnd, message, wParam, lParam);
        }
    }
    return 0;
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (WM_MBUTTONUP == wParam) bMPressed = FALSE;
    if (WM_MBUTTONDOWN == wParam && bLCTRL && bLWIN && !bMPressed) {
        logger.Out(L"%s(%d): Pressed LCTRL + LWIN + MMB", TEXT(__FUNCTION__), __LINE__);

        bMPressed = TRUE;
        hFgWnd = GetForegroundWindow();
        if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
        else hFgWnd = NULL;
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    LPKBDLLHOOKSTRUCT pkhs = { 0 };
    pkhs = reinterpret_cast<LPKBDLLHOOKSTRUCT>(lParam);
    if (WM_KEYUP == wParam) {
        if (VK_LCONTROL == pkhs->vkCode) bLCTRL = FALSE;
        if (VK_LWIN == pkhs->vkCode) bLWIN = FALSE;
        bKPressed = FALSE;
    }

    if (WM_KEYDOWN == wParam) {
        if (VK_LCONTROL == pkhs->vkCode) bLCTRL = TRUE;
        if (VK_LWIN == pkhs->vkCode) bLWIN = TRUE;

        // 'I' key
        if (KEY_I == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed) {
            logger.Out(L"%s(%d): Pressed LCTRL + LWIN + I", TEXT(__FUNCTION__), __LINE__);

            bKPressed = TRUE;
            fShowIcon = !fShowIcon;
            HandlingTrayIcon();
            return TRUE;
        }

        // 'C' key
        if (KEY_C == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV) {
            logger.Out(L"%s(%d): Pressed LCTRL + LWIN + C", TEXT(__FUNCTION__), __LINE__);

            bKPressed = TRUE;
            hFgWnd = GetForegroundWindow();
            if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
            else hFgWnd = NULL;
            return TRUE;
        }

        // 'V' key
        if (KEY_V == pkhs->vkCode && bLCTRL && bLWIN && !bKPressed && !bKEYV) {
            logger.Out(L"%s(%d): Pressed LCTRL + LWIN + V", TEXT(__FUNCTION__), __LINE__);

            bKPressed = TRUE; bKEYV = TRUE;
            hFgWnd = GetForegroundWindow();
            if (IsWindowApprooved(hFgWnd)) {
                logger.Out(L"%s(%d): Opening the 'Manual editing' dialog", TEXT(__FUNCTION__), __LINE__);

                DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_MANUAL_EDITING), hFgWnd, static_cast<DLGPROC>(DlgProc), 0L);
                SetForegroundWindow(hFgWnd);
            }
            else hFgWnd = NULL;
            bKEYV = FALSE;
            return TRUE;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT dlgmsg, WPARAM wParam, LPARAM lParam) {
    RECT rcFW = { 0 };
    int x, y, w, h;
    switch (dlgmsg) {
        case WM_INITDIALOG:
        {
            logger.Out(L"%s(%d): Initializing the 'Manual editing' dialog", TEXT(__FUNCTION__), __LINE__);

            SetWindowTextW(hDlg, szTitle);
            GetClassNameW(hFgWnd, static_cast<LPWSTR>(szWinClassBuffer), MAX_WINTITLE_BUFFER_LENGTH);
            GetWindowRect(hFgWnd, &rcFW);
            x = rcFW.left;
            y = rcFW.top;
            w = rcFW.right - rcFW.left;
            h = rcFW.bottom - rcFW.top;
            SetDlgItemInt(hDlg, IDC_EDIT_X, x, TRUE);
            SetDlgItemInt(hDlg, IDC_EDIT_Y, y, TRUE);
            SetDlgItemInt(hDlg, IDC_EDIT_WIDTH, w, FALSE);
            SetDlgItemInt(hDlg, IDC_EDIT_HEIGHT, h, FALSE);
            SetDlgItemTextW(hDlg, IDC_EDIT_TITLE, static_cast<LPCWSTR>(szWinTitleBuffer));
            SetDlgItemTextW(hDlg, IDC_EDIT_CLASS, static_cast<LPCWSTR>(szWinClassBuffer));
            UpdateWindow(hDlg);
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
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

                    logger.Out(L"%s(%d): Window with handle 0x%p was moved to %d, %d", TEXT(__FUNCTION__), __LINE__, hFgWnd, x, y);

                    return static_cast<INT_PTR>(TRUE);
                    break;
                }
                case IDC_BUTTON_CENTER:
                {
                    logger.Out(L"%s(%d): Pressed the 'Center' button", TEXT(__FUNCTION__), __LINE__);

                    bKPressed = TRUE;
                    if (IsWindowApprooved(hFgWnd)) MoveWindowToMonitorCenter(hFgWnd, bWorkArea, FALSE);
                    else hFgWnd = NULL;
                    return static_cast<INT_PTR>(TRUE);
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
    return static_cast<INT_PTR>(FALSE);
}

BOOL IsWindowApprooved(HWND hFW) {
    logger.Out(L"Entering the %s() function", TEXT(__FUNCTION__));

    BOOL bApprooved = FALSE;
    if (hFW) {
        if (GetWindowTextW(hFW, reinterpret_cast<LPWSTR>(szWinTitleBuffer), MAX_WINTITLE_BUFFER_LENGTH)) {
            logger.Out(L"%s(%d): Window handle: 0x%p. Title: '%s'", TEXT(__FUNCTION__), __LINE__, hFW, reinterpret_cast<LPWSTR>(szWinTitleBuffer));
        }

        if (IsIconic(hFW)) {
            logger.Out(L"%s(%d): The window is iconified", TEXT(__FUNCTION__), __LINE__);
        }

        if (IsZoomed(hFW)) {
            logger.Out(L"%s(%d): The window is maximized", TEXT(__FUNCTION__), __LINE__);
        }

        LONG_PTR wlp = GetWindowLongPtrW(hFW, GWL_STYLE);
        if (wlp & WS_CAPTION) {
            if (!IsIconic(hFW) && !IsZoomed(hFW)) {
                logger.Out(L"%s(%d): The window is approved!", TEXT(__FUNCTION__), __LINE__);

                bApprooved = TRUE;
            }
            else ShowError(IDS_ERR_MAXMIN, szTitle);
        }
        else {
            logger.Out(L"%s(%d): The window has no caption!", TEXT(__FUNCTION__), __LINE__);
        }
    }

    if (!bApprooved) {
        logger.Out(L"%s(%d): The window is not approved!", TEXT(__FUNCTION__), __LINE__);
    }

    logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));

    return bApprooved;
}

VOID HandlingTrayIcon() {
    logger.Out(L"Entering the %s() function, fShowIcon = %s", TEXT(__FUNCTION__), fShowIcon ? L"True" : L"False");

    if (fShowIcon) {
        BOOL bResult1 = Shell_NotifyIconW(NIM_ADD, &nid);
        logger.Out(L"%s(%d): Shell_NotifyIconW(NIM_ADD) returned %s", TEXT(__FUNCTION__), __LINE__, bResult1 ? L"True" : L"False");

        BOOL bResult2 = Shell_NotifyIconW(NIM_SETVERSION, &nid);
        logger.Out(L"%s(%d): Shell_NotifyIconW(NIM_SETVERSION) returned %s", TEXT(__FUNCTION__), __LINE__, bResult2 ? L"True" : L"False");

        if (!bResult1 || !bResult2) {
            logger.Out(L"%s(%d): Error creating trayicon!", TEXT(__FUNCTION__), __LINE__);

            ShowError(IDS_ERR_ICON, szTitle);
            Shell_NotifyIconW(NIM_DELETE, &nid);
            fShowIcon = FALSE;
        }
    }
    else {
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }

    logger.Out(L"Exit from the %s() function", TEXT(__FUNCTION__));
}

VOID ShowError(UINT uID, LPCWSTR szAppTitle) {
    WCHAR szErrorText[MAX_LOADSTRING]; // Error's text
    LoadStringW(GetModuleHandleW(NULL), uID, szErrorText, _countof(szErrorText));
    MessageBoxW(hFgWnd, szErrorText, szAppTitle, MB_OK | MB_ICONERROR | MB_TOPMOST);
}

VOID ShowPopupMenu(HWND hMainWnd, POINT pt) {
    SetForegroundWindow(hMainWnd);
    UINT uFlags = TPM_RIGHTBUTTON;
    GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0 ? uFlags |= TPM_RIGHTALIGN : uFlags |= TPM_LEFTALIGN;
    TrackPopupMenuEx(hPopup, uFlags, pt.x, pt.y, hMainWnd, NULL);
    PostMessageW(hMainWnd, WM_NULL, 0, 0);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    INITCOMMONCONTROLSEX icex = { 0 };
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LINK_CLASS;
    InitCommonControlsEx(&icex);

    switch (message) {
        case WM_INITDIALOG:
        {
            logger.Out(L"%s(%d): Initializing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

            WCHAR szAboutProgName[MAX_LOADSTRING];
            WCHAR szAboutCopyright[MAX_LOADSTRING];
            WCHAR szAboutBuildTime[MAX_LOADSTRING];
            MultiByteToWideChar(1251, 0, PRODUCT_NAME_FULL, _countof(PRODUCT_NAME_FULL), szAboutProgName, MAX_LOADSTRING);
            MultiByteToWideChar(1251, 0, PRODUCT_COPYRIGHT, _countof(PRODUCT_COPYRIGHT), szAboutCopyright, MAX_LOADSTRING);
            MultiByteToWideChar(1251, 0, ABOUT_BUILD, _countof(ABOUT_BUILD), szAboutBuildTime, MAX_LOADSTRING);
            SetDlgItemTextW(hDlg, IDC_ABOUT_PROGNAME, szAboutProgName);
            SetDlgItemTextW(hDlg, IDC_ABOUT_COPYRIGHT, szAboutCopyright);
            SetDlgItemTextW(hDlg, IDC_ABOUT_BUILDTIME, szAboutBuildTime);
#ifdef NO_DONATION
            HWND hLink = GetDlgItem(hDlg, IDC_DONATIONLINK);
            if (hLink) DestroyWindow(hLink);
            HWND hText = GetDlgItem(hDlg, IDC_DONATIONTEXT);
            if (hText) DestroyWindow(hText);
#endif // !NO_DONATION

            logger.Out(L"%s(%d): End of initializing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

            return static_cast<INT_PTR>(TRUE);
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pNMHdr = (LPNMHDR)lParam;
            if ((NM_CLICK == pNMHdr->code || NM_RETURN == pNMHdr->code) && IDC_DONATIONLINK == pNMHdr->idFrom) {
                PNMLINK pNMLink = (PNMLINK)pNMHdr;
                LITEM item = pNMLink->item;
                ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

                logger.Out(L"%s(%d): Pressed the donation link! :-)", TEXT(__FUNCTION__), __LINE__);

                return static_cast<INT_PTR>(TRUE);
            }
            break;
        }

        case WM_COMMAND:
        {
            if (IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam)) {
                EndDialog(hDlg, LOWORD(wParam));

                logger.Out(L"%s(%d): Closing the 'About' dialog", TEXT(__FUNCTION__), __LINE__);

                return static_cast<INT_PTR>(TRUE);
            }
            break;
        }
    }
    return static_cast<INT_PTR>(FALSE);
}
