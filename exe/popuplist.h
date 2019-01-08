#pragma once

#include <Windows.h>
#include <vector>
#include <string>

class PopupList {
public:
    static int exec(const std::vector<std::string> & items);
};
