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
constexpr int MENU_EDITCONFIG = 8;
constexpr int MENU_RELOADCONFIG = 9;

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
        case MENU_EDITCONFIG:
            g_app->editConfigFile();
            return true;
        case MENU_RELOADCONFIG:
            g_app->reload(g_app->m_hotkeys.m_enabled);
            return true;
    }
    return false;
}
