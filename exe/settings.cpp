#include "settings.h"
#include "main.h"
#include "hotkeys.h"
#include "keynames.h"
#include "action.h"
#include "popupmenu.h"
#include "popuplist.h"
#include "resource.h"
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <unordered_map>
#include <shlobj.h>
#include <windows.h>
#include <tlhelp32.h>
#include "pybind11/embed.h"
#include "pybind11/stl.h"
#include "atlbase.h"

namespace py = pybind11;

namespace {

static constexpr int MAX_CONTROLLERS = 127;

template<typename T>
bool parseConfig(py::dict & config, const char * name, T & out) {
    if (!config.contains(name))
        return false;
    try {
        out = py::cast<std::remove_reference_t<decltype(out)>>(config[name]);
    } catch (const py::cast_error &) {
        throw std::runtime_error(std::string("config[\"") + name + std::string("\"] is of wrong type."));
    }
    return true;
}

bool parseKeyConfig(py::dict & config, const char * name, UINT & out) {
    std::string key;
    if (parseConfig(config, name, key)) {
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        if (auto it = Keynames::getMap().find(key); it != Keynames::getMap().end()) {
            out = it->second;
            return true;
        }
        throw std::runtime_error(std::string("Unknown key \"") + key + "\" in config[\"" + name + "\"].");
    }
    return false;
}

py::handle pyHWND(HWND hwnd) {
    static PyObject * ctypes_mod = PyImport_ImportModule("ctypes");
    static PyObject * wintypes = PyObject_GetAttrString(ctypes_mod, "wintypes");
    static PyObject * pyHWND = PyObject_GetAttrString(wintypes, "HWND");
    PyObject * p = PyObject_CallFunction(pyHWND, "O", PyLong_FromVoidPtr((void*) hwnd));
    return py::handle(p);
}

HWND pyHWND(py::object o) {
    HWND hwnd = NULL;
    PyObject * obj = o.ptr();
    PyObject * value = PyObject_GetAttr(obj, PyUnicode_FromString("value"));
    if (value == nullptr) {
        return NULL;
    }
    PyObject * tmp = PyNumber_Long(value);
    Py_DECREF(value);
    if (tmp) {
        hwnd = (HWND) PyLong_AsVoidPtr(tmp);
        Py_DECREF(tmp);
    }
    return hwnd;
}
static BOOL CALLBACK EnumChildWindowsCallback(HWND hwnd, LPARAM lParam) {
    std::vector<py::handle> & result = *(std::vector<py::handle> *) lParam;
    result.push_back(pyHWND(hwnd));
    return TRUE;
}

static BOOL CALLBACK EnumThreadWindowsCallback(HWND hwnd, LPARAM lParam) {
    std::vector<py::handle> & result = *(std::vector<py::handle> *) lParam;
    result.push_back(pyHWND(hwnd));
    return TRUE;
}

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    std::vector<py::handle> & result = *(std::vector<py::handle> *) lParam;
    result.push_back(pyHWND(hwnd));
    return TRUE;
}

static std::tuple<int, int, int, int> makeTuple(RECT & rect) {
    return std::tuple<int, int, int, int>(rect.left, rect.top, rect.right, rect.right);
}

static std::tuple<int, int> makeTuple(POINT & point) {
    return std::tuple<int, int>(point.x, point.y);
}

static void buildMenu(PopupMenu & menu, py::list items) {
    for (auto item : items) {
        py::tuple t = py::cast<py::tuple>(item);
        std::string text = py::cast<std::string>(t[0]);
        if (py::isinstance<py::list>(t[1])) {
            PopupMenu subMenu;
            buildMenu(subMenu, py::cast<py::list>(t[1]));
            menu.append(text, subMenu);
        } else {
            int value = py::cast<int>(t[1]);
            menu.append(text, value);
        }
    }
}

