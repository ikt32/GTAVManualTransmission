#include "ScriptControls.hpp"

#include <Windows.h>
#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"

ScriptControls::ScriptControls(): buttonState(0), wheelState(nullptr) {
	wheel = new WheelInput();
	controller = new XboxController(1);
}

ScriptControls::~ScriptControls() {}

void ScriptControls::UpdateValues(InputDevices prevInput) {
	if (controller->IsConnected()) {
		buttonState = controller->GetState().Gamepad.wButtons;
		controller->UpdateButtonChangeStates();
	}

	if (wheel->IsConnected()) {
		wheelState = wheel->GetState();
		wheel->UpdateButtonChangeStates();
	}
	
	switch (prevInput) {
		case Keyboard:
			ThrottleVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
			BrakeVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
			ClutchVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
			break;
		case Controller:
			ThrottleVal = controller->GetAnalogValue(controller->StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState);
			BrakeVal = controller->GetAnalogValue(controller->StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]), buttonState);
			ClutchVal = controller->GetAnalogValue(controller->StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]), buttonState);
			break;
		case Wheel:
			ThrottleVal = 1.0f - static_cast<float>(wheelState->lY) / 65535.0f;
			BrakeVal = 1.0f - static_cast<float>(wheelState->lRz) / 65535.0f;
			ClutchVal = 1.0f - static_cast<float>(wheelState->rglSlider[1]) / 65535.0f;
			break;
		default: break;
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
	KBControlCurr[static_cast<int>(control)] = (GetAsyncKeyState(key) & 0x8000) != 0;

	// raising edge
	if (KBControlCurr[static_cast<int>(control)] && !KBControlPrev[static_cast<int>(control)]) {
		KBControlPrev[static_cast<int>(control)] = KBControlCurr[static_cast<int>(control)];
		return true;
	}

	KBControlPrev[static_cast<int>(control)] = KBControlCurr[static_cast<int>(control)];
	return false;
}

// Limitation: Only works for hardcoded input types. Currently throttle.
ScriptControls::InputDevices ScriptControls::GetLastInputDevice(InputDevices previousInput) {
	if (IsKeyJustPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)], KeyboardControlType::Throttle) ||
		IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)])) {
		return Keyboard;
	}
	if (controller->IsButtonJustPressed(controller->StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState) ||
		controller->IsButtonPressed(controller->StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState)) {
		return Controller;
	}
	if (wheel->IsConnected() &&
		wheelState != nullptr &&
		1.0f - static_cast<float>(wheelState->lY) / 65535.0f > 0.5f) {
		return Wheel;
	}
	return previousInput;
}


bool ScriptControls::ButtonPressed(ControllerControlType control) {
	if (controller->IsButtonJustPressed(controller->StringToButton(ControlXbox[static_cast<int>(control)]), buttonState)) {
		return true;
	}
	return false;
}

bool ScriptControls::ButtonPressed(KeyboardControlType control) {
	if (IsKeyJustPressed(KBControl[static_cast<int>(control)], control)) {
		return true;
	}
	return false;
}

//TODO: Implement for DInput
bool ScriptControls::ButtonPressed(WheelControlType control) {
	return false;
}

bool ScriptControls::ButtonHeld(ControllerControlType control) {
	if (controller->WasButtonHeldForMs(controller->StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, CToggleTime)) {
		return true;
	}
	return false;
}
