#pragma once
#include <array>

class LegacyController
{
public:
	enum GameButtons {
		ControlFrontendDown		 ,
		ControlFrontendUp		 ,
		ControlFrontendLeft		 ,
		ControlFrontendRight	 ,
		ControlFrontendRdown	 ,
		ControlFrontendRup		 ,
		ControlFrontendRleft	 ,
		ControlFrontendRright	 ,
		ControlFrontendAxisX	 ,
		ControlFrontendAxisY	 ,
		ControlFrontendRightAxisX,
		ControlFrontendRightAxisY,
		ControlFrontendPause	 ,
		ControlFrontendAccept	 ,
		ControlFrontendCancel	 ,
		ControlFrontendX		 ,
		ControlFrontendY		 ,
		ControlFrontendLb		 ,
		ControlFrontendRb		 ,
		ControlFrontendLt		 ,
		ControlFrontendRt		 ,
		ControlFrontendLs		 ,
		ControlFrontendRs		 ,
		ControlFrontendDelete	 ,
		ControlFrontendSelect	 ,
		UNKNOWN,
		SIZEOF_GameButtons
	};

	std::array<int, SIZEOF_GameButtons> GameEnums = {
		ControlFrontendDown		 ,
		ControlFrontendUp		 ,
		ControlFrontendLeft		 ,
		ControlFrontendRight	 ,
		ControlFrontendRdown	 ,
		ControlFrontendRup		 ,
		ControlFrontendRleft	 ,
		ControlFrontendRright	 ,
		ControlFrontendAxisX	 ,
		ControlFrontendAxisY	 ,
		ControlFrontendRightAxisX,
		ControlFrontendRightAxisY,
		ControlFrontendPause	 ,
		ControlFrontendAccept	 ,
		ControlFrontendCancel	 ,
		ControlFrontendX		 ,
		ControlFrontendY		 ,
		ControlFrontendLb		 ,
		ControlFrontendRb		 ,
		ControlFrontendLt		 ,
		ControlFrontendRt		 ,
		ControlFrontendLs		 ,
		ControlFrontendRs		 ,
		ControlFrontendDelete	 ,
		ControlFrontendSelect,
		-1 // UNKNOWN
	};

private:
	std::array<__int64, SIZEOF_GameButtons> pressTime;
	std::array<__int64, SIZEOF_GameButtons> releaseTime;
	std::array<bool, SIZEOF_GameButtons> gameButtonCurr;
	std::array<bool, SIZEOF_GameButtons> gameButtonPrev;

public:
	LegacyController();
	~LegacyController();

	bool IsButtonPressed(GameButtons gameButton);
	bool IsButtonJustPressed(GameButtons gameButton);
	bool IsButtonJustReleased(GameButtons gameButton);
	bool WasButtonHeldForMs(GameButtons gameButton, int milliseconds);

	float GetAnalogValue(GameButtons gameButton);
	GameButtons EControlToButton(int eControlItem);

};

