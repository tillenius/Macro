#include "contextmenu.h"
#include "main.h"
#include "settings.h"
#include "resource.h"

namespace {

constexpr int MENU_QUIT = 3;
constexpr int MENU_INACTIVATE = 4;
constexpr int MENU_EDITCONFIG = 7;
constexpr int MENU_RELOADCONFIG = 8;

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
    mii.wID = MENU_EDITCONFIG;
    mii.dwTypeData = (char *) "Edit Configuration";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_RELOADCONFIG;
    mii.dwTypeData = (char *) "Reload Configuration";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
    mii.wID = MENU_QUIT;
    mii.dwTypeData = (char *) "Quit";
    mii.cch = (UINT) strlen(mii.dwTypeData);

    InsertMenuItem(hMenu, 1, FALSE, &mii);

    return hMenu;
}

} // namespace

ContextMenu::ContextMenu() {
    m_hMenu = initMenu();
}

ContextMenu::~ContextMenu() {
    DestroyMenu(m_hMenu);
}

void ContextMenu::show(HWND hWnd) {
    POINT p;
    GetCursorPos(&p);
    SetForegroundWindow(hWnd); // required (see docs for TrackPopupMenu)
    TrackPopupMenu(m_hMenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_RIGHTBUTTON, p.x, p.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0); // force a task switch (see docs for TrackPopupMenu)
}

bool ContextMenu::handleCommand(WORD id, HINSTANCE hInstance, HWND hWnd) {
    switch (id) {
        case MENU_QUIT: 
            PostQuitMessage(0);
            return true;
        case MENU_INACTIVATE:
            g_app->inactivate();
            CheckMenuItem(m_hMenu, MENU_INACTIVATE, g_app->m_state == MacroApp::recordState_t::INACTIVE ? MF_CHECKED : MF_UNCHECKED );
            return true;
        case MENU_EDITCONFIG:
            g_app->editConfigFile();
            return true;
        case MENU_RELOADCONFIG:
            g_app->reload(g_app->m_hotkeys.m_enabled);
            return true;
    }
    return false;
}
