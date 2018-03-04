#include "CarControls.hpp"

#include <Windows.h>
#include "keyboard.h"
#include "Util/Logger.hpp"
#include "Util/MathExt.h"

CarControls::CarControls(): PrevInput(Keyboard),
                                WheelControl(),
                                SteerAxisType(WheelAxisType::Steer),
                                controller{1},
                                buttonState(0) {
    std::fill(ControlXboxBlocks.begin(), ControlXboxBlocks.end(), -1);
}

CarControls::~CarControls() { }

void CarControls::InitWheel() {
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

void CarControls::updateKeyboard() {
    ThrottleVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)]) ? 1.0f : 0.0f);
    BrakeVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)]) ? 1.0f : 0.0f);
    ClutchVal = (IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)]) ? 1.0f : 0.0f);
    ClutchValRaw = ClutchVal;
}

void CarControls::updateController() {
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

void CarControls::updateWheel() {
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

void CarControls::UpdateValues(InputDevices prevInput, bool skipKeyboardInput) {
    if (UseLegacyController) {
        lcontroller.UpdateButtonChangeStates();
    }

    if (!UseLegacyController && controller.IsConnected()) {
        buttonState = controller.GetState().Gamepad.wButtons;
        controller.UpdateButtonChangeStates();
    }

    WheelControl.Update();
    WheelControl.UpdateButtonChangeStates();

    if (!skipKeyboardInput)   
        CheckCustomButtons();

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

    averageBrakeVals[movAvgIndex] = BrakeVal;
    BrakeValAvg = 0.0f;
    for (float val : averageBrakeVals) {
        BrakeValAvg += val;
    }
    BrakeValAvg /= AVERAGEWINDOW;
    movAvgIndex = (movAvgIndex + 1) % AVERAGEWINDOW;
}

// Limitation: Only works for hardcoded input types. Currently throttle.
CarControls::InputDevices CarControls::GetLastInputDevice(InputDevices previousInput, bool enableWheel) {
    auto kbThrottleIdx = static_cast<int>(KeyboardControlType::Throttle);
    auto kbBrakeIdx = static_cast<int>(KeyboardControlType::Brake);
    if (IsKeyJustPressed(KBControl[kbThrottleIdx], KeyboardControlType::Throttle) ||
        IsKeyPressed(KBControl[kbThrottleIdx]) ||
        IsKeyJustPressed(KBControl[kbBrakeIdx], KeyboardControlType::Brake) ||
        IsKeyPressed(KBControl[kbBrakeIdx])) {
        return Keyboard;
    }

    if (UseLegacyController) {
        auto lcThrottleIdx = static_cast<int>(LegacyControlType::Throttle);
        auto lcThrottleBtn = lcontroller.EControlToButton(LegacyControls[lcThrottleIdx]);
        auto lcBrakeIdx = static_cast<int>(LegacyControlType::Brake);
        auto lcBrakeBtn = lcontroller.EControlToButton(LegacyControls[lcBrakeIdx]);
        if (lcontroller.IsButtonJustPressed(lcThrottleBtn) ||
            lcontroller.IsButtonPressed(lcThrottleBtn) ||
            lcontroller.IsButtonJustPressed(lcBrakeBtn) ||
            lcontroller.IsButtonPressed(lcBrakeBtn)) {
            return Controller;
        }
    } 
    else {
        auto cThrottleIdx = static_cast<int>(ControllerControlType::Throttle);
        auto cThrottleBtn = controller.StringToButton(ControlXbox[cThrottleIdx]);
        auto cBrakeIdx = static_cast<int>(ControllerControlType::Brake);
        auto cBrakeBtn = controller.StringToButton(ControlXbox[cBrakeIdx]);
        if (controller.IsButtonJustPressed(cThrottleBtn, buttonState) ||
            controller.IsButtonPressed(cThrottleBtn, buttonState) ||
            controller.IsButtonJustPressed(cBrakeBtn, buttonState) ||
            controller.IsButtonPressed(cBrakeBtn, buttonState)) {
            return Controller;
        }
    }
    if (enableWheel && WheelControl.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
        auto throttleAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Throttle)]);
        auto throttleGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Throttle)];
        int raw_t = WheelControl.GetAxisValue(throttleAxis, throttleGUID);
        auto tempThrottle = map((float)raw_t, (float)ThrottleMin, (float)ThrottleMax, 0.0f, 1.0f);
        if (raw_t == -1) {
            tempThrottle = 0.0f;
        }
        if (InvertThrottle) {
            tempThrottle = 1.0f - tempThrottle;
        }
        if (tempThrottle > 0.5f) {
            return Wheel;
        }

        auto clutchGUID = WheelAxesGUIDs[static_cast<int>(WheelAxisType::Clutch)];
        auto clutchAxis = WheelControl.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::Clutch)]);
        int raw_c = WheelControl.GetAxisValue(clutchAxis, clutchGUID);
        auto tempClutch = map((float)raw_c, (float)ClutchMin, (float)ClutchMax, 0.0f, 1.0f);
        if (raw_c == -1) {
            tempClutch = 0.0f;
        }
        if (InvertClutch) {
            tempClutch = 1.0f - tempClutch;
        }
        if (tempClutch > 0.5f) {
            return Wheel;
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

bool CarControls::IsKeyPressed(int key) {
    //if (!IsWindowFocused()) return false;
    //if (GetAsyncKeyState(key) & 0x8000) return true;
    if (IsKeyDown(key)) return true;
    return false;
}

bool CarControls::IsKeyJustPressed(int key, KeyboardControlType control) {
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

bool CarControls::ButtonJustPressed(KeyboardControlType control) {
    if (IsKeyJustPressed(KBControl[static_cast<int>(control)], control))
        return true;
    return false;
}

/*
* Controller section
*/

bool CarControls::ButtonJustPressed(ControllerControlType control) {
    if (!controller.IsConnected())
        return false;
    if (controller.IsButtonJustPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
        return true;
    return false;
}

bool CarControls::ButtonReleased(ControllerControlType control) {
    if (!controller.IsConnected())
        return false;
    if (controller.IsButtonJustReleased(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
        return true;
    return false;
}

bool CarControls::ButtonReleasedAfter(ControllerControlType control, int time) {
    if (!controller.IsConnected())
        return false;
    if (controller.WasButtonHeldForMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, time))
        return true;
    return false;
}

bool CarControls::ButtonHeld(ControllerControlType control) {
    if (!controller.IsConnected())
        return false;
    if (controller.WasButtonHeldForMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, CToggleTime))
        return true;
    return false;
}

bool CarControls::ButtonHeldOver(ControllerControlType control, int millis) {
    if (!controller.IsConnected())
        return false;
    if (controller.WasButtonHeldOverMs(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, millis))
        return true;
    return false;
}

XboxController::TapState CarControls::ButtonTapped(ControllerControlType control) {
    if (!controller.IsConnected())
        return XboxController::TapState::ButtonUp;
    return (controller.WasButtonTapped(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState, MaxTapTime));
}

bool CarControls::ButtonIn(ControllerControlType control) {
    if (!controller.IsConnected())
        return false;
    if (controller.IsButtonPressed(controller.StringToButton(ControlXbox[static_cast<int>(control)]), buttonState))
        return true;
    return false;
}

void CarControls::SetXboxTrigger(float value) {
    controller.TriggerValue = value;
    lcontroller.TriggerValue = value;
}

float CarControls::GetXboxTrigger() {
    return controller.TriggerValue;
}

/*
 * Legacy stuff
 */
bool CarControls::ButtonJustPressed(LegacyControlType control) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.IsButtonJustPressed(gameButton))
        return true;
    return false;
}
bool CarControls::ButtonReleased(LegacyControlType control) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.IsButtonJustReleased(gameButton))
        return true;
    return false;
}

