#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

class Midi {
public:

    struct Entry {
        bool hires = false;
        int hiData = 0;
        std::function<void(int)> callback;
        Entry() {}
        Entry(std::function<void(int)> callback) : callback(callback) {}
        Entry(bool hires, std::function<void(int)> callback) : hires(hires), callback(callback) {}
    };

    std::unordered_map<int, std::unordered_map<int, Entry>> m_channelMap; // channel -> controller -> entry

    HMIDIIN m_hMidiIn = NULL;
    HMIDIOUT m_hMidiOut = NULL;

    bool m_midiInStarted = false;
    bool m_midiOutStarted = false;

    static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    ~Midi();
    bool init(const std::string & midiDeviceName);
    void add(int channel, int controller, std::function<void(int)> callback);
    void receive(int channel, int controller, int data);
    bool sendControlChange(char channel, char controller, char value);
    bool sendSysex(int size, char * data);
    bool addButton(int button, int channel, int controller);
    bool addRelativeEncoder(int encoder, int channel, int controller);
};
