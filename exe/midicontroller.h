#pragma once

#include <string>

class MidiController {
public:
    static constexpr int NUM_CLIPBOARDS = 4;

    MidiController();
    bool Handle(int type, int channel, int controller, int data);
    void SetClipboard(int index);
    void PasteClipboard(int index);

    HWND m_hwnd = NULL;
    bool m_lctrl = false;
    bool m_rctrl = false;
    bool m_lshift = false;
    bool m_rshift = false;
    bool m_breakpoints = true;
    std::wstring m_clipboard[NUM_CLIPBOARDS];
};
