#include "contextmenu.h"
#include "main.h"
#include "settings.h"
#include "resource.h"
#include "../dll/Dll.h"

namespace {

constexpr int MENU_QUIT = 3;
constexpr int MENU_INACTIVATE = 4;
constexpr int MENU_COUNTERSETTINGS = 5;
constexpr int MENU_COUNTERRESET = 6;
constexpr int MENU_SAVEMACRO = 7;

static HMENU initMenu() {
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);

    HMENU hMenu = CreatePopupMenu();

    if (hMenu == NULL)
        return NULL;

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_UNCHECKED;
    mii.wID = MENU_INACTIVATE;
    mii.dwTypeData = (char *) "Inactivate";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 0, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_COUNTERSETTINGS;
    mii.dwTypeData = (char *) "Counter Settings";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_COUNTERRESET;
    mii.dwTypeData = (char *) "Reset Counter";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_SAVEMACRO;
    mii.dwTypeData = (char *) "Save Macro";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_QUIT;
    mii.dwTypeData = (char *) "Quit";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    return hMenu;
}

INT_PTR CALLBACK counterSettings(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static char buf[15];

    MacroApp * app = (MacroApp *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {
        case WM_INITDIALOG: {
            app = (MacroApp *) lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) app);

            wsprintf(buf, "%d", app->m_settings.m_counterDigits);
            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_COUNTERDIGITS), buf);

            if (app->m_settings.m_counterHex)
                wsprintf(buf, "%x", app->m_settings.m_counter);
            else
                wsprintf(buf, "%d", app->m_settings.m_counter);

            SetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_COUNTERDIGITS), buf, 14);
                    app->m_settings.m_counterDigits = atol(buf);

                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf, 14);
                    app->m_settings.m_counter = strtoul(buf, NULL, (app->m_settings.m_counterHex ? 16 : 10));

                    EndDialog(hWnd, 0);
                    break;

                case IDC_CHECK_HEXCOUNTER: {
                    GetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf, 14);
                    int value = strtoul(buf, NULL, (app->m_settings.m_counterHex ? 16 : 10));

                    app->m_settings.m_counterHex = (IsDlgButtonChecked(hWnd, IDC_CHECK_HEXCOUNTER) == BST_CHECKED);

                    if (app->m_settings.m_counterHex)
                        wsprintf(buf, "%x", value);
                    else
                        wsprintf(buf, "%d", value);

                    SetWindowText(GetDlgItem(hWnd, IDC_EDIT_NEXTCOUNTERVALUE), buf);
                    break;
                }

                default:
                    return FALSE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hWnd, 0);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

} // namespace

ContextMenu::ContextMenu() {
    m_hMenu = initMenu();
}

void ContextMenu::show(HWND hWnd) {
    POINT p;
    GetCursorPos(&p);
    TrackPopupMenu(m_hMenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_RIGHTBUTTON, p.x, p.y, 0, hWnd, NULL);
}

bool ContextMenu::handleCommand(WORD id, HINSTANCE hInstance, HWND hWnd) {
    switch (id) {
        case MENU_QUIT: 
            PostQuitMessage(0);
            return true;
        case MENU_INACTIVATE:
            g_app->inactivate();
            CheckMenuItem(m_hMenu, MENU_INACTIVATE, g_app->m_bActive ? MF_UNCHECKED : MF_CHECKED);
            return true;
        case MENU_COUNTERSETTINGS:
            g_app->m_settingsDlg.show(hInstance, hWnd);
            return true;
        case MENU_COUNTERRESET:
            g_app->resetCounter();
            return true;
        case MENU_SAVEMACRO:
            g_app->saveMacro();
            return true;
    }
    return false;
}
