#include "ScriptControls.hpp"

#include <Windows.h>
#include "TimeHelper.hpp"

ScriptControls::ScriptControls()
{
}


ScriptControls::~ScriptControls()
{
}


bool ScriptControls::IsKeyPressed(int key) {
	if (GetAsyncKeyState(key) & 0x8000)
		return true;
	return false;
}

bool ScriptControls::IsKeyJustPressed(int key, ControlType control) {
	ControlCurr[(int)control] = (GetAsyncKeyState(key) & 0x8000) != 0;

	// raising edge
	if (ControlCurr[(int)control] && !ControlPrev[(int)control]) {
		ControlPrev[(int)control] = ControlCurr[(int)control];
		return true;
	}

	ControlPrev[(int)control] = ControlCurr[(int)control];
	return false;
}

bool ScriptControls::WasControlPressedForMs(int control, int ms) {
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, control)) {
		pressTime = milliseconds_now();
	}
	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, control)) {
		releaseTime = milliseconds_now();
	}

	if ((releaseTime - pressTime) >= ms) {
		pressTime = 0;
		releaseTime = 0;
		return true;
	}
	return false;
}
