#include "activate.h"
#include "threaditerator.h"
#include "windowiterator.h"
#include "processiterator.h"
#include "wildcards.h"

#include <vector>
#include <string>

namespace {

  void getThreadIds(DWORD dwPid, std::vector<DWORD> &dwThreads) {
    for (ThreadIterator i; !i.end(); ++i)
      if (i->th32OwnerProcessID == dwPid)
        dwThreads.push_back(i->th32ThreadID);
  }

  bool isMainWindow(HWND hWnd) {
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

  void getHwndsByPid(DWORD dwPid, std::vector<HWND> &hwnds) {
    std::vector<DWORD> dwThreads;
    getThreadIds(dwPid, dwThreads);
    for (size_t i = 0; i < dwThreads.size(); ++i)
      getMainWindows(dwThreads[i], hwnds);
  }

  void getAppHwnd(const std::string &appName, std::vector<HWND> &hwnds) {
    std::vector<DWORD> dwPids;

    for (ProcessIterator i; !i.end(); ++i)
      if (Wildcards::match(appName, i->szExeFile))
        dwPids.push_back(i->th32ProcessID);

    for (size_t i = 0; i < dwPids.size(); ++i)
      getHwndsByPid(dwPids[i], hwnds);
    return;
  }
}

HWND Activate::getHWnd(const std::string &exeName,
                       const std::string &windowName,
                       const std::string &className)
{
  std::vector<HWND> hwnds;
  if (exeName == "*") {
    for (WindowIterator i; !i.end(); ++i)
      if (isMainWindow(i.getHWND()))
        hwnds.push_back(i.getHWND());
  }
  else
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
    return hwnds[(i+1) % accepted.size()];
  }
  return accepted[0];
}
