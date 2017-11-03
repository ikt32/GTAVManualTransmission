#include "ScriptControls.hpp"

#include <Windows.h>
#include "keyboard.h"
#include "Util/Logger.hpp"

ScriptControls::ScriptControls(): PrevInput(Keyboard),
                                WheelControl(),
                                SteerAxisType(WheelAxisType::Steer),
                                controller{1},
                                buttonState(0) {
    std::fill(ControlXboxBlocks.begin(), ControlXboxBlocks.end(), -1);
}

ScriptControls::~ScriptControls() { }

void ScriptControls::InitWheel() {
    if (!WheelControl.InitWheel()) {
        // Initialization failed somehow, so we skip
        return;
    }

    auto steerGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)];
    auto steerAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)]);
    auto ffAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]);
    
    if (WheelControl.InitFFB(steerGUID, steerAxis)) {
        WheelControl.UpdateCenterSteering(steerGUID, ffAxis);
    }
}

void ScriptControls::updateKeyboard() {
    ThrottleVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
    BrakeVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
    ClutchVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
    ClutchValRaw = ClutchVal;
}

void ScriptControls::updateController() {
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
}

float map(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void ScriptControls::updateWheel() {
    auto throttleAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]);
    auto brakeAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Brake)]);
    auto clutchAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]);
    auto steerAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Steer)]);
    auto handbrakeAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]);

    auto throttleGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)];
    auto brakeGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Brake)];
    auto clutchGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)];
    auto steerGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)];
    auto handbrakeGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Handbrake)];

    int raw_t = WheelControl.GetAxisValue(throttleAxis, throttleGUID);
    int raw_b = WheelControl.GetAxisValue(brakeAxis, brakeGUID);
    int raw_c = WheelControl.GetAxisValue(clutchAxis, clutchGUID);
    int raw_s = WheelControl.GetAxisValue(steerAxis, steerGUID);
    int raw_h = WheelControl.GetAxisValue(handbrakeAxis, handbrakeGUID);
            
    ThrottleVal = map((float)raw_t, (float)ThrottleMin, (float)ThrottleMax, 0.0f, 1.0f);
    BrakeVal = map((float)raw_b, (float)BrakeMin, (float)BrakeMax, 0.0f, 1.0f);
            
    SteerValRaw = map((float)raw_s, (float)SteerMin, (float)SteerMax, 0.0f, 1.0f);
    if (DZSteer > 0.005f) {
        float center = 0.5f + DZSteerOffset;
        float minLimit = center - DZSteer;
        float maxLimit = center + DZSteer;
        if (SteerValRaw < minLimit) {
            SteerVal = map(SteerValRaw, 0.0f, minLimit, 0.0f, 0.5f);
        }
        else if (SteerValRaw > maxLimit) {
            SteerVal = map(SteerValRaw, maxLimit, 1.0f, 0.5f, 1.0f);
        }
        else {
            SteerVal = 0.5f;
        }
    }
    else {
        SteerVal = SteerValRaw;
    }

    ClutchValRaw = map((float)raw_c, (float)ClutchMin, (float)ClutchMax, 0.0f, 1.0f);
    if (WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]) == WheelDirectInput::UNKNOWN_AXIS ||
        WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)] == GUID()) {
        // wtf why would u :/
        if (WheelButton[static_cast<int>(WheelControlType::Clutch)] != -1) {
            ClutchVal = ButtonIn(WheelControlType::Clutch) ? 1.0f : 0.0f;
        } else {
            ClutchVal = 0.0f;
        }
    } else {
        ClutchVal = ClutchValRaw;
    }

    if (WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Handbrake)]) == WheelDirectInput::UNKNOWN_AXIS) {
        HandbrakeVal = 0.0f;
    } else {
        HandbrakeVal = map((float)raw_h, (float)HandbrakeMin, (float)HandbrakeMax, 0.0f, 1.0f);
    }


    if (raw_h == -1) { HandbrakeVal = 0.0f; }
    if (raw_t == -1) { ThrottleVal = 0.0f; }
    if (raw_b == -1) { BrakeVal = 0.0f; }
    if (raw_c == -1 &&
        (WheelButton[static_cast<int>(WheelControlType::Clutch)] == -1 ||
            WheelButtonGUIDs[static_cast<int>(WheelControlType::Clutch)] == GUID())) {
        ClutchVal = 0.0f; ClutchValRaw = 0.0f;
    }

    if (InvertSteer) { SteerVal = 1.0f - SteerVal; SteerValRaw = 1.0f - SteerValRaw; }
    if (InvertThrottle) { ThrottleVal = 1.0f - ThrottleVal; }
    if (InvertBrake) { BrakeVal = 1.0f - BrakeVal; }
    if (InvertClutch) { ClutchVal = 1.0f - ClutchVal; ClutchValRaw = 1.0f - ClutchValRaw; }
}

