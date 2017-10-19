/*
 * "Legacy" is a bad name, initially chosen because native controls were
 * replaced with Xinput. Turns out it's still desired, so it's back.
 * This class just mirrors XboxController except it uses natives.
 * 
 * No plans to make a DirectInput controller thing though as I 
 * already have the wheel thing.
 */

#pragma once
#include <array>
#include "inc/enums.h"

class LegacyController
{
public:
    enum class TapState {
        ButtonUp,
        ButtonDown,
        Tapped
    };

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
        eControl::ControlFrontendDown		,
        eControl::ControlFrontendUp			,
        eControl::ControlFrontendLeft		,
        eControl::ControlFrontendRight		,
        eControl::ControlFrontendRdown		,
        eControl::ControlFrontendRup		,
        eControl::ControlFrontendRleft		,
        eControl::ControlFrontendRright		,
        eControl::ControlFrontendAxisX		,
        eControl::ControlFrontendAxisY		,
        eControl::ControlFrontendRightAxisX	,
        eControl::ControlFrontendRightAxisY	,
        eControl::ControlFrontendPause		,
        eControl::ControlFrontendAccept		,
        eControl::ControlFrontendCancel		,
        eControl::ControlFrontendX			,
        eControl::ControlFrontendY			,
        eControl::ControlFrontendLb			,
        eControl::ControlFrontendRb			,
        eControl::ControlFrontendLt			,
        eControl::ControlFrontendRt			,
        eControl::ControlFrontendLs			,
        eControl::ControlFrontendRs			,
        eControl::ControlFrontendDelete		,
        eControl::ControlFrontendSelect		,
        -1 // UNKNOWN
    };
    float TriggerValue;

private:
    std::array<__int64, SIZEOF_GameButtons> pressTime;
    std::array<__int64, SIZEOF_GameButtons> releaseTime;
    std::array<__int64, SIZEOF_GameButtons> tapPressTime;
    std::array<__int64, SIZEOF_GameButtons> tapReleaseTime;
    std::array<bool, SIZEOF_GameButtons> gameButtonCurr;
    std::array<bool, SIZEOF_GameButtons> gameButtonPrev;

public:
    LegacyController();
    ~LegacyController();

    bool IsButtonPressed(GameButtons gameButton);
    bool IsButtonJustPressed(GameButtons gameButton);
    bool IsButtonJustReleased(GameButtons gameButton);
    bool WasButtonHeldForMs(GameButtons gameButton, int milliseconds);
    bool WasButtonHeldOverMs(GameButtons gameButton, int millis);
    TapState WasButtonTapped(GameButtons buttonType, int milliseconds);
    void UpdateButtonChangeStates();

    float GetAnalogValue(GameButtons gameButton);
    GameButtons EControlToButton(int eControlItem);

};

