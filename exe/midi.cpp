#include "midi.h"
#include "main.h"
#include <string>
#include <vector>

#pragma comment(lib, "winmm.lib")

bool Midi::init(HWND hwnd, const std::string & midiDeviceName) {
    stop();
    const UINT num_in_devs = midiInGetNumDevs();
    for (UINT i = 0; i < num_in_devs; i++) {
        MIDIINCAPS mic;
        if (!midiInGetDevCaps(i, &mic, sizeof(MIDIINCAPS))) {
            if (m_debug) {
                OutputDebugString((std::string("MIDI: Input device ") + std::to_string((int)i+1) + "/" + std::to_string((int)num_in_devs) + ": " + mic.szPname + "\n").c_str());
            }
            if (midiDeviceName.empty() || midiDeviceName == mic.szPname) {
                MMRESULT res = midiInOpen(&m_hMidiIn, i, reinterpret_cast<DWORD_PTR>(hwnd), reinterpret_cast<DWORD_PTR>(this), CALLBACK_WINDOW);
                if (res != 0) {
                    m_hMidiIn = NULL;
                    return false;
                }
                midiInStart(m_hMidiIn);
                if (!m_debug) {
                    break;
                }
            }
        }
    }

    const UINT num_out_devs = midiOutGetNumDevs();
    for (UINT i = 0; i < num_out_devs; i++) {
        MIDIOUTCAPS moc;
        if (!midiOutGetDevCaps(i, &moc, sizeof(MIDIOUTCAPS))) {
            if (m_debug) {
                OutputDebugString((std::string("MIDI: Output device ") + std::to_string((int)i+1) + "/" + std::to_string((int)num_out_devs) + ": " + moc.szPname + "\n").c_str());
            }
            if (midiDeviceName == moc.szPname) {
                MMRESULT res = midiOutOpen(&m_hMidiOut, i, NULL, reinterpret_cast<DWORD_PTR>(this), CALLBACK_NULL);
                if (res != 0) {
                    m_hMidiOut = NULL;
                    return false;
                }
                if (!m_debug) {
                    break;
                }
            }
        }
    }
    return true;
}

Midi::~Midi() {
    stop();
}

void Midi::stop() {
    if (m_hMidiIn != NULL) {
        midiInStop(m_hMidiIn);
        midiInClose(m_hMidiIn);
        m_hMidiIn = NULL;
    }
    if (m_hMidiOut != NULL) {
        midiOutClose(m_hMidiOut);
        m_hMidiOut = NULL;
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

void Midi::receive(DWORD dwParam1) {
    static constexpr int CHANNEL_MASK = 0x0f;
    static constexpr int TYPE_MASK = 0xf0;
    static constexpr int TYPE_NOTE_ON = 0x9;
    static constexpr int TYPE_NOTE_OFF = 0x8;
    static constexpr int TYPE_CONTROL_CHANGE = 0xb;
    static constexpr int TYPE_PROGRAM_CHANGE = 0xc;
    const int type = ( dwParam1 & TYPE_MASK ) >> 4;
    const int channel = 1 + dwParam1 & CHANNEL_MASK;
    const int controller = (dwParam1 & 0xff00) >> 8;
    const int data = (dwParam1 & 0xff0000) >> 16;
    if (m_debug) {
        OutputDebugString((std::string("MIDI: MIM_DATA type = ") + std::to_string(type) +
                            " channel = " + std::to_string(channel) +
                            " controller=" + std::to_string(controller) +
                            " data=" + std::to_string(data) + "\n").c_str());
    }
    if (type == TYPE_CONTROL_CHANGE || type == TYPE_NOTE_ON || type == TYPE_NOTE_OFF ) {
        receive(type, channel, controller, data);
    }
}

void Midi::receive(int type, int channel, int controller, int data) {
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

	try {
		entry.callback(type, data + entry.hiData);
	}
	catch (const std::exception & ex) {
		SwitchToThisWindow(g_app->m_hWnd, TRUE);
		MessageBox(0, ex.what(), 0, MB_OK);
	}
	entry.hiData = 0;
}

void Midi::add(int channel, int controller, std::function<void(int,int)> callback) {
    m_channelMap[channel][controller] = Entry(callback);
}
