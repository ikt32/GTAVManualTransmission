#pragma once
#include <string>
#include <utility>

#include "XInputController.hpp"
#include "WheelDirectInput.hpp"
#include "NativeController.h"

struct Device {
    Device(std::string name, GUID guid)
        : name(std::move(name)), guid(guid) {}
    std::string name;
    GUID guid;
};

class CarControls {
public:
    enum class ControllerControlType {
        ShiftUp,
        ShiftDown,
        Clutch,
        Throttle,
        Brake,
        Toggle,
        ToggleH,
        Engine,
        CycleAssists,
        SIZEOF_ControllerControlType
    };

    enum class LegacyControlType {
        ShiftUp,
        ShiftDown,
        Clutch,
        Throttle,
        Brake,
        Toggle,
        ToggleH,
        Engine,
        CycleAssists,
        SIZEOF_LegacyControlType
    };

    enum class KeyboardControlType : uint32_t {
        HR = 0,
        H1 = 1,
        H2 = 2,
        H3 = 3,
        H4 = 4,
        H5 = 5,
        H6 = 6,
        H7 = 7,
        H8 = 8,
        H9 = 9,
        H10 = 10,
        HN,
        ShiftUp,
        ShiftDown,
        Clutch,
        Throttle,
        Brake,
        Engine,
        Toggle,
        ToggleH,
        CycleAssists,
        SIZEOF_KeyboardControlType
    };

    enum class WheelControlType : uint32_t {
        HR = 0,
        H1 = 1,
        H2 = 2,
        H3 = 3,
        H4 = 4,
        H5 = 5,
        H6 = 6,
        H7 = 7,
        H8 = 8,
        H9 = 9,
        H10 = 10,
        HN,
        ShiftUp,
        ShiftDown,
        Clutch,
        Throttle,
        Brake,
        Engine,
        Toggle,
        ToggleH,
        Handbrake,
        Horn,
        Lights,
        LookBack,
        LookLeft,
        LookRight,
        Camera,
        RadioPrev,
        RadioNext,
        IndicatorLeft,
        IndicatorRight,
        IndicatorHazard,
        APark,
        AReverse,
        ANeutral,
        ADrive,
        CycleAssists,
        ToggleABS,
        ToggleESC,
        ToggleTCS,
        UNKNOWN,
        SIZEOF_WheelControlType
    };

    enum class WheelAxisType {
        Throttle,
        Brake,
        Clutch,
        Steer,
        Handbrake,
        ForceFeedback,
        SIZEOF_WheelAxisType
    };

    enum InputDevices {
        Keyboard = 0,
        Controller = 1,
        Wheel = 2
    };

    template <typename TControl>
    struct SInput {
        SInput()
            : Guid({}), Control() {}

        SInput(std::string configTag, GUID guid, TControl control, std::string name, std::string description)
            : ConfigTag(std::move(configTag))
            , Guid(guid)
            , Control(control)
            , Name(std::move(name))
            , Description(std::move(description)) {}

        std::string ConfigTag;
        GUID Guid;
        TControl Control;
        std::string Name;
        std::string Description;
    };

    CarControls();
    ~CarControls();

    void InitWheel();
    void InitFFB();
    void updateKeyboard();
    void updateController();
    float getInputValue(WheelAxisType axisType, WheelControlType buttonType, float minRaw, float maxRaw);
    float filterDeadzone(float input, float deadzone, float deadzoneOffset);
    void updateWheel();
    void UpdateValues(InputDevices prevInput, bool skipKeyboardInput);
    InputDevices GetLastInputDevice(InputDevices previousInput, bool enableWheel = true);

    // Keyboard controls	
    bool ButtonJustPressed(KeyboardControlType control);
    bool IsKeyPressed(int key);
    bool IsKeyJustPressed(int key, KeyboardControlType control);

    // Controller controls
    bool ButtonJustPressed(ControllerControlType control);
    bool ButtonReleased(ControllerControlType control);
    bool ButtonReleasedAfter(ControllerControlType control, int time);
    bool ButtonHeld(ControllerControlType control);
    bool ButtonHeldOver(ControllerControlType control, int millis);
    XInputController::TapState ButtonTapped(ControllerControlType control);
    bool ButtonIn(ControllerControlType control);

    bool ButtonJustPressed(LegacyControlType control);
    bool ButtonReleased(LegacyControlType control);
    bool ButtonHeld(LegacyControlType control);
    bool ButtonHeldOver(LegacyControlType control, int millis);
    bool ButtonIn(LegacyControlType control);
    NativeController::TapState ButtonTapped(LegacyControlType control);
    bool ButtonReleasedAfter(LegacyControlType control, int time);

    // Wheel controls
    bool ButtonJustPressed(WheelControlType control);
    bool ButtonReleased(WheelControlType control);
    bool ButtonHeld(WheelControlType control, int delay);
    bool ButtonIn(WheelControlType control);
    void CheckCustomButtons();

    void CheckGUIDs(const std::vector<_GUID> &guids);
    void PlayFFBDynamics(int totalForce, int damperForce);
    void PlayFFBCollision(int collisionForce);
    void PlayLEDs(float rpm, float firstLed, float lastLed);
    float GetAxisSpeed(WheelAxisType axis);
    bool WheelAvailable();
    WheelDirectInput& GetWheel();

    InputDevices PrevInput = Controller;

    float ThrottleVal = 0.0f;
    float BrakeVal = 0.0f;
    float ClutchVal = 0.0f; 	// 1 = Pressed, 0 = Not pressed
    float SteerVal = 0.5f;
    float SteerValRaw = 0.0f;   // For readout purposes. SteerVal is used for gameplay.
    float HandbrakeVal = 0.0f;

    std::array<SInput<std::string>, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXbox = {};
    std::array<int, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXboxBlocks = {};

    std::array<SInput<eControl>, static_cast<int>(LegacyControlType::SIZEOF_LegacyControlType)> LegacyControls = {};
    std::array<int, static_cast<int>(LegacyControlType::SIZEOF_LegacyControlType)> ControlNativeBlocks = {};

    std::array<SInput<int>, static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)> KBControl = {};

    std::array<SInput<std::string>, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxes = {};

    std::array<SInput<int>, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelButton = {};

    std::array<std::vector<int>, MAX_RGBBUTTONS> WheelToKey = {};
    std::unordered_map<int, std::vector<int>> WheelToKeyPov = {};

    GUID WheelToKeyGUID = {};

    std::vector<Device> FreeDevices{};

    XInputController& GetController() {
        return mXInputController;
    }

private:
    WheelDirectInput mWheelInput;
    NativeController mNativeController;
    XInputController mXInputController;

    bool KBControlCurr[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
    bool KBControlPrev[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};

    void updateKeyInputEvents(int button, const std::vector<int>& keys);
};
