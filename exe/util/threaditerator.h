#ifndef __THREADITERATOR_H__
#define __THREADITERATOR_H__

#include <windows.h>
#include <tlhelp32.h>

class ThreadIterator {
private:
    HANDLE hThreadSnap;
    THREADENTRY32 te32;
    bool active;
public:
    ThreadIterator() : active(false) {
        hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap == INVALID_HANDLE_VALUE)
            return;

        te32.dwSize = sizeof(THREADENTRY32);
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
};

#endif // __THREADITERATOR_H__