bool CarControls::ButtonHeld(LegacyControlType control) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.WasButtonHeldForMs(gameButton, CToggleTime))
        return true;
    return false;
}

bool CarControls::ButtonHeldOver(LegacyControlType control, int millis) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.WasButtonHeldOverMs(gameButton, millis))
        return true;
    return false;
}

bool CarControls::ButtonIn(LegacyControlType control) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.IsButtonPressed(gameButton))
        return true;
    return false;
}

LegacyController::TapState CarControls::ButtonTapped(LegacyControlType control) {
    if (!UseLegacyController) return LegacyController::TapState::ButtonUp;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return lcontroller.WasButtonTapped(gameButton, MaxTapTime);
}

bool CarControls::ButtonReleasedAfter(LegacyControlType control, int time) {
    if (!UseLegacyController) return false;
    auto gameButton = lcontroller.EControlToButton(LegacyControls[static_cast<int>(control)]);
    if (lcontroller.WasButtonHeldForMs(gameButton, time))
        return true;
    return false;
}

/*
 * Wheel section
 */

bool CarControls::ButtonJustPressed(WheelControlType control) {
    if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
        WheelButton[static_cast<int>(control)] == -1) {
        return false;
    }
    if (WheelControl.IsButtonJustPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
        return true;
    return false;
}

