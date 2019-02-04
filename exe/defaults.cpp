const char * DEFAULT_CONFIG_FILE = 
R"#(import macro;
import ctypes;
import os;

from ctypes.wintypes import HWND

OutputDebugString = ctypes.windll.kernel32.OutputDebugStringW
GetWindowRect = ctypes.windll.user32.GetWindowRect
RECT = ctypes.wintypes.RECT
HWND = ctypes.wintypes.HWND

prev_desc = ""
prev_command = ""

config = {
    # number of digits in the counter
    "counter_digits": 3,

    # if the counter should be in hexadecimal
    "counter_hex": False,

    # the value the counter starts from
    "conter_start": 0,

    # the key to start and stop recording a keyboard macro
    "rec": "SCROLL",

    # the key used to replay the recorded macro
    "play": "OEM_5",

    # which midi interface to use (both input and b-control output)
    "midiinterface": "BCR2000",

    # default channel, when none is specified explicitly
    "midichannel": 1
}

bcl_setup = [
    ("button", 41, {"controller": 1}),
    ("encoder", 41, {"controller": 2, "type": "relative"}),
    ("encoder", 42, {"controller": 3, "type": "relative"}),
    ("encoder", 49, {"controller": 4, "type": "relative"}),
    ("encoder", 50, {"controller": 5, "type": "relative"}),
    ("encoder", 51, {"controller": 6, "type": "relative", "resolution": "10"})
]

def switch_window(value):
    if value > 0:
        next_window()
    else:
        prev_window()

def show_menu():
    global prev_desc
    global prev_command
    prev_size_cmd = ""
    hwnd = macro.get_current_window()
    rect = RECT()
    GetWindowRect(hwnd, ctypes.byref(rect))
    window_text = macro.get_window_title(hwnd)
    procid = macro.get_window_processid(hwnd)
    exename = macro.get_process_filename(procid)
    basename = os.path.basename(exename)
    classname = macro.get_class_name(hwnd)
    size_text = str(rect.left) + "," + str(rect.top) + "," + str(rect.right) + "," + str(rect.bottom)
    size_wh = str(rect.left) + "," + str(rect.top) + "," + str(rect.right-rect.left) + "," + str(rect.bottom-rect.top)

    choice = macro.menu( [
        ("Current window", [
            ("Exe: " + exename, 10),
            ("Title: " + window_text, 11),
            ("Class: " + classname, 12),
            ("Size: (" + size_text + ")", 13),
            ("Copy activate cmd", 14),
            ("Kill owning process", 15)
            ]),
        ("debug", [
            ("open window", 0),
            ("debug midi", 1),
            ("list", [
                ("windows", 20),
                ("processes", 21),
                ("modules", 22)
                ]),
            ]),
        ("snippets", [
            ("comment block", 50),
            ]),
        ("set window size", 30),
        ("copy current macro", 40),
        (prev_desc, 41)
        ]);
    if choice == 0:
        macro.show_debug(True)
    elif choice == 1:
        macro.midi_debug(True)
    elif choice == 10:
        macro.set_clipboard_text(exename);
    elif choice == 11:
        macro.set_clipboard_text(window_text);
    elif choice == 12:
        macro.set_clipboard_text(classname);
    elif choice == 13:
        prev_desc = "Set size (" + size_text + ")"
        prev_command = "macro.set_window_pos(macro.get_current_window(), " + size_wh + ")"
        macro.set_clipboard_text("macro.set_window_pos(macro.get_current_window()," + size_wh + ");");
    elif choice == 14:
        prev_desc = "Activate " + basename
        prev_command = "macro.activate(\""+basename+"\",\""+window_text+"\",\""+classname+"\")";
        macro.set_clipboard_text(prev_command)
        macro.notify(exename + "\n" + classname + "\n" + window_text)
    elif choice == 15:
        macro.terminate_process(macro.get_window_processid(hwnd))
    elif choice == 20:
        macro.list([ macro.get_window_title(hwnd) for hwnd in macro.get_windows() ])
    elif choice == 21:
        macro.list([ x["szExeFile"] for x in macro.get_processes() ])
    elif choice == 22:
        macro.list([ x["szModule"]+":"+x["szExePath"] for x in macro.get_modules() ])
    elif choice == 30:
        macro.set_window_pos(hwnd,10,10,1000,800);
    elif choice == 40:
        macro.set_clipboard_text(macro.get_macro_as_python())
    elif choice == 41:
        eval(prev_command)
    elif choice == 50:
        macro.set_clipboard_text("//=====================================================\n//\n//=====================================================\n")
        macro.paste()

def test():
    macro.notify("Hello!")

hotkeys = [
    ("key", "win+OEM_5",   lambda: show_menu()),
    ("key", "win+numpad1", lambda: test())
]
)#";
