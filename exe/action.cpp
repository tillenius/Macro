#include "action.h"
#include "main.h"
#include "macro.h"
#include "systray.h"
#include "resource.h"
#include "../dll/dll.h"
#include "util/threaditerator.h"
#include "util/windowiterator.h"
#include "util/processiterator.h"
#include "util/wildcards.h"

#include <vector>
#include <string>
#include "psapi.h"

void Action::getThreadIds(DWORD pid, std::vector<DWORD> & threadids) {
    for (ThreadIterator i; !i.end(); ++i)
        if (i->th32OwnerProcessID == pid)
            threadids.push_back(i->th32ThreadID);
}

void Action::getWindowsFromThread(DWORD threadid, std::vector<HWND> & hwnds) {
    for (WindowIterator i; !i.end(); ++i) {
        if (i.getThreadId() == threadid)
            hwnds.push_back(i.getHWND());
    }
}

void Action::getPidsFromExe(const std::string & exeName, std::vector<DWORD> & pids) {
    for (ProcessIterator i; !i.end(); ++i)
        if (Wildcards::match(exeName, i->szExeFile))
            pids.push_back(i->th32ProcessID);
}

void Action::getWindowsFromPid(DWORD dwPid, std::vector<HWND> &hwnds) {
    std::vector<DWORD> dwThreads;
    getThreadIds(dwPid, dwThreads);
    for (size_t i = 0; i < dwThreads.size(); ++i)
        getWindowsFromThread(dwThreads[i], hwnds);
}

void Action::getWindowsFromExe(const std::string & exeName, std::vector<HWND> & hwnds) {
    std::vector<DWORD> pids;
    getPidsFromExe(exeName, pids);

    for (size_t i = 0; i < pids.size(); ++i)
        getWindowsFromPid(pids[i], hwnds);
    return;
}

bool Action::isMainWindow(HWND hWnd) {
    if ((GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE) == 0) {
        return false;
    }
    return GetAncestor(hWnd, GA_ROOT) == hWnd;
}

HWND Action::getMainWindowFromThread(DWORD threadid) {
    for (WindowIterator i; !i.end(); ++i) {
        if (i.getThreadId() == threadid) {
            HWND hwnd = i.getHWND();
            if (isMainWindow(hwnd))
                return hwnd;
        }
    }
    return NULL;
}

HWND Action::getMainWindowFromPid(DWORD pid) {
    std::vector<DWORD> threadids;
    Action::getThreadIds(pid, threadids);
    for (size_t i = 0; i < threadids.size(); ++i) {
        HWND hwnd = getMainWindowFromThread(threadids[i]);
        if (hwnd != NULL) {
            return hwnd;
        }
    }
    return NULL;
}

HWND Action::findWindow(const std::string & exeName, const std::string & windowName, const std::string & className) {
    std::vector<HWND> hwnds;
    const bool allExes = exeName == "*";
    const bool allClasses = className == "*";
    const bool allTitles = windowName == "*";
    if (allExes) {
        for (WindowIterator i; !i.end(); ++i)
            if (isMainWindow(i.getHWND()))
                hwnds.push_back(i.getHWND());
    } else if (!allClasses) {
        HWND hWnd = ::FindWindowEx(NULL, NULL, className.c_str(), NULL);
        while (hWnd != NULL) {
            if ((GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE) != 0) {
                DWORD processid;
                DWORD threadid = GetWindowThreadProcessId(hWnd, &processid);
                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processid);
                std::vector<char> filename(1024);
                ::GetModuleFileNameEx(hProc, NULL, filename.data(), (DWORD) filename.size());
                std::string fname = filename.data();
                if (fname.ends_with(exeName)) {
                    hwnds.push_back(hWnd);
                }
                CloseHandle(hProc);
            }
            hWnd = ::FindWindowEx(NULL, hWnd, className.c_str(), NULL);
        }
    } else {
        std::vector<DWORD> pids;
        Action::getPidsFromExe(exeName, pids);
        for (size_t i = 0; i < pids.size(); ++i) {
            std::vector<DWORD> threads;
            Action::getThreadIds(pids[i], threads);
            for (size_t j = 0; j < threads.size(); ++j) {
                HWND hwnd = Action::getMainWindowFromThread(threads[j]);
                if (hwnd != NULL) {
                    hwnds.push_back(hwnd);
                }
            }
        }
    }

    if (hwnds.size() == 0)
        return NULL;

    if (hwnds.size() == 1 && allTitles)
        return hwnds[0];

    std::vector<HWND> accepted;

    for (size_t i = 0; i < hwnds.size(); ++i) {

        if (!allTitles) {
            int size = GetWindowTextLength(hwnds[i]);
            std::vector<char> titleCurr(size + 2);
            GetWindowText(hwnds[i], &titleCurr[0], size + 1);
            if (!Wildcards::match(windowName, &titleCurr[0]))
                continue;
        }

        if (allExes && !allClasses) {
            std::vector<char> cls(260);
            GetClassName(hwnds[i], &cls[0], 248);
            if (!Wildcards::match(className, &cls[0]))
                continue;
        }
        accepted.push_back(hwnds[i]);
    }

    if (accepted.size() == 0)
        return NULL;

    HWND hWndCurr = GetForegroundWindow();
    for (size_t i = 0; i < accepted.size(); ++i) {
        if (hwnds[i] != hWndCurr)
            continue;
        return hwnds[(i + 1) % accepted.size()];
    }

    return accepted[0];
}

