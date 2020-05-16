#include "CarControls.hpp"

#include "../ScriptSettings.hpp"
#include "../Util/Logger.hpp"
#include "../Util/MathExt.h"
#include "../Util/Util.hpp"
#include "keyboard.h"
#include <Windows.h>

extern ScriptSettings g_settings;

CarControls::CarControls()
    : PrevInput(Keyboard)
    , mXInputController(1) {
    std::fill(ControlXboxBlocks.begin(), ControlXboxBlocks.end(), -1);
}

CarControls::~CarControls() = default;

void CarControls::InitWheel() {
    if (!mWheelInput.InitWheel()) {
        // Initialization failed somehow, so we skip
        return;
    }

    InitFFB();
}

void CarControls::InitFFB() {

    auto steerGUID = WheelAxes[static_cast<int>(WheelAxisType::Steer)].Guid;
    auto ffAxis = mWheelInput.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)].Control);

    if (mWheelInput.InitFFB(steerGUID, ffAxis)) {
        mWheelInput.UpdateCenterSteering(steerGUID, ffAxis);
    }
}

void CarControls::updateKeyboard() {
    ThrottleVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)].Control) ? 1.0f : 0.0f;
    BrakeVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)].Control) ? 1.0f : 0.0f;
    ClutchVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)].Control) ? 1.0f : 0.0f;
}

void CarControls::updateController() {
    if (g_settings.Controller.Native.Enable) {
        ThrottleVal = mNativeController.GetAnalogValue(mNativeController.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Throttle)]));
        BrakeVal = mNativeController.GetAnalogValue(mNativeController.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Brake)]));
        ClutchVal = mNativeController.GetAnalogValue(mNativeController.EControlToButton(LegacyControls[static_cast<int>(LegacyControlType::Clutch)]));
    } else {
        ThrottleVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)]));
        BrakeVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)]));
        ClutchVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)]));
    }
}

// analog > button
float CarControls::getInputValue(WheelAxisType axisType, WheelControlType buttonType, float minRaw, float maxRaw) {
    float inputValue;
    auto axis = mWheelInput.StringToAxis(WheelAxes[static_cast<int>(axisType)].Control);
    auto axisGUID = WheelAxes[static_cast<int>(axisType)].Guid;

    int axisValue = mWheelInput.GetAxisValue(axis, axisGUID);

    if (axisValue != -1) {
        inputValue = map(static_cast<float>(axisValue), minRaw, maxRaw, 0.0f, 1.0f);
    }
    else {
        inputValue = ButtonIn(buttonType) ? 1.0f : 0.0f;
    }
    inputValue = std::clamp(inputValue, 0.0f, 1.0f);
    return inputValue;
}

float CarControls::filterDeadzone(float input, float deadzone, float deadzoneOffset) {
    float filtered;
    if (deadzone > 0.005f) {
        float center = 0.5f + deadzoneOffset;
        float minLimit = center - deadzone;
        float maxLimit = center + deadzone;
        if (input < minLimit) {
            filtered = map(input, 0.0f, minLimit, 0.0f, 0.5f);
        }
        else if (input > maxLimit) {
            filtered = map(input, maxLimit, 1.0f, 0.5f, 1.0f);
        }
        else {
            filtered = 0.5f;
        }
    }
    else {
        filtered = input;
    }
    return filtered;
}

void CarControls::updateWheel() {
    ThrottleVal = getInputValue(WheelAxisType::Throttle, WheelControlType::Throttle, 
        static_cast<float>(g_settings.Wheel.Throttle.Min), static_cast<float>(g_settings.Wheel.Throttle.Max));
    BrakeVal = getInputValue(WheelAxisType::Brake, WheelControlType::Brake, 
        static_cast<float>(g_settings.Wheel.Brake.Min), static_cast<float>(g_settings.Wheel.Brake.Max));
    ClutchVal = getInputValue(WheelAxisType::Clutch, WheelControlType::Clutch, 
        static_cast<float>(g_settings.Wheel.Clutch.Min), static_cast<float>(g_settings.Wheel.Clutch.Max));
    HandbrakeVal = getInputValue(WheelAxisType::Handbrake, WheelControlType::Handbrake, 
        static_cast<float>(g_settings.Wheel.HandbrakeA.Min), static_cast<float>(g_settings.Wheel.HandbrakeA.Max));
    SteerValRaw = getInputValue(WheelAxisType::Steer, WheelControlType::UNKNOWN, 
        static_cast<float>(g_settings.Wheel.Steering.Min), static_cast<float>(g_settings.Wheel.Steering.Max));
    SteerVal = filterDeadzone(SteerValRaw, g_settings.Wheel.Steering.DeadZone, g_settings.Wheel.Steering.DeadZoneOffset);
}

