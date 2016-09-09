#include "ScriptControls.hpp"

#include <Windows.h>
#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"

ScriptControls::ScriptControls(): controller(1), buttonState(0) {}

ScriptControls::~ScriptControls() {}

void ScriptControls::UpdateValues(InputDevices prevInput) {
	if (controller.IsConnected()) {
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	switch (prevInput) {
		case Keyboard:
			ThrottleVal = (IsKeyPressed(Control[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
			BrakeVal = (IsKeyPressed(Control[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
			ClutchVal = (IsKeyPressed(Control[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
			break;
		case Controller:
			ThrottleVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState);
			BrakeVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]), buttonState);
			ClutchVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]), buttonState);
			break;
		default: break;
			//case InputDevices::Wheel: // Wheel
			//	ThrottleVal = logiWheel.GetLogiThrottleVal();
			//	BrakeVal = logiWheel.GetLogiBrakeVal();
			//	ClutchVal = 1 - logiWheel.GetLogiClutchVal();
			//	break;
	}

	AccelValGTA = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	AccelValGTAf = (AccelValGTA - 127) / 127.0f;
}

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

// Limitation: Only works for hardcoded input types. Currently throttle.
ScriptControls::InputDevices ScriptControls::GetLastInputDevice(InputDevices previousInput) {
	if (IsKeyJustPressed(Control[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)], ScriptControls::KeyboardControlType::Throttle) ||
		IsKeyPressed(Control[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)])) {
		return InputDevices::Keyboard;
	}
	if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]), buttonState) ||
		controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]), buttonState)) {
		return InputDevices::Controller;
	}
	/*if (logiWheel.IsActive(&settings) &&
	(logiWheel.GetLogiThrottleVal() > 0.1f ||
	logiWheel.GetLogiBrakeVal() > 0.1f ||
	logiWheel.GetLogiClutchVal() < 0.9f)) {
	return InputDevices::Wheel;
	}*/
	return previousInput;
}


bool ScriptControls::ButtonPressed(ControllerControlType control) {
	if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState)) {
		return true;
	}
	return false;
}

bool ScriptControls::ButtonPressed(KeyboardControlType control) {
	if (IsKeyJustPressed(Control[static_cast<int>(control)], control)) {
		return true;
	}
	return false;
}

bool ScriptControls::ButtonHeld(ControllerControlType control) {
	if (controller.WasButtonHeldForMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, CToggleTime)) {
		return true;
	}
	return false;
}
