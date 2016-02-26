#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <string>

class XboxController
{
public:
	enum XboxButtons {
		DpadUp,
		DpadDown,
		DpadLeft,
		DpadRight,
		Start,
		Back,
		LeftThumb,
		RightThumb,
		LeftShoulder,
		RightShoulder,
		A,
		B,
		X,
		Y,
		LeftTrigger,
		RightTrigger,
		LeftThumbLeft,
		LeftThumbRight,
		RightThumbLeft,
		RightThumbRight,
		LeftThumbUp,
		LeftThumbDown,
		RightThumbUp,
		RightThumbDown,
		UNKNOWN,
		SIZEOF_XboxButtons
	};

	int XboxButtonMasks[SIZEOF_XboxButtons] = {
		XINPUT_GAMEPAD_DPAD_UP,
		XINPUT_GAMEPAD_DPAD_DOWN,
		XINPUT_GAMEPAD_DPAD_LEFT,
		XINPUT_GAMEPAD_DPAD_RIGHT,
		XINPUT_GAMEPAD_START,
		XINPUT_GAMEPAD_BACK,
		XINPUT_GAMEPAD_LEFT_THUMB,
		XINPUT_GAMEPAD_RIGHT_THUMB,
		XINPUT_GAMEPAD_LEFT_SHOULDER,
		XINPUT_GAMEPAD_RIGHT_SHOULDER,
		XINPUT_GAMEPAD_A,
		XINPUT_GAMEPAD_B,
		XINPUT_GAMEPAD_X,
		XINPUT_GAMEPAD_Y,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	std::string XboxButtonsHelper[SIZEOF_XboxButtons] = {
		"DpadUp",
		"DpadDown",
		"DpadLeft",
		"DpadRight",
		"Start",
		"Back",
		"LeftThumb",
		"RightThumb",
		"LeftShoulder",
		"RightShoulder",
		"A",
		"B",
		"X",
		"Y",
		"LeftTrigger",
		"RightTrigger",
		"LeftThumbLeft",
		"LeftThumbRight",
		"RightThumbLeft",
		"RightThumbRight",
		"LeftThumbUp",
		"LeftThumbDown",
		"RightThumbUp",
		"RightThumbDown",
		"UNKNOWN"
	};

private:
	XINPUT_STATE controllerState;
	int controllerNum;
	__int64 pressTime[SIZEOF_XboxButtons];
	__int64 releaseTime[SIZEOF_XboxButtons];

public:
	XboxController(int playerNumber);
	XINPUT_STATE GetState();
	bool IsConnected();
	void Vibrate(int leftval = 0, int rightval = 0);

	bool IsButtonPressed(XboxButtons buttonType, WORD buttonState);
	bool IsButtonJustPressed(XboxButtons buttonType, WORD buttonState);
	bool IsButtonJustReleased(XboxButtons buttonType, WORD buttonState);
	bool WasButtonHeldForMs(XboxButtons buttonType, WORD buttonState, int milliseconds);
	void UpdateButtonChangeStates();

	bool XboxButtonCurr[SIZEOF_XboxButtons];
	bool XboxButtonPrev[SIZEOF_XboxButtons];

	XboxButtons StringToButton(std::string buttonString);

	// Returns a 0.0 to 1.0 value for any button
	float GetAnalogValue(XboxButtons buttonType, WORD buttonState);
};

