#include "NativeController.h"
#include <inc/natives.h>
#include "Util/TimeHelper.hpp"


NativeController::NativeController(): triggerValue(0.75f) {}


NativeController::~NativeController()
{
}

bool NativeController::IsButtonPressed(GameButtons gameButton) {
    if (CONTROLS::IS_CONTROL_PRESSED(0, GameEnums[gameButton])) {
        return true;
    }
    if (CONTROLS::GET_CONTROL_NORMAL(0, GameEnums[gameButton]) > triggerValue) {
        return true;
    }
    return false;
}

bool NativeController::IsButtonJustPressed(GameButtons gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // raising edge
    if (gameButtonCurr[gameButton] && !gameButtonPrev[gameButton]) {
        return true;
    }
    return false;
}

bool NativeController::IsButtonJustReleased(GameButtons gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // falling edge
    if (!gameButtonCurr[gameButton] && gameButtonPrev[gameButton]) {
        return true;
    }
    return false;
}

bool NativeController::WasButtonHeldForMs(GameButtons gameButton, int milliseconds) {
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

bool NativeController::WasButtonHeldOverMs(GameButtons gameButton, int milliseconds) {
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

NativeController::TapState NativeController::WasButtonTapped(GameButtons buttonType, int milliseconds) {
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

void NativeController::Update() {
    for (int i = 0; i < SIZEOF_GameButtons; i++) {
        gameButtonPrev[i] = gameButtonCurr[i];
    }
}

float NativeController::GetAnalogValue(GameButtons gameButton) {
    return CONTROLS::GET_CONTROL_NORMAL(0, GameEnums[gameButton]);
}

NativeController::GameButtons NativeController::EControlToButton(int eControlItem) {
    for (int i = 0; i < SIZEOF_GameButtons; i++) {
        if (eControlItem == GameEnums[i]) {
            return static_cast<GameButtons>(i);
        }
    }
    return UNKNOWN;
}

void NativeController::SetTriggerValue(float value) {
    triggerValue = value;
}
