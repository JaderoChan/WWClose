#include <string>

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include <easy_translate.hpp>
#include <keyboard_tools/keyboard_tools.hpp>
#include <keyboard_tools/key_utility.hpp>

#include <config.h>
#include "language.h"
#include "focused_window_getter.hpp"

// ============================================================
// Constants
// ============================================================

constexpr UINT WM_TRAY_ICON         = (WM_USER + 1);
constexpr UINT WM_TRIGGERED         = (WM_USER + 2);

constexpr UINT IDM_AUTOSTART        = 1001;
constexpr UINT IDM_LANG_EN          = 1002;
constexpr UINT IDM_LANG_ZH          = 1003;
constexpr UINT IDM_ABOUT            = 1004;
constexpr UINT IDM_EXIT             = 1005;

constexpr UINT        TRAY_ICON_ID  = 1;
constexpr const char* WNDCLASS_NAME = APP_TITLE "_Class";
constexpr const char* REG_APP_PATH  = "SOFTWARE\\" APP_ORGANIZATION "\\" APP_TITLE;
constexpr const char* REG_AUTOSTART = "AutoStart";
constexpr const char* REG_LANGUAGE  = "Language";
constexpr const char* REG_RUN_PATH  = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

// ============================================================
// Globals
// ============================================================

static HWND            gHwnd        = nullptr;
static NOTIFYICONDATAW gNid         = {};
static bool            gAutoStart   = false;
static Language        gLang        = LANG_EN;

// ============================================================
// Utility: UTF-8 to wide string
// ============================================================

static std::wstring toWide(const char* utf8)
{
    if (!utf8 || !*utf8)
        return L"";

    int n = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (n <= 0)
        return L"";

    std::wstring buf(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf.data(), n);
    // MultiByteToWideChar counts the null terminator in n, remove it.
    if (!buf.empty() && buf.back() == L'\0')
        buf.pop_back();

    return buf;
}

// ============================================================
// Registry helpers
// ============================================================

static bool regReadDWORD(const char* path, const char* key, DWORD& value)
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    bool ok = (RegQueryValueExA(hKey, key, nullptr, &type,
        reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);

    return ok;
}

static bool regWriteDWORD(const char* path, const char* key, DWORD value)
{
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr,
        0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return false;

    bool ok = (RegSetValueExA(hKey, key, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&value), sizeof(DWORD)) == ERROR_SUCCESS);
    RegCloseKey(hKey);

    return ok;
}

static bool regReadString(const char* path, const char* key, std::string& value)
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;

    char buf[256] = {};
    DWORD size = sizeof(buf);
    bool ok = (RegQueryValueExA(hKey, key, nullptr, nullptr,
        reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    if (ok)
        value = buf;

    return ok;
}

static bool regWriteString(const char* path, const char* key, const char* value)
{
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, 0,
        KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return false;

    bool ok = (RegSetValueExA(hKey, key, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value), static_cast<DWORD>(strlen(value) + 1)) == ERROR_SUCCESS);
    RegCloseKey(hKey);

    return ok;
}

// ============================================================
// Settings
// ============================================================

static void loadSettings()
{
    DWORD val = 0;
    if (regReadDWORD(REG_APP_PATH, REG_AUTOSTART, val))
        gAutoStart = (val != 0);

    std::string lang;
    if (regReadString(REG_APP_PATH, REG_LANGUAGE, lang))
        gLang = (lang == "ZH") ? LANG_ZH : LANG_EN;
    else
        gLang = getCurrentSystemLang();
}

// ============================================================
// Auto-start
// ============================================================

static bool applyAutoStart(bool enable)
{
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, REG_RUN_PATH, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return false;

    LONG result;
    if (enable)
        result = RegSetValueExA(hKey, APP_TITLE, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(exePath), static_cast<DWORD>(strlen(exePath) + 1));
    else
        result = RegDeleteValueA(hKey, APP_TITLE);

    RegCloseKey(hKey);
    return (result == ERROR_SUCCESS) || (!enable && result == ERROR_FILE_NOT_FOUND);
}

// ============================================================
// Tray icon
// ============================================================

static void updateTrayTooltip()
{
    wcsncpy_s(gNid.szTip, toWide(EASYTR("App.Name")).c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &gNid);
}

static bool addTrayIcon(HWND hwnd, HINSTANCE hInst)
{
    // Attempt to load the embedded application icon, fall back to the system default.
    HANDLE image = LoadImageA(hInst, "APP_ICON", IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    HICON hIcon = reinterpret_cast<HICON>(image);
    if (!hIcon)
        hIcon = LoadIconA(nullptr, IDI_APPLICATION);

    ZeroMemory(&gNid, sizeof(gNid));
    gNid.cbSize = sizeof(gNid);
    gNid.hWnd = hwnd;
    gNid.uID = TRAY_ICON_ID;
    gNid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    gNid.uCallbackMessage = WM_TRAY_ICON;
    gNid.hIcon = hIcon;
    wcsncpy_s(gNid.szTip, toWide(EASYTR("App.Name")).c_str(), _TRUNCATE);

    return Shell_NotifyIconW(NIM_ADD, &gNid) != FALSE;
}

static void removeTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &gNid);
}

