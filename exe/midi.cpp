#include "midi.h"
#include <string>
#include <vector>

#pragma comment(lib, "winmm.lib")

class BCL {
public:
    struct bcl_msg_t {
        int idx;
        std::vector<char> data;

        bcl_msg_t() : idx(0) {}

        void operator()(const char *format, ...) {
            data.push_back('\xf0'); // SysEx
            data.push_back('\x00'); data.push_back(0x20); data.push_back(0x32); // Manufacturer
            data.push_back('\x7f'); // DeviceID: any
            data.push_back('\x15'); // Model: BCR2000
            data.push_back('\x20'); // Command: BCL message
            data.push_back(static_cast<char>((idx & 0xff00) >> 8));
            data.push_back(static_cast<char>(idx & 0xff));

            char buffer[80];
            va_list argptr;
            va_start(argptr, format);
            vsnprintf(buffer, sizeof(buffer), format, argptr);
            va_end(argptr);

            for (char *ptr = buffer; *ptr != '\0'; ++ptr) {
                data.push_back(*ptr);
            }

            data.push_back('\xf7'); // End
            ++idx;
        }
    };

    static bcl_msg_t button(int button, int channel, int controller) {
        bcl_msg_t m;
        m("$rev R1");
        m("$button %d", button);
        m("  .showvalue on");
        m("  .easypar CC %d %d 0 1 toggleon", channel, controller);
        m("  .mode down");
        m("$end");
        return m;
    }

    static bcl_msg_t toggleButton(int button, int channel, int controller) {
        bcl_msg_t m;
        m("$rev R1");
        m("$button %d", button);
        m("  .showvalue on");
        m("  .easypar CC %d %d 0 1 toggleoff", channel, controller);
        m("  .mode toggle");
        m("$end");
        return m;
    }

    static bcl_msg_t onOffButton(int button, int channel, int controller) {
        bcl_msg_t m;
        m("$rev R1");
        m("$button %d", button);
        m("  .showvalue on");
        m("  .easypar CC %d %d 0 1 toggleoff", channel, controller);
        m("  .mode updown");
        m("$end");
        return m;
    }

    static bcl_msg_t relativeEncoder(int encoder, int channel, int controller) {
        bcl_msg_t m;
        m("$rev R1");
        m("$encoder %d", encoder);
        m("  .showvalue off");
        m("  .easypar CC %d %d 0 127 relative-2", channel, controller);
        m("  .resolution 96 192 768 2304");
        m("  .mode 1dot/off");
        m("$end");
        return m;
    }

    static bcl_msg_t absoluteEncoder(int encoder, int channel, int controller, int minVal, int maxVal, bool hires) {
        bcl_msg_t m;
        m("$rev R1");
        m("$encoder %d", encoder);
        m("  .showvalue on");
        m("  .easypar CC %d %d %d %d absolute%s", channel, controller, minVal, maxVal, hires ? "/14" : "");
        m("  .mode bar");
        m("$end");
        return m;
    }
};

bool Midi::init(const std::string & midiDeviceName) {
    const UINT num_in_devs = midiInGetNumDevs();
    for (UINT i = 0; i < num_in_devs; i++) {
        MIDIINCAPS mic;
        if (!midiInGetDevCaps(i, &mic, sizeof(MIDIINCAPS))) {
            if (midiDeviceName.empty() || midiDeviceName == mic.szPname) {
                MMRESULT res = midiInOpen(&m_hMidiIn, i, reinterpret_cast<DWORD_PTR>(MidiInProc), reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION);
                if (res != 0) {
                    m_hMidiIn = NULL;
                    return false;
                }
                midiInStart(m_hMidiIn);
                break;
            }
        }
    }

    if (!midiDeviceName.empty()) {
        const UINT num_out_devs = midiOutGetNumDevs();
        for (UINT i = 0; i < num_out_devs; i++) {
            MIDIOUTCAPS moc;
            if (!midiOutGetDevCaps(i, &moc, sizeof(MIDIOUTCAPS))) {
                if (midiDeviceName == moc.szPname) {
                    MMRESULT res = midiOutOpen(&m_hMidiOut, i, NULL, reinterpret_cast<DWORD_PTR>(this), CALLBACK_NULL);
                    if (res != 0) {
                        m_hMidiOut = NULL;
                        return false;
                    }
                    break;
                }
            }
        }
    }
    return true;
}

