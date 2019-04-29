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

void SysTray::Notification(const wchar_t * message) {
    NOTIFYICONDATAW iconData;

    iconData.cbSize = sizeof(NOTIFYICONDATAW);
    iconData.uFlags = NIF_INFO;
    iconData.uID = 0;
    iconData.hWnd = hWnd;
    iconData.szInfoTitle[0] = '\0';
    iconData.szInfo[0] = '\0';
    iconData.uTimeout = 0;
    Shell_NotifyIconW(NIM_MODIFY, &iconData);
    wcscpy_s(iconData.szInfoTitle, L"Macro");
    wcsncpy_s(iconData.szInfo, message, sizeof(iconData.szInfo)/sizeof(iconData.szInfo[0]));
    Shell_NotifyIconW(NIM_MODIFY, &iconData);
}
