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
constexpr int WM_USER_RELOAD = WM_USER + 3;
constexpr int WM_USER_GOTDEFERREDKEY = WM_USER + 4;

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
    std::vector<DWORD> m_waitingToPlay;
    Midi m_midi;
    HMENU m_hMenu;
    std::vector<int> m_waitFor;
    enum class recordState_t {IDLE, RECORD, INACTIVE, WAIT};
    recordState_t m_state = recordState_t::IDLE;
    std::wstring m_settingsPath;
    std::wstring m_settingsFile;

    MacroApp(HINSTANCE hInstance, HWND hWnd);
    ~MacroApp();
    void record();
    void playback(const std::vector<DWORD> & macro, bool wait);
    void hotkey(int index) { m_hotkeys.execute(index); }
    void key(WPARAM wParam, LPARAM lParam);
    void deferredKey(WPARAM wParam, LPARAM lParam);
    void inactivate();
    void resetCounter();
    void editConfigFile();
    bool reload(bool enable);
    bool hasHook() {
        return m_state == recordState_t::RECORD || m_state == recordState_t::WAIT; 
    }
};

extern std::unique_ptr<MacroApp> g_app;
std::string wstr_to_utf8(WCHAR * s);
std::string wstr_to_utf8(const std::wstring & s);
bool fileExists(LPCWSTR szPath);
