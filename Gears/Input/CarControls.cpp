#include "CarControls.hpp"

#include "../ScriptSettings.hpp"
#include "../Util/Logger.hpp"
#include "../Util/MathExt.h"
#include "../Util/Strings.hpp"
#include "../Util/GUID.h"
#include "keyboard.h"
#include <Windows.h>

extern ScriptSettings g_settings;

namespace {
    // For Wheel2Key mouse click
    MOUSEINPUT MouseButton2Event(int keyval, bool up) {
        DWORD mouseFlag = 0;
        DWORD mouseData = 0;
        switch (keyval) {
            case VK_LBUTTON:
                mouseFlag = MOUSEEVENTF_LEFTDOWN;
                break;
            case VK_RBUTTON:
                mouseFlag = MOUSEEVENTF_RIGHTDOWN;
                break;
            case VK_MBUTTON:
                mouseFlag = MOUSEEVENTF_MIDDLEDOWN;
                break;
            case VK_XBUTTON1:
                mouseFlag = MOUSEEVENTF_XDOWN;
                mouseData = XBUTTON1;
                break;
            case VK_XBUTTON2:
                mouseFlag = MOUSEEVENTF_XDOWN;
                mouseData = XBUTTON2;
                break;
        }
        if (up)
            mouseFlag <<= 1;

        return { 0, 0, mouseData, mouseFlag, 0, NULL };
    }
}

CarControls::CarControls()
    : PrevInput(Keyboard)
    , mXInputController(1) {
    std::fill(ControlXboxBlocks.begin(), ControlXboxBlocks.end(), -1);
}

CarControls::~CarControls() = default;

void CarControls::InitWheel() {
    if (!mWheelInput.InitWheel()) {
        logger.Write(INFO, "[Wheel] Wheel not initialized, skipping FFB initialization");
        return;
    }

    auto steerGUID = WheelAxes[static_cast<int>(WheelAxisType::Steer)].Guid;
    auto ffAxis = mWheelInput.StringToAxis(WheelAxes[static_cast<int>(WheelAxisType::ForceFeedback)].Control);

    if (!mWheelInput.InitFFB(steerGUID, ffAxis)) {
        logger.Write(ERROR, "[Wheel] Force feedback initialization failed");
    }
}

void CarControls::updateKeyboard() {
    ThrottleVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Throttle)].Control) ? 1.0f : 0.0f;
    BrakeVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Brake)].Control) ? 1.0f : 0.0f;
    ClutchVal = IsKeyPressed(KBControl[static_cast<int>(KeyboardControlType::Clutch)].Control) ? 1.0f : 0.0f;
}

void CarControls::updateController() {
    if (g_settings.Controller.Native.Enable) {
        ThrottleVal = mNativeController.GetAnalogValue(LegacyControls[static_cast<int>(LegacyControlType::Throttle)].Control);
        BrakeVal = mNativeController.GetAnalogValue(LegacyControls[static_cast<int>(LegacyControlType::Brake)].Control);
        ClutchVal = mNativeController.GetAnalogValue(LegacyControls[static_cast<int>(LegacyControlType::Clutch)].Control);
    } else {
        ThrottleVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Throttle)].Control));
        BrakeVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Brake)].Control));
        ClutchVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::Clutch)].Control));

        SteerVal = mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::SteerLeft)].Control),  g_settings.Controller.CustomDeadzone) -
                   mXInputController.GetAnalogValue(mXInputController.StringToButton(ControlXbox[static_cast<int>(ControllerControlType::SteerRight)].Control), g_settings.Controller.CustomDeadzone);
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

    auto canUseAxis = [this](WheelAxisType axisType) -> bool {
        auto axis = mWheelInput.StringToAxis(WheelAxes[static_cast<int>(axisType)].Control);
        auto axisGUID = WheelAxes[static_cast<int>(axisType)].Guid;
        int axisValue = mWheelInput.GetAxisValue(axis, axisGUID);

        if (axisValue != -1) {
            return true;
        }
        return false;
    };

    if (canUseAxis(WheelAxisType::Handbrake)) {
        UseAnalogHandbrake = true;
    }
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
        auto lcThrottleBtn = LegacyControls[lcThrottleIdx].Control;
        auto lcBrakeIdx = static_cast<int>(LegacyControlType::Brake);
        auto lcBrakeBtn = LegacyControls[lcBrakeIdx].Control;
        if (mNativeController.IsButtonJustPressed(lcThrottleBtn) ||
            mNativeController.IsButtonPressed(lcThrottleBtn) ||
            mNativeController.IsButtonJustPressed(lcBrakeBtn) ||
            mNativeController.IsButtonPressed(lcBrakeBtn)) {
            return Controller;
        }
    } 
    else {
        auto cThrottleIdx = static_cast<int>(ControllerControlType::Throttle);
        auto cThrottleBtn = mXInputController.StringToButton(ControlXbox[cThrottleIdx].Control);
        auto cBrakeIdx = static_cast<int>(ControllerControlType::Brake);
        auto cBrakeBtn = mXInputController.StringToButton(ControlXbox[cBrakeIdx].Control);
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
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control));
}

bool CarControls::ButtonReleased(ControllerControlType control) {
    return mXInputController.IsButtonJustReleased(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control));
}

bool CarControls::ButtonReleasedAfter(ControllerControlType control, int time) {
    return mXInputController.WasButtonHeldForMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control), time);
}

