#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <string>
#include <array>

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

	std::array<int, SIZEOF_XboxButtons> XboxButtonMasks = {
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

	std::array<std::string, SIZEOF_XboxButtons> XboxButtonsHelper = {
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
	std::array<__int64, SIZEOF_XboxButtons> pressTime;
	std::array<__int64, SIZEOF_XboxButtons> releaseTime;
	float filterDeadzone(XboxButtons buttonType, int input);

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

	std::array<bool, SIZEOF_XboxButtons> XboxButtonCurr;
	std::array<bool, SIZEOF_XboxButtons> XboxButtonPrev;

	XboxButtons StringToButton(std::string buttonString);

	// Returns a 0.0 to 1.0 value for any button
	float GetAnalogValue(XboxButtons buttonType, WORD buttonState);
	bool SetAnalogValue(XboxButtons buttonType, BYTE value);
};
