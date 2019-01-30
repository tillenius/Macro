#include "popupmenu.h"
#include "main.h"

PopupMenu::PopupMenu() {
    m_hMenu = CreatePopupMenu();
}

void PopupMenu::append(const std::string & text, int value) {
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    if (text.empty()) {
        mii.fType = MFT_SEPARATOR;
        mii.fMask = MIIM_FTYPE;
    } else {
        mii.fType = MFT_STRING;
        mii.dwTypeData = (char *) text.c_str();
        mii.cch = (UINT) strlen(mii.dwTypeData);
        if (value == -1) {
            mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_STRING;
            mii.fState = MFS_DISABLED;
        } else {
            mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
            mii.wID = value + 1;
        }
    }
    InsertMenuItem(m_hMenu, m_count++, TRUE, &mii);
}

void PopupMenu::append(const std::string & text, PopupMenu & subMenu) {
    MENUITEMINFO mii;
    ZeroMemory(&mii, sizeof(MENUITEMINFO));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.dwTypeData = (char *) text.c_str();
    mii.hSubMenu = subMenu.m_hMenu;
    mii.cch = (UINT) strlen(mii.dwTypeData);
    InsertMenuItem(m_hMenu, m_count++, TRUE, &mii);
}

void PopupMenu::destroy() {
    DestroyMenu(m_hMenu);
}

int PopupMenu::exec() {
    POINT p;
    GetCursorPos(&p);

    HWND prevhwnd = GetForegroundWindow();
    SwitchToThisWindow(g_app->m_hWnd, TRUE);
    const int cmd = TrackPopupMenu(m_hMenu, TPM_CENTERALIGN | TPM_VCENTERALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, p.x, p.y, 0, g_app->m_hWnd, NULL);
    SwitchToThisWindow(prevhwnd, TRUE);
    destroy();
    return cmd - 1;
}
