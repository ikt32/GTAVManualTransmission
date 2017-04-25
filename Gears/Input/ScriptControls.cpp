#include "ScriptControls.hpp"

#include <Windows.h>
#include "keyboard.h"

ScriptControls::ScriptControls(): WheelControl(),
                                PrevInput(Keyboard),
                                SteerAxisType(WheelAxisType::Steer),
                                controller{1},
                                buttonState(0) { }

ScriptControls::~ScriptControls() { }

void ScriptControls::InitWheel(bool initffb) {
	if (!WheelControl.InitWheel()) {
		// Initialization failed somehow, so we skip
		return;
	}

	auto steerGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)];
	auto steerAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)]);
	auto ffAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]);
	
	if (initffb && WheelControl.InitFFB(steerGUID, steerAxis)) {
		WheelControl.UpdateCenterSteering(steerGUID, ffAxis);
	}
}

void ScriptControls::UpdateValues(InputDevices prevInput, bool ignoreClutch, bool justPeekingWheelKb) {
#ifdef GAME_BUILD
	if (UseLegacyController) {
		lcontroller.UpdateButtonChangeStates();
	}

	if (!UseLegacyController && controller.IsConnected()) {
#else
	if (controller.IsConnected()) {
#endif
		buttonState = controller.GetState().Gamepad.wButtons;
		controller.UpdateButtonChangeStates();
	}

	WheelControl.Update();
	WheelControl.UpdateButtonChangeStates();

	CheckCustomButtons(justPeekingWheelKb);

	// Update ThrottleVal, BrakeVal and ClutchVal ranging from 0.0f (no touch) to 1.0f (full press)
	switch (prevInput) {
		case Keyboard: {
			ThrottleVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
			BrakeVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
			ClutchVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
			ClutchValRaw = ClutchVal;
			break;
		}			
		case Controller: {
#ifdef GAME_BUILD
			if (UseLegacyController) {
				ThrottleVal = lcontroller.GetAnalogValue(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)]));
				BrakeVal = lcontroller.GetAnalogValue(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Brake)]));
				ClutchVal = lcontroller.GetAnalogValue(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Clutch)]));
				ClutchValRaw = ClutchVal;
			} else {
				ThrottleVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState);
				BrakeVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]), buttonState);
				ClutchVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]), buttonState);
				ClutchValRaw = ClutchVal;
			}
#else
			ThrottleVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState);
			BrakeVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]), buttonState);
			ClutchVal = controller.GetAnalogValue(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]), buttonState);
			ClutchValRaw = ClutchVal;
#endif
			break;
		}
		case Wheel: {
			int RawT = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)]);
			int RawB = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Brake)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Brake)]);
			int RawC = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)]);
			int RawS = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)]);
			int RawH = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]), 
			                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Handbrake)]);
			
			ThrottleVal = 0.0f + 1.0f / (ThrottleDown - ThrottleUp)*(static_cast<float>(RawT) - ThrottleUp);
			BrakeVal = 0.0f + 1.0f / (BrakeDown - BrakeUp)*(static_cast<float>(RawB) - BrakeUp);

			if (WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]) == WheelDirectInput::UNKNOWN_AXIS) {
				ClutchVal = 0.0f;
			} else {
				ClutchVal = 0.0f + 1.0f / (ClutchDown - ClutchUp)*(static_cast<float>(RawC) - ClutchUp);
			}
			ClutchValRaw = 0.0f + 1.0f / (ClutchDown - ClutchUp)*(static_cast<float>(RawC) - ClutchUp);


			if (WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]) == WheelDirectInput::UNKNOWN_AXIS) {
				HandbrakeVal = 0.0f;
			} else {
				HandbrakeVal = 0.0f + 1.0f / (HandbrakeUp - HandbrakeDown)*(static_cast<float>(RawH) - HandbrakeDown);
			}

			SteerVal = 0.0f + 1.0f / (SteerRight - SteerLeft)*(static_cast<float>(RawS) - SteerLeft);

			if (RawH == -1) { HandbrakeVal = 0.0f; }
			if (RawC == -1) { ClutchVal = 0.0f; ClutchValRaw = 0.0f; }
			if (RawT == -1) { ThrottleVal = 0.0f; }
			if (RawB == -1) { BrakeVal = 0.0f; }

			if (ThrottleVal > 1.0f) { ThrottleVal = 1.0f; }
			if (BrakeVal > 1.0f) { BrakeVal = 1.0f; }
			if (ClutchVal > 1.0f) { ClutchVal = 1.0f; }
			if (ClutchValRaw > 1.0f) { ClutchValRaw = 1.0f; }


			if (ThrottleVal < 0.0f) { ThrottleVal = 0.0f; }
			if (BrakeVal < 0.0f) { BrakeVal = 0.0f; }
			if (ClutchVal < 0.0f) { ClutchVal = 0.0f; }
			if (ClutchValRaw < 0.0f) { ClutchValRaw = 0.0f; }

			if (InvertSteer) { SteerVal = 1.0f - SteerVal; }
			if (InvertThrottle) { ThrottleVal = 1.0f - ThrottleVal; }
			if (InvertBrake) { BrakeVal = 1.0f - BrakeVal; }
			if (InvertClutch) { ClutchVal = 1.0f - ClutchVal; ClutchValRaw = 1.0f - ClutchValRaw; }

			break;
		}
		default: break;
	}
	if (ignoreClutch) {
		ClutchVal = 0.0f;
	}
}

