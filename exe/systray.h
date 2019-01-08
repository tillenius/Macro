#pragma once

#include <windows.h>
#include <shellapi.h>

class SysTray {
private:
    HWND hWnd;
    HINSTANCE hInstance;
    bool bShow = false;

public:
    SysTray(HINSTANCE hInstance, HWND hWnd) : hWnd(hWnd), hInstance(hInstance) {}
    ~SysTray();
    void Icon(int icon);
    void Notification(const char * message);
};