static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, const char * argument) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    CComBSTR bstr(argument);
    VARIANTARG variant_arg[1];
    VariantInit(&variant_arg[0]);
    variant_arg[0].vt = VT_BSTR;
    variant_arg[0].bstrVal = bstr;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, int arg1, int arg2) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    VARIANTARG variant_arg[2];
    VariantInit(&variant_arg[0]);
    VariantInit(&variant_arg[1]);
    variant_arg[0].vt = VT_INT;
    variant_arg[0].intVal = arg1;
    variant_arg[1].vt = VT_INT;
    variant_arg[1].intVal = arg2;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

static CComPtr<IDispatch> PropertyGetIDispatch(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return CComPtr<IDispatch>();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return CComPtr<IDispatch>();
    return variant_result.pdispVal;
}

static int PropertyGetInt(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return -1;

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return -1;
    return variant_result.iVal;
}

static std::wstring PropertyGetBStr(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return std::wstring();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return std::wstring();
    return std::wstring(variant_result.bstrVal, SysStringLen(variant_result.bstrVal));
}

} // namespace

char buffer[4096];
wchar_t wbuffer[4096];

PYBIND11_EMBEDDED_MODULE(macro, m) {

    // Enumerate things

    m.def("get_threadprocids", []() {
        std::vector<std::tuple<DWORD, DWORD>> threadprocids;

        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap == INVALID_HANDLE_VALUE)
            return threadprocids;

        THREADENTRY32 te32;
        ::ZeroMemory(&te32, sizeof(te32));
        te32.dwSize = sizeof(THREADENTRY32);
        if (!Thread32First(hThreadSnap, &te32)) {
            CloseHandle(hThreadSnap);
            return threadprocids;
        }
        do {
            threadprocids.push_back(std::tuple<DWORD, DWORD>(te32.th32ThreadID, te32.th32OwnerProcessID));
        } while (Thread32Next(hThreadSnap, &te32) == TRUE);
        CloseHandle(hThreadSnap);
        return threadprocids;
    });

    m.def("get_threadids", [](DWORD procId) {
        std::vector<DWORD> threadids;

        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap == INVALID_HANDLE_VALUE)
            return threadids;

        THREADENTRY32 te32;
        ::ZeroMemory(&te32, sizeof(te32));
        te32.dwSize = sizeof(THREADENTRY32);
        if (!Thread32First(hThreadSnap, &te32)) {
            CloseHandle(hThreadSnap);
            return threadids;
        }
        do {
            if (te32.th32OwnerProcessID == procId) {
                threadids.push_back(te32.th32ThreadID);
            }
        } while (Thread32Next(hThreadSnap, &te32) == TRUE);
        CloseHandle(hThreadSnap);
        return threadids;
    });

    m.def("get_processes", []() {
        std::vector<py::dict> processes;

        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
            return processes;

        PROCESSENTRY32 pe32;
        ::ZeroMemory(&pe32, sizeof(PROCESSENTRY32));
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            return processes;
        }
        do {
            py::dict proc;
            proc["th32ProcessID"] = pe32.th32ProcessID;
            proc["cntThreads"] = pe32.cntThreads;
            proc["th32ParentProcessID"] = pe32.th32ParentProcessID;
            proc["szExeFile"] = std::string(pe32.szExeFile);
            processes.push_back(proc);
        } while (Process32Next(hProcessSnap, &pe32) == TRUE);
        CloseHandle(hProcessSnap);
        return processes;
    });

    m.def("get_modules", []() {
        std::vector<py::dict> modules;

        HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
        if (hModuleSnap == INVALID_HANDLE_VALUE)
            return modules;

        MODULEENTRY32 me32;
        ::ZeroMemory(&me32, sizeof(MODULEENTRY32));
        me32.dwSize = sizeof(MODULEENTRY32);
        if (!Module32First(hModuleSnap, &me32)) {
            CloseHandle(hModuleSnap);
            return modules;
        }
        do {
            py::dict proc;
            proc["th32ProcessID"] = me32.th32ProcessID;
            proc["modBaseAddr"] = me32.modBaseAddr;
            proc["modBaseSize"] = me32.modBaseSize;
            proc["hModule"] = (void *) me32.hModule;
            proc["szModule"] = std::string(me32.szModule);
            proc["szExePath"] = std::string(me32.szExePath);
            modules.push_back(proc);
        } while (Module32Next(hModuleSnap, &me32) == TRUE);
        CloseHandle(hModuleSnap);
        return modules;
    });

    m.def("get_windows", []() {
        std::vector<py::handle> windows;
        for (HWND hwnd = ::GetTopWindow(0); hwnd != NULL; hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT)) {
            windows.push_back(pyHWND(hwnd));
        }
        return windows;
    });

    m.def("enum_child_windows", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        std::vector<py::handle> result;
        EnumChildWindows(hwnd, EnumChildWindowsCallback, (LPARAM) &result);
        return result;
    });

    m.def("enum_thread_windows", [](DWORD threadId) {
        std::vector<py::handle> result;
        EnumThreadWindows(threadId, EnumThreadWindowsCallback, (LPARAM) &result);
        return result;
    });

    m.def("enum_windows", []() {
        std::vector<py::handle> result;
        ::EnumWindows(EnumWindowsCallback, (LPARAM) &result);
        return result;
    });

    // Clipboard

    m.def("empty_clipboard", []() {
        if (::OpenClipboard(g_app->m_hWnd) == 0) {
            return false;
        }
        bool ret = ::EmptyClipboard() != 0;
        ::CloseClipboard();
        return ret;
    });

    m.def("is_clipboard_format_available", [](UINT format) {
        return ::IsClipboardFormatAvailable(format) != 0;
    });

    m.def("get_clipboard_text", []() {
        std::string ret;
        if (::OpenClipboard(g_app->m_hWnd) == 0) {
            return ret;
        }
        HANDLE h = ::GetClipboardData(CF_TEXT); // @TODO: CF_UNICODETEXT?
        if (h != NULL) {
            char * data = (char *) ::GlobalLock(h);
            if (data != NULL) {
                strcpy_s(buffer, data);
                ret = std::string(buffer);
                ::GlobalUnlock(h);
            }
        }
        ::CloseClipboard();
        return ret;
    });

    m.def("set_clipboard_text", [](const std::string & text) {
        if (::OpenClipboard(g_app->m_hWnd) == 0) {
            return false;
        }
        ::EmptyClipboard();

        HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, text.length() + 1);
        if (hglbCopy != NULL) {
            char * buffer = (char *) GlobalLock(hglbCopy);
            memcpy(buffer, text.c_str(), text.length());
            buffer[text.length()] = 0;
            GlobalUnlock(hglbCopy);

            ::SetClipboardData(CF_TEXT, hglbCopy); // @TODO: CF_UNICODETEXT?
        }
        ::CloseClipboard();
        return true;
    });

    // Macro

    m.def("play", [](const std::vector<DWORD> & macro, bool wait) {
        g_app->playback(macro, wait);
    });

    m.def("copy", []() {
        Action::copy();
    });
    m.def("cut", []() {
        Action::cut();
    });
    m.def("paste", []() {
        Action::paste();
    });
    m.def("screenshot", []() {
        Action::screenshot();
    });

    m.def("get_macro_as_python", []() {
        const std::vector<DWORD> & macro = g_app->m_macro.get();
        if (macro.empty()) {
            return std::string();
        }
        std::string ret = "macro.play([\n";
        for (int i = 0; i < macro.size(); ++i) {
            int scancode = (WORD) (((macro[i]) >> 16) & 0xff);
            bool up = (macro[i] & 0x80000000) != 0;
            char buffer[30];
            sprintf_s(buffer, "    0x%08x", macro[i]);
            ret += buffer;
            ::GetKeyNameTextA((LONG) macro[i], buffer, sizeof(buffer));
            ret += ", # ";
            ret += buffer;
            ret += up ? " up\n" : "\n";
        }
        ret += "], True);";
        return ret;
    });

    // Activate and run programs

    m.def("activate", [](const std::string & exeName, const std::string & windowName, const std::string & className) {
        Action::activate(exeName, windowName, className);
    });

    m.def("run", [](const std::string & appName, const std::string & cmdLine, const std::string & currDir) {
        Action::run(appName, cmdLine, currDir);
    });

    m.def("activate_or_run", [](const std::string & exeName, const std::string & windowName, const std::string & className,
                                const std::string & appName, const std::string & cmdLine, const std::string & currDir) {
        Action::activateOrRun(exeName, windowName, className, appName, cmdLine, currDir);
    });

    // GUI

    m.def("menu", [](const std::vector<std::string> & items) -> int {
        PopupMenu menu;
        for (int i = 0; i < items.size(); ++i) {
            menu.append(items[i], i + 1);
        }
        return menu.exec();
    });
    m.def("menu", [](py::list items) -> int {
        PopupMenu menu;
        buildMenu(menu, items);
        return menu.exec();
    });
    m.def("list", [](const std::vector<std::string> & items) -> int {
        return PopupList::exec(items);
    });
    m.def("notify", [](const std::string & message) {
        return g_app->m_systray.Notification(message.c_str());
    });

    // Build-in windows management

    m.def("show_window", [](py::object hwnd_, int cmdShow) {
        HWND hwnd = pyHWND(hwnd_);
        ::ShowWindow(hwnd, cmdShow);
    });

    m.def("switch_to_this_window", [](py::object hwnd_, bool alttab) {
        HWND hwnd = pyHWND(hwnd_);
        ::SwitchToThisWindow(hwnd, alttab ? TRUE : FALSE);
    });

    m.def("find_window", [](std::string className, std::string windowName) {
        return pyHWND(::FindWindow(className.empty() ? NULL : className.c_str(), windowName.empty() ? NULL : windowName.c_str()));
    });

    m.def("find_windows", [](py::object hwnd_, std::string className, std::string windowName) {
        HWND hwnd = pyHWND(hwnd_);
        std::vector<py::handle> res;
        HWND h = NULL;
        for (;;) {
            h = ::FindWindowEx(hwnd, h, className.empty() ? NULL : className.c_str(), windowName.empty() ? NULL : windowName.c_str());
            if (h == NULL) {
                break;
            }
            res.push_back(pyHWND(h));
        }
        return res;
    });

    m.def("get_sb_text", [](py::object hwnd_, int index) {
        HWND hwnd = pyHWND(hwnd_);

        DWORD pid;
        ::GetWindowThreadProcessId(hwnd, &pid);

        HANDLE process = ::OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pid);
        if (process == NULL) {
            return std::string();
        }

        int count = LOWORD(SendMessage(hwnd, SB_GETTEXTLENGTH, index, 0));
        if (count <= 0) {
            CloseHandle(process);
            return std::string();
        }

        char * buffer = (char *) VirtualAllocEx(process, NULL, count + 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (buffer == NULL) {
            CloseHandle(process);
            return std::string();
        }

        SendMessage(hwnd, SB_GETTEXT, index, (LPARAM) buffer);

        std::vector<char> memLocal(count + 2);
        if (::ReadProcessMemory(process, buffer, &memLocal[0], memLocal.size(), NULL) == 0) {
            ::VirtualFreeEx(process, buffer, count + 2, MEM_DECOMMIT | MEM_RELEASE);
            CloseHandle(process);
            return std::string();
        }

        ::VirtualFreeEx(process, buffer, count + 2, MEM_DECOMMIT | MEM_RELEASE);
        CloseHandle(process);

        return std::string(&memLocal[0]);
    });

    m.def("get_current_window", []() {
        return pyHWND(::GetAncestor(::GetForegroundWindow(), GA_ROOT));
    });

    m.def("get_foreground_window", []() {
        return pyHWND(::GetForegroundWindow());
    });

    m.def("get_active", []() {
        GUITHREADINFO gti;
        ::ZeroMemory(&gti, sizeof(gti));
        gti.cbSize = sizeof(GUITHREADINFO);
        if (::GetGUIThreadInfo(0, &gti) == 0) {
            return pyHWND(0);
        }
        return pyHWND(gti.hwndActive);
    });

    m.def("get_focus", []() {
        GUITHREADINFO gti;
        ::ZeroMemory(&gti, sizeof(gti));
        gti.cbSize = sizeof(GUITHREADINFO);
        if (::GetGUIThreadInfo(0, &gti) == 0) {
            return pyHWND(0);
        }
        return pyHWND(gti.hwndFocus);
    });

    m.def("is_main_window", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        return (GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) != 0;
    });

    m.def("get_class_name", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        if (::GetClassName(hwnd, buffer, sizeof(buffer)) == 0) {
            return std::string();
        }
        return std::string(&buffer[0]);
    });

    m.def("get_window_text", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        int len = (int) ::SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
        std::vector<char> buffer(len + 2);
        ::SendMessage(hwnd, WM_GETTEXT, (WPARAM) (len + 1), (LPARAM) &buffer[0]);
        return std::string(&buffer[0]);
    }, py::arg("hwnd").noconvert());

    m.def("set_window_text", [](py::object hwnd_, const std::string & s) {
        HWND hwnd = pyHWND(hwnd_);
        ::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) s.c_str());
    });

    m.def("get_window_title", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        int len = ::GetWindowTextLengthW(hwnd);
        std::vector<char> buffer(len + 2);
        ::GetWindowTextW(hwnd, &wbuffer[0], len + 1);
        return std::wstring(&wbuffer[0]);
    });

    m.def("set_window_title", [](py::object hwnd_, const std::string & s) {
        HWND hwnd = pyHWND(hwnd_);
        ::SetWindowText(hwnd, s.c_str());
    });

    m.def("get_process_filename", [](DWORD procId) {
        HANDLE hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);
        if (hProcess == NULL) {
            return std::string();
        }
        DWORD size = sizeof(buffer);
        if (::QueryFullProcessImageNameA(hProcess, 0, buffer, &size) == 0) {
            CloseHandle(hProcess);
            return std::string();
        }
        CloseHandle(hProcess);
        return std::string(&buffer[0]);
    });

    m.def("next_window", []() {
        Action::nextWindow();
    });

    m.def("prev_window", []() {
        Action::prevWindow();
    });

    m.def("set_window_pos", [](py::object hwnd_, int x, int y) {
        HWND hwnd = pyHWND(hwnd_);
        WINDOWPLACEMENT wp = {0};
        wp.length = sizeof(WINDOWPLACEMENT);

        GetWindowPlacement(hwnd, &wp);
        if (wp.showCmd != SW_SHOWNORMAL) {
            return;
        }

        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            MoveWindow(hwnd, x, y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
        }
    });

    m.def("set_window_pos", [](py::object hwnd_, int x, int y, int cx, int cy) {
        HWND hwnd = pyHWND(hwnd_);
        WINDOWPLACEMENT wp = {0};
        wp.length = sizeof(WINDOWPLACEMENT);

        GetWindowPlacement(hwnd, &wp);
        if (wp.showCmd != SW_SHOWNORMAL) {
            return;
        }

        MoveWindow(hwnd, x, y, cx, cy, TRUE);
    });

    m.def("get_window_threadid", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        return ::GetWindowThreadProcessId((HWND) hwnd, NULL);
    });

    m.def("get_window_processid", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        DWORD processId;
        ::GetWindowThreadProcessId((HWND) hwnd, &processId);
        return processId;
    });

    m.def("get_cursor_pos", []() {
        POINT p;
        if (::GetCursorPos(&p) == 0) {
            return std::tuple<int, int>(-1, -1);
        }
        return std::tuple<int, int>(p.x, p.y);
    });

    m.def("window_from_point", [](int x, int y) {
        POINT p;
        p.x = x;
        p.y = y;
        return pyHWND(::WindowFromPoint(p));
    });

    m.def("child_window_from_point", [](py::object hwnd_, int x, int y) {
        HWND hwnd = pyHWND(hwnd_);
        POINT p;
        p.x = x;
        p.y = y;
        return pyHWND(::ChildWindowFromPoint(hwnd, p));
    });

    m.def("main_window_from_pid", [](DWORD pid) {
        return pyHWND(Action::getMainWindowFromPid(pid));
    });

    m.def("main_window_from_thread", [](DWORD threadid) {
        return pyHWND(Action::getMainWindowFromThread(threadid));
    });

    // Dialogs

    m.def("get_dlg_ctrl_id", [](py::object hwnd_) {
        HWND hwnd = pyHWND(hwnd_);
        return ::GetDlgCtrlID(hwnd);
    });

    m.def("get_dlg_item", [](py::object hwnd_, int id) {
        HWND hwnd = pyHWND(hwnd_);
        return pyHWND(::GetDlgItem(hwnd, id));
    });

    m.def("get_dlg_item_int", [](py::object hwnd_, int id) {
        HWND hwnd = pyHWND(hwnd_);
        return ::GetDlgItemInt(hwnd, id, NULL, TRUE);
    });

    m.def("get_dlg_item_text", [](py::object hwnd_, int id) {
        HWND hwnd = pyHWND(hwnd_);
        if (::GetDlgItemText(hwnd, id, &buffer[0], sizeof(buffer)) == 0) {
            return std::string();
        }
        return std::string(&buffer[0]);
    });

    // Processes

    m.def("terminate_process", [](DWORD processId) {
        HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, 0, processId);
        if (hProcess == NULL) {
            return false;
        }
        ::TerminateProcess(hProcess, 0);
        ::CloseHandle(hProcess);
        return true;
    });

    // DEBUG 

    m.def("show_debug", [](bool show) {
        ::ShowWindow(g_app->m_hWnd, show ? SW_SHOWNORMAL : SW_HIDE);
    });
    m.def("midi_debug", [](bool state) {
        g_app->m_midi.m_debug = state;
    });

    m.def("open_in_vs", [](std::string filename, int line) {
        CLSID clsid;
        if (FAILED(::CLSIDFromProgID(L"VisualStudio.DTE.15.0", &clsid)))
            return false;

        CComPtr<IUnknown> punk;
        if (FAILED(::GetActiveObject(clsid, NULL, &punk)))
            return false;

        CComPtr<IDispatch> pDisp;
        if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp))))
            return false;

        if (!InvokeMethod(pDisp, L"ExecuteCommand", (std::string("File.OpenFile ") + filename).c_str()))
            return false;

        CComPtr<IDispatch> activeDocument = PropertyGetIDispatch(pDisp, L"ActiveDocument");
        if (!activeDocument)
            return false;

        CComPtr<IDispatch> selection = PropertyGetIDispatch(activeDocument, L"Selection");
        if (!selection)
            return false;

        if (!InvokeMethod(pDisp, L"MoveToDisplayColumn", 1, line))
            return false;

        return true;
    });

    m.def("execute_in_vs", [](std::string command) {
        CLSID clsid;
        if (FAILED(::CLSIDFromProgID(L"VisualStudio.DTE.15.0", &clsid)))
            return false;

        CComPtr<IUnknown> punk;
        if (FAILED(::GetActiveObject(clsid, NULL, &punk)))
            return false;

        CComPtr<IDispatch> pDisp;
        if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp))))
            return false;

        if (!InvokeMethod(pDisp, L"ExecuteCommand", command.c_str()))
            return false;

        return true;
    });

    m.def("vs_active_document", []() {
        py::dict dict;

        CLSID clsid;
        if (FAILED(::CLSIDFromProgID(L"VisualStudio.DTE.15.0", &clsid)))
            return dict;

        CComPtr<IUnknown> punk;
        if (FAILED(::GetActiveObject(clsid, NULL, &punk)))
            return dict;

        CComPtr<IDispatch> pDisp;
        if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp))))
            return dict;

        CComPtr<IDispatch> activeDocument = PropertyGetIDispatch(pDisp, L"ActiveDocument");
        if (!activeDocument)
            return dict;

        CComPtr<IDispatch> selection = PropertyGetIDispatch(activeDocument, L"Selection");
        if (!selection)
            return dict;

        dict["CurrentLine"] = PropertyGetInt(selection, L"CurrentLine");
        dict["CurrentColumn"] = PropertyGetInt(selection, L"CurrentColumn");
        dict["Fullname"] = PropertyGetBStr(activeDocument, L"Fullname");
        dict["Name"] = PropertyGetBStr(activeDocument, L"Name");
        dict["Path"] = PropertyGetBStr(activeDocument, L"Path");
        dict["Language"] = PropertyGetBStr(activeDocument, L"Language");

        return dict;
    });

}

