#include "hotkeys.h"
#include "main.h"
#include "taskswitch.h"
#include "winampctrl.h"
#include "macro.h"
#include "systray.h"
#include "resource.h"
#include "dll\dll.h"
#include "activate.h"

using std::string;

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
Hotkeys::Hotkeys(HWND hWnd) {
	this->hWnd = hWnd;
	bEnabled = false;
}

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
Hotkeys::~Hotkeys() {
	if (bEnabled)
		Disable();
	for (size_t i = 0; i < lHotkeys.size(); i++)
		delete lHotkeys[i];
}

//-----------------------------------------------------------------------------
// Enable
//-----------------------------------------------------------------------------
bool Hotkeys::Enable(void) {

	for (size_t i = 0; i < lHotkeys.size(); i++) {

		if (RegisterHotKey(hWnd, i, lHotkeys[i]->fsModifiers, lHotkeys[i]->vk) == FALSE) {

			bEnabled = false;

			for (size_t j = 0; j < i; j++)
				UnregisterHotKey(hWnd, j);

			char msg[200];
			wsprintf(msg, "Failed to register hotkey #%d, (MOD=%d VKEY = %d)", i,
					lHotkeys[i]->fsModifiers, lHotkeys[i]->vk);
			MessageBox(0, msg, 0, MB_OK);
			return false;
		}
	}
	
	bEnabled = true;
	return true;
}

//-----------------------------------------------------------------------------
// Record
//-----------------------------------------------------------------------------
void Record(void) {
	if (!g_bRecord) {
		g_lpSysTray->Icon(IDI_ICON3);
		g_macro.Clear();
		Hook();
		g_bRecord = true;

		#ifdef _DEBUG
		g_lpOut->AddLine("Record : Hook");
		#endif
	} else {
		g_lpSysTray->Icon(IDI_ICON1);
		Unhook();
		g_bRecord = false;

		#ifdef _DEBUG
		g_lpOut->AddLine("~Record : Unhook");
		#endif
	}
}


//-----------------------------------------------------------------------------
// Disable
//-----------------------------------------------------------------------------
void Hotkeys::Disable(void) {

	for (size_t i = 0; i < lHotkeys.size(); i++)
		UnregisterHotKey(hWnd, i);

	bEnabled = false;
}

//============================================================================
// ActionID :: execute
//============================================================================
void ActionID::execute(void) {
	switch (ID) {
  	case HOTKEY_RECORD:						Record();	break;
		case HOTKEY_PLAYBACK:					if (!g_bRecord)	g_macro.Playback();	break;
		case HOTKEY_PREVWIN:					TaskSwitch::PrevWindow(); break;
		case HOTKEY_NEXTWIN:					TaskSwitch::NextWindow(); break;
		case HOTKEY_WINAMP_PREV:			WinAmpCtrl::Prev(); break;
		case HOTKEY_WINAMP_PLAY:			WinAmpCtrl::Play(); break;
		case HOTKEY_WINAMP_PAUSE:			WinAmpCtrl::Pause(); break;
		case HOTKEY_WINAMP_PLAYPAUSE:	WinAmpCtrl::PlayPause(); break;
		case HOTKEY_WINAMP_STOP:			WinAmpCtrl::Stop(); break;
		case HOTKEY_WINAMP_NEXT:			WinAmpCtrl::Next(); break;
		case HOTKEY_WINAMP_VOLUP:			WinAmpCtrl::VolUp(); break;
		case HOTKEY_WINAMP_VOLDOWN:		WinAmpCtrl::VolDown(); break;

    case HOTKEY_WINAMP_FF:        WinAmpCtrl::FastForward(); break;
    case HOTKEY_WINAMP_FR:        WinAmpCtrl::FastRewind(); break;

		case HOTKEY_MINIMIZE:
			{
				HWND hWnd = GetForegroundWindow();

				while (GetParent(hWnd) != NULL)
					hWnd = GetParent(hWnd);

				WINDOWPLACEMENT wp = {0};
				wp.length = sizeof(WINDOWPLACEMENT);

				GetWindowPlacement(hWnd, &wp);
				wp.showCmd = SW_MINIMIZE;
				SetWindowPlacement(hWnd, &wp);
			}
			break;

		default:
			break;
	}
}

//============================================================================
// ThreadFunc
//============================================================================
DWORD ThreadFunc( LPVOID lpv ) {

	WaitForSingleObject(((PROCESS_INFORMATION*) lpv)->hProcess, INFINITE);
	CloseHandle(((PROCESS_INFORMATION*) lpv)->hThread);
	CloseHandle(((PROCESS_INFORMATION*) lpv)->hProcess);

	return 0;
}

//============================================================================
// RunAction :: execute
//============================================================================
void RunAction::execute(void) {
	PROCESS_INFORMATION *lpProcessInfo = new PROCESS_INFORMATION();
	STARTUPINFO siStartupInfo;
	ZeroMemory(&siStartupInfo, sizeof (STARTUPINFO));
	siStartupInfo.cb = sizeof (STARTUPINFO);

	const char *szCurrDir = currDir.empty() ? NULL : currDir.c_str();

	if (CreateProcess(
		appName.c_str(),		// name of executable module
		(LPSTR) cmdLine.c_str(),		// command line string
		NULL,								// SD
		NULL,								// SD
		FALSE,							// handle inheritance option
		NULL,								// creation flags
		NULL,								// new environment block
		szCurrDir,					// current directory name
		&siStartupInfo,			// startup information
		lpProcessInfo				// process information
		))
	{
		DWORD  g_dwThreadId;

		CreateThread(
			NULL,																// default security attributes
			0,																	// use default stack size 
			(LPTHREAD_START_ROUTINE) ThreadFunc,// thread function
			lpProcessInfo,											// argument to thread function
			0,																	// use default creation flags
			&g_dwThreadId);											// returns the thread identifier
	}
	else {
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);

		MessageBox( NULL, (LPCTSTR)lpMsgBuf, appName.c_str(), MB_OK | MB_ICONINFORMATION );

		LocalFree( lpMsgBuf );
	}
}

//============================================================================
// ActivateAction :: execute
//============================================================================
void ActivateAction::execute(void) {
  HWND hWnd = Activate::getHWnd(exeName, windowName, className);
	if (hWnd != NULL)
		SwitchToThisWindow(hWnd, TRUE);
}

//============================================================================
// RunOrActivate :: execute
//============================================================================
void RunOrActivate::execute(void) {
  HWND hWnd = Activate::getHWnd(exeName, windowName, className);
	if (hWnd != NULL)
		SwitchToThisWindow(hWnd, TRUE);
	else
		RunAction::execute();
}
