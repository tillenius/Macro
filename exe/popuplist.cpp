#include "popuplist.h"
#include "main.h"
#include "resource.h"
#include <windows.h>

namespace {

INT_PTR CALLBACK popupListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static bool done = false;
    switch (message) {
        case WM_INITDIALOG: {
            done = false;
            const std::vector<std::string> & items = *(const std::vector<std::string> *) lParam;
            for (int i = 0; i < items.size(); ++i) {
                SendDlgItemMessage(hWnd, IDC_LIST1, LB_ADDSTRING, 0, (LPARAM) items[i].c_str());
            }
            SendDlgItemMessage(hWnd, IDC_LIST1, LB_SETCURSEL, 0, 0);
            SwitchToThisWindow(hWnd, TRUE);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    int sel = (int) SendDlgItemMessage(hWnd, IDC_LIST1, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR) {
                        done = true;
                        EndDialog(hWnd, sel);
                        return TRUE;
                    }
                    return FALSE;
                }
                case IDCANCEL:
                    EndDialog(hWnd, -1);
                    return TRUE;
            }
            return FALSE;
        case WM_ACTIVATE:
            if (wParam == WA_INACTIVE) {
                if (!done) {
                    EndDialog(hWnd, -1);
                }
                return TRUE;
            }
            return FALSE;
        case WM_CLOSE:
            if (!done) {
                EndDialog(hWnd, -1);
            }
            return TRUE;
        default:
            return FALSE;
    }
    return TRUE;
}

} // namespace

int PopupList::exec(const std::vector<std::string> & items) {
    static bool busy = false;
    if (busy) {
        return -1;
    }
    busy = true;
    HWND prevhwnd = GetForegroundWindow();
    const int cmd = (int) DialogBoxParam(g_app->m_hInstance, MAKEINTRESOURCE(IDD_LISTDIALOG), g_app->m_hWnd, popupListWndProc, (LPARAM) &items);
    DWORD res = GetLastError();
    static volatile DWORD dd = res;
    SwitchToThisWindow(prevhwnd, TRUE);
    busy = false;
    return cmd;
}
