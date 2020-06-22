#pragma once
#include <inc/enums.h>

#include <unordered_map>
#include <stdexcept>
#include <string>

class NativeController
{
public:
    enum class TapState {
        ButtonUp,
        ButtonDown,
        Tapped
    };

    static inline const std::unordered_map<eControl, std::string> NativeGamepadInputs = {
        { ControlFrontendDown      , "Dpad down" },
        { ControlFrontendUp        , "Dpad up" },
        { ControlFrontendLeft      , "Dpad left" },
        { ControlFrontendRight     , "Dpad right" },
        { ControlFrontendAxisX     , "Left stick X" },
        { ControlFrontendAxisY     , "Left stick Y" },
        { ControlFrontendRightAxisX, "Right stick X" },
        { ControlFrontendRightAxisY, "Right stick Y" },
        { ControlFrontendPause     , "Start" },
        { ControlFrontendAccept    , "A" },
        { ControlFrontendCancel    , "B" },
        { ControlFrontendX         , "X" },
        { ControlFrontendY         , "Y" },
        { ControlFrontendLb        , "Left shoulder" },
        { ControlFrontendRb        , "Right shoulder" },
        { ControlFrontendLt        , "Left trigger" },
        { ControlFrontendRt        , "Right trigger" },
        { ControlFrontendLs        , "Left stick click" },
        { ControlFrontendRs        , "Right stick click" },
        { ControlFrontendSelect    , "Select" },
    };

    NativeController();

    bool IsButtonPressed(eControl gameButton);
    bool IsButtonJustPressed(eControl gameButton);
    bool IsButtonJustReleased(eControl gameButton);
    bool WasButtonHeldForMs(eControl gameButton, int milliseconds);
    bool WasButtonHeldOverMs(eControl gameButton, int milliseconds);
    TapState WasButtonTapped(eControl gameButton, int milliseconds);
    void Update();

    float GetAnalogValue(eControl gameButton);

    static std::string GetControlName(int control) {
        if (control == -1)
            return "None";

        auto it = NativeGamepadInputs.find(static_cast<eControl>(control));

        if (it == NativeGamepadInputs.end()) {
            return "Unknown input type";
        }

        return it->second;
    }

private:
    std::unordered_map<eControl, __int64> pressTime;
    std::unordered_map<eControl, __int64> releaseTime;
    std::unordered_map<eControl, __int64> tapPressTime;
    std::unordered_map<eControl, __int64> tapReleaseTime;
    std::unordered_map<eControl, bool> gameButtonCurr;
    std::unordered_map<eControl, bool> gameButtonPrev;
};

