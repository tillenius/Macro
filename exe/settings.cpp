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
    if (key.size() > 2 && key[0] == '0' && key[1] == 'x')
        return strtoul(key.c_str() + 2, 0, 16);

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

bool Settings::readConfig(Hotkeys & hotkeys, std::string & errorMsg) {

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

        struct { const char * name; int numArgs; bool key; } table[] = {
            {"COUNTER_DIGITS", 1, false},
            {"COUNTER_START", 1, false},
            {"COUNTER_HEX", 1, false},
            {"RECBUTTON", 0, true},
            {"PLAYBUTTON", 0, true},
            {"MINIMIZE", 0, true},
            {"PROGRAM", 3, true},
            {"ACTIVATE", 3, true},
            {"RUNORACTIVATE", 6, true},
            {"FILE", 1, true}
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

        if (table[index].key) {
            if (!readKey(words, z, mod, key, msg)) {
                errorMsg = "Line " + std::to_string(lineCount) + ": " + msg + " for command " + words[0] + ".";
                return false;
            }
        }

        if (words.size() != z + table[index].numArgs) {
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
        } else if (words[0] == "MINIMIZE") {
            hotkeys.add(mod, key, []() { Action::minimize(); });
        } else if (words[0] == "PROGRAM") {
            const std::string & appName = words[z++];
            const std::string & cmdLine = words[z++];
            const std::string & currDir = words[z++];
            hotkeys.add(mod, key, [appName, cmdLine, currDir]() { Action::run(appName, cmdLine, currDir, SW_SHOW); });
        } else if (words[0] == "ACTIVATE") {
            const std::string & exeName = words[z++];
            const std::string & windowName = words[z++];
            const std::string & className = words[z++];
            hotkeys.add(mod, key, [exeName, windowName, className]() { Action::activate(exeName, windowName, className); });
        } else if (words[0] == "RUNORACTIVATE") {
            const std::string & appName = words[z++];
            const std::string & cmdLine = words[z++];
            const std::string & currDir = words[z++];
            const std::string & exeName = words[z++];
            const std::string & windowName = words[z++];
            const std::string & className = words[z++];
            hotkeys.add(mod, key, [appName, cmdLine, currDir, exeName, windowName, className]() { Action::activateOrRun(exeName, windowName, className, appName, cmdLine, currDir, SW_SHOW); });
        } else if (words[0] == "FILE") {
            const std::string & fileName = words[z++];
            hotkeys.add(mod, key, [fileName, this]() { Action::runSaved(fileName, *this); });
        } else {
            errorMsg = "Line " + std::to_string(lineCount) + ": Unknown command " + words[0];
            return false;
        }
    }

    hotkeys.add(0, m_recbutton, []() { g_app->record(); });
    hotkeys.add(0, m_playbutton, []() { g_app->playback(); });

    return true;
}
