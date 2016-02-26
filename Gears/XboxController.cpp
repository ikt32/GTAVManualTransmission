#include "XboxController.hpp"
#include "TimeHelper.hpp"

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

void XboxController::Vibrate(int leftVal, int rightVal)
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

bool XboxController::IsButtonPressed(XboxButtons buttonType) {
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
		return(GetAnalogValue(buttonType) > 0.75f);
	}
	else {
		WORD buttonState = controllerState.Gamepad.wButtons;
		return (buttonState & XboxButtonMasks[buttonType]) != 0;
	}
}

bool XboxController::IsButtonJustPressed(XboxButtons buttonType) {
	XboxButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// raising edge
	if (XboxButtonCurr[buttonType] && !XboxButtonPrev[buttonType]) {
		XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
		return true;
	}

	XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
	return false;
}

bool XboxController::IsButtonJustReleased(XboxButtons buttonType) {
	XboxButtonCurr[buttonType] = IsButtonPressed(buttonType);

	// falling edge
	if (!XboxButtonCurr[buttonType] && XboxButtonPrev[buttonType]) {
		XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
		return true;
	}

	XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
	return false;
}

bool XboxController::WasButtonHeldForMs(XboxButtons buttonType, int milliseconds) {
	WORD buttonState = controllerState.Gamepad.wButtons;
	if (IsButtonJustPressed(buttonType)) {
		pressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType)) {
		releaseTime[buttonType] = milliseconds_now();
	}

	if ((releaseTime[buttonType] - pressTime[buttonType]) >= milliseconds) {
		pressTime[buttonType] = 0;
		releaseTime[buttonType] = 0;
		return true;
	}
	return false;
}

XboxController::XboxButtons XboxController::StringToButton(std::string buttonString) {
	for (int i = 0; i < SIZEOF_XboxButtons; i++) {
		if (buttonString == XboxButtonsHelper[i]) {
			return (XboxButtons)i;
		}
	}
	return UNKNOWN;
}

float XboxController::GetAnalogValue(XboxButtons buttonType) {
	if (!IsConnected()) {
		return 0.0f;
	}
	switch (buttonType) {
	case LeftTrigger:
		return (float)controllerState.Gamepad.bLeftTrigger / 255;
	case RightTrigger:
		return (float)controllerState.Gamepad.bRightTrigger / 255;
	case LeftThumbLeft:
		return fmaxf(0, -(float)controllerState.Gamepad.sThumbLX / 32767);
	case LeftThumbRight:
		return fmaxf(0, (float)controllerState.Gamepad.sThumbLX / 32767);
	case RightThumbLeft:
		return fmaxf(0, -(float)controllerState.Gamepad.sThumbRX / 32767);
	case RightThumbRight:
		return fmaxf(0, (float)controllerState.Gamepad.sThumbRX / 32767);
	case LeftThumbUp:
		return fmaxf(0, (float)controllerState.Gamepad.sThumbLY / 32767);
	case LeftThumbDown:
		return fmaxf(0, -(float)controllerState.Gamepad.sThumbLY / 32767);
	case RightThumbUp:
		return fmaxf(0, (float)controllerState.Gamepad.sThumbRY / 32767);
	case RightThumbDown:
		return fmaxf(0, -(float)controllerState.Gamepad.sThumbRY / 32767);
	default:
		return IsButtonPressed(buttonType) ? 1.0f : 0.0f;
	}
}
