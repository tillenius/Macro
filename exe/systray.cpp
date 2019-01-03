#include "systray.h"
#include "main.h"

SysTray::~SysTray() {
    if (bShow) {
        NOTIFYICONDATA iconData;
        ZeroMemory(&iconData, sizeof(NOTIFYICONDATA));

        iconData.cbSize = sizeof(NOTIFYICONDATA);
        iconData.uID = 0;
        iconData.hWnd = hWnd;

        Shell_NotifyIcon(NIM_DELETE, &iconData);
    }
}

void SysTray::Icon(int nIcon) {

    NOTIFYICONDATA iconData;

    iconData.cbSize = sizeof(NOTIFYICONDATA);
    iconData.uFlags = NIF_ICON | NIF_MESSAGE;
    iconData.uCallbackMessage = WM_USER_SYSTRAY;
    iconData.uID = 0;
    iconData.hWnd = hWnd;
    iconData.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(nIcon));

    if (bShow) {
        Shell_NotifyIcon(NIM_MODIFY, &iconData);
    } else {
        Shell_NotifyIcon(NIM_ADD, &iconData);
        bShow = true;
    }
}
