#include "action.h"
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

namespace {

static void getThreadIds(DWORD dwPid, std::vector<DWORD> &dwThreads) {
    for (ThreadIterator i; !i.end(); ++i)
        if (i->th32OwnerProcessID == dwPid)
            dwThreads.push_back(i->th32ThreadID);
}

static bool isMainWindow(HWND hWnd) {
    return (GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE) != 0;
}

static void getMainWindows(DWORD dwThread, std::vector<HWND> &hwnds) {
    for (WindowIterator i; !i.end(); ++i) {
        if (i.getThreadId() == dwThread) {
            HWND hWnd = i.getHWND();
            if (isMainWindow(hWnd))
                hwnds.push_back(hWnd);
        }
    }
}

static void getHwndsByPid(DWORD dwPid, std::vector<HWND> &hwnds) {
    std::vector<DWORD> dwThreads;
    getThreadIds(dwPid, dwThreads);
    for (size_t i = 0; i < dwThreads.size(); ++i)
        getMainWindows(dwThreads[i], hwnds);
}

static void getAppHwnd(const std::string &appName, std::vector<HWND> &hwnds) {
    std::vector<DWORD> dwPids;

    for (ProcessIterator i; !i.end(); ++i)
        if (Wildcards::match(appName, i->szExeFile))
            dwPids.push_back(i->th32ProcessID);

    for (size_t i = 0; i < dwPids.size(); ++i)
        getHwndsByPid(dwPids[i], hwnds);
    return;
}

static HWND getHWnd(const std::string & exeName, const std::string & windowName, const std::string & className) {
    std::vector<HWND> hwnds;
    if (exeName == "*") {
        for (WindowIterator i; !i.end(); ++i)
            if (isMainWindow(i.getHWND()))
                hwnds.push_back(i.getHWND());
    } else
        getAppHwnd(exeName, hwnds);

    if (hwnds.size() == 0)
        return NULL;

    std::vector<HWND> accepted;

    for (size_t i = 0; i < hwnds.size(); ++i) {

        if (windowName != "*") {
            int size = GetWindowTextLength(hwnds[i]);
            std::vector<char> titleCurr(size + 2);
            GetWindowText(hwnds[i], &titleCurr[0], size);
            if (!Wildcards::match(windowName, &titleCurr[0]))
                continue;
        }

        if (className != "*") {
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

static HWND GetCurrentWindow() {
    HWND hWnd = GetForegroundWindow();

    while (GetParent(hWnd) != NULL)
        hWnd = GetParent(hWnd);

    return hWnd;
}

} // namespace

bool Action::activate(const std::string & exeName, const std::string & windowName, const std::string & className) {
    HWND hWnd = getHWnd(exeName, windowName, className);
    if (hWnd != NULL) {
        SwitchToThisWindow(hWnd, TRUE);
        return true;
    }
    return false;
}

DWORD RunThreadFunc(LPVOID lpv) {
    PROCESS_INFORMATION * pi = (PROCESS_INFORMATION *) lpv;
    WaitForSingleObject(pi->hProcess, INFINITE);
    CloseHandle(pi->hThread);
    CloseHandle(pi->hProcess);
    return 0;
}

void Action::run(const std::string & appName, const std::string & cmdLine, const std::string & currDir, WORD wShowWindow) {
    PROCESS_INFORMATION lpProcessInfo{0};
    STARTUPINFO siStartupInfo{0};
    siStartupInfo.cb = sizeof(STARTUPINFO);

    const char *szCurrDir = currDir.empty() ? NULL : currDir.c_str();

    if (CreateProcess(appName.c_str(), (LPSTR) cmdLine.c_str(), NULL, NULL, FALSE, NULL, NULL, szCurrDir, &siStartupInfo, &lpProcessInfo)) {
        DWORD  g_dwThreadId;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) RunThreadFunc, &lpProcessInfo, 0, &g_dwThreadId);
    } else {
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
        MessageBox(NULL, (LPCTSTR) lpMsgBuf, appName.c_str(), MB_OK | MB_ICONINFORMATION);
        LocalFree(lpMsgBuf);
    }
}

void Action::activateOrRun(const std::string & exeName, const std::string & windowName, const std::string & className,
                           const std::string & appName, const std::string & cmdLine, const std::string & currDir, WORD wShowWindow) {
    if (activate(exeName, windowName, className)) {
        return;
    }
    run(appName, cmdLine, currDir, wShowWindow);
}

void Action::minimize() {
    HWND hWnd = GetCurrentWindow();

    WINDOWPLACEMENT wp = {0};
    wp.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(hWnd, &wp);
    wp.showCmd = SW_MINIMIZE;
    SetWindowPlacement(hWnd, &wp);
}

void Action::moveWindow(int deltaX, int deltaY, int deltaCX, int deltaCY) {
    HWND hWnd = GetCurrentWindow();

    WINDOWPLACEMENT wp = {0};
    wp.length = sizeof(WINDOWPLACEMENT);

    GetWindowPlacement(hWnd, &wp);
    if (wp.showCmd != SW_SHOWNORMAL) {
        return;
    }

    RECT rect;
    if (GetWindowRect(hWnd, &rect)) {
        MoveWindow(hWnd, rect.left + deltaX, rect.top + deltaY, rect.right - rect.left + deltaCX, rect.bottom - rect.top + deltaCY, TRUE);
    }
}

void Action::runSaved(const std::string & fileName, Settings & settings) {
    Macro m(settings);
    m.load(fileName.c_str());
    m.playback();
}
