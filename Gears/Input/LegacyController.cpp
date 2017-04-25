#include "LegacyController.h"
#include "inc/natives.h"
#include "../Util/TimeHelper.hpp"


LegacyController::LegacyController()
{
}


LegacyController::~LegacyController()
{
}

bool LegacyController::IsButtonPressed(GameButtons gameButton) {
	if (CONTROLS::IS_CONTROL_PRESSED(0, GameEnums[gameButton])) {
		return true;
	}
	return false;
}

bool LegacyController::IsButtonJustPressed(GameButtons gameButton) {
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, GameEnums[gameButton]))
		return true;
	return false;
}

bool LegacyController::IsButtonJustReleased(GameButtons gameButton) {
	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, GameEnums[gameButton]))
		return true;
	return false;
}

bool LegacyController::WasButtonHeldForMs(GameButtons gameButton, int milliseconds) {
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, GameEnums[gameButton])) {
		pressTime[gameButton] = milliseconds_now();
	}
	if (CONTROLS::IS_CONTROL_JUST_RELEASED(0, GameEnums[gameButton])) {
		releaseTime[gameButton] = milliseconds_now();
	}

	if ((releaseTime[gameButton] - pressTime[gameButton]) >= milliseconds) {
		pressTime[gameButton] = 0;
		releaseTime[gameButton] = 0;
		return true;
	}
	return false;
}

float LegacyController::GetAnalogValue(GameButtons gameButton) {
	return CONTROLS::GET_CONTROL_NORMAL(0, GameEnums[gameButton]);
}

LegacyController::GameButtons LegacyController::EControlToButton(int eControlItem) {
	for (int i = 0; i < SIZEOF_GameButtons; i++) {
		if (eControlItem == GameEnums[i]) {
			return static_cast<GameButtons>(i);
		}
	}
	return UNKNOWN;
}
