#include "ScriptControls.hpp"

#include <Windows.h>
#include "keyboard.h"

ScriptControls::ScriptControls(): controller{1},
                                  buttonState(0) {
}

ScriptControls::~ScriptControls() {
}

void ScriptControls::InitWheel() {
	WheelDI.InitWheel();
	if (!WheelDI.InitFFB(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)],
					WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]))) {
		WheelDI.NoFeedback = true;
	}
}

void ScriptControls::UpdateValues(InputDevices prevInput, bool ignoreClutch) {
	if (controller.IsConnected()) {
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	//if (/*WheelDI.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])*/1) {
		WheelDI.UpdateState();
		WheelDI.UpdateButtonChangeStates();
		CheckCustomButtons();
	//}

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
			int RawT = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)]);
			int RawB = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Brake)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Brake)]);
			int RawC = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)]);
			int RawS = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)]);
			int RawH = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Handbrake)]);
			
			// You're outta luck if you have a single-axis 3-pedal freak combo or something
			if (WheelAxes[static_cast<int>(WheelAxisType::Throttle)] == WheelAxes[static_cast<int>(WheelAxisType::Brake)]) {
				ClutchVal = 1.0f - static_cast<float>(RawC) / (ClutchDown - ClutchUp);
				int pivot;
				if (ThrottleUp == BrakeUp) {
					pivot = BrakeUp;
				} else if (ThrottleDown == BrakeUp) {
					pivot = BrakeUp;
				} else if (ThrottleUp == BrakeDown) {
					pivot = BrakeDown;
				} else if (ThrottleDown == BrakeDown) {
					pivot = BrakeDown;
				} else {
					// something to notify the user the should fix their thing
					return;
				}

				// there has to be a better way
				if (pivot == BrakeUp) {
					if (BrakeDown > pivot) { // TMIN = BMIN
						// 0 TMAX < TMIN/PIVOT/BMIN < BMAX 65535
						if (RawT < pivot) { // Throttle
							ThrottleVal = 1.0f - static_cast<float>(RawT - ThrottleDown) / static_cast<float>(pivot);
							BrakeVal = 0;
						} else { // Brake
							ThrottleVal = 0;
							BrakeVal = static_cast<float>(RawT - BrakeUp) / static_cast<float>(pivot);
						}
					} // Only this has been verified
					else { // TMAX = BMIN
						// 0 BMAX < BMIN/PIVOT/TMAX < TMIN 65535
						if (RawT > pivot) { // Throttle
							ThrottleVal = 1.0f - static_cast<float>(RawT - ThrottleDown) / static_cast<float>(pivot);
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = 1.0f - static_cast<float>(RawT - BrakeDown) / static_cast<float>(pivot);
						}
					}
				}
				if (pivot == BrakeDown) {
					if (BrakeUp > pivot) { //TMIN = BMAX
						// TMAX X < TMIN/PIVOT/BMAX < BMIN 65535
						if (RawT < pivot) { // Throttle
							ThrottleVal = 1.0f - static_cast<float>(RawT - ThrottleDown) / static_cast<float>(pivot);
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = 1.0f - static_cast<float>(RawT - BrakeDown) / static_cast<float>(pivot);
						}
					}
					else { // TMAX = BMAX
						// 0 BMIN < BMAX/PIVOT/TMAX < TMIN 65535
						if (RawT > pivot) { // Throttle
							ThrottleVal = 1.0f - static_cast<float>(RawT - ThrottleDown) / static_cast<float>(pivot);
							BrakeVal = 0;
						}
						else { // Brake
							ThrottleVal = 0;
							BrakeVal = static_cast<float>(RawT - BrakeUp) / static_cast<float>(pivot);
						}
					}
				}
			} // End single-axis mumbo jumbo
 			else {
				ThrottleVal = 0.0f + 1.0f / (ThrottleDown - ThrottleUp)*(static_cast<float>(RawT) - ThrottleUp);
				BrakeVal = 0.0f + 1.0f / (BrakeDown - BrakeUp)*(static_cast<float>(RawB) - BrakeUp);
				ClutchVal = 0.0f + 1.0f/(ClutchDown - ClutchUp)*(static_cast<float>(RawC) - ClutchUp);
			}
			if (WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]) == WheelDirectInput::UNKNOWN_AXIS) {
				ClutchVal = 0.0f;
			}
			HandbrakeVal = 0.0f + 1.0f / (HandbrakeUp - HandbrakeDown)*(static_cast<float>(RawH) - HandbrakeDown);

			if (WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]) == WheelDirectInput::UNKNOWN_AXIS) {
				HandbrakeVal = 0.0f;
			}
			SteerVal = 0.0f + 1.0f / (SteerRight - SteerLeft)*(static_cast<float>(RawS) - SteerLeft);
			//if (SteerVal < 0.0)
			//	SteerVal = 1.0f + SteerVal;
			break;
		}
		default: break;
	}
	if (ignoreClutch)
		ClutchVal = 0.0f;
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
	if (WheelDI.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
		// Oh my god I hate single-axis throttle/brake steering wheels
		// Looking at you DFGT.
		int RawT = WheelDI.GetAxisValue(WheelDI.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]), 
		                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)]);
		if (WheelAxes[static_cast<int>(WheelAxisType::Throttle)] == WheelAxes[static_cast<int>(WheelAxisType::Brake)]) {
			
			// get throttle range
			if (ThrottleDown > ThrottleUp &&
				RawT < ThrottleDown &&
				RawT > ThrottleUp + (ThrottleDown - ThrottleUp)*0.5) {
				return Wheel;
			}
			if (ThrottleDown < ThrottleUp && // Which twisted mind came up with this
				RawT > ThrottleDown &&
				RawT < ThrottleUp - (ThrottleUp - ThrottleDown)*0.5) {
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
	//if (GetAsyncKeyState(key) & 0x8000)
	if (IsKeyDown(key))
		return true;
	return false;
}

bool ScriptControls::IsKeyJustPressed(int key, KeyboardControlType control) {
	//KBControlCurr[static_cast<int>(control)] = (GetAsyncKeyState(key) & 0x8000) != 0;
	KBControlCurr[static_cast<int>(control)] = IsKeyDown(key);
	
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
	if (!WheelDI.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonJustReleased(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonJustPressed(WheelControlType control) {
	if (!WheelDI.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonJustPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonIn(WheelControlType control) {
	if (!WheelDI.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelDI.IsButtonPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

void ScriptControls::CheckCustomButtons() {
	if (!WheelDI.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
		return;
	}
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		if (WheelToKey[i] != -1) {
			INPUT input;
			input.type = INPUT_KEYBOARD;
			input.ki.dwExtraInfo = 0;
			input.ki.wVk = 0;
			input.ki.wScan = MapVirtualKey(WheelToKey[i], MAPVK_VK_TO_VSC);

			if (WheelDI.IsButtonJustPressed(i,WheelToKeyGUID)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE;
				SendInput(1, &input, sizeof(INPUT));
			}
			if (WheelDI.IsButtonJustReleased(i, WheelToKeyGUID)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}
}