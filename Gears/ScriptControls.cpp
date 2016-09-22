#include "ScriptControls.hpp"

#include <Windows.h>
#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"

ScriptControls::ScriptControls(): controller{1},
                                  buttonState(0) {
}

ScriptControls::~ScriptControls() {
}

void ScriptControls::InitWheel() {
	WheelDI.InitWheel(FFAxis);
}

void ScriptControls::UpdateValues(InputDevices prevInput) {
	if (controller.IsConnected()) {
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	if (WheelDI.IsConnected()) {
		WheelDI.UpdateState();
		WheelDI.UpdateButtonChangeStates();
		CheckCustomButtons();
	}

	// Update ThrottleVal, BrakeVal and ClutchVal ranging from 0.0f (no touch) to 1.0f (full press)
	switch (prevInput) {
		case Keyboard: {
			ThrottleVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
			BrakeVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
			ClutchVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
			break;
		}			
		case Controller: {
			ThrottleVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState);
			BrakeVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]), buttonState);
			ClutchVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]), buttonState);
			break;
		}
		case Wheel: {
			int RawT = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]));
			int RawB = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Brake)]));
			int RawC = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]));
			int RawS = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]));
			
			// You're outta luck if you have a single-axis 3-pedal freak combo or something
			if (WheelAxes[static_cast<int>(WheelAxisType::Throttle)] == WheelAxes[static_cast<int>(WheelAxisType::Brake)]) {
				ClutchVal = 1.0f - static_cast<float>(RawC) / 65535.0f;
				int pivot;
				if (ThrottleMin == BrakeMin) {
					pivot = BrakeMin;
				} else
				if (ThrottleMax == BrakeMin) {
					pivot = BrakeMin;
				} else
				if (ThrottleMin == BrakeMax) {
					pivot = BrakeMax;
				} else
				if (ThrottleMax == BrakeMax) {
					pivot = BrakeMax;
				} else {
					// you fucked up bro
					return;
				}

				// oh my god fucking kill me I'm too dumb for this shit
				if (pivot == BrakeMin) {
					if (BrakeMax > pivot) { // TMIN = BMIN
						// 0 TMAX < TMIN/PIVOT/BMIN < BMAX 65535
						if (RawT < pivot) { // Throttle
							ThrottleVal = 1.0f - (float)(RawT - ThrottleMax) / (float)pivot;
							BrakeVal = 0;
						} else { // Brake
							ThrottleVal = 0;
							BrakeVal = (float)(RawT - BrakeMin) / (float)pivot;
						}
					} // Only this has been verified
					else { // TMAX = BMIN
						// 0 BMAX < BMIN/PIVOT/TMAX < TMIN 65535
						if (RawT > pivot) { // Throttle
							ThrottleVal = 1.0f - (float)(RawT - ThrottleMax) / (float)pivot;
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = 1.0f - (float)(RawT - BrakeMax) / (float)pivot;
						}
					}
				}
				if (pivot == BrakeMax) {
					if (BrakeMin > pivot) { //TMIN = BMAX
						// TMAX X < TMIN/PIVOT/BMAX < BMIN 65535
						if (RawT < pivot) { // Throttle
							ThrottleVal = 1.0f - (float)(RawT - ThrottleMax) / (float)pivot;
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = 1.0f - (float)(RawT - BrakeMax) / (float)pivot;
						}
					}
					else { // TMAX = BMAX
						// 0 BMIN < BMAX/PIVOT/TMAX < TMIN 65535
						if (RawT > pivot) { // Throttle
							ThrottleVal = 1.0f - (float)(RawT - ThrottleMax) / (float)pivot;
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = (float)(RawT - BrakeMin) / (float)pivot;
						}
					}
				}
			} // End single-axis mumbo jumbo
 			else {
				ThrottleVal = 1.0f - static_cast<float>(RawT) / static_cast<float>(ThrottleMax - ThrottleMin);
				BrakeVal = 1.0f - static_cast<float>(RawB) / static_cast<float>(BrakeMax - BrakeMin);
				ClutchVal = 1.0f - static_cast<float>(RawC) / static_cast<float>(ClutchMax - ClutchMin);
			}
			if (ClutchDisable) {
				ClutchVal = 0.0f;
			}
			SteerVal = RawS; // Todo - Give a value or something idk
			break;
		}
		default: break;
	}

	AccelValGTA = CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate);
	AccelValGTAf = (AccelValGTA - 127) / 127.0f;

}