void CarControls::UpdateValues(InputDevices prevInput, bool skipKeyboardInput) {
    if (g_settings.Controller.Native.Enable) {
        mNativeController.Update();
    }
    else {
        mXInputController.Update();
    }

    mWheelInput.Update();
    mWheelInput.UpdateButtonChangeStates();

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
}

// Limitation: Only works for hardcoded input types. Currently throttle.
CarControls::InputDevices CarControls::GetLastInputDevice(InputDevices previousInput, bool enableWheel) {
    auto kbThrottleIdx = static_cast<int>(KeyboardControlType::Throttle);
    auto kbBrakeIdx = static_cast<int>(KeyboardControlType::Brake);
    if (IsKeyJustPressed(KBControl[kbThrottleIdx].Control, KeyboardControlType::Throttle) ||
        IsKeyPressed(KBControl[kbThrottleIdx].Control) ||
        IsKeyJustPressed(KBControl[kbBrakeIdx].Control, KeyboardControlType::Brake) ||
        IsKeyPressed(KBControl[kbBrakeIdx].Control)) {
        return Keyboard;
    }

    if (g_settings.Controller.Native.Enable) {
        auto lcThrottleIdx = static_cast<int>(LegacyControlType::Throttle);
        auto lcThrottleBtn = mNativeController.EControlToButton(LegacyControls[lcThrottleIdx]);
        auto lcBrakeIdx = static_cast<int>(LegacyControlType::Brake);
        auto lcBrakeBtn = mNativeController.EControlToButton(LegacyControls[lcBrakeIdx]);
        if (mNativeController.IsButtonJustPressed(lcThrottleBtn) ||
            mNativeController.IsButtonPressed(lcThrottleBtn) ||
            mNativeController.IsButtonJustPressed(lcBrakeBtn) ||
            mNativeController.IsButtonPressed(lcBrakeBtn)) {
            return Controller;
        }
    } 
    else {
        auto cThrottleIdx = static_cast<int>(ControllerControlType::Throttle);
        auto cThrottleBtn = mXInputController.StringToButton(ControlXbox[cThrottleIdx]);
        auto cBrakeIdx = static_cast<int>(ControllerControlType::Brake);
        auto cBrakeBtn = mXInputController.StringToButton(ControlXbox[cBrakeIdx]);
        if (mXInputController.IsButtonJustPressed(cThrottleBtn) ||
            mXInputController.IsButtonPressed(cThrottleBtn) ||
            mXInputController.IsButtonJustPressed(cBrakeBtn) ||
            mXInputController.IsButtonPressed(cBrakeBtn)) {
            return Controller;
        }
    }
    if (enableWheel && mWheelInput.IsConnected(WheelAxes[static_cast<int>(WheelAxisType::Steer)].Guid)) {
        float throttleVal = getInputValue(WheelAxisType::Throttle, WheelControlType::Throttle, 
            static_cast<float>(g_settings.Wheel.Throttle.Min), static_cast<float>(g_settings.Wheel.Throttle.Max));
        float brakeVal = getInputValue(WheelAxisType::Brake, WheelControlType::Brake, 
            static_cast<float>(g_settings.Wheel.Brake.Min), static_cast<float>(g_settings.Wheel.Brake.Max));
        float clutchVal = getInputValue(WheelAxisType::Clutch, WheelControlType::Clutch, 
            static_cast<float>(g_settings.Wheel.Clutch.Min), static_cast<float>(g_settings.Wheel.Clutch.Max));

        if (throttleVal > 0.5f ||
            brakeVal > 0.5f ||
            clutchVal > 0.5f) {
            return Wheel;
        }
    }
    return previousInput;
}

/*
 * Keyboard section
 */

