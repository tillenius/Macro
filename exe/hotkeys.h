#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

class Hotkeys {
private:

    HWND m_hWnd = NULL;

public:
    struct Entry {
        UINT fsModifiers;
        UINT vk;
        std::function<void()> fn;
        Entry(UINT fsModifiers, UINT vk, std::function<void()> fn) : fsModifiers(fsModifiers), vk(vk), fn(fn) {}
    };

    bool m_enabled = false;

    ~Hotkeys();

    void add(UINT fsModifiers, UINT vk, std::function<void()> fn) {
        m_hotkeys.push_back(Entry(fsModifiers, vk, fn));
    }

    void enable(HWND hWnd);
    void disable();
    void execute(int ID);

    std::vector<Entry> m_hotkeys;
};
