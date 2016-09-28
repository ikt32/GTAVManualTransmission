/*
		THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
					http://dev-c.com
				(C) Alexander Blade 2015
*/

#include "keyboard.h"
#include <map>

const int KEYS_SIZE = 255;

struct {
	DWORD time;
	BOOL isWithAlt;
	BOOL wasDownBefore;
	BOOL isUpNow;
} keyStates[KEYS_SIZE];

void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	if (key < KEYS_SIZE)
	{
		keyStates[key].time = GetTickCount();
		keyStates[key].isWithAlt = isWithAlt;
		keyStates[key].wasDownBefore = wasDownBefore;
		keyStates[key].isUpNow = isUpNow;
	}
}

// >5 minute holding W or S is improbable
const int NOW_PERIOD = 100, MAX_DOWN = 300000; // ms

bool IsKeyDown(DWORD key)
{
	return (key < KEYS_SIZE) ? ((GetTickCount() < keyStates[key].time + MAX_DOWN) && !keyStates[key].isUpNow) : false;
}

bool IsKeyJustUp(DWORD key, bool exclusive)
{
	bool b = (key < KEYS_SIZE) ? (GetTickCount() < keyStates[key].time + NOW_PERIOD && keyStates[key].isUpNow) : false;
	if (b && exclusive)
		ResetKeyState(key);
	return b;
}

void ResetKeyState(DWORD key)
{
	if (key < KEYS_SIZE)
		memset(&keyStates[key], 0, sizeof(keyStates[0]));
}

