#pragma once

#include <windows.h>
#include <string>
#include <vector>

class Action {
public:
    static bool activate(const std::string & exeName, const std::string & windowName, const std::string & className);
    static void run(const std::string & appName, const std::string & cmdLine, const std::string & currDir);
    static void activateOrRun(const std::string & exeName, const std::string & windowName, const std::string & className,
                              const std::string & appName, const std::string & cmdLine, const std::string & currDir);
    static void nextWindow();
    static void prevWindow();
    static void copy();
    static void cut();
    static void paste();
    static void screenshot();

    static void getThreadIds(DWORD pid, std::vector<DWORD> & threadids);
    static void getWindowsFromThread(DWORD threadid, std::vector<HWND> & hwnds);
    static void getPidsFromExe(const std::string & exeName, std::vector<DWORD> & pids);
    static void getWindowsFromPid(DWORD pid, std::vector<HWND> & hwnds);
    static void getWindowsFromExe(const std::string & exeName, std::vector<HWND> & hwnds);

    // The "Main Window" is the first found visible root Window.
    static HWND getMainWindowFromThread(DWORD threadid);
    static HWND getMainWindowFromPid(DWORD pid);
};
