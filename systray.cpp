#include "systray.h"

SysTray::SysTray(HINSTANCE hInstance, HWND hWnd)
{
	SysTray::hWnd = hWnd;
	SysTray::bShow = false;
	SysTray::hInstance = hInstance;
}

SysTray::~SysTray(void)
{
	if (bShow) {
		NOTIFYICONDATA iconData;
		ZeroMemory(&iconData, sizeof(NOTIFYICONDATA));

		iconData.cbSize = sizeof(NOTIFYICONDATA);
		iconData.uID = 0;
		iconData.hWnd = hWnd;

		Shell_NotifyIcon( NIM_DELETE, &iconData );
	}
}

void SysTray::Icon(int nIcon) {

	NOTIFYICONDATA iconData;

	iconData.cbSize = sizeof(NOTIFYICONDATA);
	iconData.uFlags = NIF_ICON | NIF_MESSAGE; // | NIF_ICON | NIF_TIP;
	iconData.uCallbackMessage = WM_USER_SYSTRAY;
	iconData.uID = 0;
	iconData.hWnd = hWnd;
	iconData.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(nIcon));	// IDI_APPLICATION

	//	strcpy( iconData.szTip, "tooltip" );

	if (bShow) {
		Shell_NotifyIcon( NIM_MODIFY, &iconData );
	}
	else {
		Shell_NotifyIcon( NIM_ADD, &iconData );
		bShow = true;
	}
}
