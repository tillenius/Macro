
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include "tokenizer.h"

using namespace std;

DWORD readMod(vector<string> words, size_t &i) {
  DWORD modifier = 0;

  for (; i < words.size(); i++) {
    if (words[i] == "ALT" ) {
      if ( (modifier & MOD_ALT) != 0) return 0xFFFFFFFF;
      else modifier = modifier | MOD_ALT;
    }
    else if (words[i] == "CONTROL" || words[i] == "CTRL" ) {
      if ( (modifier & MOD_CONTROL) != 0) return 0xFFFFFFFF;
      else modifier = modifier | MOD_CONTROL;
    }
    else if (words[i] == "SHIFT" ) {
      if ( (modifier & MOD_SHIFT) != 0) return 0xFFFFFFFF;
      else modifier = modifier | MOD_SHIFT;
    }
    else if ( words[i] == "WIN" ) {
      if ( (modifier & MOD_WIN) != 0) return 0xFFFFFFFF;
      else modifier = modifier | MOD_WIN;
    }
    else
      return modifier;
  }
  return modifier;
}

DWORD readKeyName(string key) {
  static map<string, DWORD> keys;
  if (keys.empty()) {
    #define KEYNAME(a, b) keys[a]=b;
    #include "keynames.h"
  }

  // hex
  if (key.size() > 2 && key[0] == '0' && key[1] == 'x')
    return strtoul(key.c_str()+2, 0, 16);

  // name
  if (keys.find(key) == keys.end())
    return 0xffffffff;

  return keys[key];
}

bool readKey(vector<string> words, size_t &idx, UINT &mod, UINT &vk) {
  mod = readMod(words, idx);
  if (mod == 0xffffffff)
    return false;
  vk = readKeyName(words[idx++]);
  if (vk == 0xffffffff)
    return false;
  return true;
}

bool readConfig(void) {

  ifstream prefs;

  prefs.open("macro.cfg");
  if (!prefs.is_open())
    return false;

  string line;

  while (!prefs.eof()) {
    getline(prefs, line);
    Tokenizer t(line, string(" ,()\t\n\r"));
    vector <string> words;
    while (t.hasNext())
      words.push_back(t.next());

    if (words.size() < 1)
      continue;

    if (words.size() == 1)
      return false;

    size_t z = 1;

    UINT mod;
    UINT key;
    if (words[0] == "COUNTER_DIGITS") {
      DWORD num = atoi(words[1].c_str());
      if (num != 0 && num < 1024)
        g_nCounterDigits = num;
      else
        return false;
    }
    else if (words[0] == "COUNTER_START") {
      g_nCounter = atoi(words[1].c_str());
    }
    else if (words[0] == "COUNTER_HEX") {
      if (words[1] == "ON")
        g_bCounterHex = true;
      else if (words[1] == "OFF")
        g_bCounterHex = false;
    }
    else if (words[0] == "RECBUTTON")    { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_RECORD)); }
    else if (words[0] == "PLAYBUTTON")   { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_PLAYBACK)); }
    else if (words[0] == "MINIMIZE")     { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_MINIMIZE)); }
    else if (words[0] == "WINAMP_PREV")  { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_PREV)); }
    else if (words[0] == "WINAMP_PLAY")  { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_PLAY));}
    else if (words[0] == "WINAMP_PAUSE") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_PAUSE));}
    else if (words[0] == "WINAMP_PLAYPAUSE") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_PLAYPAUSE));}
    else if (words[0] == "WINAMP_STOP")  { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_STOP));}
    else if (words[0] == "WINAMP_NEXT")  { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_NEXT));}
    else if (words[0] == "WINAMP_VOLUP") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_VOLUP));}
    else if (words[0] == "WINAMP_VOLDOWN") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_VOLDOWN));}
    else if (words[0] == "WINAMP_FASTFORWARD") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_FF));}
    else if (words[0] == "WINAMP_FASTREWIND") { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_WINAMP_FR));}
    else if (words[0] == "NEXTWIN")      { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_NEXTWIN));}
    else if (words[0] == "PREVWIN")      { if (!readKey(words, z, mod, key)) return false; g_hotkeys->Add(new ActionID(mod, key, HOTKEY_PREVWIN));}
    else if (words[0] == "PROGRAM") {
      if (!readKey(words, z, mod, key))
        return false;
      if (words.size() != z+3)
        return false;
      g_hotkeys->Add(new RunAction(mod, key, words[z].c_str(), words[z+1].c_str(), words[z+2].c_str(), SW_SHOW));
    }
    else if (words[0] == "ACTIVATE") {
      if (!readKey(words, z, mod, key))
        return false;
      if (words.size() != z+3)
        return false;
      g_hotkeys->Add(new ActivateAction(mod, key, words[z].c_str(), words[z+1].c_str(), words[z+2].c_str()));
    }
    else if (words[0] == "RUNORACTIVATE") {
      if (!readKey(words, z, mod, key))
        return false;
      if (words.size() != z+6)
        return false;
      g_hotkeys->Add(new RunOrActivate(mod, key, words[z].c_str(), words[z+1].c_str(), words[z+2].c_str(), SW_SHOW,
                                       words[z+3].c_str(), words[z+4].c_str(), words[z+5].c_str()));
    }
    else if (words[0] == "FILE") {
      if (!readKey(words, z, mod, key))
        return false;
      if (words.size() != z+1)
        return false;
      g_hotkeys->Add(new RunSaved(mod, key, words[z].c_str()));
    }
  }
  return true;
}
