#include "NativeInput.h"
#include <inc/natives.h>
#include "../Util/TimeHelper.hpp"
#include "../ScriptSettings.hpp"

extern ScriptSettings g_settings;

NativeInput::NativeInput() = default;

#ifndef NO_NATIVES
bool NativeInput::IsButtonPressed(eControl gameButton) {
    if (PAD::IS_DISABLED_CONTROL_PRESSED(0, gameButton)) {
        return true;
    }
    if (PAD::GET_DISABLED_CONTROL_NORMAL(0, gameButton) > g_settings.Controller.TriggerValue) {
        return true;
    }
    return false;
}

bool NativeInput::IsButtonJustPressed(eControl gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // raising edge
    return gameButtonCurr[gameButton] && !gameButtonPrev[gameButton];
}

bool NativeInput::IsButtonJustReleased(eControl gameButton) {
    gameButtonCurr[gameButton] = IsButtonPressed(gameButton);

    // falling edge
    return !gameButtonCurr[gameButton] && gameButtonPrev[gameButton];
}

bool NativeInput::WasButtonHeldForMs(eControl gameButton, int milliseconds) {
    if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, gameButton)) {
        pressTime[gameButton] = milliseconds_now();
    }
    if (PAD::IS_DISABLED_CONTROL_JUST_RELEASED(0, gameButton)) {
        releaseTime[gameButton] = milliseconds_now();
    }

    if ((releaseTime[gameButton] - pressTime[gameButton]) >= milliseconds) {
        pressTime[gameButton] = 0;
        releaseTime[gameButton] = 0;
        return true;
    }
    return false;
}

bool NativeInput::WasButtonHeldOverMs(eControl gameButton, int milliseconds) {
    if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, gameButton)) {
        pressTime[gameButton] = milliseconds_now();
    }

    return PAD::IS_CONTROL_PRESSED(0, gameButton) && pressTime[gameButton] != 0 && (milliseconds_now() -
        pressTime[gameButton]) >= milliseconds;
}

NativeInput::TapState NativeInput::WasButtonTapped(eControl gameButton, int milliseconds) {
    if (IsButtonJustPressed(gameButton)) {
        tapPressTime[gameButton] = milliseconds_now();
        //showText(0.7f, 0.125f, 0.5f, fmt::format("just press"));
    }
    if (IsButtonJustReleased(gameButton)) {
        tapReleaseTime[gameButton] = milliseconds_now();
        //showText(0.7f, 0.125f, 0.5f, fmt::format("just rel"));
    }

    if ((tapReleaseTime[gameButton] - tapPressTime[gameButton]) > 1 &&
        (tapReleaseTime[gameButton] - tapPressTime[gameButton]) <= milliseconds) {
        tapPressTime[gameButton] = 0;
        tapReleaseTime[gameButton] = 0;
        //showText(0.7f, 0.10f, 0.5f, fmt::format("tapped"));
        return TapState::Tapped;
    }
    if ((milliseconds_now() - tapPressTime[gameButton]) <= milliseconds) {
        //showText(0.7f, 0.10f, 0.5f, fmt::format("down"));
        return TapState::ButtonDown;
    }
    //showText(0.7f, 0.10f, 0.5f, fmt::format("up"));
    return TapState::ButtonUp;
}


void NativeInput::Update() {
    for (const auto& input : gameButtonCurr) {
        gameButtonPrev[input.first] = gameButtonCurr[input.first];
    }
}

float NativeInput::GetAnalogValue(eControl gameButton) {
    return PAD::GET_DISABLED_CONTROL_NORMAL(0, gameButton);
}

#else

// Stubs
bool NativeController2::IsButtonPressed(eControl gameButton) { return false; }
bool NativeController2::IsButtonJustPressed(eControl gameButton) { return false; }
bool NativeController2::IsButtonJustReleased(eControl gameButton) { return false; }
bool NativeController2::WasButtonHeldForMs(eControl, int) { return false; }
bool NativeController2::WasButtonHeldOverMs(eControl, int) { return false; }
NativeController2::TapState NativeController::WasButtonTapped(eControl, int) { return TapState::ButtonUp; }
void NativeController2::Update() {}
float NativeController2::GetAnalogValue(eControl gameButton) { return false; }

#endif