// Limitation: Only works for hardcoded input types. Currently throttle.
ScriptControls::InputDevices ScriptControls::GetLastInputDevice(InputDevices previousInput, bool enableWheel) {
	if (IsKeyJustPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)], KeyboardControlType::Throttle) ||
		IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)])) {
		return Keyboard;
	}
#ifdef GAME_BUILD
	if (UseLegacyController) {
		if (lcontroller.IsButtonJustPressed(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)])) ||
			lcontroller.IsButtonPressed(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)]))) {
			return Controller;
		}
	} else {
#endif
		if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState) ||
			controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState)) {
			return Controller;
		}
#ifdef GAME_BUILD
	}
#endif
	if (enableWheel && WheelControl.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
		// Oh my god I hate single-axis throttle/brake steering wheels
		// Looking at you DFGT.
		int RawT = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]), 
		                                WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)]);
		auto tempThrottle = 0.0f + 1.0f / (ThrottleDown - ThrottleUp)*(static_cast<float>(RawT) - ThrottleUp);

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
		else if (tempThrottle > 0.5f) {
			return Wheel;
		}

		int RawC = WheelControl.GetAxisValue(WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]),
											 WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)]);
		auto tempClutch = 0.0f + 1.0f / (ClutchDown - ClutchUp)*(static_cast<float>(RawC) - ClutchUp);
		if (tempClutch > 0.5f) {
			return Wheel;
		}
	}
	return previousInput;
}

/*
 * Keyboard section
 */

bool ScriptControls::IsKeyPressed(int key) {
	//if (!IsWindowFocused()) return false;
	//if (GetAsyncKeyState(key) & 0x8000) return true;
	if (IsKeyDown(key)) return true;
	return false;
}

