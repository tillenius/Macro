#include "main.h"
#include "systray.h"
#include "macro.h"
#include "hotkeys.h"
#include "settings.h"
#include "resource.h"
#include "../dll/dll.h"

#include <windows.h>
#include <memory>
#include <fstream>
#include <shlobj.h>

HINSTANCE g_hInstance;
std::unique_ptr<MacroApp> g_app;

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static char keyname[80];
    switch (message) {
        case WM_KEYDOWN:
            ::GetKeyNameTextA((LONG) lParam, keyname, sizeof(keyname));
            OutputDebugString((std::string("WM_KEYDOWN ") + std::to_string((int) wParam) + " " + keyname + "\n").c_str());
            return 0;
        case WM_KEYUP:
            ::GetKeyNameTextA((LONG) lParam, keyname, sizeof(keyname));
            OutputDebugString((std::string("WM_KEYUP ") + std::to_string((int) wParam) + " " + keyname + "\n").c_str());
            return 0;
        case WM_HOTKEY:
            g_app->hotkey((int) wParam);
            return 0;
        case WM_USER_GOTKEY:
            g_app->key(wParam, lParam);
            return 0;
        case WM_USER_GOTDEFERREDKEY:
            g_app->deferredKey(wParam, lParam);
            return 0;
        case WM_USER_RELOAD:
            g_app->reload(true);
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
            g_app = std::make_unique<MacroApp>(g_hInstance, hWnd);
            g_app->m_systray.Icon(IDI_ICON4);
            g_app->reload(true);
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

    HWND hWnd = CreateWindow("keymacro", "keymacro", WS_OVERLAPPEDWINDOW, 0, 0, 400, 200, NULL, NULL, hInstance, NULL);
    if (hWnd == NULL) {
        MessageBox(0, "CreateWindow failed", 0, MB_OK);
        return 0;
    }

    MSG msg;
    while (BOOL bRet = GetMessage(&msg, hWnd, 0, 0) != 0) {
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

    // stop recording if currently so
    if (m_state == recordState_t::RECORD) {
        m_state = recordState_t::IDLE;
        Unhook();
        m_macro.clear();
    }

    // disable hotkeys
    if (m_hotkeys.m_enabled) {
        m_hotkeys.disable();
    }

    // copy new settings over
    std::swap(newSettings.m_settings, m_settings);
    std::swap(newSettings.m_hotkeys, m_hotkeys);

    // enable new hotkeys
    if (enable) {
        m_hotkeys.enable(m_hWnd);
        m_systray.Icon(IDI_ICON1);
    } else {
        m_systray.Icon(IDI_ICON4);
    }

    // enable new midi config
    m_midi.init(m_settings.m_midiInterface);
    m_midi.sendMessage(newSettings.m_bclMessage);
    m_midi.setChannelMap(newSettings.m_channelMap);

    return true;
}

MacroApp::~MacroApp() {
    if (m_state == recordState_t::RECORD)
        Unhook();
}

void MacroApp::record() {
    if (m_state == recordState_t::RECORD) {
        m_systray.Icon(IDI_ICON1);
        Unhook();
        m_state = recordState_t::IDLE;
    } else {
        m_systray.Icon(IDI_ICON3);
        m_macro.clear();
        if (!hasHook()) {
            Hook();
        }
        m_state = recordState_t::RECORD;
    }
}

void MacroApp::playback(const std::vector<DWORD> & macro, bool wait) {
    if (wait) {
        if (m_state != recordState_t::IDLE) {
            return;
        }

        for (int i = 8; i < 256; ++i) {
            if (i == 0x10 || i == 0x11 || i == 0x12) {
                // ignore VK_SHIFT, VK_CONTROL and VK_MENU.
                // care about VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU instead
                continue;
            }
            if ((::GetAsyncKeyState(i) & 0x8000) != 0) {
                m_waitFor.push_back(i);
            }
        }

        if (m_waitFor.empty()) {
            Macro::playback(m_settings, macro);
        } else {
            Hook();
            m_state = recordState_t::WAIT;
            m_waitingToPlay = macro;
        }
        return;
    }
    else {
        if (!hasHook())
            Macro::playback(m_settings, macro);
    }
}

void MacroApp::deferredKey(WPARAM wParam, LPARAM lParam) {
    bool pressed = (lParam & (1 << 31)) == 0;
    if (pressed) {
        // skip if already waiting for the pressed key
        for (int i = 0; i < m_waitFor.size(); ++i) {
            if (m_waitFor[i] == wParam) {
                return;
            }
        }

        // start waiting for newly pressed key
        m_waitFor.push_back((int) wParam);
        return;
    }

    for (int i = 0; i < m_waitFor.size(); ) {
        if (m_waitFor[i] == wParam || ::GetAsyncKeyState(m_waitFor[i]) == 0) {
            if (i != m_waitFor.size() - 1) {
                m_waitFor[i] = m_waitFor.back();
            }
            m_waitFor.pop_back();
        } else {
            ++i;
        }
    }

    if (m_waitFor.empty()) {
        m_state = recordState_t::IDLE;
        Unhook();
        Macro::playback(m_settings, m_waitingToPlay);
    }
}

void MacroApp::key(WPARAM wParam, LPARAM lParam) {
    if (m_state == recordState_t::WAIT) {
        // ::GetAsyncKeyState() might not be updated with this key event.
        // Hopefully reposting it ensures that ::GetAsyncKeyState is up to date.
        //OutputDebugString((std::string("Key: ") + std::to_string(wParam) + " GetAsyncKeyState: " + std::to_string(::GetAsyncKeyState(wParam)) + "\n").c_str());
        PostMessage(m_hWnd, WM_USER_GOTDEFERREDKEY, wParam, lParam);
        return;
    }

    if (lParam & 0x80000000)
        m_systray.Icon(IDI_ICON3);
    else
        m_systray.Icon(IDI_ICON2);
    m_macro.gotKey(m_settings, wParam, (DWORD) lParam);
}

void MacroApp::inactivate() {
    if (m_state == recordState_t::INACTIVE) {
        m_systray.Icon(IDI_ICON1);
        m_hotkeys.enable(m_hWnd);
        m_state = recordState_t::IDLE;
    } else {
        if (m_state == recordState_t::RECORD) {
            Unhook();
        }
        m_hotkeys.disable();
        m_systray.Icon(IDI_ICON4);
        m_state = recordState_t::INACTIVE;
    }
}

void MacroApp::resetCounter() {
    m_settings.m_counter = 0;
}

void MacroApp::editConfigFile() {
    const std::string file = wstr_to_utf8(m_settingsFile);

    PROCESS_INFORMATION lpProcessInfo{0};
    STARTUPINFO siStartupInfo{0};
    siStartupInfo.cb = sizeof(STARTUPINFO);
    const std::string editor = "C:\\Program Files\\Sublime Text 3\\sublime_text.exe"; // @TODO
    if (CreateProcess(editor.c_str(),
        (LPSTR) (std::string("\"") + editor + "\" \"" + file + "\"").c_str(), NULL, NULL, FALSE, NULL, NULL, NULL, &siStartupInfo, &lpProcessInfo)) {
        CloseHandle(lpProcessInfo.hThread);
        CloseHandle(lpProcessInfo.hProcess);
    }
}
