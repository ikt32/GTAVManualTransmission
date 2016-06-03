#include "ScriptControls.hpp"

#include <Windows.h>
#include "TimeHelper.hpp"

ScriptControls::ScriptControls() {
}


ScriptControls::~ScriptControls() {
}


bool ScriptControls::IsKeyPressed(int key) {
	if (GetAsyncKeyState(key) & 0x8000)
		return true;
	return false;
}

bool ScriptControls::IsKeyJustPressed(int key, KeyboardControlType control) {
	ControlCurr[(int)control] = (GetAsyncKeyState(key) & 0x8000) != 0;

	// raising edge
	if (ControlCurr[(int)control] && !ControlPrev[(int)control]) {
		ControlPrev[(int)control] = ControlCurr[(int)control];
		return true;
	}

	ControlPrev[(int)control] = ControlCurr[(int)control];
	return false;
}
