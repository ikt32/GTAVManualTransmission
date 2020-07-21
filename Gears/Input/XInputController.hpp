#pragma once
#include <Windows.h>
#include <Xinput.h>
#include <array>
#include <string>

class XInputController {
public:
    enum class TapState {
        ButtonUp,
        ButtonDown,
        Tapped
    };

    enum XboxButtons {
        DpadUp,
        DpadDown,
        DpadLeft,
        DpadRight,
        Start,
        Back,
        LeftThumb,
        RightThumb,
        LeftShoulder,
        RightShoulder,
        A,
        B,
        X,
        Y,
        LeftTrigger,
        RightTrigger,
        LeftThumbLeft,
        LeftThumbRight,
        RightThumbLeft,
        RightThumbRight,
        LeftThumbUp,
        LeftThumbDown,
        RightThumbUp,
        RightThumbDown,
        UNKNOWN,
        SIZEOF_XboxButtons
    };

    std::array<int, SIZEOF_XboxButtons> XboxButtonMasks = {
        XINPUT_GAMEPAD_DPAD_UP,
        XINPUT_GAMEPAD_DPAD_DOWN,
        XINPUT_GAMEPAD_DPAD_LEFT,
        XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_START,
        XINPUT_GAMEPAD_BACK,
        XINPUT_GAMEPAD_LEFT_THUMB,
        XINPUT_GAMEPAD_RIGHT_THUMB,
        XINPUT_GAMEPAD_LEFT_SHOULDER,
        XINPUT_GAMEPAD_RIGHT_SHOULDER,
        XINPUT_GAMEPAD_A,
        XINPUT_GAMEPAD_B,
        XINPUT_GAMEPAD_X,
        XINPUT_GAMEPAD_Y,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    };

    std::array<std::string, SIZEOF_XboxButtons> XboxButtonsHelper = {
        "DpadUp",
        "DpadDown",
        "DpadLeft",
        "DpadRight",
        "Start",
        "Back",
        "LeftThumb",
        "RightThumb",
        "LeftShoulder",
        "RightShoulder",
        "A",
        "B",
        "X",
        "Y",
        "LeftTrigger",
        "RightTrigger",
        "LeftThumbLeft",
        "LeftThumbRight",
        "RightThumbLeft",
        "RightThumbRight",
        "LeftThumbUp",
        "LeftThumbDown",
        "RightThumbUp",
        "RightThumbDown",
        "UNKNOWN"
    };

    XInputController(int playerNumber);
    ~XInputController();

    bool IsButtonPressed(XboxButtons xboxButton);
    bool IsButtonJustPressed(XboxButtons xboxButton);
    bool IsButtonJustReleased(XboxButtons xboxButton);
    bool WasButtonHeldForMs(XboxButtons xboxButton, int milliseconds);
    bool WasButtonHeldOverMs(XboxButtons xboxButton, int millis);
    TapState WasButtonTapped(XboxButtons xboxButton, int milliseconds);
    void Update();

    float GetAnalogValue(XboxButtons buttonType);
    XboxButtons StringToButton(const std::string &buttonString);

    void Vibrate(int leftval = 0, int rightval = 0);

private:
    std::array<__int64, SIZEOF_XboxButtons> pressTime;
    std::array<__int64, SIZEOF_XboxButtons> releaseTime;
    std::array<__int64, SIZEOF_XboxButtons> tapPressTime;
    std::array<__int64, SIZEOF_XboxButtons> tapReleaseTime;
    std::array<bool, SIZEOF_XboxButtons> xboxButtonCurr;
    std::array<bool, SIZEOF_XboxButtons> xboxButtonPrev;

    XINPUT_STATE controllerState;
    WORD buttonState;
    int controllerNum;
    DWORD lastResult;

    XINPUT_STATE getState();
    bool isConnected();
    float filterDeadzone(XboxButtons buttonType, int input);
};
