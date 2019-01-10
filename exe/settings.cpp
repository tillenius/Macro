#include "settings.h"
#include "main.h"
#include "hotkeys.h"
#include "util/tokenizer.h"
#include "keynames.h"
#include "action.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <Windows.h>

namespace {

static DWORD readMod(std::vector<std::string> words, size_t & i, std::string & errorMsg) {
    DWORD modifier = 0;

    for (; i < words.size(); i++) {
        const int mod = Keynames::GetModValue(words[i]);
        if (mod != 0) {
            if ((modifier & mod) != 0) {
                errorMsg = "Modifier " + words[i] + " repeated";
                return 0xFFFFFFFF;
            }
            modifier |= mod;
        } else {
            return modifier;
        }
    }
    return modifier;
}

static DWORD readKeyName(const std::string & key, std::string & errorMsg) {

    // hex
    if (key.size() > 2 && key[0] == '#')
        return strtoul(key.c_str() + 1, 0, 16);

    // name
    std::unordered_map<std::string, DWORD> keys = Keynames::GetMap();
    if (auto it = keys.find(key); it != keys.end()) {
        return it->second;
    }
    errorMsg = "Unknown key \"" + key + "\"";
    return 0xffffffff;
}

static bool readKey(std::vector<std::string> words, size_t &idx, UINT &mod, UINT &vk, std::string & errorMsg) {
    if (!(idx < words.size())) {
        errorMsg = "Key not specified";
        return false;
    }
    mod = readMod(words, idx, errorMsg);
    if (mod == 0xffffffff) {
        return false;
    }
    vk = readKeyName(words[idx++], errorMsg);
    if (vk == 0xffffffff) {
        return false;
    }
    return true;
}

} // namespace

