#pragma once
#include <Windows.h>
#include <Xinput.h>

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
		XINPUT_GAMEPAD_Y
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

	bool IsButtonJustPressed(WORD buttonState, XboxButtons buttonType);
	bool IsButtonJustReleased(WORD buttonState, XboxButtons buttonType);
	bool WasButtonHeldForMs(WORD buttonState, XboxButtons buttonType, int milliseconds);

	bool XboxButtonCurr[SIZEOF_XboxButtons];
	bool XboxButtonPrev[SIZEOF_XboxButtons];
};

