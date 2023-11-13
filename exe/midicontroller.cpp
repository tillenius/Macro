
#include <Windows.h>
#include "midicontroller.h"

#include <gdiplus.h>
#include "midi.h"
#include "main.h"
#include "action.h"

//const char * tbl[] = {
//    /*      0       1         2     3        4       5       6       7        8       9       A        B        C      D         E      F*/
//    /*0*/   "ESC",  "F1",     "F2", "F3",    "F4",   "F5",   "F6",   "F7",    "F8",   "F9",   "F10",   "F11",   "F12", "",       "",    "",
//    /*1*/   "§",    "1",      "2",  "3",     "4",    "5",    "6",    "7",     "8",    "9",    "0",     "+",     "´",   "",       "",    "",
//    /*2*/   "TAB",  "Q",      "W",  "E",     "R",    "T",    "Y",    "U",     "I",    "O",    "P",     "Å",     "^",   "",       "",    "",
//    /*3*/   "CAPS", "A",      "S",  "D",     "F",    "G",    "H",    "J",     "K",    "L",    "Ö",     "Ä",     "\n",  "'",      "",    "",
//    /*4*/   "UP",   "LSHIFT", "<",  "Z",     "X",    "C",    "V",    "B",     "N",    "M",    ",",     ".",     "-",   "RSHIFT", "",    "",
//    /*5*/   "LEFT", "DOWN",   "",   "LCTRL", "MENY", "ALT",  "",     " ",     "LEFT", "FN",   "RCTRL", "ALTGR", "",    "WIN",    "",    "RIGHT",
//    /*6*/   "",     "",       "",   "DEL",   "",     "PGDN", "",     "",      "",     "",     "",      "",      "",    "",       "END", "",
//    /*7*/   "",     "",       "",   "",      "",     "",     "PGUP", "",      "",     "HOME", "INS",   "BACK",  "",    "",       "",    "",
//};

extern HINSTANCE g_hInstance;
constexpr int WIDTH = 1200;
constexpr int HEIGHT = 500;

static MidiController * g_self;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT: {

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hwnd, &ps );

            Gdiplus::FontFamily fontFamilyHeader(L"Segoe UI");
            Gdiplus::Font fontHeader(&fontFamilyHeader, 20, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);

            Gdiplus::FontFamily fontFamily(L"Lucida Console");
            Gdiplus::Font font(&fontFamily, 12, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush brush(Gdiplus::Color::Black);

            Gdiplus::Graphics graphics(hwnd);
            Gdiplus::StringFormat stringFormat;
            Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0));

            constexpr int maxWidth = WIDTH - 20;
            constexpr int maxHeight = 100;

            float ypos = 10.0f;
            for (int i = 0; i < MidiController::NUM_CLIPBOARDS; ++i) {
                std::wstring label = std::to_wstring(i + 1);

                Gdiplus::PointF pos(30.0f, ypos);

                graphics.DrawString(label.c_str(), (int) label.length(), &fontHeader, Gdiplus::PointF(10.0f, ypos - 9.0f), &stringFormat, &brush);

                if (g_self->m_clipboard[i].empty()) {
                    ypos += 20.0f;
                } else {
                    Gdiplus::RectF rect;
                    graphics.MeasureString(g_self->m_clipboard[i].c_str(), (int) g_self->m_clipboard[i].length(), &font, pos, &stringFormat, &rect);
                    if (rect.Width > maxWidth) {
                        rect.Width = maxWidth;
                    }
                    if (rect.Height > maxHeight) {
                        rect.Height = maxHeight;
                    }
                    rect.Inflate(1.0, 1.0f);
                    graphics.DrawRectangle(&pen, rect);
                    rect.X += 1;
                    rect.Y += 2;
                    graphics.DrawString(g_self->m_clipboard[i].c_str(), (int) g_self->m_clipboard[i].length(), &font, rect, &stringFormat, &brush);

                    ypos += rect.Height + 10.0f;
                }
            }
            EndPaint(hwnd, &ps);

            return 0;
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

