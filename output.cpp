#include "output.h"
#include "resource.h"
#include <commctrl.h>

INT_PTR CALLBACK Output_DialogFunc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_INITDIALOG: 
		return TRUE; 

	case WM_SIZE:

		HWND hDlg = GetDlgItem(hWnd, IDC_LIST3);

		RECT rect;
		GetWindowRect(hDlg,	&rect);
		
		POINT one;

		one.x = rect.left;
		one.y = rect.top;
		ScreenToClient(hWnd, &one);

		MoveWindow(hDlg, one.x, one.y, LOWORD(lParam), HIWORD(lParam), TRUE);
		
		break;

	}

	//case WM_CLOSE: 
	// EndDialog ( hWnd, 0);
	// return TRUE; 
	//}

	return FALSE;
}

Output::Output(HINSTANCE hInstance) {
	hWnd = CreateDialog(
		hInstance,						// handle to module
		MAKEINTRESOURCE(IDD_DIALOG1),   // dialog box template name
		NULL,							// handle to owner window
		(DLGPROC) Output_DialogFunc		// dialog box procedure
		);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}

Output::~Output(void) {
	EndDialog ( hWnd, 0 ); 
}

void Output::AddLine(char *text) {

	SendDlgItemMessage(
		hWnd,			// handle to dialog box
		IDC_LIST3,		// control identifier
		LB_ADDSTRING,	// message to send
		0,				// first message parameter
		LPARAM(text)	// second message parameter
		);

}

void Output::Clear(void) {
	SendDlgItemMessage(
		hWnd,				// handle to dialog box
		IDC_LIST3,			// control identifier
		LB_RESETCONTENT,	// message to send
		0,					// first message parameter
		0					// second message parameter
		);
}

void Output::Show(void) {
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
}
