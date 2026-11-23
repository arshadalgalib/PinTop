#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
constexpr UINT kPinTopSystemCommand = 0x1F00; // WM_SYSCOMMAND IDs use the low four bits.
constexpr wchar_t kMenuLabel[] = L"Always on Top";
HHOOK g_callWndHook = nullptr;

void UpdateSystemMenu(HWND hwnd, HMENU menu) {
    if (!hwnd || !menu) return;
    const int existing = GetMenuState(menu, kPinTopSystemCommand, MF_BYCOMMAND);
    if (existing == static_cast<int>(-1)) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, kPinTopSystemCommand, kMenuLabel);
    }

    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    CheckMenuItem(menu, kPinTopSystemCommand,
                  MF_BYCOMMAND | ((exStyle & WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED));
}

LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        const auto* message = reinterpret_cast<const CWPSTRUCT*>(lParam);
        if (message && message->hwnd) {
            if (message->message == WM_INITMENUPOPUP) {
                HMENU systemMenu = GetSystemMenu(message->hwnd, FALSE);
                if (systemMenu && reinterpret_cast<HMENU>(message->wParam) == systemMenu)
                    UpdateSystemMenu(message->hwnd, systemMenu);
            } else if (message->message == WM_SYSCOMMAND &&
                       (message->wParam & 0xFFF0) == kPinTopSystemCommand) {
                HWND hwnd = message->hwnd;
                const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
                const bool currentlyTopmost = (exStyle & WS_EX_TOPMOST) != 0;
                SetWindowPos(hwnd, currentlyTopmost ? HWND_NOTOPMOST : HWND_TOPMOST,
                             0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
            }
        }
    }
    return CallNextHookEx(g_callWndHook, code, wParam, lParam);
}
}

extern "C" __declspec(dllexport) BOOL WINAPI InstallPinTopSystemMenuHook() {
    if (g_callWndHook) return TRUE;
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&CallWndProc), &module))
        return FALSE;
    g_callWndHook = SetWindowsHookExW(WH_CALLWNDPROC, CallWndProc, module, 0);
    return g_callWndHook != nullptr;
}

extern "C" __declspec(dllexport) void WINAPI RemovePinTopSystemMenuHook() {
    if (g_callWndHook) {
        UnhookWindowsHookEx(g_callWndHook);
        g_callWndHook = nullptr;
    }
}
