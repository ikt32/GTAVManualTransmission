#include "controls.h"
#include "Input/keyboard.h"


Controls::Controls() {
	std::fill(controlPrev, std::end(controlPrev), false);
	std::fill(controlCurr, std::end(controlCurr), false);
	std::fill(controlKeys, std::end(controlKeys), -1);
}

Controls::~Controls() { }

bool Controls::IsKeyPressed(int key) {
	if (IsKeyDown(key))
		return true;
	return false;
}

bool Controls::IsKeyJustPressed(ControlType control) {
	int key = controlKeys[control];
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

// alex bruh y
void Controls::GetButtonState(bool *a, bool *b, bool *up, bool *down, bool *l, bool *r)
{
	if (a) *a =			IsKeyJustPressed(MenuSelect);
	if (b) *b =			IsKeyJustPressed(MenuCancel);
	if (up) *up =		IsKeyJustPressed(MenuUp);
	if (down) *down =	IsKeyJustPressed(MenuDown);
	if (r) *r =			IsKeyJustPressed(MenuRight);
	if (l) *l =			IsKeyJustPressed(MenuLeft);
}