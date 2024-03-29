#include "main.h"
#include "systray.h"
#include "hotkeys.h"
#include "settings.h"
#include "resource.h"
#include "action.h"

#include <windows.h>
#include <memory>
#include <fstream>
#include <shlobj.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib, "Gdiplus.lib")

HINSTANCE g_hInstance;
bool g_hookEnabled = false;
std::unique_ptr<MacroApp> g_app;
int g_overlay_x;
int g_overlay_y;
int g_overlay_cx;
int g_overlay_cy;
bool g_overlay_restart = false;
bool g_overlay_fullscreen = false;
DWORD g_overlay_endtime = 0;
HWND g_hWnd = NULL;
HWND g_hwndOverlay = NULL;
HWND g_hWndAltTab = NULL;
HHOOK g_hook = NULL;

bool g_alttab_window = false;
bool g_request_alttab_on = false;
bool g_request_alttab_off = false;
bool g_capslock = false;
bool g_appswitch = true;

void Hook() { g_hookEnabled = true; }
void Unhook() { g_hookEnabled = false; }

std::string wstr_to_utf8(WCHAR * s) {
    const int slength = (int) wcslen(s);
    int len = WideCharToMultiByte(CP_ACP, 0, s, slength, 0, 0, 0, 0);
    std::vector<char> buf(len);
    WideCharToMultiByte(CP_ACP, 0, s, slength, &buf[0], len, 0, 0);
    return std::string(&buf[0]);
}

std::string wstr_to_utf8(const std::wstring & s) {
    int slength = (int) s.length() + 1;
    int len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
    std::vector<char> buf(len);
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &buf[0], len, 0, 0);
    return std::string(&buf[0]);
}

