#include "XboxController.hpp"
#include "TimeHelper.hpp"
#include <string>

XboxController::XboxController(int playerNumber) {
	controllerNum = playerNumber - 1;
}

XINPUT_STATE XboxController::GetState() {
	// Zeroise the state
	ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

	// Get the state
	XInputGetState(controllerNum, &controllerState);

	return controllerState;
}

bool XboxController::IsConnected()
{
	// Zeroise the state
	ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

	// Get the state
	DWORD Result = XInputGetState(controllerNum, &controllerState);

	if (Result == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void XboxController::Vibrate(int leftVal, int rightVal) const
{
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

bool XboxController::IsButtonPressed(XboxButtons buttonType, WORD buttonState) {
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
		return(GetAnalogValue(buttonType, buttonState) > 0.25f);
	}
	else {
		return (buttonState & XboxButtonMasks[buttonType]) != 0;
	}
}

bool XboxController::IsButtonJustPressed(XboxButtons buttonType, WORD buttonState) {
	XboxButtonCurr[buttonType] = IsButtonPressed(buttonType, buttonState);

	// raising edge
	if (XboxButtonCurr[buttonType] && !XboxButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool XboxController::IsButtonJustReleased(XboxButtons buttonType, WORD buttonState) {
	XboxButtonCurr[buttonType] = IsButtonPressed(buttonType, buttonState);

	// falling edge
	if (!XboxButtonCurr[buttonType] && XboxButtonPrev[buttonType]) {
		return true;
	}
	return false;
}

bool XboxController::WasButtonHeldForMs(XboxButtons buttonType, WORD buttonState, int milliseconds) {
	if (IsButtonJustPressed(buttonType, buttonState)) {
		pressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType, buttonState)) {
		releaseTime[buttonType] = milliseconds_now();
	}

	if ((releaseTime[buttonType] - pressTime[buttonType]) >= milliseconds) {
		pressTime[buttonType] = 0;
		releaseTime[buttonType] = 0;
		return true;
	}
	return false;
}

// KILL ME NOW
void XboxController::UpdateButtonChangeStates()
{
	for (int i = 0; i < SIZEOF_XboxButtons; i++) {
		XboxButtonPrev[i] = XboxButtonCurr[i];
	}
}

XboxController::XboxButtons XboxController::StringToButton(std::string buttonString) {
	for (int i = 0; i < SIZEOF_XboxButtons; i++) {
		if (buttonString == XboxButtonsHelper[i]) {
			return static_cast<XboxButtons>(i);
		}
	}
	return UNKNOWN;
}

float XboxController::GetAnalogValue(XboxButtons buttonType, WORD buttonState) {
	switch (buttonType) {
	case LeftTrigger:
		return static_cast<float>(controllerState.Gamepad.bLeftTrigger) / 255;
	case RightTrigger:
		return static_cast<float>(controllerState.Gamepad.bRightTrigger) / 255;
	case LeftThumbLeft:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLX);
	case LeftThumbRight:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLX);
	case RightThumbLeft:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRX);
	case RightThumbRight:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRX);
	case LeftThumbUp:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLY);
	case LeftThumbDown:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbLY);
	case RightThumbUp:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRY);
	case RightThumbDown:
		return filterDeadzone(buttonType, controllerState.Gamepad.sThumbRY);
	default:
		return IsButtonPressed(buttonType, buttonState) ? 1.0f : 0.0f;
	}
}

float XboxController::filterDeadzone(XboxButtons buttonType, int input) {
	int deadzone;
	switch (buttonType) {
	case LeftThumbLeft:
	case LeftThumbRight:
	case LeftThumbUp:
	case LeftThumbDown:
		deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		break;
	case RightThumbLeft:
	case RightThumbRight:
	case RightThumbUp:
	case RightThumbDown:
		deadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		break;
	default:
		deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
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
	else {
		return 0.0f;
	}
}
