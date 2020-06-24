#pragma once
#include <inc/enums.h>
#include <unordered_map>

// NativeController class, but usable for any control
// Not used for "normal" input and filtering.
class NativeInput
{
public:
    enum class TapState {
        ButtonUp,
        ButtonDown,
        Tapped
    };

    NativeInput();

    bool IsButtonPressed(eControl gameButton);
    bool IsButtonJustPressed(eControl gameButton);
    bool IsButtonJustReleased(eControl gameButton);
    bool WasButtonHeldForMs(eControl gameButton, int milliseconds);
    bool WasButtonHeldOverMs(eControl gameButton, int milliseconds);
    TapState WasButtonTapped(eControl gameButton, int milliseconds);
    void Update();

    float GetAnalogValue(eControl gameButton);

private:
    std::unordered_map<eControl, __int64> pressTime;
    std::unordered_map<eControl, __int64> releaseTime;
    std::unordered_map<eControl, __int64> tapPressTime;
    std::unordered_map<eControl, __int64> tapReleaseTime;
    std::unordered_map<eControl, bool> gameButtonCurr;
    std::unordered_map<eControl, bool> gameButtonPrev;

    std::unordered_map<eControl, bool> inputs;
};

