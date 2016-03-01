// Dll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Dll.h"

#pragma data_seg(".hookdata")
HHOOK g_hook = NULL;
//HHOOK g_hook2 = NULL;
HWND g_hWnd = NULL;
#pragma data_seg()

#pragma comment(linker, "/SECTION:.hookdata,RWS")

//#if (_WIN32_WINNT >= 0x0400)
//#define WH_KEYBOARD_LL     13
//#define WH_MOUSE_LL        14
//#endif // (_WIN32_WINNT >= 0x0400)

HINSTANCE g_hInstance = NULL;

//LRESULT CALLBACK HookProcMouseLL(int nCode, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// DllMain
//-----------------------------------------------------------------------------
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved )
{
	switch (ul_reason_for_call) {
	  case DLL_PROCESS_ATTACH:
	  case DLL_THREAD_ATTACH:
	  case DLL_THREAD_DETACH:
	  case DLL_PROCESS_DETACH:
		  break;
	}

	g_hInstance = (HINSTANCE) hModule;
  return TRUE;
}


//-----------------------------------------------------------------------------
// Hook
//-----------------------------------------------------------------------------
DLL_API void Hook(void) {
	g_hWnd = FindWindow("keymacro", "keymacro");
	if (g_hWnd == NULL)
		MessageBox(NULL, "Cannot find macro program", 0, MB_OK);

  g_hook = SetWindowsHookEx(WH_KEYBOARD, HookProc, g_hInstance, NULL);
	if (g_hook == NULL)
		MessageBox(NULL, "SetWindowsHookEx failed", 0, MB_OK);

 // g_hook2 = SetWindowsHookEx(WH_MOUSE_LL, HookProcMouseLL, g_hInstance, NULL);
	//if (g_hook2 == NULL)
	//	MessageBox(NULL, "SetWindowsHookEx2 failed", 0, MB_OK);
}

//-----------------------------------------------------------------------------
// RemoveHook
//-----------------------------------------------------------------------------
DLL_API void Unhook(void) {
  if (g_hook != NULL) {
	  UnhookWindowsHookEx(g_hook);
    g_hook = NULL;
  }
  //if (g_hook2 != NULL) {
	 // UnhookWindowsHookEx(g_hook2);
  //  g_hook2 = NULL;
  //}
}

//-----------------------------------------------------------------------------
// HookProc
//-----------------------------------------------------------------------------
DLL_API LRESULT CALLBACK HookProc(int scancode, WPARAM wParam, LPARAM lParam) {
	if ((scancode >= 0) && !(scancode & HC_NOREMOVE)) {
		PostMessage(g_hWnd, WM_USER+1, wParam, lParam);
	}
	return CallNextHookEx(g_hook, scancode, wParam, lParam);
}

////-----------------------------------------------------------------------------
//// HookProcMouseLL
////-----------------------------------------------------------------------------
//LRESULT CALLBACK HookProcMouseLL(int nCode, WPARAM wParam, LPARAM lParam) {
//	if ((nCode >= 0) && !(nCode & HC_NOREMOVE)) {
//		PostMessage(g_hWnd, WM_USER+3, wParam, lParam);
//	}
//	return CallNextHookEx(g_hook2, nCode, wParam, lParam);
//}
