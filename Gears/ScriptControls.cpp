#include "ScriptControls.h"

#include <Windows.h>
#include "TimeHelper.h"

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
	if (GetAsyncKeyState(key) & 0x8000)
		ControlCurr[control] = true;
	else
		ControlCurr[control] = false;

	// raising edge
	if (ControlCurr[control] == true && ControlPrev[control] == false) {
		ControlPrev[control] = ControlCurr[control];
		return true;
	}

	ControlPrev[control] = ControlCurr[control];
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