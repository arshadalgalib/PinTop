#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
constexpr UINT kPinTopSystemCommand = 0x1F00;
constexpr wchar_t kMenuLabel[] = L"Always on Top";

HHOOK g_callWndHook = nullptr;
HHOOK g_cbtHook = nullptr;

void UpdateSystemMenu(HWND hwnd, HMENU menu) {
    if (!hwnd || !menu) return;

    if (GetMenuState(menu, kPinTopSystemCommand, MF_BYCOMMAND) == static_cast<UINT>(-1)) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kPinTopSystemCommand, kMenuLabel);
    }

    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    CheckMenuItem(menu, kPinTopSystemCommand,
        MF_BYCOMMAND | ((exStyle & WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED));
}

void ToggleTopmost(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return;

    // A system-menu command applies to the root-level window.
    hwnd = GetAncestor(hwnd, GA_ROOT);
    if (!hwnd || !IsWindow(hwnd)) return;

    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    const bool currentlyTopmost = (exStyle & WS_EX_TOPMOST) != 0;

    SetWindowPos(hwnd,
        currentlyTopmost ? HWND_NOTOPMOST : HWND_TOPMOST,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        const auto* message = reinterpret_cast<const CWPSTRUCT*>(lParam);
        if (message && message->hwnd && message->message == WM_INITMENUPOPUP) {
            HMENU systemMenu = GetSystemMenu(message->hwnd, FALSE);
            if (systemMenu && reinterpret_cast<HMENU>(message->wParam) == systemMenu)
                UpdateSystemMenu(message->hwnd, systemMenu);
        }
    }
    return CallNextHookEx(g_callWndHook, code, wParam, lParam);
}

// For HCBT_SYSCOMMAND, lParam contains mouse coordinates, NOT an HWND.
// Because this hook runs on the thread processing the system menu,
// GetActiveWindow() identifies that thread's active window.
LRESULT CALLBACK CbtProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HCBT_SYSCOMMAND &&
        (static_cast<UINT>(wParam) & 0xFFF0u) == kPinTopSystemCommand) {
        HWND target = GetActiveWindow();
        if (!target) target = GetForegroundWindow();
        ToggleTopmost(target);
        return 1; // Consume the custom system-menu command.
    }
    return CallNextHookEx(g_cbtHook, code, wParam, lParam);
}

HMODULE ThisModule() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&CallWndProc), &module))
        return nullptr;
    return module;
}
}

extern "C" __declspec(dllexport) BOOL WINAPI InstallPinTopSystemMenuHook() {
    if (g_callWndHook && g_cbtHook) return TRUE;

    HMODULE module = ThisModule();
    if (!module) return FALSE;

    if (!g_callWndHook)
        g_callWndHook = SetWindowsHookExW(WH_CALLWNDPROC, CallWndProc, module, 0);
    if (!g_cbtHook)
        g_cbtHook = SetWindowsHookExW(WH_CBT, CbtProc, module, 0);

    if (!g_callWndHook || !g_cbtHook) {
        if (g_callWndHook) UnhookWindowsHookEx(g_callWndHook);
        if (g_cbtHook) UnhookWindowsHookEx(g_cbtHook);
        g_callWndHook = nullptr;
        g_cbtHook = nullptr;
        return FALSE;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void WINAPI RemovePinTopSystemMenuHook() {
    if (g_callWndHook) {
        UnhookWindowsHookEx(g_callWndHook);
        g_callWndHook = nullptr;
    }
    if (g_cbtHook) {
        UnhookWindowsHookEx(g_cbtHook);
        g_cbtHook = nullptr;
    }
}
