#include "focused_window_getter.hpp"

static bool isDesktopWindow(HWND hwnd)
{
    wchar_t cls[64] = {};
    GetClassNameW(hwnd, cls, 64);
    return wcscmp(cls, L"Progman") == 0 || wcscmp(cls, L"WorkerW") == 0;
}

HWND getFocusedWindow()
{
    HWND hwnd = GetForegroundWindow();
    // When on the desktop (no foreground window, or the foreground window is
    // the shell desktop), fall back to the taskbar window. Shell_TrayWnd
    // handles WM_SYSCOMMAND SC_CLOSE by showing the shutdown dialog, which
    // is the same behaviour as pressing Alt+F4 on the desktop.
    if (!hwnd || isDesktopWindow(hwnd))
        hwnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    return hwnd;
}