// ============================================================
// Context menu
// ============================================================

static void showContextMenu(HWND hwnd)
{
    HMENU hLangMenu = CreatePopupMenu();
    AppendMenuW(hLangMenu, MF_STRING | (gLang == LANG_EN ? MF_CHECKED : MF_UNCHECKED),
        IDM_LANG_EN, toWide(EASYTR("Tray.Language.EN")).c_str());
    AppendMenuW(hLangMenu, MF_STRING | (gLang == LANG_ZH ? MF_CHECKED : MF_UNCHECKED),
        IDM_LANG_ZH, toWide(EASYTR("Tray.Language.ZH")).c_str());

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING | (gAutoStart ? MF_CHECKED : MF_UNCHECKED),
        IDM_AUTOSTART, toWide(EASYTR("Tray.AutoStart")).c_str());
    AppendMenuW(hMenu, MF_POPUP,
        reinterpret_cast<UINT_PTR>(hLangMenu), toWide(EASYTR("Tray.Language")).c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_ABOUT, toWide(EASYTR("Tray.About")).c_str());
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT, toWide(EASYTR("Tray.Exit")).c_str());

    POINT pt;
    GetCursorPos(&pt);
    // Required so the menu dismisses when the user clicks elsewhere.
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    // Required after TrackPopupMenu for proper cleanup.
    PostMessageA(hwnd, WM_NULL, 0, 0);
    // DestroyMenu recursively destroys hLangMenu as well.
    DestroyMenu(hMenu);
}

// ============================================================
// About dialog
// ============================================================

static HRESULT CALLBACK aboutDlgCallback(HWND, UINT msg, WPARAM, LPARAM lParam, LONG_PTR)
{
    // Open hyperlinks in the default browser.
    if (msg == TDN_HYPERLINK_CLICKED)
        ShellExecuteW(nullptr, L"open", reinterpret_cast<LPCWSTR>(lParam),
            nullptr, nullptr, SW_SHOWNORMAL);
    return S_OK;
}

