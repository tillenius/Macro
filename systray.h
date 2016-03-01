#ifndef _SYSTRAY_H_
#define _SYSTRAY_H_

#include <windows.h>
#include <shellapi.h>

#define WM_USER_SYSTRAY WM_USER+2

class SysTray {
private:
	HWND hWnd;
	HINSTANCE hInstance;
	bool bShow;

public:
	SysTray::SysTray(HINSTANCE hInstance, HWND hWnd);
	SysTray::~SysTray(void);
	void SysTray::Icon(int icon);
};


#endif // _SYSTRAY_H
