#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <string>
#include <unordered_set>
#include <iterator>
#include <cwchar>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

namespace {
constexpr wchar_t kClassName[] = L"PinTop.Native.MessageWindow";
constexpr wchar_t kAppName[] = L"PinTop";
constexpr UINT kTrayId = 1;
constexpr UINT kHotkeyId = 1001;
constexpr UINT kCleanupTimerId = 1002;
constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT IDM_TOGGLE_STARTUP = 4001;
constexpr UINT IDM_EXIT = 4002;
constexpr UINT IDM_ABOUT = 4003;
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"PinTop";
constexpr wchar_t kSettingsKey[] = L"Software\\PinTop";
constexpr wchar_t kStartupConfigured[] = L"StartupConfigured";

HWND g_window = nullptr;
HHOOK g_mouseHook = nullptr;
NOTIFYICONDATAW g_tray{};
std::unordered_set<HWND> g_pinned;
bool g_hotkeyRegistered = false;
bool g_startupEnabled = false;

void Notify(const std::wstring& title, const std::wstring& message, DWORD flags = NIIF_INFO) {
    if (!g_tray.cbSize) return;
    g_tray.uFlags = NIF_INFO;
    wcsncpy_s(g_tray.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(g_tray.szInfo, message.c_str(), _TRUNCATE);
    g_tray.dwInfoFlags = flags;
    g_tray.uTimeout = 2500;
    Shell_NotifyIconW(NIM_MODIFY, &g_tray);
    g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
}

std::wstring WindowTitle(HWND hwnd) {
    wchar_t title[256]{};
    GetWindowTextW(hwnd, title, static_cast<int>(std::size(title)));
    if (title[0]) return title;
    return L"Selected window";
}

bool IsPinned(HWND hwnd) { return g_pinned.find(hwnd) != g_pinned.end(); }

void TogglePin(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd) || hwnd == g_window) return;
    hwnd = GetAncestor(hwnd, GA_ROOT);
    if (!hwnd || !IsWindow(hwnd) || hwnd == g_window) return;

    const bool currentlyPinned = IsPinned(hwnd);
    const HWND insertAfter = currentlyPinned ? HWND_NOTOPMOST : HWND_TOPMOST;
    if (SetWindowPos(hwnd, insertAfter, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER)) {
        if (currentlyPinned) {
            g_pinned.erase(hwnd);
            Notify(kAppName, WindowTitle(hwnd) + L" — unpinned");
        } else {
            g_pinned.insert(hwnd);
            Notify(kAppName, WindowTitle(hwnd) + L" — pinned on top");
        }
    } else {
        Notify(kAppName, L"Windows could not change this window's topmost state.", NIIF_WARNING);
    }
}

bool ReadStartupEnabled() {
    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0;
    DWORD size = 0;
    const LONG result = RegQueryValueExW(key, kRunValue, nullptr, &type, nullptr, &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_SZ && size > sizeof(wchar_t);
}

bool StartupWasConfigured() {
    HKEY key{};
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD value = 0, type = 0, size = sizeof(value);
    const LONG result = RegQueryValueExW(key, kStartupConfigured, nullptr, &type,
        reinterpret_cast<BYTE*>(&value), &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_DWORD && value == 1;
}

void MarkStartupConfigured() {
    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &key, nullptr) == ERROR_SUCCESS) {
        DWORD value = 1;
        RegSetValueExW(key, kStartupConfigured, 0, REG_DWORD,
                       reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(key);
    }
}

bool SetStartup(bool enabled) {
    HKEY key{};
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;

    bool ok = false;
    if (enabled) {
        wchar_t path[MAX_PATH]{};
        if (GetModuleFileNameW(nullptr, path, MAX_PATH) > 0) {
            std::wstring command = L"\"" + std::wstring(path) + L"\"";
            const DWORD bytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
            ok = RegSetValueExW(key, kRunValue, 0, REG_SZ,
                reinterpret_cast<const BYTE*>(command.c_str()), bytes) == ERROR_SUCCESS;
        }
    } else {
        const LONG result = RegDeleteValueW(key, kRunValue);
        ok = result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
    }
    RegCloseKey(key);
    if (ok) {
        g_startupEnabled = enabled;
        MarkStartupConfigured();
    }
    return ok;
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING | (g_startupEnabled ? MF_CHECKED : 0), IDM_TOGGLE_STARTUP,
                L"Start with Windows");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_ABOUT, L"About PinTop");
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit PinTop");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(g_window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, g_window, nullptr);
    DestroyMenu(menu);
    PostMessageW(g_window, WM_NULL, 0, 0);
}

LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && wParam == WM_MBUTTONUP) {
        const auto* data = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
        POINT pt = data->pt;
        HWND target = WindowFromPoint(pt);
        if (target) {
            HWND root = GetAncestor(target, GA_ROOT);
            if (root && root != g_window) {
                LPARAM screenPoint = MAKELPARAM(static_cast<SHORT>(pt.x), static_cast<SHORT>(pt.y));
                LRESULT hit = SendMessageW(root, WM_NCHITTEST, 0, screenPoint);
                if (hit == HTCAPTION) TogglePin(root);
            }
        }
    }
    return CallNextHookEx(g_mouseHook, code, wParam, lParam);
}

void CleanupPinned() {
    for (auto it = g_pinned.begin(); it != g_pinned.end();) {
        if (!IsWindow(*it)) it = g_pinned.erase(it);
        else ++it;
    }
}

void AddTrayIcon() {
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = g_window;
    g_tray.uID = kTrayId;
    g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray.uCallbackMessage = WM_TRAYICON;
    g_tray.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(g_tray.szTip, L"PinTop — window pinning");
    Shell_NotifyIconW(NIM_ADD, &g_tray);
    g_tray.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &g_tray);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_HOTKEY:
        if (wParam == kHotkeyId) TogglePin(GetForegroundWindow());
        return 0;
    case WM_TRAYICON: {
        // With NOTIFYICON_VERSION_4, the event code is in HIWORD(lParam).
        const UINT event = g_tray.uVersion == NOTIFYICON_VERSION_4
            ? HIWORD(lParam) : LOWORD(lParam);
        if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) ShowTrayMenu();
        else if (event == WM_LBUTTONUP || event == NIN_SELECT)
            Notify(kAppName, L"Middle-click a window title bar to pin it. Right-click the tray icon for options.");
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_TOGGLE_STARTUP:
            if (SetStartup(!g_startupEnabled)) Notify(kAppName, L"Could not update startup setting.", NIIF_ERROR);
            else Notify(kAppName, g_startupEnabled ? L"PinTop will start with Windows." : L"Automatic startup disabled.");
            return 0;
        case IDM_ABOUT:
            MessageBoxW(hwnd, L"PinTop 1.0\nA tiny native Windows utility to keep windows always on top.\n\nWin + Space: toggle the active window (if Windows allows the shortcut).\nMiddle-click a window title bar: toggle pinning.\n\nUses the native Win32 API; no .NET runtime required.", kAppName, MB_OK | MB_ICONINFORMATION);
            return 0;
        case IDM_EXIT:
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == kCleanupTimerId) CleanupPinned();
        return 0;
    case WM_DESTROY:
        if (g_hotkeyRegistered) UnregisterHotKey(hwnd, kHotkeyId);
        if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
        KillTimer(hwnd, kCleanupTimerId);
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\PinTop.Native.SingleInstance");
    if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (mutex) CloseHandle(mutex);
        return 0;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);

    g_window = CreateWindowExW(WS_EX_TOOLWINDOW, kClassName, kAppName, WS_POPUP,
                               0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!g_window) {
        ReleaseMutex(mutex); CloseHandle(mutex); return 1;
    }

    AddTrayIcon();
    // Enable startup on first run only; preserve the user's later tray-menu choice.
    if (!StartupWasConfigured()) {
        if (!SetStartup(true)) Notify(kAppName, L"Could not enable automatic startup. Check your user registry permissions.", NIIF_WARNING);
    } else {
        g_startupEnabled = ReadStartupEnabled();
    }

    // Win+Space is reserved by Windows on many systems. Registration can fail;
    // title-bar middle-click remains available in that case.
    g_hotkeyRegistered = RegisterHotKey(g_window, kHotkeyId, MOD_WIN | MOD_NOREPEAT, VK_SPACE) != FALSE;
    if (g_hotkeyRegistered)
        Notify(kAppName, L"Ready. Win + Space toggles the active window; middle-click a title bar also works.");
    else
        Notify(kAppName, L"Ready. Windows has reserved Win + Space on this PC. Middle-click a title bar to pin/unpin windows.", NIIF_WARNING);

    g_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, instance, 0);
    if (!g_mouseHook) Notify(kAppName, L"Mouse hook could not be installed. Run PinTop in your normal desktop session.", NIIF_WARNING);
    SetTimer(g_window, kCleanupTimerId, 5000, nullptr);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return static_cast<int>(msg.wParam);
}