bool Settings::readConfig(Hotkeys & hotkeys, Midi & midi, std::string & errorMsg) {

    std::ifstream prefs;

    prefs.open("macro.cfg");
    if (!prefs.is_open()) {
        errorMsg = "Cannot open file \"macro.cfg\" for reading.";
        return false;
    }

    std::string line;
    int lineCount = 0;

    while (!prefs.eof()) {
        std::getline(prefs, line);
        ++lineCount;
        Tokenizer t(line, " ,()\t\n\r");
        std::vector<std::string> words;
        while (t.hasNext())
            words.push_back(t.next());

        if (words.size() < 1)
            continue;

        size_t z = 1;

        const int NONE = 0;
        const int KEY = 1;
        const int MIDI = 2;
        struct { const char * name; int numArgs; int input; } table[] = {
            {"COUNTER_DIGITS", 1, NONE},
            {"COUNTER_START", 1, NONE},
            {"COUNTER_HEX", 1, NONE},
            {"MIDIINTERFACE", -1, NONE},
            {"MIDIBUTTON", 3, NONE},
            {"MIDIRELENC", -1, NONE},
            {"RECBUTTON", 0, KEY | MIDI},
            {"PLAYBUTTON", 0, KEY | MIDI},
            {"MINIMIZE", 0, KEY | MIDI},
            {"NEXTWINDOW", 0, KEY | MIDI},
            {"PREVWINDOW", 0, KEY | MIDI},
            {"PROGRAM", 3, KEY | MIDI},
            {"ACTIVATE", 3, KEY | MIDI},
            {"RUNORACTIVATE", 6, KEY | MIDI},
            {"FILE", 1, KEY | MIDI},
            {"MOVEWINDOW", -1, KEY | MIDI},
            {"MOVEWINDOWX", 0, MIDI},
            {"MOVEWINDOWY", 0, MIDI},
            {"RESIZEWINDOWX", 0, MIDI},
            {"RESIZEWINDOWY", 0, MIDI},
            {"SWITCHWINDOW", 0, MIDI},
        };

        int index = -1;
        for (int i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
            if (words[0] == table[i].name) {
                index = i;
                break;
            }
        }
        
        if (index == -1) {
            errorMsg = "Line " + std::to_string(lineCount) + ": Unknown command " + words[0] + ".";
            return false;
        }

        std::string msg;
        UINT mod;
        UINT key;
        int midiChannel = -1;
        int midiController = -1;

        if ((table[index].input & MIDI) != 0) {
            if (words.size() > z + 2 && words[z] == "MIDI") {
                ++z;
                midiChannel = std::stoi(words[z++]);
                midiController = std::stoi(words[z++]);
            } else if (table[index].input == MIDI) {
                errorMsg = "Line " + std::to_string(lineCount) + ": invalid midi config for command " + words[0] + ".";
            }
        }
        if (midiChannel == -1 && (table[index].input & KEY) != 0) {
            if (!readKey(words, z, mod, key, msg)) {
                errorMsg = "Line " + std::to_string(lineCount) + ": " + msg + " for command " + words[0] + ".";
                return false;
            }
        }

        if (words.size() != z + table[index].numArgs && table[index].numArgs != -1) {
            errorMsg = "Line " + std::to_string(lineCount) + ": Wrong number of arguments for command " + words[0] + ".";
            return false;
        }

        if (words[0] == "COUNTER_DIGITS") {
            DWORD num = atoi(words[1].c_str());
            if (num != 0 && num < 10)
                m_counterDigits = num;
            else {
                errorMsg = "Line " + std::to_string(lineCount) + ": COUNTER_DIGITS must be between 1 and 9.";
                return false;
            }
        } else if (words[0] == "COUNTER_START") {
            m_counter = atoi(words[1].c_str());
        } else if (words[0] == "COUNTER_HEX") {
            if (words[1] == "ON")
                m_counterHex = true;
            else if (words[1] == "OFF")
                m_counterHex = false;
        } else if (words[0] == "RECBUTTON") {
            m_recbutton = key;
        } else if (words[0] == "PLAYBUTTON") {
            m_playbutton = key;
        } else if (words[0] == "MIDIINTERFACE") {
            if (words.size() > 1) {
                midi.init(words[1]);
            } else {
                midi.init("");
            }
        } else if (words[0] == "MIDIBUTTON") {
            const int button = std::stoi(words[1]);
            const int channel = std::stoi(words[2]);
            const int controller = std::stoi(words[3]);
            midi.addButton(button, channel, controller);
        } else if (words[0] == "MIDIRELENC") {
            if (3 < words.size()) {
                const int encoder = std::stoi(words[1]);
                const int channel = std::stoi(words[2]);
                const int controller = std::stoi(words[3]);
                if (4 < words.size()) {
                    midi.addRelativeEncoder(encoder, channel, controller, words[4]);
                } else {
                    midi.addRelativeEncoder(encoder, channel, controller, "");
                }
            }
        } else if (words[0] == "MINIMIZE") {
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [](int data) { Action::minimize(); });
            } else {
                hotkeys.add(mod, key, []() { Action::minimize(); });
            }
        } else if (words[0] == "NEXTWINDOW") {
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [](int data) { Action::nextWindow(); });
            } else {
                hotkeys.add(mod, key, []() { Action::nextWindow(); });
            }
        } else if (words[0] == "PREVWINDOW") {
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [](int data) { Action::prevWindow(); });
            } else {
                hotkeys.add(mod, key, []() { Action::prevWindow(); });
            }
        } else if (words[0] == "MOVEWINDOW") {
            if (z + 2 <= words.size()) {
                const int x = atoi(words[z++].c_str());
                const int y = atoi(words[z++].c_str());
                int cx = -1;
                int cy = -1;
                if (z + 2 <= words.size()) {
                    cx = atoi(words[z++].c_str());
                    cy = atoi(words[z++].c_str());
                }
                if (midiChannel != -1) {
                    midi.add(midiChannel, midiController, [x,y,cx,cy](int data) { Action::setWindowPos(x, y, cx, cy); });
                } else {
                    hotkeys.add(mod, key, [x,y,cx,cy]() { Action::setWindowPos(x, y, cx, cy); });
                }
            }
        } else if (words[0] == "MOVEWINDOWX") {
            midi.add(midiChannel, midiController, [](int data) { Action::moveWindow(data - 64, 0, 0, 0); });
        } else if (words[0] == "MOVEWINDOWY") {
            midi.add(midiChannel, midiController, [](int data) { Action::moveWindow(0, data - 64, 0, 0); });
        } else if (words[0] == "RESIZEWINDOWX") {
            midi.add(midiChannel, midiController, [](int data) { Action::moveWindow(0, 0, data - 64, 0); });
        } else if (words[0] == "RESIZEWINDOWY") {
            midi.add(midiChannel, midiController, [](int data) { Action::moveWindow(0, 0, 0, data - 64); });
        } else if (words[0] == "SWITCHWINDOW") {
            midi.add(midiChannel, midiController, [](int data) { Action::switchWindow(data - 64); });
        } else if (words[0] == "PROGRAM") {
            const std::string & appName = words[z++];
            const std::string & cmdLine = words[z++];
            const std::string & currDir = words[z++];
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [appName, cmdLine, currDir](int data) { Action::run(appName, cmdLine, currDir, SW_SHOW); });
            } else {
                hotkeys.add(mod, key, [appName, cmdLine, currDir]() { Action::run(appName, cmdLine, currDir, SW_SHOW); });
            }
        } else if (words[0] == "ACTIVATE") {
            const std::string & exeName = words[z++];
            const std::string & windowName = words[z++];
            const std::string & className = words[z++];
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [exeName, windowName, className](int data) { Action::activate(exeName, windowName, className); });
            } else {
                hotkeys.add(mod, key, [exeName, windowName, className]() { Action::activate(exeName, windowName, className); });
            }
        } else if (words[0] == "RUNORACTIVATE") {
            const std::string & appName = words[z++];
            const std::string & cmdLine = words[z++];
            const std::string & currDir = words[z++];
            const std::string & exeName = words[z++];
            const std::string & windowName = words[z++];
            const std::string & className = words[z++];
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [appName, cmdLine, currDir, exeName, windowName, className](int data) { Action::activateOrRun(exeName, windowName, className, appName, cmdLine, currDir, SW_SHOW); });
            } else {
                hotkeys.add(mod, key, [appName, cmdLine, currDir, exeName, windowName, className]() { Action::activateOrRun(exeName, windowName, className, appName, cmdLine, currDir, SW_SHOW); });
            }
        } else if (words[0] == "FILE") {
            const std::string & fileName = words[z++];
            if (midiChannel != -1) {
                midi.add(midiChannel, midiController, [fileName, this](int data) { Action::runSaved(fileName, *this); });
            } else {
                hotkeys.add(mod, key, [fileName, this]() { Action::runSaved(fileName, *this); });
            }
        } else {
            errorMsg = "Line " + std::to_string(lineCount) + ": Unknown command " + words[0];
            return false;
        }
    }

    hotkeys.add(0, m_recbutton, []() { g_app->record(); });
    hotkeys.add(0, m_playbutton, []() { g_app->playback(); });

    return true;
}
