#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <windows.h>

class Output {

private:
	HWND hWnd;
public:
	Output(HINSTANCE hInstance);
	~Output(void);

	void Clear(void);
	void Show(void);

	void AddLine(char *text);
};

#endif _OUTPUT_H_