Midi::~Midi() {
    if (m_hMidiIn != NULL) {
        midiInStop(m_hMidiIn);
        midiInClose(m_hMidiIn);
    }
    if (m_hMidiOut != NULL) {
        midiOutClose(m_hMidiOut);
    }
}

bool Midi::sendControlChange(char channel, char controller, char value) {
    if (m_hMidiOut == NULL) {
        return false;
    }
    DWORD msg = 0xb0 | channel | (controller << 8) | (value << 16);
    MMRESULT res = midiOutShortMsg(m_hMidiOut, msg);
    if (res != MMSYSERR_NOERROR) {
        return false;
    }
    return true;
}

bool Midi::sendSysex(int size, char * data) {
    if (m_hMidiOut == NULL) {
        return false;
    }
    MIDIHDR midiHeader;
    midiHeader.dwFlags = 0;
    midiHeader.lpData = data;
    midiHeader.dwBufferLength = size;

    MMRESULT res = midiOutPrepareHeader(m_hMidiOut, &midiHeader, sizeof(midiHeader));
    if (res != MMSYSERR_NOERROR) {
        return false;
    }
    res = midiOutLongMsg(m_hMidiOut, &midiHeader, sizeof(midiHeader));
    if (res != MMSYSERR_NOERROR) {
        return false;
    }
    res = midiOutUnprepareHeader(m_hMidiOut, &midiHeader, sizeof(midiHeader));
    if (res != MMSYSERR_NOERROR) {
        return false;
    }
    return true;
}

void Midi::receive(int channel, int controller, int data) {
    auto it = m_channelMap.find(channel);
    if (it == m_channelMap.end()) {
        return;
    }

    bool extended = false;
    auto ctrlIt = it->second.find(controller);
    if (ctrlIt == it->second.end()) {
        return;
    }

    Entry & entry = ctrlIt->second;

    if (entry.hires && controller >= 32 && controller <= 63) {
        ctrlIt = it->second.find(controller - 32);
        if (ctrlIt == it->second.end()) {
            return; // should never happen
        }
        ctrlIt->second.hiData = data << 7;
        return;
    }

    entry.callback(data + entry.hiData);
    entry.hiData = 0;
}

void Midi::add(int channel, int controller, std::function<void(int)> callback) {
    m_channelMap[channel][controller] = Entry(callback);
}

void CALLBACK Midi::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (wMsg == MIM_DATA) {
        Midi * midi = (Midi *) dwInstance;
        const int CHANNEL_MASK = 0x0f;
        const int TYPE_MASK = 0xf0;
        const int TYPE_CONTROL_CHANGE = 0xb0;
        if ((dwParam1 & TYPE_MASK) == TYPE_CONTROL_CHANGE) {
            midi->receive(1 + dwParam1 & CHANNEL_MASK, (dwParam1 & 0xff00) >> 8, (dwParam1 & 0xff0000) >> 16);
        }
    }
    return;
}

bool Midi::addButton(int button, int channel, int controller) {
    BCL::bcl_msg_t m{BCL::button(button, channel, controller)};
    return sendSysex((int) m.data.size(), &m.data[0]);
}

bool Midi::addRelativeEncoder(int encoder, int channel, int controller) {
    BCL::bcl_msg_t m{BCL::relativeEncoder(encoder, channel, controller)};
    return sendSysex((int) m.data.size(), &m.data[0]);
}
