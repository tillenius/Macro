#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

class BCLMessage {
public:
    int idx = 0;
    std::vector<char> data;

    void operator()(const char * buffer) {
        data.push_back('\xf0'); // SysEx
        data.push_back('\x00'); data.push_back(0x20); data.push_back(0x32); // Manufacturer
        data.push_back('\x7f'); // DeviceID: any
        data.push_back('\x15'); // Model: BCR2000
        data.push_back('\x20'); // Command: BCL message
        data.push_back(static_cast<char>((idx & 0xff00) >> 8));
        data.push_back(static_cast<char>(idx & 0xff));
        for (const char *ptr = buffer; *ptr != '\0'; ++ptr) {
            data.push_back(*ptr);
        }
        data.push_back('\xf7'); // End
        ++idx;
    }
};

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
    bool m_debug = false;

    static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    Midi & operator=(const Midi &) = delete;
    Midi(const Midi &) = delete;
    Midi() = default;

    ~Midi();
    bool init(const std::string & midiDeviceName);
    void stop();
    void add(int channel, int controller, std::function<void(int)> callback);
    void receive(int channel, int controller, int data);
    bool sendControlChange(char channel, char controller, char value);
    bool sendSysex(int size, char * data);
    bool sendMessage(BCLMessage & msg) { return sendSysex((int) msg.data.size(), &msg.data[0]); }
    void setChannelMap(std::unordered_map<int, std::unordered_map<int, Entry>> & channelMap) { m_channelMap.swap(channelMap); }
};
