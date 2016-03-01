#ifndef __MACRO_H__
#define __MACRO_H__

#include <windows.h>
#include <vector>

#ifndef KEYEVENTF_SCANCODE
#define KEYEVENTF_SCANCODE    0x0008
#endif

class Macro {
private:
	std::vector <DWORD> macro;

	void insertCounter(std::vector <INPUT> &lKeys);
	void insertCounter(void);
public:
	DWORD m_nIgnoreCount;
	Macro(void) {
		m_nIgnoreCount = 0;
	}

	void Playback(void);
	void GotKey(WPARAM wParam, LPARAM lParam);
	void Clear(void);

  void save(const char* filename) {
    HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
      return;
    DWORD dwWritten;
    for (size_t i = 0; i < macro.size(); i++)
      WriteFile(hFile, &macro[i], 4, &dwWritten, NULL);
    CloseHandle(hFile);
  }

  void load(const char* filename) {
    macro.clear();

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
      macro.push_back(dw);
    }
    CloseHandle(hFile);
  }
};


#endif // __MACRO_H__