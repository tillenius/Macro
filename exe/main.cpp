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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_HOTKEY:
            g_app->hotkey((int) wParam);
            return 0;
        case WM_USER_GOTKEY:
            g_app->key(wParam, lParam);
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
        case WM_CREATE:
            g_app = std::make_unique<MacroApp>(g_hInstance, hWnd);
            return 0;
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

    HWND hWnd = CreateWindow("keymacro", "keymacro", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
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

MacroApp::MacroApp(HINSTANCE hInstance, HWND hWnd) : m_hInstance(hInstance), m_hWnd(hWnd), m_systray(hInstance, hWnd), m_hotkeys(hWnd), m_macro(m_settings) {
    std::string errorMsg;
    if (!m_settings.readConfig(m_hotkeys, errorMsg)) {
        errorMsg = "Error reading configuration file\n" + errorMsg;
        MessageBox(0, errorMsg.c_str(), 0, MB_OK);
    }

    m_hotkeys.enable();
    m_systray.Icon(IDI_ICON1);
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
        m_macro.playback();
}

void MacroApp::key(WPARAM wParam, LPARAM lParam) {
    if (lParam & 0x80000000)
        m_systray.Icon(IDI_ICON3);
    else
        m_systray.Icon(IDI_ICON2);
    m_macro.gotKey(wParam, (DWORD) lParam);
}

void MacroApp::inactivate() {
    m_bActive = !m_bActive;
    if (m_bActive) {
        m_systray.Icon(IDI_ICON1);
        m_hotkeys.enable();
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
