#pragma once

#include <windows.h>
#include <string>

class Settings;

class Action {
public:
    static void minimize();
    static void runSaved(const std::string & fileName, Settings & settings);
    static bool activate(const std::string & exeName, const std::string & windowName, const std::string & className);
    static void run(const std::string & appName, const std::string & cmdLine, const std::string & currDir, WORD wShowWindow);
    static void activateOrRun(const std::string & exeName, const std::string & windowName, const std::string & className,
                              const std::string & appName, const std::string & cmdLine, const std::string & currDir, WORD wShowWindow);
};
