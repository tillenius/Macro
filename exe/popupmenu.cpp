#include "popupmenu.h"
#include "main.h"

int PopupMenu::exec(const std::vector<std::string> & items) {
    POINT p;
    GetCursorPos(&p);

    HMENU hMenu = CreatePopupMenu();
    if (hMenu == NULL)
        return -1;

    MENUITEMINFO mii;
    for (int i = 0; i < items.size(); ++i) {
        ZeroMemory(&mii, sizeof(MENUITEMINFO));
        mii.cbSize = sizeof(MENUITEMINFO);
        if (items[i].empty()) {
            mii.fMask = MIIM_FTYPE;
            mii.fType = MFT_SEPARATOR;
        } else {
            mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
            mii.fType = MFT_STRING;
            mii.wID = i + 1;
            mii.dwTypeData = (char *) items[i].c_str();
            mii.cch = (UINT) strlen(mii.dwTypeData);
        }
        InsertMenuItem(hMenu, i, TRUE, &mii);
    }

    HWND prevhwnd = GetForegroundWindow();
    SwitchToThisWindow(g_app->m_hWnd, TRUE);
    const int cmd = TrackPopupMenu(hMenu, TPM_CENTERALIGN | TPM_VCENTERALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, p.x, p.y, 0, g_app->m_hWnd, NULL);
    SwitchToThisWindow(prevhwnd, TRUE);
    DestroyMenu(hMenu);
    return cmd - 1;
}
