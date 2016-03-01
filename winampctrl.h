#ifndef __WINAMPCTRL_H__
#define __WINAMPCTRL_H__


//Previous track button
//40044
// 
//Next track button
//40048
// 
//Play button
//40045
// 
//Pause/Unpause button
//40046
// 
//Stop button
//40047
// 
//Fadeout and stop
//40147
// 
//Stop after current track
//40157
// 
//Fast-forward 5 seconds
//40148
// 
//Fast-rewind 5 seconds
//40144
// 
//Start of playlist
//40154
// 
//Go to end of playlist 
//40158
// 
//Open file dialog
//40029
// 
//Open URL dialog
//40155
// 
//Open file info box
//40188
// 
//Set time display mode to elapsed
//40037
// 
//Set time display mode to remaining
//40038
// 
//Toggle preferences screen
//40012
// 
//Open visualization options
//40190
// 
//Open visualization plug-in options
//40191
// 
//Execute current visualization plug-in
//40192
// 
//Toggle about box
//40041
// 
//Toggle title Autoscrolling
//40189
// 
//Toggle always on top
//40019
// 
//Toggle Windowshade
//40064
// 
//Toggle Playlist Windowshade
//40266
// 
//Toggle doublesize mode
//40165
// 
//Toggle EQ
//40036
// 
//Toggle playlist editor
//40040
// 
//Toggle main window visible
//40258
// 
//Toggle minibrowser
//40298
// 
//Toggle easymove
//40186
// 
//Raise volume by 1%
//40058
// 
//Lower volume by 1%
//40059
// 
//Toggle repeat
//40022
// 
//Toggle shuffle
//40023
// 
//Open jump to time dialog
//40193
// 
//Open jump to file dialog
//40194
// 
//Open skin selector
//40219
// 
//Configure current visualization plug-in
//40221
// 
//Reload the current skin
//40291
// 
//Close Winamp
//40001
// 
//Moves back 10 tracks in playlist
//40197
// 
//Show the edit bookmarks
//40320
// 
//Adds current track as a bookmark
//40321
// 
//Play audio CD
//40323
// 
//Load a preset from EQ
//40253
// 
//Save a preset to EQF
//40254
// 
//Opens load presets dialog
//40172
// 
//Opens auto-load presets dialog
//40173
// 
//Load default preset
//40174
// 
//Opens save preset dialog
//40175
// 
//Opens auto-load save preset
//40176
// 
//Opens delete preset dialog
//40178
// 
//Opens delete an auto load preset dialog
//40180

class WinAmpCtrl {
private:
	static void WinAmpCtrl::Command(int nCmd) {
		HWND hWndWinAmp = FindWindow("Winamp v1.x", NULL);
		if (hWndWinAmp == NULL)
			return;
		SendMessage(hWndWinAmp, WM_COMMAND, nCmd, 0);
	}

public:

	static void WinAmpCtrl::Prev(void)    { Command(40044); }
	static void WinAmpCtrl::Play(void)    { Command(40045); }
	static void WinAmpCtrl::Pause(void)   { Command(40046); }
	static void WinAmpCtrl::Stop(void)    { Command(40047); }
	static void WinAmpCtrl::Next(void)    { Command(40048); }
	static void WinAmpCtrl::VolUp(void)   { Command(40058); }
	static void WinAmpCtrl::VolDown(void) { Command(40059); }
  static void WinAmpCtrl::FastForward(void) { Command(40148); }
  static void WinAmpCtrl::FastRewind(void)  { Command(40144); }

	static void WinAmpCtrl::PlayPause(void) {
		HWND hWndWinAmp = FindWindow("Winamp v1.x", NULL);
		if (hWndWinAmp == NULL)
			return;

		LRESULT res = SendMessage(hWndWinAmp, WM_USER, 0, 104);
		if (res == 1 || res == 3) {
			// currently playing or paused => pause
			Pause();
		} else {
			// currently stopped => play
			Play();
		}
	}

};

#endif __WINAMPCTRL_H__