// Limitation: Only works for hardcoded input types. Currently throttle.
ScriptControls::InputDevices ScriptControls::GetLastInputDevice(InputDevices previousInput) {
	if (IsKeyJustPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)], KeyboardControlType::Throttle) ||
		IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)])) {
		return Keyboard;
	}
	if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState) ||
		controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState)) {
		return Controller;
	}
	if (WheelDI.IsConnected()) {
		// Oh my god I hate single-axis throttle/brake steering wheels
		// Looking at you DFGT.
		int RawT = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]));
		if (WheelAxes[static_cast<int>(WheelAxisType::Throttle)] == WheelAxes[static_cast<int>(WheelAxisType::Brake)]) {
			
			// get throttle range
			if (ThrottleMax > ThrottleMin &&
				RawT < ThrottleMax &&
				RawT > ThrottleMin + (ThrottleMax - ThrottleMin)*0.5) {
				return Wheel;
			}
			if (ThrottleMax < ThrottleMin && // Which twisted mind came up with this
				RawT > ThrottleMax &&
				RawT < ThrottleMin - (ThrottleMin - ThrottleMax)*0.5) {
				return Wheel;
			}
		}
		else if (1.0f - static_cast<float>(RawT) / 65535.0f > 0.5f) {
			return Wheel;
		}
	}
	return previousInput;
}

/*
 * Keyboard section
 */

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

bool ScriptControls::ButtonJustPressed(KeyboardControlType control) {
	if (IsKeyJustPressed(KBControl[static_cast<int>(control)], control))
		return true;
	return false;
}

/*
* Controller section
*/

bool ScriptControls::ButtonJustPressed(ControllerControlType control) {
	if (!controller.IsConnected())
		return false;
	if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
		return true;
	return false;
}

bool ScriptControls::ButtonReleased(ControllerControlType control) {
	if (!controller.IsConnected())
		return false;
	if (controller.IsButtonJustReleased(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
		return true;
	return false;
}

bool ScriptControls::ButtonHeld(ControllerControlType control) {
	if (!controller.IsConnected())
		return false;
	if (controller.WasButtonHeldForMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, CToggleTime))
		return true;
	return false;
}

bool ScriptControls::ButtonIn(ControllerControlType control) {
	if (!controller.IsConnected())
		return false;
	if (controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
		return true;
	return false;
}

void ScriptControls::SetXboxTrigger(int value) {
	controller.TriggerValue = static_cast<float>(value) / 100.0f;
}

/*
 * Wheel section
 */

bool ScriptControls::ButtonReleased(WheelControlType control) {
	if (!WheelDI.IsConnected() ||
		WheelControl[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonJustReleased(WheelControl[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonJustPressed(WheelControlType control) {
	if (!WheelDI.IsConnected() ||
		WheelControl[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonJustPressed(WheelControl[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonIn(WheelControlType control) {
	if (!WheelDI.IsConnected() ||
		WheelControl[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonPressed(WheelControl[static_cast<int>(control)]))
		return true;
	return false;
}

void ScriptControls::CheckCustomButtons() {
	if (!WheelDI.IsConnected()) {
		return;
	}
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		if (WheelToKey[i] != -1) {
			INPUT input;
			input.type = INPUT_KEYBOARD;
			input.ki.dwExtraInfo = 0;
			input.ki.wVk = 0;
			input.ki.wScan = MapVirtualKey(WheelToKey[i], MAPVK_VK_TO_VSC);

			if (WheelDI.IsButtonJustPressed(i)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE;
				SendInput(1, &input, sizeof(INPUT));
			}
			if (WheelDI.IsButtonJustReleased(i)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}
}