MidiController::MidiController() {
    WNDCLASS wClass;
    wClass.lpszClassName = "keymacro-midicontroller";
    wClass.hInstance = g_hInstance;
    wClass.lpfnWndProc = WndProc;
    wClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wClass.hIcon = 0;
    wClass.lpszMenuName = NULL;
    wClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wClass.style = 0;
    wClass.cbClsExtra = 0;
    wClass.cbWndExtra = 0;
    if (RegisterClass(&wClass) == 0) {
        MessageBox(0, "RegisterClass() failed", 0, MB_OK);
    }
    g_self = this;
}

void MidiController::SetClipboard(int index) {
    if (::OpenClipboard(g_app->m_hWnd) == 0) {
        return;
    }
    HANDLE h = ::GetClipboardData(CF_UNICODETEXT);
    if (h != NULL) {
        wchar_t * data = (wchar_t *) ::GlobalLock(h);
        if (data != NULL) {
            m_clipboard[index] = data;
            ::GlobalUnlock(h);
        }
    }
    ::CloseClipboard();
}

void MidiController::PasteClipboard(int index) {
    if (::OpenClipboard(g_app->m_hWnd) == 0) {
        return;
    }

    ::EmptyClipboard();

    HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t)*(m_clipboard[index].length() + 1));
    if (hglbCopy != NULL) {
        wchar_t * buffer = (wchar_t *) GlobalLock(hglbCopy);
        if (buffer == nullptr) {
            return;
        }
        memcpy(buffer, m_clipboard[index].c_str(), sizeof(wchar_t)*m_clipboard[index].length());
        buffer[m_clipboard[index].length()] = 0;
        GlobalUnlock(hglbCopy);

        ::SetClipboardData(CF_UNICODETEXT, hglbCopy); // @TODO: CF_UNICODETEXT?
    }
    ::CloseClipboard();
    Action::paste();
    return;

}

bool MidiController::Handle(int type, int channel, int controller, int data) {
    if (type != Midi::TYPE_CONTROL_CHANGE) {
        return false;
    }
    if (channel != 1) {
        return false;
    }

    switch (controller) {
        case 0x53: { // LCTRL
            m_lctrl = data == 1;
            return true;
        }
        case 0x5a: { // RCTRL
            m_rctrl = data == 1;
            return true;
        }
        case 0x4D: { // RSHIFT:
            m_rshift = data == 1;
            return true;
        }

        case 0x41: { // LSHIFT
            m_lshift = data == 1;
            if (data == 0) {
                DestroyWindow(m_hwnd);
                m_hwnd = NULL;
            } else if (data == 1) {
                m_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, "keymacro-midicontroller", "midicontroller", WS_POPUP, 0, 0, WIDTH, HEIGHT, NULL, NULL, g_hInstance, NULL);
                ::ShowWindow(m_hwnd, SW_SHOW);
            }
            return true;
        }
    }

    if (data == 0) {
        return true;
    }

    switch (controller) {
        case 0x05: { // "F5"
            Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.Contiue");
            return true;
        }
        case 0x09: { // "F9"
            Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.ToggleBreakpoint");
            return true;
        }
        case 0x0A: { // "F10"
            Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.StepOver");
            return true;
        }
        case 0x0B: { // "F11"
            if (m_lshift || m_rshift) {
                Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.StepOver");
            } else {
                Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.StepOut");
            }
            return true;
        }  
        case 0x0C: { // "F12"
            if (m_breakpoints) {
                Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.DisableAllBreakpoints");
                m_breakpoints = false;
            } else {
                Action::execute_in_vs(g_app->m_settings.m_envdte, "Debug.EnableAllBreakpoints");
                m_breakpoints = true;
            }
            return true;
        }

        case 0x11:   // "1"
        case 0x12:   // "2"
        case 0x13:   // "3"
        case 0x14: { // "4"
            const int index = [controller]() {
                switch (controller) {
                    case 0x11: return 0;
                    case 0x12: return 1;
                    case 0x13: return 2;
                    case 0x14: return 3;
                    default:
                        return 0;
                    }
                }();

            if (m_lctrl || m_rctrl) {
                SetClipboard(index);
            } else {
                PasteClipboard(index);
            }
            return true;
        }
    }
    return true;
}
