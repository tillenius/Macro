#pragma once

#include <Windows.h>
#include <vector>
#include <string>

class PopupMenu {
public:

    PopupMenu();
    HMENU m_hMenu;
    int m_count = 0;

    void destroy();
    void append(const std::string & text, int value);
    void append(const std::string & text, PopupMenu & submenu);
    int exec();
};
