#pragma once

#include "windows.h"
#include <string>

class Hotkeys;
class Midi;

class Settings {
public:
    int m_counter = 0;
    int m_counterDigits = 3;
    bool m_counterHex = false;
    UINT m_recbutton = VK_SCROLL;
    UINT m_playbutton = VK_OEM_5;

    bool readConfig(Hotkeys & hotkeys, Midi & midi, std::string & errorMsg);
};
