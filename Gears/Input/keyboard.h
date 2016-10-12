/*
		THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
					http://dev-c.com
				(C) Alexander Blade 2015
*/

#pragma once

#include <windows.h>
#include <string>

// parameters are the same as with aru's ScriptHook for IV
void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);

bool IsKeyDown(DWORD key);
bool IsKeyJustUp(DWORD key, bool exclusive = true);
void ResetKeyState(DWORD key);

// I stole this from Scene Director
// https://github.com/elsewhat/gtav-mod-scene-director/commit/14d30944af64418265a34d2fedf4bf4d735be36d
DWORD str2key(std::string humanReadableKey);