#include "ScriptControls.hpp"

#include <Windows.h>

ScriptControls::ScriptControls() {}


ScriptControls::~ScriptControls() {}


bool ScriptControls::IsKeyPressed(int key) {
	if (GetAsyncKeyState(key) & 0x8000)
		return true;
	return false;
}

bool ScriptControls::IsKeyJustPressed(int key, KeyboardControlType control) {
	ControlCurr[static_cast<int>(control)] = (GetAsyncKeyState(key) & 0x8000) != 0;

	// raising edge
	if (ControlCurr[static_cast<int>(control)] && !ControlPrev[static_cast<int>(control)]) {
		ControlPrev[static_cast<int>(control)] = ControlCurr[static_cast<int>(control)];
		return true;
	}

	ControlPrev[static_cast<int>(control)] = ControlCurr[static_cast<int>(control)];
	return false;
}
