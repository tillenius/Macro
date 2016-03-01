#ifndef __HOTKEYS_H__
#define __HOTKEYS_H__

#include <windows.h>
#include <vector>
#include <string>
#include "macro.h"

#define HOTKEY_PLAYBACK					0xbfff
#define HOTKEY_RECORD						0xbffe
#define HOTKEY_NEXTWIN					0xbffd
#define HOTKEY_PREVWIN					0xbffc
#define HOTKEY_CLOSE						0xbffb
#define HOTKEY_WINAMP_PREV			0xbffa
#define HOTKEY_WINAMP_PLAY			0xbff9
#define HOTKEY_WINAMP_PAUSE			0xbff8
#define HOTKEY_WINAMP_STOP			0xbff7
#define HOTKEY_WINAMP_NEXT			0xbff6
#define HOTKEY_WINAMP_VOLUP			0xbff5
#define HOTKEY_WINAMP_VOLDOWN		0xbff4
#define HOTKEY_MINIMIZE					0xbff3
#define HOTKEY_WINAMP_PLAYPAUSE 0xbff2
#define HOTKEY_WINAMP_FF				0xbff1
#define HOTKEY_WINAMP_FR        0xbff0

using std::string;

//============================================================================================
// class Action
//============================================================================================
class Action {
public:
	UINT fsModifiers;
	UINT vk;
	Action(UINT mod, UINT vk_) : fsModifiers(mod), vk(vk_) {}
	virtual void execute(void) = 0;
};

//============================================================================================
// class ActionID
//============================================================================================
class ActionID : public Action {
private:
	int ID;

public:
	ActionID(UINT mod, UINT vk_, int ID) : Action(mod, vk_) {
		this->ID = ID;
	}

	virtual void execute(void);
};

//============================================================================================
// class RunAction
//============================================================================================
class RunAction : public Action {
private:
	string appName;
	string cmdLine;
	string currDir;
	WORD wShowWindow;

public:
	RunAction(UINT mod, UINT vk_, string appName, string cmdLine, string currDir, WORD wShowWindow) 
		: Action(mod, vk_) {
		this->appName = appName;
		this->cmdLine = cmdLine;
		this->currDir = currDir;
		this->wShowWindow = wShowWindow;
	}
	virtual void execute(void);
};

//============================================================================================
// class ActivateAction
//============================================================================================
class ActivateAction : public Action {
private:
  string exeName;
	string className;
	string windowName;
public:
	ActivateAction(UINT mod, UINT vk_, const string &exeName,
                 const string &windowName, const string &className)
    : Action(mod, vk_)
  {
    this->exeName = exeName;
		this->windowName = windowName;
		this->className = className;
	}
	virtual void execute(void);
};

//============================================================================================
// class RunOrActivate
//============================================================================================
class RunOrActivate : public RunAction {
private:
  string exeName;
	string className;
	string windowName;
public:
	RunOrActivate(UINT mod, UINT vk_, const string &appName, const string &cmdLine, const string &currDir,
                WORD wShowWindow, const string &exeName, const string &windowName, const string &className)
    : RunAction(mod, vk_, appName, cmdLine, currDir, wShowWindow)
	{
    this->exeName = exeName;
		this->windowName = windowName;
		this->className = className;
	}
	virtual void execute(void);
};

//============================================================================================
// class RunSaved
//============================================================================================
class RunSaved : public Action {
private:
	string fileName;
	string windowName;
public:
	RunSaved(UINT mod, UINT vk_, const char *fileName) : Action(mod, vk_) {
		this->fileName = fileName;
	}
  virtual void execute(void) {
    Macro m;
    m.load(fileName.c_str());
    m.Playback();
  }
};

//============================================================================================
// class Hotkeys
//============================================================================================
class Hotkeys {
private:
	std::vector<Action *> lHotkeys;
	HWND hWnd;
	bool bEnabled;

public:
	Hotkeys(HWND hWnd);
	~Hotkeys(void);

  void Add(Action *action) {
    lHotkeys.push_back(action);
  }

	//void Add(UINT fsModifiers, UINT vk, const char *appName, const char *cmdLine, const char *currDir, WORD wShowWindow) {
	//	lHotkeys.push_back(new RunAction(fsModifiers, vk, appName, cmdLine, currDir, wShowWindow));
	//}

	//void Add(UINT fsModifiers, UINT vk, int ID) {
	//	lHotkeys.push_back(new ActionID(fsModifiers, vk, ID));
	//}

	//void Add(UINT fsModifiers, UINT vk, const string &exeName, const string &className, const string &windowName) {
	//	lHotkeys.push_back(new ActivateAction(fsModifiers, vk, exeName, className, windowName));
	//}

	//void Add(UINT fsModifiers, UINT vk, const char *appName, const char *cmdLine, const char *currDir,
	//	       WORD wShowWindow, const string &exeName, const string &className, const string &windowName) {
	//	lHotkeys.push_back(new RunOrActivate(fsModifiers, vk, appName, cmdLine, currDir, wShowWindow, 
 //                      exeName, className, windowName));
	//}

 // void Add(UINT fsModifiers, UINT vk, const char *fileName) {
	//	lHotkeys.push_back(new RunSaved(fsModifiers, vk, fileName));
 // }

	bool Enable(void);
	void Disable(void);
	void execute(int ID) { lHotkeys[ID]->execute(); }

};

#endif // __HOTKEYS_H__
