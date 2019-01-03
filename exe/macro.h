#pragma once

#include <windows.h>
#include <vector>

class Settings;

class Macro {
private:
    std::vector<DWORD> m_macro;
    size_t m_nIgnoreCount = 0;
    Settings & m_settings;

public:
    Macro(Settings & settings) : m_settings(settings) {}

    void playback();
    void gotKey(WPARAM wParam, DWORD lParam);
    void clear();
    void save(const char * filename);
    void load(const char * filename);

private:
    void insertCounter(std::vector<INPUT> &lKeys);
};
