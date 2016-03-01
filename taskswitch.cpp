#include "taskswitch.h"

//-----------------------------------------------------------------------------
// NextWindow
//-----------------------------------------------------------------------------
void TaskSwitch::NextWindow(void) {
	INPUT lKey[5];

	// alt down

	lKey[0].ki.wScan = (WORD) 0x38;
	lKey[0].ki.dwFlags = KEYEVENTF_SCANCODE;
	lKey[0].type = INPUT_KEYBOARD;

	// esc down

	lKey[1].ki.wScan = (WORD) 0x01;
	lKey[1].ki.dwFlags = KEYEVENTF_SCANCODE;
	lKey[1].type = INPUT_KEYBOARD;

	// esc up

	lKey[2].ki.wScan = (WORD) 0x01;
	lKey[2].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	lKey[2].type = INPUT_KEYBOARD;

	// alt up

	lKey[3].ki.wScan = (WORD) 0x38;
	lKey[3].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	lKey[3].type = INPUT_KEYBOARD;

	SendInput(4, lKey, sizeof(INPUT));
}

//-----------------------------------------------------------------------------
// PrevWindow
//-----------------------------------------------------------------------------
void TaskSwitch::PrevWindow(void) {
	INPUT lKey[6];

	// alt down

	lKey[0].ki.wScan = (WORD) 0x38;
	lKey[0].ki.dwFlags = KEYEVENTF_SCANCODE;
	lKey[0].type = INPUT_KEYBOARD;

	// shift down

	lKey[1].ki.wScan = (WORD) 0x2a;
	lKey[1].ki.dwFlags = KEYEVENTF_SCANCODE;
	lKey[1].type = INPUT_KEYBOARD;

	// esc down

	lKey[2].ki.wScan = (WORD) 0x01;
	lKey[2].ki.dwFlags = KEYEVENTF_SCANCODE;
	lKey[2].type = INPUT_KEYBOARD;

	// esc up

	lKey[3].ki.wScan = (WORD) 0x01;
	lKey[3].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	lKey[3].type = INPUT_KEYBOARD;

	// shift up

	lKey[4].ki.wScan = (WORD) 0x2a;
	lKey[4].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	lKey[4].type = INPUT_KEYBOARD;

	// alt up

	lKey[5].ki.wScan = (WORD) 0x38;
	lKey[5].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	lKey[5].type = INPUT_KEYBOARD;

	SendInput(6, lKey, sizeof(INPUT));

}
