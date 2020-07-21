#include "NativeController.h"
#include <inc/natives.h>
#include "../ScriptSettings.hpp"

extern ScriptSettings g_settings;

NativeController::NativeController() = default;

#ifndef NO_NATIVES
bool NativeController::IsButtonPressed(eControl gameButton) {
    if (PAD::IS_CONTROL_PRESSED(0, gameButton)) {
        return true;
    }
    if (PAD::GET_CONTROL_NORMAL(0, gameButton) > g_settings.Controller.TriggerValue) {
        return true;
    }
    return false;
}

bool NativeController::IsButtonJustPressed(eControl gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // raising edge
    return gameButtonCurr[gameButton] && !gameButtonPrev[gameButton];
}

bool NativeController::IsButtonJustReleased(eControl gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // falling edge
    return !gameButtonCurr[gameButton] && gameButtonPrev[gameButton];
}

bool NativeController::WasButtonHeldForMs(eControl gameButton, int milliseconds) {
    if (PAD::IS_CONTROL_JUST_PRESSED(0, gameButton)) {
        pressTime[gameButton] = GetTickCount64();
    }
    if (PAD::IS_CONTROL_JUST_RELEASED(0, gameButton)) {
        releaseTime[gameButton] = GetTickCount64();
    }

    if ((releaseTime[gameButton] - pressTime[gameButton]) >= milliseconds) {
        pressTime[gameButton] = 0;
        releaseTime[gameButton] = 0;
        return true;
    }
    return false;
}

bool NativeController::WasButtonHeldOverMs(eControl gameButton, int milliseconds) {
    if (PAD::IS_CONTROL_JUST_PRESSED(0, gameButton)) {
        pressTime[gameButton] = GetTickCount64();
    }

    return PAD::IS_CONTROL_PRESSED(0, gameButton) && pressTime[gameButton] != 0 && (GetTickCount64() -
        pressTime[gameButton]) >= milliseconds;
}

NativeController::TapState NativeController::WasButtonTapped(eControl gameButton, int milliseconds) {
    if (IsButtonJustPressed(gameButton)) {
        tapPressTime[gameButton] = GetTickCount64();
    }
    if (IsButtonJustReleased(gameButton)) {
        tapReleaseTime[gameButton] = GetTickCount64();
    }

    if ((tapReleaseTime[gameButton] - tapPressTime[gameButton]) > 1 &&
        (tapReleaseTime[gameButton] - tapPressTime[gameButton]) <= milliseconds) {
        tapPressTime[gameButton] = 0;
        tapReleaseTime[gameButton] = 0;
        return TapState::Tapped;
    }
    if ((GetTickCount64() - tapPressTime[gameButton]) <= milliseconds) {
        return TapState::ButtonDown;
    }
    return TapState::ButtonUp;
}

void NativeController::Update() {
    for (const auto& input : NativeGamepadInputs) {
        gameButtonPrev[input.first] = gameButtonCurr[input.first];
    }
}

float NativeController::GetAnalogValue(eControl gameButton) {
    return PAD::GET_CONTROL_NORMAL(0, gameButton);
}

#else

// Stubs
bool NativeController::IsButtonPressed(eControl gameButton) { return false; }
bool NativeController::IsButtonJustPressed(eControl gameButton) { return false; }
bool NativeController::IsButtonJustReleased(eControl gameButton) { return false; }
bool NativeController::WasButtonHeldForMs(eControl, int) { return false; }
bool NativeController::WasButtonHeldOverMs(eControl, int) { return false; }
NativeController::TapState NativeController::WasButtonTapped(eControl, int) { return TapState::ButtonUp; }
void NativeController::Update() {}
float NativeController::GetAnalogValue(eControl gameButton) { return false; }

#endif