static void showAboutDialog(HWND parent)
{
    HINSTANCE hInst = GetModuleHandleA(nullptr);

    // Load the application icon at the large system size (DPI-aware).
    HICON hIcon = static_cast<HICON>(LoadImageA(hInst, "APP_ICON", IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    if (!hIcon)
        hIcon = LoadIconA(nullptr, IDI_APPLICATION);

    std::wstring content;
    content += toWide(EASYTR("App.Description"));
    content += L"\n\n";
    content += toWide(EASYTR("About.Version"));
    content += L": ";
    content += toWide(APP_VERSION);
    content += L"\n";
    content += toWide(EASYTR("About.Author"));
    content += L": ";
    content += toWide(APP_AUTHOR);
    content += L"\n";
    content += toWide(EASYTR("About.Github"));
    content += L": <a href=\"";
    content += toWide(APP_GITHUB_URL);
    content += L"\">";
    content += toWide(APP_GITHUB_URL);
    content += L"</a>";

    std::wstring title = toWide(EASYTR("About.Title"));
    std::wstring instruction = toWide(EASYTR("App.Name"));

    TASKDIALOGCONFIG tc     = {};
    tc.cbSize               = sizeof(tc);
    tc.hwndParent           = parent;
    tc.hInstance            = hInst;
    tc.dwFlags              = TDF_ENABLE_HYPERLINKS | TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION;
    tc.dwCommonButtons      = TDCBF_OK_BUTTON;
    tc.pszWindowTitle       = title.c_str();
    tc.hMainIcon            = hIcon;
    tc.pszMainInstruction   = instruction.c_str();
    tc.pszContent           = content.c_str();
    tc.pfCallback           = aboutDlgCallback;
    tc.cxWidth              = 220;

    TaskDialogIndirect(&tc, nullptr, nullptr, nullptr);

    DestroyIcon(hIcon);
}

// ============================================================
// Window procedure
// ============================================================

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_TRIGGERED:
        {
            HWND target = reinterpret_cast<HWND>(wParam);
            if (target && target != hwnd)
                PostMessageA(target, WM_SYSCOMMAND, SC_CLOSE, 0);
            return 0;
        }

        case WM_TRAY_ICON:
        {
            switch (LOWORD(lParam))
            {
                case WM_RBUTTONUP:
                case WM_LBUTTONUP:
                    showContextMenu(hwnd);
                    break;
                case WM_LBUTTONDBLCLK:
                    showAboutDialog(hwnd);
                    break;
                default:
                    break;
            }

            return 0;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDM_AUTOSTART:
                    gAutoStart = !gAutoStart;
                    applyAutoStart(gAutoStart);
                    regWriteDWORD(REG_APP_PATH, REG_AUTOSTART, gAutoStart ? 1u : 0u);
                    break;

                case IDM_LANG_EN:
                    if (gLang != LANG_EN)
                    {
                        gLang = LANG_EN;
                        setLanguage(gLang);
                        updateTrayTooltip();
                        regWriteString(REG_APP_PATH, REG_LANGUAGE, getLanguageStringId(gLang));
                    }
                    break;

                case IDM_LANG_ZH:
                    if (gLang != LANG_ZH)
                    {
                        gLang = LANG_ZH;
                        setLanguage(gLang);
                        updateTrayTooltip();
                        regWriteString(REG_APP_PATH, REG_LANGUAGE, getLanguageStringId(gLang));
                    }
                    break;

                case IDM_ABOUT:
                    showAboutDialog(hwnd);
                    break;

                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    break;

                default:
                    break;
            }

            return 0;
        }

        case WM_DESTROY:
        {
            removeTrayIcon();
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

// ============================================================
// Keyboard hook handler
// ============================================================

static bool keyEventHandler(kbt::KeyEvent e)
{
    // Layout: [BeforeKey, Win, Ctrl, Shift, Alt, AfterKey].
    static uint32_t pressedKeys[6] = {0};

    kbt::Key key = kbt::keyFromNativeKey(e.nativeKey);

    switch (key)
    {
        case kbt::Key_Meta:
        case kbt::Key_Meta_Left:
        case kbt::Key_Meta_Right:
            pressedKeys[1] = (e.type == kbt::KET_PRESSED ? kbt::Key_Meta : 0);
            break;
        case kbt::Key_Ctrl:
        case kbt::Key_Ctrl_Left:
        case kbt::Key_Ctrl_Right:
            pressedKeys[2] = (e.type == kbt::KET_PRESSED ? kbt::Key_Ctrl : 0);
            break;
        case kbt::Key_Shift:
        case kbt::Key_Shift_Left:
        case kbt::Key_Shift_Right:
            pressedKeys[3] = (e.type == kbt::KET_PRESSED ? kbt::Key_Shift : 0);
            break;
        case kbt::Key_Alt:
        case kbt::Key_Alt_Left:
        case kbt::Key_Alt_Right:
            pressedKeys[4] = (e.type == kbt::KET_PRESSED ? kbt::Key_Alt : 0);
            break;
        default:
            if (e.type == kbt::KET_RELEASED)
            {
                pressedKeys[0] = pressedKeys[5] = 0;
                break;
            }

            if (pressedKeys[1] == 0 && pressedKeys[2] == 0 &&
                pressedKeys[3] == 0 && pressedKeys[4] == 0)
                pressedKeys[0] = key;
            else
                pressedKeys[5] = key;

            break;
    }

    // Win+W: trigger close and suppress the system hotkey.
    if (pressedKeys[1] == kbt::Key_Meta && pressedKeys[5] == kbt::Key_W &&
        pressedKeys[0] == 0             && pressedKeys[2] == 0 &&
        pressedKeys[3] == 0             && pressedKeys[4] == 0)
    {
        HWND target = getFocusedWindow();
        if (gHwnd)
            PostMessageA(gHwnd, WM_TRIGGERED, reinterpret_cast<WPARAM>(target), 0);
        return false;
    }

    return true;
}

// ============================================================
// Entry points
// ============================================================

static int run(HINSTANCE hInst)
{
    loadSettings();
    setLanguage(gLang);

    // Register hidden window class.
    WNDCLASSEXA wc = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = wndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = WNDCLASS_NAME;
    if (!RegisterClassExA(&wc))
        return 1;

    // Create an invisible, zero-size top-level window used solely for message dispatch.
    gHwnd = CreateWindowExA(0, WNDCLASS_NAME, APP_TITLE, 0,
        0, 0, 0, 0, nullptr, nullptr, hInst, nullptr);
    if (!gHwnd)
        return 1;

    if (!addTrayIcon(gHwnd, hInst))
        return 1;

    kbt::EventHookService& hooker = kbt::EventHookService::getInstance();
    if (hooker.run() != KBT_RC_SUCCESS)
        return 1;
    hooker.setEventHandler(&keyEventHandler);

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    hooker.stop();

    // Dump text ID.
    easytr::updateTranslationsFiles();

    return static_cast<int>(msg.wParam);
}

#ifdef KEEP_TERMINAL
// Console-subsystem entry point (KEEP_TERMINAL ON, development).
int main()
{
    return run(GetModuleHandleA(nullptr));
}
#else
// Windows-subsystem entry point (KEEP_TERMINAL OFF).
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    return run(hInst);
}
#endif
