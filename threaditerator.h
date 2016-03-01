#ifndef __THREADITERATOR_H__
#define __THREADITERATOR_H__

#include <windows.h>
#include <tlhelp32.h>

class ThreadIterator {
protected:
  HANDLE hThreadSnap;
  THREADENTRY32 te32;
  bool active;
public:
  ThreadIterator() : active(false) {
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
      return;

    te32.dwSize = sizeof( THREADENTRY32 );
    if (Thread32First(hThreadSnap, &te32))
      active = true;
  }
  ~ThreadIterator() {
    if (hThreadSnap != INVALID_HANDLE_VALUE)
      CloseHandle(hThreadSnap);
  }
  void operator++() {
    active = (Thread32Next(hThreadSnap, &te32) == TRUE);
  }
  bool end() {
    return !active;
  }
  THREADENTRY32 *operator->() {
    return &te32;
  }
  THREADENTRY32 *operator*() {
    return &te32;
  }
        //printf( "  THREAD ID 0x%08X\n", te32.th32ThreadID ); 
        ////printf( "\n     Base priority  = %d", te32.tpBasePri ); 
        ////printf( "\n     Delta priority = %d", te32.tpDeltaPri ); 
};

#endif // __THREADITERATOR_H__
