#include "NativeController.h"
#include <inc/natives.h>
#include "Util/TimeHelper.hpp"


NativeController::NativeController(): pressTime()
                                    , releaseTime()
                                    , tapPressTime()
                                    , tapReleaseTime()
                                    , gameButtonCurr()
                                    , gameButtonPrev()
                                    , triggerValue(0.75f) {}

NativeController::~NativeController() = default;

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
    return gameButtonCurr[gameButton] && !gameButtonPrev[gameButton];
}

bool NativeController::IsButtonJustReleased(GameButtons gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // falling edge
    return !gameButtonCurr[gameButton] && gameButtonPrev[gameButton];
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

    return CONTROLS::IS_CONTROL_PRESSED(0, GameEnums[gameButton]) && pressTime[gameButton] != 0 && (milliseconds_now() -
        pressTime[gameButton]) >= milliseconds;
}

NativeController::TapState NativeController::WasButtonTapped(GameButtons gameButton, int milliseconds) {
    if (IsButtonJustPressed(gameButton)) {
        tapPressTime[gameButton] = milliseconds_now();
    }
    if (IsButtonJustReleased(gameButton)) {
        tapReleaseTime[gameButton] = milliseconds_now();
    }

    if ((tapReleaseTime[gameButton] - tapPressTime[gameButton]) > 1 &&
        (tapReleaseTime[gameButton] - tapPressTime[gameButton]) <= milliseconds) {
        tapPressTime[gameButton] = 0;
        tapReleaseTime[gameButton] = 0;
        return TapState::Tapped;
    }
    if ((milliseconds_now() - tapPressTime[gameButton]) <= milliseconds) {
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
