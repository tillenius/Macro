#pragma once
#include <Windows.h>
#include <memory>
#include "systray.h"
#include "hotkeys.h"
#include "settings.h"
#include "contextmenu.h"
#include "midi.h"
#include "midicontroller.h"
#include "app.h"

namespace Gdiplus {
class Bitmap;
class Graphics;
}

constexpr int WM_USER_GOTKEY = WM_USER + 1;
constexpr int WM_USER_SYSTRAY = WM_USER + 2;
constexpr int WM_USER_RELOAD = WM_USER + 3;
constexpr int WM_USER_ALTTAB_ON = WM_USER + 5;
constexpr int WM_USER_ALTTAB_OFF = WM_USER + 6;

class MacroApp {
public:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    SysTray m_systray;
    Hotkeys m_hotkeys;
    Settings m_settings;
    ContextMenu m_contextMenu;
    Midi m_midi;
    MidiController m_midiController;
    HMENU m_hMenu = NULL;
    enum class recordState_t {IDLE, INACTIVE};
    recordState_t m_state = recordState_t::IDLE;
    std::wstring m_settingsPath;
    std::wstring m_settingsFile;
    std::vector<app_t> m_apps;
    std::unique_ptr<Gdiplus::Bitmap> m_switch_screen;

    MacroApp(HINSTANCE hInstance, HWND hWnd);
    void hotkey(int index) { m_hotkeys.execute(index); }
    void inactivate();
    void editConfigFile();
    bool reload(bool enable);

    HDC hdcBackBuffer = NULL;
    HBITMAP hbmBackBuffer = NULL;
    Gdiplus::Bitmap * bitmap = NULL;
    Gdiplus::Graphics * g1 = NULL;
};

extern std::unique_ptr<MacroApp> g_app;
std::string wstr_to_utf8(WCHAR * s);
std::string wstr_to_utf8(const std::wstring & s);
bool fileExists(LPCWSTR szPath);
