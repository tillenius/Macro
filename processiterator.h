#ifndef __PROCESSITERATOR_H__
#define __PROCESSITERATOR_H__

#include <windows.h>
#include <tlhelp32.h>

class ProcessIterator {
protected:
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  bool active;
public:
  ProcessIterator() : active(false) {
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
      return;

    pe32.dwSize = sizeof( PROCESSENTRY32 );
    if (Process32First(hProcessSnap, &pe32))
      active = true;
  }
  ~ProcessIterator() {
    if (hProcessSnap != INVALID_HANDLE_VALUE)
      CloseHandle(hProcessSnap);
  }
  void operator++() {
    active = (Process32Next(hProcessSnap, &pe32) == TRUE);
  }
  bool end() {
    return !active;
  }
  PROCESSENTRY32 *operator->() {
    return &pe32;
  }
  PROCESSENTRY32 *operator*() {
    return &pe32;
  }
      //printf("PID 0x%08X   PPID 0x%08X   #Threads %d\n", 
      //  pe32.th32ProcessID,
      //  pe32.th32ParentProcessID,
      //  pe32.cntThreads );

};

#endif // __PROCESSITERATOR_H__
