#include "controls.h"
#include "Input/keyboard.h"


MenuControls::MenuControls() {
	std::fill(controlPrev, std::end(controlPrev), false);
	std::fill(controlCurr, std::end(controlCurr), false);
	std::fill(ControlKeys, std::end(ControlKeys), -1);
}

MenuControls::~MenuControls() { }

bool MenuControls::IsKeyPressed(int key) {
	if (IsKeyDown(key))
		return true;
	return false;
}

bool MenuControls::IsKeyJustPressed(ControlType control) {
	int key = ControlKeys[control];
	if (IsKeyDown(key))
		controlCurr[control] = true;
	else
		controlCurr[control] = false;

	// raising edge
	if (controlCurr[control] == true && controlPrev[control] == false) {
		controlPrev[control] = controlCurr[control];
		return true;
	}

	controlPrev[control] = controlCurr[control];
	return false;
}
