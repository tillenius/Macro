#pragma once

#include <windows.h>
#include <vector>

class Settings;

class Macro {
private:
    std::vector<DWORD> m_macro;
    size_t m_nIgnoreCount = 0;

public:
    static void playback(Settings & settings, const std::vector<DWORD> & macro);
    void gotKey(Settings & settings, WPARAM wParam, DWORD lParam);
    void clear();
    void save(const char * filename);
    void load(const char * filename);
    const std::vector<DWORD> & get() { return m_macro; };

private:
    static void insertCounter(Settings & settings, std::vector<INPUT> & lKeys);
};