bool CarControls::ButtonReleased(WheelControlType control) {
    if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
        WheelButton[static_cast<int>(control)] == -1) {
        return false;
    }
    if (WheelControl.IsButtonJustReleased(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
        return true;
    return false;
}

bool CarControls::ButtonHeld(WheelControlType control, int delay) {
    if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
        WheelButton[static_cast<int>(control)] == -1) {
        return false;
    }
    if (WheelControl.WasButtonHeldForMs(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)], delay))
        return true;
    return false;
}

bool CarControls::ButtonIn(WheelControlType control) {
    if (!WheelControl.IsConnected(WheelButtonGUIDs[static_cast<int>(control)]) ||
        WheelButton[static_cast<int>(control)] == -1) {
        return false;
    }
    if (WheelControl.IsButtonPressed(WheelButton[static_cast<int>(control)], WheelButtonGUIDs[static_cast<int>(control)]))
        return true;
    return false;
}

void CarControls::CheckCustomButtons() {
    if (!WheelControl.IsConnected(WheelAxesGUIDs[static_cast<int>(WheelAxisType::Steer)])) {
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
    std::wstring wGuid = szGuidW;
    return(std::string(wGuid.begin(), wGuid.end()));
}

bool isSupportedDrivingDevice(DWORD dwDevType) {
    switch (GET_DIDEVICE_TYPE(dwDevType)) {
        case DI8DEVTYPE_DRIVING:        // Wheels
        case DI8DEVTYPE_SUPPLEMENTAL:   // Pedal sets, shifters, etc
        case DI8DEVTYPE_FLIGHT:         // Yay HOTAS
            return true;
        default:
            return false;
    }
}

void CarControls::CheckGUIDs(const std::vector<_GUID> & guids) {
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
        logger.Write(WARN, "WHEEL: Used in .ini, unavailable: ");
        for (auto g : missingReg) {
            logger.Write(WARN, "WHEEL: GUID:   %s", GUID2String(g).c_str());
        }
    }

    // Enumerated but not registered
    std::set_difference(
        foundGuids.begin(), foundGuids.end(),
        reggdGuids.begin(), reggdGuids.end(), std::back_inserter(missingFnd));

    FreeDevices.clear();

    if (missingFnd.size() > 0) {
        logger.Write(INFO, "WHEEL: Not set up in .ini: ");
        for (auto g : missingFnd) {
            auto entry = WheelControl.FindEntryFromGUID(g);
            if (entry == nullptr) continue;
            std::wstring wDevName = entry->diDeviceInstance.tszInstanceName;
            auto devName = std::string(wDevName.begin(), wDevName.end());
            logger.Write(INFO, "WHEEL: Device: %s", devName.c_str());
            logger.Write(INFO, "WHEEL: GUID:   %s", GUID2String(g).c_str());
            if (isSupportedDrivingDevice(entry->diDeviceInstance.dwDevType)) {
                FreeDevices.push_back(Device(devName, g));
            }
        }
    }
}
