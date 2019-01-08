const char * DEFAULT_CONFIG_FILE = 
R"(import ctypes
import macro;

from ctypes.wintypes import HWND

OutputDebugString = ctypes.windll.kernel32.OutputDebugStringW
GetWindowText = ctypes.windll.user32.GetWindowTextW
GetWindowRect = ctypes.windll.user32.GetWindowRect
RECT = ctypes.wintypes.RECT
HWND = ctypes.wintypes.HWND

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
	hwnd = macro.get_current_window()
	#macro.notify(str(type(hwnd)))
	rect = RECT()
	GetWindowRect(hwnd, ctypes.byref(rect))
	info = macro.get_window_text(hwnd) + " " + str(rect.left) + "," + str(rect.top) + "," + str(rect.right) + "," + str(rect.bottom)
	choice = macro.menu( (info, "debug_midi", "copy_macro", "windows", "processes", "modules") )
	if choice == 0:
		macro.show_debug(True)
	elif choice == 1:
		macro.midi_debug(True)
	elif choice == 2:
		macro.set_clipboard_text(macro.get_macro_as_python())
	elif choice == 3:
		macro.list([ str(hwnd) for hwnd in macro.get_windows() ])
	elif choice == 4:
		macro.list([ x["szExeFile"] for x in macro.get_processes() ])
	elif choice == 5:
		macro.list([ x["szModule"]+":"+x["szExePath"] for x in macro.get_modules() ])

hotkeys = [
	("key", "win+OEM_5",											lambda: show_menu()),
]
)";
