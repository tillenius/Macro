#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef _DEBUG
#include "output.h"
#endif

#define RECBUTTON  VK_SCROLL
#define PLAYBUTTON VK_OEM_5

#ifdef _DEBUG
extern Output *g_lpOut;
#endif

class Macro;
class SysTray;
class Hotkeys;

extern int   g_nCounterDigits;
extern int   g_nCounter;
extern bool  g_bCounterHex;
extern bool  g_bRecord;
extern Macro g_macro;
extern SysTray *g_lpSysTray;
extern Hotkeys *g_hotkeys;

#endif // __MAIN_H__