bool CarControls::IsKeyPressed(int key) {
    return IsKeyDown(key);
}

bool CarControls::IsKeyJustPressed(int key, KeyboardControlType control) {
    KBControlCurr[static_cast<int>(control)] = IsKeyPressed(key);

    // raising edge
    if (KBControlCurr[static_cast<int>(control)] && !KBControlPrev[static_cast<int>(control)]) {
        KBControlPrev[static_cast<int>(control)] = KBControlCurr[static_cast<int>(control)];
        return true;
    }

    KBControlPrev[static_cast<int>(control)] = KBControlCurr[static_cast<int>(control)];
    return false;
}

bool CarControls::ButtonJustPressed(KeyboardControlType control) {
    return IsKeyJustPressed(KBControl[static_cast<int>(control)].Control, control);
}

/*
* Controller section
*/

bool CarControls::ButtonJustPressed(ControllerControlType control) {
    return mXInputController.IsButtonJustPressed(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]));
}

bool CarControls::ButtonReleased(ControllerControlType control) {
    return mXInputController.IsButtonJustReleased(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]));
}

bool CarControls::ButtonReleasedAfter(ControllerControlType control, int time) {
    return mXInputController.WasButtonHeldForMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]), time);
}

bool CarControls::ButtonHeld(ControllerControlType control) {
    return mXInputController.WasButtonHeldForMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]), g_settings.Controller.HoldTimeMs);
}

bool CarControls::ButtonHeldOver(ControllerControlType control, int millis) {
    return mXInputController.WasButtonHeldOverMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]), millis);
}

XInputController::TapState CarControls::ButtonTapped(ControllerControlType control) {
    return mXInputController.WasButtonTapped(mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]), g_settings.Controller.MaxTapTimeMs);
}

bool CarControls::ButtonIn(ControllerControlType control) {
    return mXInputController.IsButtonPressed(mXInputController.StringToButton(ControlXbox[static_cast<int>(control)]));
}

/*
 * Legacy stuff
 */
bool CarControls::ButtonJustPressed(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.IsButtonJustPressed(gameButton);
}
bool CarControls::ButtonReleased(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.IsButtonJustReleased(gameButton);
}

bool CarControls::ButtonHeld(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.WasButtonHeldForMs(gameButton, g_settings.Controller.HoldTimeMs);
}

bool CarControls::ButtonHeldOver(LegacyControlType control, int millis) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.WasButtonHeldOverMs(gameButton, millis);
}

bool CarControls::ButtonIn(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.IsButtonPressed(gameButton);
}

NativeController::TapState CarControls::ButtonTapped(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return NativeController::TapState::ButtonUp;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.WasButtonTapped(gameButton, g_settings.Controller.MaxTapTimeMs);
}

bool CarControls::ButtonReleasedAfter(LegacyControlType control, int time) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = mNativeController.EControlToButton(LegacyControls[static_cast<int>(control)]);
    return mNativeController.WasButtonHeldForMs(gameButton, time);
}

/*
 * Wheel section
 */

bool CarControls::ButtonJustPressed(WheelControlType control) {
    if (!mWheelInput.IsConnected(WheelButton[static_cast<int>(control)].Guid) ||
        WheelButton[static_cast<int>(control)].Control == -1) {
        return false;
    }
    return mWheelInput.IsButtonJustPressed(WheelButton[static_cast<int>(control)].Control,
                                           WheelButton[static_cast<int>(control)].Guid);
}

bool CarControls::ButtonReleased(WheelControlType control) {
    if (!mWheelInput.IsConnected(WheelButton[static_cast<int>(control)].Guid) ||
        WheelButton[static_cast<int>(control)].Control == -1) {
        return false;
    }
    return mWheelInput.IsButtonJustReleased(WheelButton[static_cast<int>(control)].Control,
                                            WheelButton[static_cast<int>(control)].Guid);
}

bool CarControls::ButtonHeld(WheelControlType control, int delay) {
    if (!mWheelInput.IsConnected(WheelButton[static_cast<int>(control)].Guid) ||
        WheelButton[static_cast<int>(control)].Control == -1) {
        return false;
    }
    return mWheelInput.WasButtonHeldForMs(WheelButton[static_cast<int>(control)].Control,
                                          WheelButton[static_cast<int>(control)].Guid, delay);
}