bool Action::activate(const std::string & exeName, const std::string & windowName, const std::string & className) {
    HWND hWnd = findWindow(exeName, windowName, className);
    if (hWnd != NULL) {
        SwitchToThisWindow(hWnd, TRUE);
        return true;
    }
    return false;
}

void Action::run(const std::string & appName, const std::string & cmdLine, const std::string & currDir) {
    PROCESS_INFORMATION lpProcessInfo{0};
    STARTUPINFO siStartupInfo{0};
    siStartupInfo.cb = sizeof(STARTUPINFO);

    const char *szCurrDir = currDir.empty() ? NULL : currDir.c_str();

    if (CreateProcess(appName.c_str(), (LPSTR) cmdLine.c_str(), NULL, NULL, FALSE, NULL, NULL, szCurrDir, &siStartupInfo, &lpProcessInfo)) {
        CloseHandle(lpProcessInfo.hThread);
        CloseHandle(lpProcessInfo.hProcess);
    } else {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
        MessageBox(NULL, (LPCTSTR) lpMsgBuf, appName.c_str(), MB_OK | MB_ICONINFORMATION);
        LocalFree(lpMsgBuf);
    }
}

void Action::activateOrRun(const std::string & exeName, const std::string & windowName, const std::string & className,
                           const std::string & appName, const std::string & cmdLine, const std::string & currDir) {
    if (activate(exeName, windowName, className)) {
        return;
    }
    run(appName, cmdLine, currDir);
}

struct key_t {
    WORD    wVk;
    DWORD   dwFlags;
};

static void sendKeys(const std::vector<key_t> & keysin, bool resetModifiers) {
    std::vector<INPUT> keys;
    std::vector<int> faked;

    INPUT input;
    input.type = INPUT_KEYBOARD;

    if (resetModifiers) {
        int mods[] = {VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU};
        for (int i = 0; i < sizeof(mods) / sizeof(mods[0]); ++i) {
            if ((::GetAsyncKeyState(i) & 0x8000) != 0) {
                faked.push_back(i);
                input.ki.wVk = i;
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                keys.push_back(input);
            }
        }
    }

    for (int i = 0; i < keysin.size(); ++i) {
        input.ki.wVk = keysin[i].wVk;
        input.ki.dwFlags = keysin[i].dwFlags;
        keys.push_back(input);
    }

    // reset state of modifiers
    for (int i = 0; i < faked.size(); ++i) {
        input.ki.wVk = faked[i];
        input.ki.dwFlags = 0;
        input.type = INPUT_KEYBOARD;
        keys.push_back(input);
    }

    SendInput((UINT)keys.size(), &keys[0], sizeof(INPUT));
}

void Action::copy() {
    sendKeys({
        {VK_LCONTROL, 0},
        {'C', 0},
        {'C',KEYEVENTF_KEYUP},
        {VK_LCONTROL, KEYEVENTF_KEYUP}}, true);
}

void Action::cut() {
    sendKeys({
        {VK_LCONTROL, 0},
        {'X', 0},
        {'X',KEYEVENTF_KEYUP},
        {VK_LCONTROL, KEYEVENTF_KEYUP}}, true);
}

void Action::paste() {
    sendKeys({
        {VK_LCONTROL, 0},
        {'V', 0},
        {'V',KEYEVENTF_KEYUP},
        {VK_LCONTROL, KEYEVENTF_KEYUP}}, true);
}

void Action::screenshot() {
    sendKeys({
        {VK_SNAPSHOT, 0},
        {VK_SNAPSHOT, KEYEVENTF_KEYUP}}, true);
}

void Action::nextWindow() {
    sendKeys({
        {VK_LMENU, 0},
        {VK_ESCAPE, 0},
        {VK_ESCAPE, KEYEVENTF_KEYUP},
        {VK_LMENU, KEYEVENTF_KEYUP}}, true);
}

void Action::prevWindow() {
    sendKeys({
        {VK_LMENU, 0},
        {VK_LSHIFT, 0},
        {VK_ESCAPE, 0},
        {VK_ESCAPE, KEYEVENTF_KEYUP},
        {VK_LSHIFT, KEYEVENTF_KEYUP},
        {VK_LMENU, KEYEVENTF_KEYUP}}, true);
}