DWORD str2key(std::string humanReadableKey)
{
	std::map<std::string, int> keymap;

	keymap["LMB"] = VK_LBUTTON;
	keymap["RMB"] = VK_RBUTTON;
	keymap["CANCEL"] = VK_CANCEL;
	keymap["MMB"] = VK_MBUTTON;
	keymap["BACKSPACE"] = VK_BACK;
	keymap["TAB"] = VK_TAB;
	keymap["CLEAR"] = VK_CLEAR;
	keymap["RETURN"] = VK_RETURN;
	keymap["SHIFT"] = VK_SHIFT;
	keymap["CTRL"] = VK_CONTROL;
	keymap["ALT"] = VK_MENU;
	keymap["PAUSE"] = VK_PAUSE;
	keymap["CAPSLOCK"] = VK_CAPITAL;
	keymap["ESCAPE"] = VK_ESCAPE;
	keymap["SPACE"] = VK_SPACE;
	keymap["PAGEUP"] = VK_PRIOR;
	keymap["PAGEDOWN"] = VK_NEXT;
	keymap["END"] = VK_END;
	keymap["HOME"] = VK_HOME;
	keymap["LEFT"] = VK_LEFT;
	keymap["UP"] = VK_UP;
	keymap["RIGHT"] = VK_RIGHT;
	keymap["DOWN"] = VK_DOWN;
	keymap["SELECT"] = VK_SELECT;
	keymap["PRINT"] = VK_PRINT;
	keymap["EXECUTE"] = VK_EXECUTE;
	keymap["PRINTSCREEN"] = VK_SNAPSHOT;
	keymap["INSERT"] = VK_INSERT;
	keymap["DELETE"] = VK_DELETE;
	keymap["HELP"] = VK_HELP;
	keymap["LWIN"] = VK_LWIN;
	keymap["RWIN"] = VK_RWIN;
	keymap["APPS"] = VK_APPS;
	keymap["SLEEP"] = VK_SLEEP;
	keymap["NUM0"] = VK_NUMPAD0;
	keymap["NUM1"] = VK_NUMPAD1;
	keymap["NUM2"] = VK_NUMPAD2;
	keymap["NUM3"] = VK_NUMPAD3;
	keymap["NUM4"] = VK_NUMPAD4;
	keymap["NUM5"] = VK_NUMPAD5;
	keymap["NUM6"] = VK_NUMPAD6;
	keymap["NUM7"] = VK_NUMPAD7;
	keymap["NUM8"] = VK_NUMPAD8;
	keymap["NUM9"] = VK_NUMPAD9;
	keymap["*"] = VK_MULTIPLY;
	keymap["PLUS"] = VK_ADD;
	keymap[","] = VK_SEPARATOR;
	keymap["MINUS"] = VK_SUBTRACT;
	keymap["."] = VK_DECIMAL;
	keymap["/"] = VK_DIVIDE;
	keymap["F1"] = VK_F1;
	keymap["F2"] = VK_F2;
	keymap["F3"] = VK_F3;
	keymap["F4"] = VK_F4;
	keymap["F5"] = VK_F5;
	keymap["F6"] = VK_F6;
	keymap["F7"] = VK_F7;
	keymap["F8"] = VK_F8;
	keymap["F9"] = VK_F9;
	keymap["F10"] = VK_F10;
	keymap["F11"] = VK_F11;
	keymap["F12"] = VK_F12;
	keymap["F13"] = VK_F13;
	keymap["F14"] = VK_F14;
	keymap["F15"] = VK_F15;
	keymap["F16"] = VK_F16;
	keymap["F17"] = VK_F17;
	keymap["F18"] = VK_F18;
	keymap["F19"] = VK_F19;
	keymap["F20"] = VK_F20;
	keymap["F21"] = VK_F21;
	keymap["F22"] = VK_F22;
	keymap["F23"] = VK_F23;
	keymap["F24"] = VK_F24;
	keymap["NUMLOCK"] = VK_NUMLOCK;
	keymap["SCROLL"] = VK_SCROLL;
	keymap["LSHIFT"] = VK_LSHIFT;
	keymap["RSHIFT"] = VK_RSHIFT;
	keymap["LCTRL"] = VK_LCONTROL;
	keymap["RCTRL"] = VK_RCONTROL;
	keymap["LMENU"] = VK_LMENU;
	keymap["RMENU"] = VK_RMENU;
	keymap["BROWSER_BACK"] = VK_BROWSER_BACK;
	keymap["BROWSER_FORWARD"] = VK_BROWSER_FORWARD;
	keymap["BROWSER_REFRESH"] = VK_BROWSER_REFRESH;
	keymap["BROWSER_STOP"] = VK_BROWSER_STOP;
	keymap["BROWSER_SEARCH"] = VK_BROWSER_SEARCH;
	keymap["BROWSER_FAVORITES"] = VK_BROWSER_FAVORITES;
	keymap["BROWSER_HOME"] = VK_BROWSER_HOME;
	keymap["VOLUME_MUTE"] = VK_VOLUME_MUTE;
	keymap["VOLUME_DOWN"] = VK_VOLUME_DOWN;
	keymap["VOLUME_UP"] = VK_VOLUME_UP;
	keymap["MEDIA_NEXT_TRACK"] = VK_MEDIA_NEXT_TRACK;
	keymap["MEDIA_PREV_TRACK"] = VK_MEDIA_PREV_TRACK;
	keymap["MEDIA_STOP"] = VK_MEDIA_STOP;
	keymap["MEDIA_PLAY_PAUSE"] = VK_MEDIA_PLAY_PAUSE;
	keymap["LAUNCH_MAIL"] = VK_LAUNCH_MAIL;
	keymap["LAUNCH_MEDIA_SELECT"] = VK_LAUNCH_MEDIA_SELECT;
	keymap["LAUNCH_APP1"] = VK_LAUNCH_APP1;
	keymap["LAUNCH_APP2"] = VK_LAUNCH_APP2;
	keymap["PLAY"] = VK_PLAY;
	keymap["ZOOM"] = VK_ZOOM;
	keymap["VK_OEM_1"] = VK_OEM_1;		// : ;
	keymap["VK_OEM_2"] = VK_OEM_2;		// ? /
	keymap["VK_OEM_3"] = VK_OEM_3;		// ~ `
	keymap["VK_OEM_4"] = VK_OEM_4;		// { [
	keymap["VK_OEM_5"] = VK_OEM_5;		// | Backslash
	keymap["VK_OEM_6"] = VK_OEM_6;		// } ]
	keymap["VK_OEM_7"] = VK_OEM_7;		// " '
	keymap["VK_OEM_8"] = VK_OEM_8;		// § !
	keymap["VK_OEM_102"] = VK_OEM_102;	// > <



	if (humanReadableKey.length() == 1)
	{
		char letter = humanReadableKey.c_str()[0];

		if ((letter >= 0x30 && letter <= 0x39) || (letter >= 0x41 && letter <= 0x5A))
		{
			return (int)letter;
		}
	}


	return keymap[humanReadableKey];
}