void SettingsFile::setupMidiButton(BCLMessage & m, int button, int channel, int controller, const py::dict & config) {
    // @TODO
    m((std::string("$button ") + std::to_string(button)).c_str());
    m("  .showvalue on");
    m((std::string("  .easypar CC ") + std::to_string(channel) + " " + std::to_string(controller) + " 0 1 toggleon").c_str());
    m("  .mode down");
}

void SettingsFile::setupMidiEncoder(BCLMessage & m, int encoder, int channel, int controller, const py::dict & config) {
    if (py::cast<std::string>(config["type"]) == "relative") {
        std::string resolution;
        if (config.contains("resolution")) {
            resolution = py::cast<std::string>(config["resolution"]);
        } else {
            resolution = "96 192 768 2304";
        }
        // @TODO
        m((std::string("$encoder ") + std::to_string(encoder)).c_str());
        m("  .showvalue off");
        m((std::string("  .easypar CC ") + std::to_string(channel) + " " + std::to_string(controller) + " 0 127 relative-2").c_str());
        m((std::string("  .resolution ") + resolution).c_str());
        m("  .mode 1dot/off");
    }
}

bool SettingsFile::load() {
    // Create an interpreter on first use. Make sure its kept alive until termination.
    // Delete the old interpreter and create a new one on re-init, to avoid caching.
    static std::unique_ptr<py::scoped_interpreter> interp;
    interp.reset();
    interp = std::make_unique<py::scoped_interpreter>();
    py::module::import("sys").attr("dont_write_bytecode") = true;

    py::dict global = py::dict();
    py::dict local = py::dict();

    m_bclMessage("$rev R1");
    m_bclMessage("$preset");

    struct controllerUsage {
        std::vector<bool> used;
        int nextFree = 1;
    };
    std::unordered_map<int, controllerUsage> usedControllers;

    auto nextController = [&usedControllers](int channel) -> int {
        controllerUsage & cu = usedControllers[channel];
        std::vector<bool> & used = cu.used;
        if (used.empty()) {
            used.resize(MAX_CONTROLLERS);
        }
        for (int & nextFree = cu.nextFree; nextFree < used.size(); ++nextFree) {
            if (!used[nextFree]) {
                used[nextFree] = true;
                return nextFree++;
            }
        }
        throw std::exception((std::string("Too many midi actions in channel ") + std::to_string(channel) + ".").c_str());
    };

    WCHAR * filepath;
    if (!SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &filepath))) {
        MessageBox(0, "SHGetKnownFolderPath() failed.", 0, MB_OK);
        return false;
    }

    m_settings.m_settingsPath = filepath;
    m_settings.m_settingsPath += L"\\Macro";
    CreateDirectoryW(m_settings.m_settingsPath.c_str(), NULL);
    py::module::import("sys").attr("path").cast<py::list>().append(wstr_to_utf8(m_settings.m_settingsPath).c_str());
    m_settings.m_settingsFile = m_settings.m_settingsPath + L"\\macro-settings.py";
 
    if (!fileExists(m_settings.m_settingsFile.c_str())) {
        std::ofstream out(m_settings.m_settingsFile.c_str());
        extern const char * DEFAULT_CONFIG_FILE;
        out << DEFAULT_CONFIG_FILE;
        out.close();
    }

    try {
        py::module settings = py::module::import("macro-settings");

        if (py::hasattr(settings, "config")) {
            py::dict config = py::cast<py::dict>(settings.attr("config"));
            parseConfig(config, "counter_digits", m_settings.m_counterDigits);
            parseConfig(config, "counter_hex", m_settings.m_counterHex);
            parseConfig(config, "conter_start", m_settings.m_counter);
            parseConfig(config, "midiinterface", m_settings.m_midiInterface);
            parseConfig(config, "midichannel", m_settings.m_midiChannel);
            parseKeyConfig(config, "rec", m_settings.m_recbutton);
            parseKeyConfig(config, "play", m_settings.m_playbutton);
        }

        if (py::hasattr(settings, "bcl_setup")) {
            py::list items = py::cast<py::list>(settings.attr("bcl_setup"));
            for (auto item : items) {
                py::tuple t = py::cast<py::tuple>(item);
                std::string type = py::cast<std::string>(t[0]);
                const int encoder = py::cast<int>(t[1]);
                py::dict config = py::cast<py::dict>(t[2]);
                const int channel = config.contains("channel") ? py::cast<int>(config["channel"]) : m_settings.m_midiChannel;
                const int controller = py::cast<int>(config["controller"]);
                std::vector<bool> & used = usedControllers[channel].used;
                if (used.empty()) {
                    used.resize(MAX_CONTROLLERS);
                }
                if (used[controller]) {
                    throw std::exception((std::string("Controller ") + std::to_string(controller) + " already used in channel " + std::to_string(channel) + ".").c_str());
                }
                used[controller] = true;
                if (type == "button") {
                    setupMidiButton(m_bclMessage, encoder, channel, controller, config);
                } else if (type == "encoder") {
                    setupMidiEncoder(m_bclMessage, encoder, channel, controller, config);
                }
            }
        }

        if (py::hasattr(settings, "hotkeys")) {
            py::list hotkeylist = py::cast<py::list>(settings.attr("hotkeys"));
            for (auto hotkey : hotkeylist) {
                py::tuple t = py::cast<py::tuple>(hotkey);
                std::string type = py::cast<std::string>(t[0]);
                py::function f = py::cast<py::function>(t[2]);
                if (type == "key") {
                    std::string key = py::cast<std::string>(t[1]);
                    UINT mod;
                    UINT vk;
                    std::string errorMsg;
                    Keynames::readKey(key, mod, vk, errorMsg);
                    if (!errorMsg.empty()) {
                        throw std::exception(errorMsg.c_str());
                    }
                    m_hotkeys.add(mod, vk, [f]() { f(); });
                } else if (type == "button" || type == "encoder" || type == "cc") {
                    py::dict config = py::cast<py::dict>(t[1]);
                    const int id = py::cast<int>(config["id"]);
                    const int channel = config.contains("channel") ? py::cast<int>(config["channel"]) : m_settings.m_midiChannel;
                    const int controller = nextController(channel);
                    if (type == "button") {
                        setupMidiButton(m_bclMessage, id, channel, controller, config);
                        m_channelMap[channel][controller] = Midi::Entry([f](int value) { f(value); });
                    } else if (type == "encoder") {
                        setupMidiEncoder(m_bclMessage, id, channel, controller, config);
                        m_channelMap[channel][controller] = Midi::Entry([f](int value) { f(value-64); });
                    } else if (type == "cc") {
                        m_channelMap[channel][controller] = Midi::Entry([f](int value) { f(value); });
                    }
                }
            }
        }
    } catch (const std::exception & ex) {
        MessageBox(0, ex.what(), 0, MB_OK);
        return false;
    }

    m_bclMessage("$end");
    m_hotkeys.add(0, m_settings.m_recbutton, []() { g_app->record(); });
    m_hotkeys.add(0, m_settings.m_playbutton, []() { g_app->playback(g_app->m_macro.get(), false); });

    return true;
}
