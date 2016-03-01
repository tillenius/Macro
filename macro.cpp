#include "macro.h"
#include "dll\dll.h"
#include "main.h"
#include <vector>

using namespace std;

//============================================================================================
// Macro :: insertCounter
//============================================================================================
void Macro::insertCounter( vector <INPUT> &lKeys ) {
	char sFormat[12];
	if (g_nCounterDigits == 0)
		if (g_bCounterHex)
			strcpy(sFormat, "%x");
		else
			strcpy(sFormat, "%d");
	else
		if (g_bCounterHex)
			wsprintf(sFormat, "%%0%dx", g_nCounterDigits);
		else
			wsprintf(sFormat, "%%0%dd", g_nCounterDigits);

	char sCounter[14];
	wsprintf(sCounter, sFormat, g_nCounter);


#ifdef _DEBUG
	char temp[200];
	wsprintf(temp, "counter: %s %s", sCounter, sFormat);
	g_lpOut->AddLine(temp);
#endif

	int nLen = strlen(sCounter);

	for (int ii = nLen - g_nCounterDigits; ii < nLen; ii++) {

		int scancode;
		if (sCounter[ii] == '0')
			scancode = 11;
		else if (sCounter[ii] <= '9')
			scancode = sCounter[ii] - '0' + 1;
		else {
			switch(sCounter[ii]) {
					case 'a': scancode = 30; break;
					case 'b': scancode = 48; break;
					case 'c': scancode = 46; break;
					case 'd': scancode = 32; break;
					case 'e': scancode = 18; break;
					case 'f': scancode = 33; break;
			}
		}

		INPUT iKey;

		iKey.ki.wScan = (WORD) scancode;
		iKey.ki.dwFlags = KEYEVENTF_SCANCODE;
		iKey.type = INPUT_KEYBOARD;

		lKeys.push_back(iKey);

#ifdef _DEBUG
		char temp[200];
		wsprintf(temp, "counter: %d %d <%c>", iKey.ki.wScan, iKey.ki.dwFlags, sCounter[ii]);
		g_lpOut->AddLine(temp);
#endif

		iKey.ki.wScan = (WORD) scancode;
		iKey.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		iKey.type = INPUT_KEYBOARD;

		lKeys.push_back(iKey);

	}
	g_nCounter++;
}


//============================================================================================
// Macro :: insertCounter
//============================================================================================
void Macro::insertCounter(void) {
	vector <INPUT> lKeys;
	insertCounter(lKeys);
	INPUT *Keys = new INPUT[lKeys.size()];
	for (unsigned int k = 0; k < lKeys.size(); k++)
		Keys[k] = lKeys[k];

	SendInput(lKeys.size(), Keys, sizeof(INPUT));
	m_nIgnoreCount += lKeys.size();
	delete [] Keys;
}

//-----------------------------------------------------------------------------
// Playback
//-----------------------------------------------------------------------------
void Macro::Playback(void)
{

	vector <INPUT> lKeys;

	std::vector <DWORD>::iterator i;

#ifdef _DEBUG
	g_lpOut->AddLine("------------------- begin play");
#endif

	INPUT iKey;

	for (i = macro.begin(); i != macro.end(); i++) {

		if (*i == 0xffffffff) {
			insertCounter(lKeys);
		}
		else {

			iKey.ki.wScan = (WORD) (((*i) >> 16) & 0xff);
			iKey.ki.dwFlags = KEYEVENTF_SCANCODE;
			if (*i & 0x80000000)
				iKey.ki.dwFlags |= KEYEVENTF_KEYUP;
			if (*i & 0x01000000)
				iKey.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

			iKey.type = INPUT_KEYBOARD;

			lKeys.push_back(iKey);
		}

#ifdef _DEBUG
		char temp[200];
		wsprintf(temp, "%08x: %02x %02x, %08x", *i, (BYTE) (WORD) ((*i >> 16) & 0xff), iKey.ki.wScan,
			iKey.ki.dwFlags);
		g_lpOut->AddLine(temp);
#endif
	}

#ifdef _DEBUG
	char buff[100];
	g_lpOut->AddLine("------------------- sending");
	wsprintf(buff, "%d", lKeys.size());
	g_lpOut->AddLine(buff);

#endif

	INPUT *Keys = new INPUT[lKeys.size()];
	for (unsigned int k = 0; k < lKeys.size(); k++)
		Keys[k] = lKeys[k];

	SendInput(lKeys.size(), Keys, sizeof(INPUT));
	delete [] Keys;

#ifdef _DEBUG
	g_lpOut->AddLine("------------------- play done");
#endif

}


//-----------------------------------------------------------------------------
// GotKey
//-----------------------------------------------------------------------------
void Macro::GotKey(WPARAM wParam, LPARAM lParam) {

	if ( m_nIgnoreCount > 0 ) {
		m_nIgnoreCount--;
		return;
	}

#ifdef _DEBUG
	char buff[100];
	char buff2[100];
	wsprintf(buff, "%08x: %08x (%2x,%2x) ", wParam, lParam, RECBUTTON, PLAYBUTTON);

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

	if (wParam != RECBUTTON && wParam != PLAYBUTTON) {
		if ((lParam & 0xc0000000) == 0x40000000) {
			macro.push_back(lParam | 0xc0000000);
		}
		macro.push_back(lParam);
	}
	else if (wParam == PLAYBUTTON) {
		macro.push_back(0xffffffff);
		insertCounter();
	}
}

//-----------------------------------------------------------------------------
// Clear
//-----------------------------------------------------------------------------
void Macro::Clear(void) {
	macro.clear();
}
