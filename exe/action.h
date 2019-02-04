#pragma once

#include <windows.h>
#include <string>

class Action {
public:
    static void runSaved(const std::string & fileName);
    static bool activate(const std::string & exeName, const std::string & windowName, const std::string & className);
    static void run(const std::string & appName, const std::string & cmdLine, const std::string & currDir);
    static void activateOrRun(const std::string & exeName, const std::string & windowName, const std::string & className,
                              const std::string & appName, const std::string & cmdLine, const std::string & currDir);
    static void nextWindow();
    static void prevWindow();
    static void paste();
};
