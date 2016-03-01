#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#include <windows.h>
#include "main.h"


#ifndef KEYEVENTF_SCANCODE
#define KEYEVENTF_SCANCODE    0x0008
#endif

class TaskSwitch {
private:
public:

	static void TaskSwitch::NextWindow(void);
	static void TaskSwitch::PrevWindow(void);

};

#endif // __TASKSWITCH_H__