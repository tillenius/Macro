#pragma once

#include <Windows.h>

class ContextMenu {
public:
    HMENU m_hMenu;

    ContextMenu();
    void show(HWND hWnd);
    bool handleCommand(WORD id, HINSTANCE hInstance, HWND hWnd);
};
