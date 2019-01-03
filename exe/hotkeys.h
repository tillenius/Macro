#pragma once

#include "macro.h"
#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

class Hotkeys {
private:
    struct Entry {
        UINT fsModifiers;
        UINT vk;
        std::function<void()> fn;
        Entry(UINT fsModifiers, UINT vk, std::function<void()> fn) : fsModifiers(fsModifiers), vk(vk), fn(fn) {}
    };

    std::vector<Entry> m_hotkeys;
    HWND m_hWnd;
    bool m_enabled = false;

public:
    Hotkeys(HWND hWnd) : m_hWnd(hWnd) {}
    ~Hotkeys();

    void add(UINT fsModifiers, UINT vk, std::function<void()> fn) {
        m_hotkeys.push_back(Entry(fsModifiers, vk, fn));
    }

    void enable();
    void disable();
    void execute(int ID) { m_hotkeys[ID].fn(); }
};
