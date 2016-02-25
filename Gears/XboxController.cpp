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

bool XboxController::IsButtonJustPressed(WORD buttonState, XboxButtons buttonType) {
	XboxButtonCurr[buttonType] = (buttonState & XboxButtonMasks[buttonType]) != 0;

	// raising edge
	if (XboxButtonCurr[buttonType] && !XboxButtonPrev[buttonType]) {
		XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
		return true;
	}

	XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
	return false;
}

bool XboxController::IsButtonJustReleased(WORD buttonState, XboxButtons buttonType) {
	XboxButtonCurr[buttonType] = (buttonState & XboxButtonMasks[buttonType]) != 0;

	// falling edge
	if (!XboxButtonCurr[buttonType] && XboxButtonPrev[buttonType]) {
		XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
		return true;
	}

	XboxButtonPrev[buttonType] = XboxButtonCurr[buttonType];
	return false;
}

bool XboxController::WasButtonHeldForMs(WORD buttonState, XboxButtons buttonType, int milliseconds) {
	if (IsButtonJustPressed(buttonState, buttonType)) {
		pressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonState, buttonType)) {
		releaseTime[buttonType] = milliseconds_now();
	}

	if ((releaseTime[buttonType] - pressTime[buttonType]) >= milliseconds) {
		pressTime[buttonType] = 0;
		releaseTime[buttonType] = 0;
		return true;
	}
	return false;
}
