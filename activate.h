#ifndef __ACTIVATE_H__
#define __ACTIVATE_H__

#include <windows.h>
#include <string>

namespace Activate {
  HWND getHWnd(const std::string &exeName,
               const std::string &windowName,
               const std::string &className);
}

#endif // __ACTIVATE_H__
