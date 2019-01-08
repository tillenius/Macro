#pragma once

#include <Windows.h>
#include <vector>
#include <string>

class PopupMenu {
public:
    static int exec(const std::vector<std::string> & items);
};
