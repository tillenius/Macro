#include "midi.h"
#include "main.h"
#include <string>
#include <vector>

#pragma comment(lib, "winmm.lib")

bool Midi::init(const std::string & midiDeviceName) {
    stop();
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

	try {
		entry.callback(data + entry.hiData);
	}
	catch (const std::exception & ex) {
		SwitchToThisWindow(g_app->m_hWnd, TRUE);
		MessageBox(0, ex.what(), 0, MB_OK);
	}
	entry.hiData = 0;
}

void Midi::add(int channel, int controller, std::function<void(int)> callback) {
    m_channelMap[channel][controller] = Entry(callback);
}

void CALLBACK Midi::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    Midi * midi = (Midi *) dwInstance;
    if (wMsg == MIM_DATA) {
        const int CHANNEL_MASK = 0x0f;
        const int TYPE_MASK = 0xf0;
        const int TYPE_CONTROL_CHANGE = 0xb;
        const int type = ( dwParam1 & TYPE_MASK ) >> 4;
        const int channel = 1 + dwParam1 & CHANNEL_MASK;
        const int controller = (dwParam1 & 0xff00) >> 8;
        const int data = (dwParam1 & 0xff0000) >> 16;
        if (midi->m_debug) {
            OutputDebugString((std::string("MIDI: MIM_DATA type = ") + std::to_string(type) +
                               " channel = " + std::to_string(channel) +
                               " controller=" + std::to_string(controller) +
                               " data=" + std::to_string(data) + "\n").c_str());
        }
        if (type == TYPE_CONTROL_CHANGE) {
            midi->receive(channel, controller, data);
        }
    } else if (midi->m_debug) {
        OutputDebugString((std::string("MIDI: msg=") + std::to_string(wMsg) + " param1=" + std::to_string(dwParam1) + " param2=" + std::to_string(dwParam2) + "\n").c_str());
    }
}
