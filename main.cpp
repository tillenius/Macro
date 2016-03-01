// Macro
// martin tillenius 2003-2008

//#define _WIN32_WINNT WINVER

#include <windows.h>
#include <list>
#include <string>
#include "output.h"
#include "systray.h"
#include "resource.h"
#include "dll\dll.h"
#include "macro.h"
#include "hotkeys.h"
#include "main.h"
#include "parser.h"

// defines

#define MENU_QUIT 3
#define MENU_INACTIVATE 4
#define MENU_COUNTERSETTINGS 5
#define MENU_COUNTERRESET 6
#define MENU_SAVEMACRO 7

#define WM_USER_GOTKEY				WM_USER+1

// globals

#ifdef _DEBUG
Output       *g_lpOut;
#endif
SysTray      *g_lpSysTray;
Hotkeys      *g_hotkeys;
HMENU         g_hMenu;
bool          g_bRecord = false;
bool          g_bActive = true;
HWND          g_hWnd;
Macro         g_macro;
HINSTANCE     g_hInstance;

int           g_nCounterDigits = 3;
int           g_nCounter = 0;
bool          g_bCounterHex = false;

//-----------------------------------------------------------------------------
// InitMenu
//-----------------------------------------------------------------------------
HMENU InitMenu(void) {
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
	mii.dwTypeData = "Inactivate";
	mii.cch = strlen(mii.dwTypeData);

	InsertMenuItem(hMenu, 0, FALSE, &mii);

	mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	mii.wID = MENU_COUNTERSETTINGS;
	mii.dwTypeData = "Counter Settings";
	mii.cch = strlen(mii.dwTypeData);

	InsertMenuItem(hMenu,	1, FALSE,	&mii);

	mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	mii.wID = MENU_COUNTERRESET;
	mii.dwTypeData = "Reset Counter";
	mii.cch = strlen(mii.dwTypeData);

	InsertMenuItem(hMenu, 1, FALSE, &mii);

	mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	mii.wID = MENU_SAVEMACRO;
	mii.dwTypeData = "Save Macro";
	mii.cch = strlen(mii.dwTypeData);

	InsertMenuItem(hMenu, 1, FALSE, &mii);


	mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
	mii.wID = MENU_QUIT;
	mii.dwTypeData = "Quit";
	mii.cch = strlen(mii.dwTypeData);

	InsertMenuItem(hMenu, 1, FALSE, &mii);


	return hMenu;
}

////-----------------------------------------------------------------------------
//// RunProgram
////-----------------------------------------------------------------------------
//void RunProgram(WPARAM wParam) {
//}

INT_PTR CALLBACK CounterSettings(HWND hWnd,
																 UINT message,
																 WPARAM wParam,
																 LPARAM lParam)
{
	static char buf[15];
	switch (message) {
	case WM_INITDIALOG:

		wsprintf(buf, "%d", g_nCounterDigits);
		SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), buf);

		if (g_bCounterHex)
			wsprintf(buf, "%x", g_nCounter);
		else
			wsprintf(buf, "%d", g_nCounter);

		SetWindowText(GetDlgItem(hWnd, IDC_EDIT2), buf);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDOK:
			GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), buf, 14);
			g_nCounterDigits = atol(buf);

			GetWindowText(GetDlgItem(hWnd, IDC_EDIT2), buf, 14);
			g_nCounter = strtoul(buf, NULL, (g_bCounterHex ? 16 : 10));

			EndDialog(hWnd, 0);
			break;

		case IDC_CHECK1:
			g_bCounterHex = (IsDlgButtonChecked(hWnd, IDC_CHECK1) == BST_CHECKED);

			if (g_bCounterHex)
				wsprintf(buf, "%x", g_nCounter);
			else
				wsprintf(buf, "%d", g_nCounter);

			SetWindowText(GetDlgItem(hWnd, IDC_EDIT2), buf);
			break;

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


void saveMacro(void) {
	OPENFILENAME ofn;			        // common dialog box structure
	char *szFile = new char[1000];	// buffer for file name(s)

	*szFile = '\0';

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize     = sizeof(OPENFILENAME);
	ofn.hwndOwner       = 0;
	ofn.hInstance       = 0;
	ofn.lpstrFile       = szFile;
	ofn.nMaxFile        = MAX_PATH;
	ofn.lpstrFilter     = "All\0*.*\0";
	ofn.nFilterIndex    = 0;
	ofn.lpstrFileTitle  = NULL;
	ofn.nMaxFileTitle   = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags           = OFN_PATHMUSTEXIST | OFN_EXPLORER;

	// Display the Open dialog box. 

	if ( GetSaveFileName(&ofn) )
    g_macro.save(ofn.lpstrFile);

	delete [] szFile;
}

