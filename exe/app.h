#pragma once

#include <string>

struct app_t {
    std::string name;
    std::string icon;
    std::string activate_exe;
    std::string activate_window;
    std::string activate_class;
    std::string run_appname;
    std::string run_cmdline;
    std::string run_currdir;
};