bool ScriptControls::IsKeyJustPressed(int key, KeyboardControlType control) {
	//KBControlCurr[static_cast<int>(control)] = (GetAsyncKeyState(key) & 0x8000) != 0;
	KBControlCurr[static_cast<int>(control)] = IsKeyPressed(key);//IsKeyDown(key);
	
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

void ScriptControls::SetXboxTrigger(float value) {
	controller.TriggerValue = value;
#ifdef GAME_BUILD
	lcontroller.TriggerValue = value;
#endif
}

float ScriptControls::GetXboxTrigger() {
	return controller.TriggerValue;
}

/*
 * Legacy stuff
 */
#ifdef GAME_BUILD
bool ScriptControls::ButtonJustPressed(LegacyControlType control) {
	if (!UseLegacyController) return false;
	auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
	if (lcontroller.IsButtonJustPressed(gameButton))
		return true;
	return false;
}
bool ScriptControls::ButtonReleased(LegacyControlType control) {
	if (!UseLegacyController) return false;
	auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
	if (lcontroller.IsButtonJustReleased(gameButton))
		return true;
	return false;
}

bool ScriptControls::ButtonHeld(LegacyControlType control) {
	if (!UseLegacyController) return false;
	auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
	if (lcontroller.WasButtonHeldForMs(gameButton, CToggleTime))
		return true;
	return false;
}

bool ScriptControls::ButtonIn(LegacyControlType control) {
	if (!UseLegacyController) return false;
	auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
	if (controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
		return true;
	return false;
}
#endif
/*
 * Wheel section
 */

bool ScriptControls::ButtonJustPressed(WheelControlType control) {
	if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelControl.IsButtonJustPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonReleased(WheelControlType control) {
	if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelControl.IsButtonJustReleased(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

bool ScriptControls::ButtonHeld(WheelControlType control) {
	if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelControl.WasButtonHeldForMs(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)], WButtonHeld))
		return true;
	return false;
}

bool ScriptControls::ButtonIn(WheelControlType control) {
	if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
		WheelButton[static_cast<int>(control)] == -1) {
		return false;
	}
	if (WheelControl.IsButtonPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
		return true;
	return false;
}

void ScriptControls::CheckCustomButtons(bool justPeeking) {
	if (!WheelControl.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
		return;
	}
	if (justPeeking) { // we do not want to send keyboard codes if we just test the device
		return;
	}
	for (int i = 0; i < MAX_RGBBUTTONS; i++) {
		if (WheelToKey[i] != -1) {
			INPUT input;
			input.type = INPUT_KEYBOARD;
			input.ki.dwExtraInfo = 0;
			input.ki.wVk = 0;
			input.ki.wScan = MapVirtualKey(WheelToKey[i], MAPVK_VK_TO_VSC);

			if (WheelControl.IsButtonJustPressed(i,WheelToKeyGUID)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE;
				SendInput(1, &input, sizeof(INPUT));
			}
			if (WheelControl.IsButtonJustReleased(i, WheelToKeyGUID)) {
				input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
				SendInput(1, &input, sizeof(INPUT));
			}
		}
	}
}

// GUID stuff...?
bool operator < (const GUID &guid1, const GUID &guid2) {
	if (guid1.Data1 != guid2.Data1) {
		return guid1.Data1 < guid2.Data1;
	}
	if (guid1.Data2 != guid2.Data2) {
		return guid1.Data2 < guid2.Data2;
	}
	if (guid1.Data3 != guid2.Data3) {
		return guid1.Data3 < guid2.Data3;
	}
	for (int i = 0; i<8; i++) {
		if (guid1.Data4[i] != guid2.Data4[i]) {
			return guid1.Data4[i] < guid2.Data4[i];
		}
	}
	return false;
}

std::string GUID2String(GUID guid) {
	wchar_t szGuidW[40] = { 0 };
	StringFromGUID2(guid, szGuidW, 40);
	std::wstring wGuid = szGuidW;//std::wstring(szGuidW);
	return(std::string(wGuid.begin(), wGuid.end()));
}

void ScriptControls::CheckGUIDs(const std::vector<_GUID> & guids) {
	auto foundGuids = WheelControl.GetGuids();
	auto reggdGuids = guids;//settings.GetGuids();
	// We're only checking for devices that should be used but aren't found
	std::sort(foundGuids.begin(), foundGuids.end());
	std::sort(reggdGuids.begin(), reggdGuids.end());
	std::vector<GUID> missingReg;
	std::vector<GUID> missingFnd;

	// Mental note since I'm dumb:
	// The difference of two sets is formed by the elements that are present in
	// the first set, but not in the second one.

	// Registered but not enumerated
	std::set_difference(
		reggdGuids.begin(), reggdGuids.end(),
		foundGuids.begin(), foundGuids.end(), std::back_inserter(missingReg));

	if (missingReg.size() > 0) {
		logger.Write("WHEEL: Used in .ini but not available: ");
		for (auto g : missingReg) {
			logger.Write(std::string("    ") + GUID2String(g));
		}
	}

	// Enumerated but not registered
	std::set_difference(
		foundGuids.begin(), foundGuids.end(),
		reggdGuids.begin(), reggdGuids.end(), std::back_inserter(missingFnd));

	if (missingFnd.size() > 0) {
		logger.Write("WHEEL: Available for use in .ini: ");
		for (auto g : missingFnd) {
			logger.Write(std::string("    ") + GUID2String(g));
		}
	}
}
