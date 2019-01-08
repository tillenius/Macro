#include "hotkeys.h"
#include "action.h"
#include "keynames.h"

Hotkeys::~Hotkeys() {
    if (m_enabled)
        disable();
}

void Hotkeys::enable(HWND hWnd) {
    m_hWnd = hWnd;
    std::string errorMsg;
    for (int i = 0; i < m_hotkeys.size(); i++) {
        if (RegisterHotKey(m_hWnd, i, m_hotkeys[i].fsModifiers, m_hotkeys[i].vk) == FALSE) {
            if (errorMsg.empty()) {
                errorMsg = "Failed to register the following hotkeys:\n";
            }
            errorMsg += "Hotkey #" + std::to_string(i) + ": " + Keynames::getName(m_hotkeys[i].fsModifiers, m_hotkeys[i].vk) + "\n";
        }
    }

    m_enabled = true;

    if (!errorMsg.empty())
        MessageBox(0, errorMsg.c_str(), 0, MB_OK);
}

void Hotkeys::disable() {
    for (int i = 0; i < m_hotkeys.size(); i++)
        UnregisterHotKey(m_hWnd, i);

    m_enabled = false;
}
