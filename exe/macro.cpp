#include "macro.h"
#include "settings.h"
#include <vector>

void Macro::insertCounter(Settings & settings, std::vector<INPUT> & lKeys) {
    char sCounter[14];

    if (settings.m_counterDigits == 0)
        if (settings.m_counterHex)
            wsprintf(sCounter, "%x", settings.m_counter);
        else
            wsprintf(sCounter, "%d", settings.m_counter);
    else {
        char sFormat[12];
        if (settings.m_counterHex)
            wsprintf(sFormat, "%%0%dx", settings.m_counterDigits);
        else
            wsprintf(sFormat, "%%0%dd", settings.m_counterDigits);
        wsprintf(sCounter, sFormat, settings.m_counter);
    }

    const size_t nLen = strlen(sCounter);
    for (size_t ii = nLen - settings.m_counterDigits; ii < nLen; ii++) {
        int scancode;
        if (sCounter[ii] == '0')
            scancode = 11;
        else if (sCounter[ii] <= '9')
            scancode = sCounter[ii] - '0' + 1;
        else {
            switch (sCounter[ii]) {
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

        iKey.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        lKeys.push_back(iKey);
    }
    ++settings.m_counter;
}

void Macro::playback(Settings & settings, const std::vector<DWORD> & macro) {
    if (macro.empty()) {
        return;
    }

    std::vector<INPUT> lKeys;
    std::vector<DWORD>::const_iterator i;

    INPUT iKey;

    for (i = macro.cbegin(); i != macro.cend(); i++) {

        if (*i == 0xffffffff) {
            insertCounter(settings, lKeys);
        }
        else {
            iKey.ki.wScan = (WORD)(((*i) >> 16) & 0xff);
            iKey.ki.dwFlags = KEYEVENTF_SCANCODE;
            if (*i & 0x80000000)
                iKey.ki.dwFlags |= KEYEVENTF_KEYUP;
            if (*i & 0x01000000)
                iKey.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;

            iKey.type = INPUT_KEYBOARD;

            lKeys.push_back(iKey);
        }
    }

    SendInput((UINT) lKeys.size(), &lKeys[0], sizeof(INPUT));
}

void Macro::gotKey(Settings & settings, WPARAM wParam, DWORD lParam) {
    if (m_nIgnoreCount > 0) {
        m_nIgnoreCount--;
        return;
    }

    if (wParam != settings.m_recbutton && wParam != settings.m_playbutton) {
        if ((lParam & 0xc0000000) == 0x40000000) {
            m_macro.push_back(lParam | 0xc0000000);
        }
        m_macro.push_back(lParam);
    }
    else if (wParam == settings.m_playbutton) {
        m_macro.push_back(0xffffffff);
        std::vector<INPUT> lKeys;
        insertCounter(settings, lKeys);

        SendInput((UINT) lKeys.size(), &lKeys[0], sizeof(INPUT));
        m_nIgnoreCount += lKeys.size();
    }
}

void Macro::clear() {
    m_macro.clear();
}

void Macro::save(const char * filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    DWORD dwWritten;
    for (size_t i = 0; i < m_macro.size(); i++)
        WriteFile(hFile, &m_macro[i], 4, &dwWritten, NULL);
    CloseHandle(hFile);
}

void Macro::load(const char * filename) {
    m_macro.clear();

    HANDLE hFile = CreateFile(filename, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        MessageBox(0, filename, 0, MB_OK);
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    DWORD dwRead;
    while (SetFilePointer(hFile, 0, 0, FILE_CURRENT) != fileSize) {
        DWORD dw;
        ReadFile(hFile, &dw, 4, &dwRead, NULL);
        m_macro.push_back(dw);
    }
    CloseHandle(hFile);
}
