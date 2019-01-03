#include <Windows.h>

#pragma data_seg(".hookdata")
HHOOK g_hook = NULL;
HWND g_hWnd = NULL;
#pragma data_seg()

#pragma comment(linker, "/SECTION:.hookdata,RWS")

HINSTANCE g_hInstance = NULL;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    g_hInstance = (HINSTANCE) hModule;
    return TRUE;
}

LRESULT CALLBACK HookProc(int scancode, WPARAM wParam, LPARAM lParam) {
    if ((scancode >= 0) && !(scancode & HC_NOREMOVE)) {
        PostMessage(g_hWnd, WM_USER + 1, wParam, lParam);
    }
    return CallNextHookEx(g_hook, scancode, wParam, lParam);
}

__declspec(dllexport) void Hook() {
    g_hWnd = FindWindow("keymacro", "keymacro");
    if (g_hWnd == NULL)
        MessageBox(NULL, "Cannot find macro program", 0, MB_OK);

    g_hook = SetWindowsHookEx(WH_KEYBOARD, HookProc, g_hInstance, NULL);
    if (g_hook == NULL)
        MessageBox(NULL, "SetWindowsHookEx failed", 0, MB_OK);
}

__declspec(dllexport) void Unhook() {
    if (g_hook != NULL) {
        UnhookWindowsHookEx(g_hook);
        g_hook = NULL;
    }
}