void ScriptControls::UpdateValues(InputDevices prevInput, bool ignoreClutch, bool justPeekingWheelKb) {
    if (UseLegacyController) {
        lcontroller.UpdateButtonChangeStates();
    }

    if (!UseLegacyController && controller.IsConnected()) {
        buttonState = controller.GetState().Gamepad.wButtons;
        controller.UpdateButtonChangeStates();
    }

    WheelControl.Update();
    WheelControl.UpdateButtonChangeStates();

    //StickControl.Update();

    CheckCustomButtons(justPeekingWheelKb);

    // Update ThrottleVal, BrakeVal and ClutchVal ranging from 0.0f (no touch) to 1.0f (full press)
    switch (prevInput) {
        case Keyboard: {
            updateKeyboard();
            break;
        }			
        case Controller: {
            updateController();
            break;
        }
        case Wheel: {
            updateWheel();
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
    if (UseLegacyController) {
        if (lcontroller.IsButtonJustPressed(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)])) ||
            lcontroller.IsButtonPressed(lcontroller.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)]))) {
            return Controller;
        }
    } else {
        if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState) ||
            controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]), buttonState)) {
            return Controller;
        }
    }
    if (enableWheel && WheelControl.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
        auto throttleAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]);
        auto throttleGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)];
        int rawThrottle = WheelControl.GetAxisValue(throttleAxis, throttleGUID);
        auto tempThrottle = 0.0f + 1.0f / (ThrottleMax - ThrottleMin)*(static_cast<float>(rawThrottle) - ThrottleMin);

        if (WheelAxes[static_cast<int>(WheelAxisType::Throttle)] == WheelAxes[static_cast<int>(WheelAxisType::Brake)]) {
            // get throttle range
            if (ThrottleMax > ThrottleMin &&
                rawThrottle < ThrottleMax &&
                rawThrottle > ThrottleMin + (ThrottleMax - ThrottleMin)*0.5) {
                return Wheel;
            }
            if (ThrottleMax < ThrottleMin && // Which twisted mind came up with this
                rawThrottle > ThrottleMax &&
                rawThrottle < ThrottleMin - (ThrottleMin - ThrottleMax)*0.5) {
                return Wheel;
            }
        }
        else if (tempThrottle > 0.5f) {
            return Wheel;
        }

        auto clutchGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)];
        if (WheelControl.IsConnected(clutchGUID)) {
            auto clutchAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]);
            int rawClutch = WheelControl.GetAxisValue(clutchAxis, clutchGUID);
            auto tempClutch = 0.0f + 1.0f / (ClutchMax - ClutchMin)*(static_cast<float>(rawClutch) - ClutchMin);
            if (tempClutch > 0.5f) {
                return Wheel;
            }
        }
        if (ButtonIn(WheelControlType::Clutch) || ButtonIn(WheelControlType::Throttle)) {
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

bool ScriptControls::ButtonReleasedAfter(ControllerControlType control, int time) {
    if (!controller.IsConnected())
        return false;
    if (controller.WasButtonHeldForMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, time))
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

bool ScriptControls::ButtonHeldOver(ControllerControlType control, int millis) {
    if (!controller.IsConnected())
        return false;
    if (controller.WasButtonHeldOverMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, millis))
        return true;
    return false;
}

XboxController::TapState ScriptControls::ButtonTapped(ControllerControlType control) {
    if (!controller.IsConnected())
        return XboxController::TapState::ButtonUp;
    return (controller.WasButtonTapped(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, MaxTapTime));
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
    lcontroller.TriggerValue = value;
}

float ScriptControls::GetXboxTrigger() {
    return controller.TriggerValue;
}

/*
 * Legacy stuff
 */
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

bool ScriptControls::ButtonHeldOver(LegacyControlType control, int millis) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.WasButtonHeldOverMs(gameButton, millis))
        return true;
    return false;
}

bool ScriptControls::ButtonIn(LegacyControlType control) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.IsButtonPressed(gameButton))
        return true;
    return false;
}

LegacyController::TapState ScriptControls::ButtonTapped(LegacyControlType control) {
    if (!UseLegacyController) return LegacyController::TapState::ButtonUp;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return lcontroller.WasButtonTapped(gameButton, MaxTapTime);
}

bool ScriptControls::ButtonReleasedAfter(LegacyControlType control, int time) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.WasButtonHeldForMs(gameButton, time))
        return true;
    return false;
}

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

bool ScriptControls::ButtonHeld(WheelControlType control, int delay) {
    if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
        WheelButton[static_cast<int>(control)] == -1) {
        return false;
    }
    if (WheelControl.WasButtonHeldForMs(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)], delay))
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
    for (int i = 0; i < WheelDirectInput::MAX_RGBBUTTONS; i++) {
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
    std::wstring wGuid = szGuidW;
    return(std::string(wGuid.begin(), wGuid.end()));
}

void ScriptControls::CheckGUIDs(const std::vector<_GUID> & guids) {
    auto foundGuids = WheelControl.GetGuids();
    auto reggdGuids = guids;
    // We're only checking for devices that should be used but aren't found
    std::sort(foundGuids.begin(), foundGuids.end());
    std::sort(reggdGuids.begin(), reggdGuids.end());
    std::vector<GUID> missingReg;
    std::vector<GUID> missingFnd;

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