//-----------------------------------------------------------------------------
// WndProc
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd,
												 UINT message,
												 WPARAM wParam,
												 LPARAM lParam)
{
	switch (message) {

	case WM_HOTKEY:
		g_hotkeys->execute(wParam);
		break;

  case WM_USER+3: {
#ifdef _DEBUG
	char buff[100];
	char buff2[100];
  MSLLHOOKSTRUCT *hs = (MSLLHOOKSTRUCT*) lParam;
	wsprintf(buff, "%08x: %08x (%08x) %08x %08x", wParam, lParam, message,
    hs->mouseData, hs->flags);


	GetKeyNameText(
		lParam,				// second parameter of keyboard message
		buff2,				// buffer for key name
		80					// maximum length of key name
		);

	strcat(buff, buff2);

	if (lParam & 0x80000000)
		strcat(buff, ": UP!");
	else
		strcat(buff, ": DOWN");

	g_lpOut->AddLine(buff);
#endif
    break;
                  }
	case WM_USER_GOTKEY:

#ifdef _DEBUG
		g_lpOut->AddLine("Gotkey");
#endif

		if (lParam & 0x80000000)
			g_lpSysTray->Icon(IDI_ICON3);
		else
			g_lpSysTray->Icon(IDI_ICON2);
		g_macro.GotKey(wParam, lParam);
		break;

	case WM_COMMAND:
		switch (wParam & 0xffff) {
		case MENU_QUIT:
			PostQuitMessage(0);
			break;

		case MENU_INACTIVATE:

			g_bActive = !g_bActive;
			if (g_bActive) {
				g_lpSysTray->Icon(IDI_ICON1);
				CheckMenuItem(g_hMenu, MENU_INACTIVATE, MF_UNCHECKED);
				g_hotkeys->Enable();
			}
			else {
				if (g_bRecord) {
					g_lpSysTray->Icon(IDI_ICON1);
					Unhook();
					g_bRecord = false;
				}
				g_hotkeys->Disable();
				g_lpSysTray->Icon(IDI_ICON4);
				CheckMenuItem(g_hMenu, MENU_INACTIVATE, MF_CHECKED);
			}

			break;

		case MENU_COUNTERSETTINGS:
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIALOG3), 0, CounterSettings); 
			break;

		case MENU_COUNTERRESET:
			g_nCounter = 0;
			break;

    case MENU_SAVEMACRO:
      saveMacro();
		}

		break;

	case WM_USER_SYSTRAY:
		switch(lParam) { 
  	case WM_RBUTTONDOWN:

			POINT p;

			GetCursorPos(&p);

			SetForegroundWindow(hWnd);

			TrackPopupMenu(
				g_hMenu,													// handle to shortcut menu
				TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_RIGHTBUTTON,			// options
				p.x,														// horizontal position
				p.y,														// vertical position
				0,															// reserved, must be zero
				hWnd,														// handle to owner window
				NULL														// ignored
				);

			PostMessage(hWnd, WM_NULL, 0, 0);

			break;

#ifdef _DEBUG
		case WM_LBUTTONDOWN:
			g_lpOut->Clear();
			break;
#endif

		}
		break;

	case WM_CREATE:
		g_hMenu = InitMenu();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	default:
		return(DefWindowProc(hWnd, message, wParam, lParam));
	}
	return 0;
}

//-----------------------------------------------------------------------------
// InitApp
//-----------------------------------------------------------------------------
bool InitApp(HINSTANCE hInstance)
{
	WNDCLASS wClass;

	wClass.lpszClassName = "keymacro";
	wClass.hInstance     = hInstance;
	wClass.lpfnWndProc   = WndProc;
	wClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wClass.hIcon         = 0;
	wClass.lpszMenuName  = NULL;
	wClass.hbrBackground = 0;
	wClass.style         = 0;
	wClass.cbClsExtra    = 0;
	wClass.cbWndExtra    = 0;

	return (RegisterClass(&wClass) != 0);
}

//-----------------------------------------------------------------------------
// InitInst
//-----------------------------------------------------------------------------
HWND InitInst(HINSTANCE hInstance) {
	return CreateWindow(
		"keymacro",              // window class name
		"keymacro",              // window title
		WS_OVERLAPPEDWINDOW,     // type of window
		0,                       // x  window location
		0,                       // y
		0,	                     // cx and size
		0,                       // cy
		NULL,                    // no parent for this window
		NULL,                    // use the class menu
		hInstance,               // who created this window
		NULL                     // no parms to pass on
		);
}

//-----------------------------------------------------------------------------
// WinMain
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,      // handle to current instance
									 HINSTANCE hPrevInstance,  // handle to previous instance
									 LPSTR lpCmdLine,          // command line
									 int nCmdShow              // show state
									 )
{

	g_hInstance = hInstance;

	if (FindWindow("keymacro", "keymacro") != NULL) {
		MessageBox(0, "Macro is already running", 0, MB_OK);
		return 0;
	}

	// Init application

	if (!InitApp(hInstance)) {
		MessageBox(0, "InitApp() failed", 0, MB_OK);
		return 0;
	}

	g_hWnd = InitInst(hInstance);

	if (g_hWnd == NULL) {
		MessageBox(0, "InitInst() failed", 0, MB_OK);
		return 0;
	}

	g_hotkeys = new Hotkeys(g_hWnd);

  if (!readConfig())
    MessageBox(0, "Error reading configuration file", 0, MB_OK);

	if (!g_hotkeys->Enable())
		return 0;

	g_lpSysTray = new SysTray(hInstance, g_hWnd);

  #ifdef _DEBUG
	g_lpOut = new Output(hInstance);
  #endif

	g_lpSysTray->Icon(IDI_ICON1);

	// Main Loop

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Destruction

	delete g_hotkeys;

	if (g_bRecord)
		Unhook();

	delete g_lpSysTray;

  #ifdef _DEBUG
	delete g_lpOut;
  #endif

	return (int) msg.wParam;
}
