#pragma once

#include "app.h"
#include "midi.h"
#include "hotkeys.h"
#include <string>

namespace pybind11 {
class dict;
}

namespace Gdiplus {
class Bitmap;
}

class Settings {
public:
    std::string m_midiInterface = "BCR2000";
	std::wstring m_envdte = L"VisualStudio.DTE";
    std::wstring m_editor = L"C:\\Program Files\\Sublime Text\\sublime_text.exe";
    int m_midiChannel = 1;
};

class SettingsFile {
public:
    Settings m_settings;
    BCLMessage m_bclMessage;
    Hotkeys m_hotkeys;
    std::vector<app_t> m_apps;
    std::unordered_map<int, std::unordered_map<int, Midi::Entry>> m_channelMap; // channel -> controller -> entry

    bool load();

    std::unique_ptr<Gdiplus::Bitmap> m_switch_screen;

private:
    void setupMidiButton(BCLMessage & m, int button, int channel, int controller, const pybind11::dict & config);
    void setupMidiEncoder(BCLMessage & m, int encoder, int channel, int controller, const pybind11::dict & config);
    void setupSwitchScreen();
};