bool CarControls::ButtonHeld(ControllerControlType control) {
    return mXInputController.WasButtonHeldForMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control), g_settings.Controller.HoldTimeMs);
}

bool CarControls::ButtonHeldOver(ControllerControlType control, int millis) {
    return mXInputController.WasButtonHeldOverMs(
        mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control), millis);
}

XInputController::TapState CarControls::ButtonTapped(ControllerControlType control) {
    return mXInputController.WasButtonTapped(mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control), g_settings.Controller.MaxTapTimeMs);
}

bool CarControls::ButtonIn(ControllerControlType control) {
    return mXInputController.IsButtonPressed(mXInputController.StringToButton(ControlXbox[static_cast<int>(control)].Control));
}

/*
 * Native stuff
 */
bool CarControls::ButtonJustPressed(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.IsButtonJustPressed(gameButton);
}
bool CarControls::ButtonReleased(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.IsButtonJustReleased(gameButton);
}

bool CarControls::ButtonHeld(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.WasButtonHeldForMs(gameButton, g_settings.Controller.HoldTimeMs);
}

bool CarControls::ButtonHeldOver(LegacyControlType control, int millis) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.WasButtonHeldOverMs(gameButton, millis);
}

bool CarControls::ButtonIn(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.IsButtonPressed(gameButton);
}

NativeController::TapState CarControls::ButtonTapped(LegacyControlType control) {
    if (!g_settings.Controller.Native.Enable) return NativeController::TapState::ButtonUp;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
    return mNativeController.WasButtonTapped(gameButton, g_settings.Controller.MaxTapTimeMs);
}

bool CarControls::ButtonReleasedAfter(LegacyControlType control, int time) {
    if (!g_settings.Controller.Native.Enable) return false;
    auto gameButton = LegacyControls[static_cast<int>(control)].Control;
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

    for (const auto& input : WheelToKey) {
        updateKeyInputEvents(input.Guid, input.Control, GetKeyFromName(input.ConfigTag));
    }
}

void CarControls::updateKeyInputEvents(GUID guid, int button, int keyCode) {
    // Mouse (VK 0x01 through 0x06 are mouse)
    if (keyCode >= 1 && keyCode <= 6) {
        INPUT input;
        input.type = INPUT_MOUSE;

        if (mWheelInput.IsButtonJustPressed(button, guid)) {
            input.mi = MouseButton2Event(keyCode, false);
            if (input.mi.dwFlags != 0) {
                SendInput(1, &input, sizeof(INPUT));
            }
        }
        if (mWheelInput.IsButtonJustReleased(button, guid)) {
            input.mi = MouseButton2Event(keyCode, true);
            if (input.mi.dwFlags != 0) {
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
    // Keyboard
    else {
        INPUT input;
        input.type = INPUT_KEYBOARD;
        input.ki.dwExtraInfo = 0;
        input.ki.wVk = 0;
        input.ki.wScan = MapVirtualKey(keyCode, MAPVK_VK_TO_VSC);

        if (mWheelInput.IsButtonJustPressed(button, guid)) {
            input.ki.dwFlags = KEYEVENTF_SCANCODE;
            SendInput(1, &input, sizeof(INPUT));
        }
        if (mWheelInput.IsButtonJustReleased(button, guid)) {
            input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
}

void CarControls::CheckGUIDs(const std::vector<_GUID> & guids) {
    auto keySelector = [](auto pair) {return pair.first; };
    const auto& devices = mWheelInput.GetDevices();
    std::vector<GUID> foundGuids(devices.size());
    transform(devices.begin(), devices.end(), foundGuids.begin(), keySelector);

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
        for (auto guid : missingFnd) {
            auto entry = mWheelInput.GetDeviceInfo(guid);
            if (!entry)
                continue;
            auto devName = entry->DeviceInstance.tszInstanceName;
            logger.Write(INFO, "[Wheel] Device: %s", devName);
            logger.Write(INFO, "[Wheel] GUID:   %s", GUID2String(guid).c_str());
            if (isSupportedDrivingDevice(entry->DeviceInstance.dwDevType)) {
                FreeDevices.push_back(Device(devName, guid));
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

bool CarControls::IsHShifterJustNeutral() {
    constexpr auto minGear = static_cast<uint32_t>(CarControls::WheelControlType::HR);
    constexpr auto topGear = static_cast<uint32_t>(CarControls::WheelControlType::H10);
    for (auto gear = minGear; gear <= topGear; ++gear) {
        if (ButtonReleased(static_cast<CarControls::WheelControlType>(gear)))
            return true;
    }
    return false;
}

bool CarControls::IsHShifterInGear() {
    return ButtonIn(CarControls::WheelControlType::HR) ||
        ButtonIn(CarControls::WheelControlType::H1) ||
        ButtonIn(CarControls::WheelControlType::H2) ||
        ButtonIn(CarControls::WheelControlType::H3) ||
        ButtonIn(CarControls::WheelControlType::H4) ||
        ButtonIn(CarControls::WheelControlType::H5) ||
        ButtonIn(CarControls::WheelControlType::H6) ||
        ButtonIn(CarControls::WheelControlType::H7) ||
        ButtonIn(CarControls::WheelControlType::H8) ||
        ButtonIn(CarControls::WheelControlType::H9) ||
        ButtonIn(CarControls::WheelControlType::H10);
}

bool CarControls::IsClutchPressed() {
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic)
        return false;
    return ClutchVal > 1.0f - g_settings().MTParams.ClutchThreshold;
}
