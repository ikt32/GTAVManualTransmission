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
		return(GetAnalogValue(buttonType, buttonState) > 0.75f);
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
			return (XboxButtons)i;
		}
	}
	return UNKNOWN;
}

float XboxController::GetAnalogValue(XboxButtons buttonType, WORD buttonState) {
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
		return IsButtonPressed(buttonType, buttonState) ? 1.0f : 0.0f;
	}
}

