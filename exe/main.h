#pragma once
#include <Windows.h>
#include <memory>
#include "macro.h"
#include "systray.h"
#include "hotkeys.h"
#include "settings.h"
#include "contextmenu.h"
#include "settingsdlg.h"
#include "midi.h"

constexpr int WM_USER_GOTKEY = WM_USER + 1;
constexpr int WM_USER_SYSTRAY = WM_USER + 2;

class MacroApp {
public:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    SysTray m_systray;
    Hotkeys m_hotkeys;
    Settings m_settings;
    ContextMenu m_contextMenu;
    SettingsDlg m_settingsDlg;
    Macro m_macro;
    Midi m_midi;
    HMENU m_hMenu;
    bool m_bRecord = false;
    bool m_bActive = true;

    MacroApp(HINSTANCE hInstance, HWND hWnd);
    ~MacroApp();
    void record();
    void playback();
    void hotkey(int index) { m_hotkeys.execute(index); }
    void key(WPARAM wParam, LPARAM lParam);
    void inactivate();
    void resetCounter();
    void saveMacro();
};

extern std::unique_ptr<MacroApp> g_app;
