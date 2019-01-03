#ifndef __PROCESSITERATOR_H__
#define __PROCESSITERATOR_H__

#include <windows.h>
#include <tlhelp32.h>

class ProcessIterator {
private:
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool active;
public:
    ProcessIterator() : active(false) {
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
            return;

        pe32.dwSize = sizeof(PROCESSENTRY32);
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
};

#endif // __PROCESSITERATOR_H__
