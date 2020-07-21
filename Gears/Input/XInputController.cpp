#include "XInputController.hpp"
#include "../ScriptSettings.hpp"
#include <string>

extern ScriptSettings g_settings;

XInputController::XInputController(int playerNumber)
    : pressTime()
    , releaseTime()
    , tapPressTime()
    , tapReleaseTime()
    , xboxButtonCurr()
    , xboxButtonPrev()
    , controllerState()
    , buttonState(0)
    , controllerNum(playerNumber - 1)
    , lastResult(ERROR_DEVICE_NOT_CONNECTED) {}

XInputController::~XInputController() = default;

XINPUT_STATE XInputController::getState() {
    // Zeroise the state
    ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

    // Get the state
    lastResult = XInputGetState(controllerNum, &controllerState);

    return controllerState;
}

bool XInputController::isConnected() {
    return lastResult == ERROR_SUCCESS;
}

void XInputController::Vibrate(int leftVal, int rightVal) {
    if (!isConnected()) return;
    // Create a Vibraton State
    XINPUT_VIBRATION Vibration;

    // Zeroise the Vibration
    ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

    // Set the Vibration Values
    Vibration.wLeftMotorSpeed = leftVal;
    Vibration.wRightMotorSpeed = rightVal;

    // Vibrate the controller
    XInputSetState(controllerNum, &Vibration);
}

bool XInputController::IsButtonPressed(XboxButtons buttonType) {
    if (buttonType == LeftTrigger ||
        buttonType == RightTrigger ||
        buttonType == LeftThumbLeft ||
        buttonType == LeftThumbRight ||
        buttonType == RightThumbLeft ||
        buttonType == RightThumbRight ||
        buttonType == LeftThumbUp ||
        buttonType == LeftThumbDown ||
        buttonType == RightThumbUp ||
        buttonType == RightThumbDown) {
        return (GetAnalogValue(buttonType) > g_settings.Controller.TriggerValue);
    }
    return (buttonState & XboxButtonMasks[buttonType]) != 0;
}

bool XInputController::IsButtonJustPressed(XboxButtons buttonType) {
    if (!isConnected()) return false;
    xboxButtonCurr[buttonType] = IsButtonPressed(buttonType);

    // raising edge
    return xboxButtonCurr[buttonType] && !xboxButtonPrev[buttonType];
}

bool XInputController::IsButtonJustReleased(XboxButtons buttonType) {
    if (!isConnected()) return false;
    xboxButtonCurr[buttonType] = IsButtonPressed(buttonType);

    // falling edge
    return !xboxButtonCurr[buttonType] && xboxButtonPrev[buttonType];
}

bool XInputController::WasButtonHeldForMs(XboxButtons buttonType, int milliseconds) {
    if (!isConnected()) return false;
    if (IsButtonJustPressed(buttonType)) {
        pressTime[buttonType] = GetTickCount64();
    }
    if (IsButtonJustReleased(buttonType)) {
        releaseTime[buttonType] = GetTickCount64();
    }

    if ((releaseTime[buttonType] - pressTime[buttonType]) >= milliseconds) {
        pressTime[buttonType] = 0;
        releaseTime[buttonType] = 0;
        return true;
    }
    return false;
}

bool XInputController::WasButtonHeldOverMs(XboxButtons buttonType, int millis) {
    if (!isConnected()) return false;
    if (IsButtonJustPressed(buttonType)) {
        pressTime[buttonType] = GetTickCount64();
    }

    return IsButtonPressed(buttonType) && pressTime[buttonType] != 0 && GetTickCount64() - pressTime[buttonType] >=
        millis;
}

XInputController::TapState XInputController::WasButtonTapped(XboxButtons buttonType, int milliseconds) {
    if (!isConnected()) return TapState::ButtonUp;
    if (IsButtonJustPressed(buttonType)) {
        tapPressTime[buttonType] = GetTickCount64();
    }
    if (IsButtonJustReleased(buttonType)) {
        tapReleaseTime[buttonType] = GetTickCount64();
    }

    if ((tapReleaseTime[buttonType] - tapPressTime[buttonType]) > 1 &&
        (tapReleaseTime[buttonType] - tapPressTime[buttonType]) <= milliseconds) {
        tapPressTime[buttonType] = 0;
        tapReleaseTime[buttonType] = 0;
        return TapState::Tapped;
    } 
    if ((GetTickCount64() - tapPressTime[buttonType]) <= milliseconds) {
        return TapState::ButtonDown;
    }
    return TapState::ButtonUp;
}

XInputController::XboxButtons XInputController::StringToButton(const std::string &buttonString) {
    for (int i = 0; i < SIZEOF_XboxButtons; i++) {
        if (buttonString == XboxButtonsHelper[i]) {
            return static_cast<XboxButtons>(i);
        }
    }
    return UNKNOWN;
}

float XInputController::GetAnalogValue(XboxButtons buttonType) {
    switch (buttonType) {
        case LeftTrigger:		return static_cast<float>(controllerState.Gamepad.bLeftTrigger) / 255;
        case RightTrigger:		return static_cast<float>(controllerState.Gamepad.bRightTrigger) / 255;
        case LeftThumbLeft:		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLX);
        case LeftThumbRight:	return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLX);
        case RightThumbLeft:	return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRX);
        case RightThumbRight:	return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRX);
        case LeftThumbUp:		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLY);
        case LeftThumbDown:		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLY);
        case RightThumbUp:		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRY);
        case RightThumbDown:	return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRY);
        default:				return IsButtonPressed(buttonType) ? 1.0f : 0.0f;
    }
}

void XInputController::Update() {
    auto state = getState();

    if (!isConnected()) return;

    buttonState = state.Gamepad.wButtons;

    for (int i = 0; i < SIZEOF_XboxButtons; i++) {
        xboxButtonPrev[i] = xboxButtonCurr[i];
    }
}

float XInputController::filterDeadzone(XboxButtons buttonType, int input) {
    int deadzone;
    switch (buttonType) {
        case LeftThumbLeft:
        case LeftThumbRight:
        case LeftThumbUp:
        case LeftThumbDown:		deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                                break;
        case RightThumbLeft:
        case RightThumbRight:
        case RightThumbUp:
        case RightThumbDown:	deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
                                break;
        default:				deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                                break;
    }

    if (buttonType == LeftThumbLeft ||
        buttonType == LeftThumbDown ||
        buttonType == RightThumbLeft ||
        buttonType == RightThumbDown) {
        input = -input;
    }

    if (input > deadzone) {
        if (input > 32767) {
            input = 32767;
        }

        input -= deadzone;
        return static_cast<float>(input) / static_cast<float>(32767 - deadzone);
    }
    return 0.0f;
}