bool CarControls::ButtonIn(WheelControlType control) {
    if (!mWheelInput.IsConnected(WheelButton[static_cast<int>(control)].Guid) ||
        WheelButton[static_cast<int>(control)].Control == -1) {
        return false;
    }
    return mWheelInput.IsButtonPressed(WheelButton[static_cast<int>(control)].Control,
                                       WheelButton[static_cast<int>(control)].Guid);
}

void CarControls::CheckCustomButtons() {
    if (!mWheelInput.IsConnected(WheelAxes[static_cast<int>(WheelAxisType::Steer)].Guid)) {
        return;
    }
    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        if (WheelToKey[i] != -1) {
            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.dwExtraInfo = 0;
            input.ki.wVk = 0;
            input.ki.wScan = MapVirtualKey(WheelToKey[i], MAPVK_VK_TO_VSC);

            if (mWheelInput.IsButtonJustPressed(i,WheelToKeyGUID)) {
                input.ki.dwFlags = KEYEVENTF_SCANCODE;
                SendInput(1, &input, sizeof(INPUT));
            }
            if (mWheelInput.IsButtonJustReleased(i, WheelToKeyGUID)) {
                input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}

void CarControls::CheckGUIDs(const std::vector<_GUID> & guids) {
    auto foundGuids = mWheelInput.GetGuids();
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

    if (!missingReg.empty()) {
        logger.Write(WARN, "[Wheel] Used in .ini, unavailable: ");
        for (auto g : missingReg) {
            logger.Write(WARN, "[Wheel] GUID:   %s", GUID2String(g).c_str());
        }
    }

    // Enumerated but not registered
    std::set_difference(
        foundGuids.begin(), foundGuids.end(),
        reggdGuids.begin(), reggdGuids.end(), std::back_inserter(missingFnd));

    FreeDevices.clear();

    if (!missingFnd.empty()) {
        logger.Write(INFO, "[Wheel] Not set up in .ini: ");
        for (auto g : missingFnd) {
            auto entry = mWheelInput.FindEntryFromGUID(g);
            if (entry == nullptr) continue;
            std::wstring wDevName = entry->diDeviceInstance.tszInstanceName;
            auto devName = StrUtil::utf8_encode(wDevName);
            logger.Write(INFO, "[Wheel] Device: %s", devName.c_str());
            logger.Write(INFO, "[Wheel] GUID:   %s", GUID2String(g).c_str());
            if (isSupportedDrivingDevice(entry->diDeviceInstance.dwDevType)) {
                FreeDevices.push_back(Device(devName, g));
            }
        }
    }
}

void CarControls::PlayFFBDynamics(int totalForce, int damperForce) {
    auto ffbCtrl = WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)];
    auto axis = mWheelInput.StringToAxis(ffbCtrl.Control);
    mWheelInput.SetConstantForce(ffbCtrl.Guid, axis, totalForce);
    mWheelInput.SetDamper(ffbCtrl.Guid, axis, damperForce);
}

void CarControls::PlayFFBCollision(int collisionForce) {
    auto ffbCtrl = WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)];
    auto axis = mWheelInput.StringToAxis(ffbCtrl.Control);
    mWheelInput.SetCollision(ffbCtrl.Guid, axis, collisionForce);
}

void CarControls::PlayLEDs(float rpm, float firstLed, float lastLed) {
    auto ctrl = WheelAxes[static_cast<int>(WheelAxisType::Steer)];
    mWheelInput.PlayLedsDInput(ctrl.Guid, rpm, firstLed, lastLed);
}

float CarControls::GetAxisSpeed(WheelAxisType axis) {
    auto ctrl = WheelAxes[static_cast<int>(axis)];
    auto diAxis = mWheelInput.StringToAxis(ctrl.Control);
    return mWheelInput.GetAxisSpeed(diAxis, ctrl.Guid);
}

bool CarControls::WheelAvailable() {
    auto ctrl = WheelAxes[static_cast<int>(WheelAxisType::Steer)];
    return mWheelInput.IsConnected(ctrl.Guid);
}

WheelDirectInput& CarControls::GetWheel() {
    return mWheelInput;
}
