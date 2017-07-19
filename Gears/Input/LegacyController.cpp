#include "LegacyController.h"
#include "inc/natives.h"
#include "../Util/TimeHelper.hpp"


LegacyController::LegacyController(): TriggerValue(0.75f) {}


LegacyController::~LegacyController()
{
}

bool LegacyController::IsButtonPressed(GameButtons gameButton) {
	if (CONTROLS::IS_CONTROL_PRESSED(0, GameEnums[gameButton])) {
		return true;
	}
	if (CONTROLS::GET_CONTROL_NORMAL(0, GameEnums[gameButton]) > TriggerValue) {
		return true;
	}
	return false;
}

bool LegacyController::IsButtonJustPressed(GameButtons gameButton) {
	gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

	// raising edge
	if (gameButtonCurr[gameButton] && !gameButtonPrev[gameButton]) {
		return true;
	}
	return false;
}

bool LegacyController::IsButtonJustReleased(GameButtons gameButton) {
	gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

	// falling edge
	if (!gameButtonCurr[gameButton] && gameButtonPrev[gameButton]) {
		return true;
	}
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

bool LegacyController::WasButtonHeldOverMs(GameButtons gameButton, int milliseconds) {
	if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, GameEnums[gameButton])) {
		pressTime[gameButton] = milliseconds_now();
	}

	if (CONTROLS::IS_CONTROL_PRESSED(0, GameEnums[gameButton]) &&
		pressTime[gameButton] != 0 &&
		(milliseconds_now() - pressTime[gameButton]) >= milliseconds) {
		return true;
	}
	return false;
}

LegacyController::TapState LegacyController::WasButtonTapped(GameButtons buttonType, int milliseconds) {
	if (IsButtonJustPressed(buttonType)) {
		tapPressTime[buttonType] = milliseconds_now();
	}
	if (IsButtonJustReleased(buttonType)) {
		tapReleaseTime[buttonType] = milliseconds_now();
	}

	if ((tapReleaseTime[buttonType] - tapPressTime[buttonType]) > 1 &&
		(tapReleaseTime[buttonType] - tapPressTime[buttonType]) <= milliseconds) {
		tapPressTime[buttonType] = 0;
		tapReleaseTime[buttonType] = 0;
		return TapState::Tapped;
	}
	if ((milliseconds_now() - tapPressTime[buttonType]) <= milliseconds) {
		return TapState::ButtonDown;
	}
	return TapState::ButtonUp;
}

void LegacyController::UpdateButtonChangeStates() {
	for (int i = 0; i < SIZEOF_GameButtons; i++) {
		gameButtonPrev[i] = gameButtonCurr[i];
	}
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
