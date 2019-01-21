#include "main.h"
#include "systray.h"
#include "macro.h"
#include "hotkeys.h"
#include "settings.h"
#include "resource.h"
#include "../dll/dll.h"

#include <windows.h>
#include <memory>

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
        case WM_USER_RELOAD:
            g_app->reload(g_app->m_hotkeys.m_enabled);
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
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hWnd);
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
    if (!reload(true)) {
        m_systray.Icon(IDI_ICON1);
    }
    static NotificationThreadData data;
    data.notificationHandle = FindFirstChangeNotificationW(m_settings.m_settingsPath.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
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
    if (m_bRecord) {
        m_bRecord = false;
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
    if (m_bRecord)
        Unhook();
}

void MacroApp::record() {
    if (!m_bRecord) {
        m_systray.Icon(IDI_ICON3);
        m_macro.clear();
        Hook();
        m_bRecord = true;
    } else {
        m_systray.Icon(IDI_ICON1);
        Unhook();
        m_bRecord = false;
    }
}

void MacroApp::playback() {
    if (!m_bRecord)
        m_macro.playback(m_settings, m_macro.get());
}

void MacroApp::key(WPARAM wParam, LPARAM lParam) {
    if (lParam & 0x80000000)
        m_systray.Icon(IDI_ICON3);
    else
        m_systray.Icon(IDI_ICON2);
    m_macro.gotKey(m_settings, wParam, (DWORD) lParam);
}

void MacroApp::inactivate() {
    m_bActive = !m_bActive;
    if (m_bActive) {
        m_systray.Icon(IDI_ICON1);
        m_hotkeys.enable(m_hWnd);
    } else {
        if (m_bRecord) {
            m_systray.Icon(IDI_ICON1);
            Unhook();
            m_bRecord = false;
        }
        m_hotkeys.disable();
        m_systray.Icon(IDI_ICON4);
    }
}

void MacroApp::resetCounter() {
    m_settings.m_counter = 0;
}

void MacroApp::saveMacro() {
    OPENFILENAME ofn;
    std::vector<char> buffer(1000);

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = 0;
    ofn.hInstance = 0;
    ofn.lpstrFile = &buffer[0];
    ofn.nMaxFile = (DWORD) buffer.size() - 1;
    ofn.lpstrFilter = "All\0*.*\0";
    ofn.nFilterIndex = 0;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER;

    if (GetSaveFileName(&ofn))
        m_macro.save(ofn.lpstrFile);
}

void MacroApp::editConfigFile() {
    const std::string file = wstr_to_utf8(m_settings.m_settingsFile);

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
