#include <Windows.h>

#pragma comment(lib, "dll.lib")

__declspec(dllimport) void Hook();
__declspec(dllimport) void Unhook();
__declspec(dllimport) LRESULT CALLBACK HookProc(int scancode, WPARAM wParam, LPARAM lParam);
