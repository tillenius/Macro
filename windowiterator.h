#ifndef __WINDOWITERATOR_H__
#define __WINDOWITERATOR_H__

#include <windows.h>

class WindowIterator {
private:
  HWND hWindow;
public:
  WindowIterator() {
    hWindow = ::GetTopWindow(0);
  }
  void operator++() {
    hWindow = ::GetNextWindow(hWindow, GW_HWNDNEXT);
  }
  bool end() {
    return hWindow == NULL;
  }
  HWND getHWND() {
    return hWindow;
  }
  DWORD getThreadId() {
    return ::GetWindowThreadProcessId(hWindow, NULL);
  }
  DWORD getProcessId() {
    DWORD dwPID;
    ::GetWindowThreadProcessId(hWindow, &dwPID);
    return dwPID;
  }
//      printf( "HWND = 0x%08X   ProcessID = 0x%08X   ThreadID = 0x%08X\n", h, pid, dwThreadID );
};

#endif // __WINDOWITERATOR_H__