bool fileExists(LPCWSTR szPath) {
    DWORD dwAttrib = GetFileAttributesW(szPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
            !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

int getTaskSwitchKey(WPARAM wParam) {
    switch (wParam) {
        case '1': return 0;
        case '2': return 1;
        case '3': return 2;
        case 'Q': return 3;
        case 'W': return 4;
        case 'E': return 5;
        case 'A': return 6;
        case 'S': return 7;
        case 'D': return 8;
        case 'Z': return 9;
        case 'X': return 10;
        case 'C': return 11;
        default: return -1;
    }
}

LRESULT CALLBACK WndProcAltTab(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_KILLFOCUS: {
            ::ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        case WM_SHOWWINDOW: {
            g_alttab_window = wParam == TRUE;
            break;
        }
        case WM_USER_ALTTAB_ON: {
            ::ShowWindow(hwnd, SW_SHOW);

            BOOL ret = ::SetForegroundWindow(hwnd);
            if (ret == FALSE) {
                // The system automatically enables calls to SetForegroundWindow if the user presses the ALT key 
                INPUT keys[2] = {0};
                keys[0].type = INPUT_KEYBOARD;
                keys[0].ki.wVk = VK_MENU;
                keys[1].type = INPUT_KEYBOARD;
                keys[1].ki.wVk = VK_MENU;
                keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(2, keys, sizeof(INPUT));
                ret = ::SetForegroundWindow(hwnd);
            }

            g_request_alttab_on = false;
            return 0;
        }
        case WM_USER_ALTTAB_OFF: {
            ::ShowWindow(hwnd, SW_HIDE);
            g_request_alttab_off = false;
            return 0;
        }
        case WM_KEYDOWN: {
            const int index = getTaskSwitchKey(wParam);
            if (0 <= index && index < g_app->m_apps.size()) {
                if (g_alttab_window) {
                    ::ShowWindow(hwnd, SW_HIDE);
                }

                app_t & app = g_app->m_apps[index];

                HWND hwndActivate = Action::findWindow(app.activate_exe, app.activate_window, app.activate_class);
                if (hwndActivate != NULL) {
                    SwitchToThisWindow(hwndActivate, TRUE);
                    Action::highlight(hwndActivate);
                    return 0;
                }
                Action::run(app.run_appname, app.run_cmdline, app.run_currdir);
            }
            return 0;
        }
        case WM_PAINT: {
            Gdiplus::Graphics graphics(hwnd);
            graphics.DrawImage(g_app->m_switch_screen.get(), 0, 0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK WndProcOverlay(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_KILLFOCUS: {
            ::DestroyWindow(hwnd);
            g_hwndOverlay = NULL;
            return 0;
        }
        case WM_TIMER: {
            ::KillTimer(hwnd, 0);
            ::DestroyWindow(hwnd);
            g_hwndOverlay = NULL;
            return 0;
        }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (hWnd == g_hWndAltTab) {
        return WndProcAltTab(hWnd, message, wParam, lParam);
    } else if (hWnd == g_hwndOverlay) {
        return WndProcOverlay(hWnd, message, wParam, lParam);
    }

    //static char keyname[80];
    //char tmp[300];
    switch (message) {
        case WM_KEYDOWN:
            //::GetKeyNameTextA((LONG) lParam, keyname, sizeof(keyname));
            //sprintf_s(tmp, "WM_KEYDOWN %d %s %d %08llx %08llx\n", (int) wParam, keyname, message, wParam, lParam);
            //OutputDebugString(tmp);
            return 0;
        case WM_KEYUP:
            //::GetKeyNameTextA((LONG) lParam, keyname, sizeof(keyname));
            //sprintf_s(tmp, "WM_KEYUP %d %s %d %08llx %08llx\n", (int) wParam, keyname, message, wParam, lParam);
            //OutputDebugString(tmp);
            return 0;
        case WM_CHAR:
            //::GetKeyNameTextA((LONG) lParam, keyname, sizeof(keyname));
            //sprintf_s(tmp, "WM_CHAR %d %s %d %08llx %08llx\n", (int) wParam, keyname, message, wParam, lParam);
            //OutputDebugString(tmp);
            return 0;
        case WM_HOTKEY:
            g_app->hotkey((int) wParam);
            return 0;
        case WM_USER_RELOAD:
            g_app->reload(true);
            return 0;
        case MM_MIM_DATA:
            g_app->m_midi.receive((DWORD) lParam);
            return 0;
        case WM_COMMAND:
            if (HIWORD(wParam) == 0) {
                if (g_app->m_contextMenu.handleCommand(LOWORD(wParam), g_hInstance, hWnd)) {
                    return 0;
                }
            }
            break;
        case WM_USER_SYSTRAY:
            switch (lParam) {
                case WM_RBUTTONDOWN:
                    g_app->m_contextMenu.show(hWnd);
                    return 0;
            }
            return 0;
        case WM_CREATE: {
            if (!g_app) {
                g_app = std::make_unique<MacroApp>(g_hInstance, hWnd);
                g_app->m_systray.Icon(IDI_ICON4);
                g_app->reload(true);
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (g_hWnd != NULL && nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT * kbdllHookStruct = (KBDLLHOOKSTRUCT *) lParam;
        if (g_appswitch) {
            if (kbdllHookStruct->vkCode == VK_CAPITAL) {
                if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0) {
                    INPUT keys[2] = { 0 };
                    keys[0].type = INPUT_KEYBOARD;
                    keys[0].ki.wVk = VK_CAPITAL;
                    keys[1].type = INPUT_KEYBOARD;
                    keys[1].ki.wVk = VK_CAPITAL;
                    keys[1].ki.dwFlags = KEYEVENTF_KEYUP;
                    SendInput(2, keys, sizeof(INPUT));
                }
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    g_capslock = true;
                    if (!g_alttab_window && !g_request_alttab_on) {
                        g_request_alttab_on = true;
                        PostMessage(g_hWndAltTab, WM_USER_ALTTAB_ON, 0, 0);
                    }
                } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                    g_capslock = false;
                    if (g_alttab_window && !g_request_alttab_off) {
                        g_request_alttab_off = true;
                        PostMessage(g_hWndAltTab, WM_USER_ALTTAB_OFF, 0, 0);
                    }
                }
                return 1;
            }
            if (g_capslock && wParam == WM_KEYDOWN) {
                //if ((GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0) 
                //if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
                {
                    if (getTaskSwitchKey(kbdllHookStruct->vkCode) != -1) {
                        PostMessage(g_hWndAltTab, WM_KEYDOWN, kbdllHookStruct->vkCode, 0);
                        return 1;
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    if (FindWindow("keymacro", "keymacro") != NULL) {
        MessageBox(0, "Macro is already running", 0, MB_OK);
        return 0;
    }

    g_hInstance = hInstance;

    WNDCLASS wClass;
    wClass.lpszClassName = "keymacro";
    wClass.hInstance = hInstance;
    wClass.lpfnWndProc = WndProc;
    wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wClass.hIcon = 0;
    wClass.lpszMenuName = NULL;
    wClass.hbrBackground = 0;
    wClass.style = 0;
    wClass.cbClsExtra = 0;
    wClass.cbWndExtra = 0;
    if (RegisterClass(&wClass) == 0) {
        MessageBox(0, "RegisterClass() failed", 0, MB_OK);
        return 0;
    }

	/* TODO: check destructors of globals like Application... */
    struct GdiPlus {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiPlus() {
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        }
        ~GdiPlus() {
            g_app->m_switch_screen.reset();
            GdiplusShutdown(gdiplusToken);
        }
    } gdiplus;

    g_hWnd = CreateWindow("keymacro", "keymacro", WS_OVERLAPPEDWINDOW, 0, 0, 400, 200, NULL, NULL, hInstance, NULL);
    if (g_hWnd == NULL) {
        MessageBox(0, "CreateWindow failed", 0, MB_OK);
        return 0;
    }

    if (g_appswitch) {
        const int width = g_app->m_switch_screen->GetWidth();
        const int height = g_app->m_switch_screen->GetHeight();
        const int cx = ::GetSystemMetrics(SM_CXSCREEN);
        const int cy = ::GetSystemMetrics(SM_CYSCREEN);
        g_hWndAltTab = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "keymacro", "keymacro-alttab", WS_POPUP, (cx - width) / 2, (cy - height) / 2, width, height, NULL, NULL, hInstance, NULL);
        if (g_hWndAltTab == NULL) {
            MessageBox(0, "CreateWindow3 failed", 0, MB_OK);
            return 0;
        }
    }

    struct WindowsHook {
        WindowsHook(HINSTANCE hInstance) {
            if (g_appswitch) {
                g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, NULL);
                if (g_hook == NULL) {
                    MessageBox(NULL, "SetWindowsHookEx failed", 0, MB_OK);
                }
            }
        }
        ~WindowsHook() {
            if (g_hook != NULL) {
                UnhookWindowsHookEx(g_hook);
            }
        }
    } windowshook(hInstance);

    MSG msg;
    while (BOOL bRet = GetMessage(&msg, NULL /*g_hWnd*/, 0, 0) != 0) {
        if (bRet == -1) {
            return 0;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

struct NotificationThreadData {
    HANDLE notificationHandle;
    HWND hwnd;
};

DWORD WINAPI FileChangeNotificationThread(LPVOID lpParam) {
    NotificationThreadData * data = (NotificationThreadData *) lpParam;
    while (WaitForSingleObject(data->notificationHandle, INFINITE) == WAIT_OBJECT_0) {
        // reset
        if (FindNextChangeNotification(data->notificationHandle) == FALSE) {
            return 0;
        }
        // keep eating notifications until a short timeout times out
        for (DWORD ret = WAIT_OBJECT_0; ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT; ret = WaitForSingleObject(data->notificationHandle, 500)) {
            if (ret == WAIT_TIMEOUT) {
                break;
            }
            if (FindNextChangeNotification(data->notificationHandle) == FALSE) {
                return 0;
            }
        }
        PostMessage(data->hwnd, WM_USER_RELOAD, 0, 0);
    }
    return 0;
}

MacroApp::MacroApp(HINSTANCE hInstance, HWND hWnd) : m_hInstance(hInstance), m_hWnd(hWnd), m_systray(hInstance, hWnd) {
    static NotificationThreadData data;
    WCHAR * filepath;
    if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &filepath))) {
        MessageBox(0, "SHGetKnownFolderPath() failed.", 0, MB_OK);
        return;
    }
    m_settingsPath = filepath;
    m_settingsPath += L"\\Macro";
    CreateDirectoryW(m_settingsPath.c_str(), NULL);
    m_settingsFile = m_settingsPath + L"\\macro-settings.py";

    if (!fileExists(m_settingsFile.c_str())) {
        std::ofstream out(m_settingsFile.c_str());
        extern const char * DEFAULT_CONFIG_FILE;
        out << DEFAULT_CONFIG_FILE;
        out.close();
    }

    data.notificationHandle = FindFirstChangeNotificationW(m_settingsPath.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (data.notificationHandle != INVALID_HANDLE_VALUE) {
        data.hwnd = m_hWnd;
        CreateThread(NULL, 0, FileChangeNotificationThread, &data, 0, NULL);
    }
}

bool MacroApp::reload(bool enable) {
    SettingsFile newSettings;
    if (!newSettings.load()) {
        return false;
    }

    // disable hotkeys
    if (m_hotkeys.m_enabled) {
        m_hotkeys.disable();
    }

    // copy new settings over
    std::swap(newSettings.m_settings, m_settings);
    std::swap(newSettings.m_hotkeys, m_hotkeys);
    std::swap(newSettings.m_apps, m_apps);
    std::swap(newSettings.m_switch_screen, m_switch_screen);

    // enable new hotkeys
    if (enable) {
        m_hotkeys.enable(m_hWnd);
        m_systray.Icon(IDI_ICON1);
    } else {
        m_systray.Icon(IDI_ICON4);
    }

    // enable new midi config
    m_midi.init(m_hWnd, m_settings.m_midiInterface);
    if (m_settings.m_midiInterface == "BCR2000") {
        m_midi.sendMessage(newSettings.m_bclMessage);
    }
    m_midi.setChannelMap(newSettings.m_channelMap);

    if (g_hWndAltTab != NULL) {
        const int width = m_switch_screen->GetWidth();
        const int height = m_switch_screen->GetHeight();
        const int cx = ::GetSystemMetrics(SM_CXSCREEN);
        const int cy = ::GetSystemMetrics(SM_CYSCREEN);
        ::MoveWindow(g_hWndAltTab, (cx - width) / 2, (cy - height) / 2, width, height, FALSE);
    }

    return true;
}

void MacroApp::inactivate() {
    if (m_state == recordState_t::INACTIVE) {
        m_systray.Icon(IDI_ICON1);
        m_hotkeys.enable(m_hWnd);
        m_state = recordState_t::IDLE;
    } else {
        m_hotkeys.disable();
        m_systray.Icon(IDI_ICON4);
        m_state = recordState_t::INACTIVE;
    }
}

void MacroApp::editConfigFile() {
    PROCESS_INFORMATION lpProcessInfo{0};
    STARTUPINFOW siStartupInfo{0};
    siStartupInfo.cb = sizeof(STARTUPINFO);
    const std::wstring editor = m_settings.m_editor;
    if (CreateProcessW(editor.c_str(),
        (LPWSTR) (std::wstring(L"\"") + editor + L"\" \"" + m_settingsFile + L"\"").c_str(), NULL, NULL, FALSE, NULL, NULL, NULL, &siStartupInfo, &lpProcessInfo)) {
        CloseHandle(lpProcessInfo.hThread);
        CloseHandle(lpProcessInfo.hProcess);
    }
}
