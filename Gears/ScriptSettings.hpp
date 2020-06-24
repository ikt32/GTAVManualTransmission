#pragma once

#include "Input/CarControls.hpp"
#include "Util/Logger.hpp"

#include <simpleini/SimpleIni.h>
#include <vector>
#include <string>

// oh god why
#define APPLY_CONFIG_VALUE(option, value) \
    if (g_settings.MTOptions.Override && g_activeConfig != nullptr) { \
        g_activeConfig->option = (value);                             \
        g_settings.SetVehicleConfig(g_activeConfig);                  \
    }                                                                 \
    else {                                                            \
        g_settings.option = (value);                                  \
    }

class VehicleConfig;

enum class EShiftMode : int {
    Sequential = 0,
    HPattern = 1,
    Automatic = 2,
};

EShiftMode Next(EShiftMode mode);

class ScriptSettings {
public:
    ScriptSettings();
    void SetFiles(const std::string &general, const std::string& controls, const std::string &wheel);
    void Read(CarControls* scriptControl);
    void SaveGeneral() const;
    void SaveController(CarControls* scriptControl) const;
    void SaveWheel() const;

    void SetVehicleConfig(VehicleConfig* cfg);
    ScriptSettings operator()();

    struct TimerParams {
        std::string Unit;
        float LimA;
        float LimB;
        float Tolerance;
    };

    // settings_general.ini parts
    // [MT_OPTIONS]
    struct {
        bool Enable = true;
        // 0 Seq, 1 H, 2 Auto
        EShiftMode ShiftMode = EShiftMode::Sequential;
        bool Override = true;

        bool EngDamage = false;
        // H-Pattern
        bool EngStallH = true;
        bool EngStallS = false;
        bool EngBrake = false;
        bool EngLock = false;
        bool ClutchCreep = false;
        bool ClutchShiftH = true;
        bool ClutchShiftS = false;
        bool HardLimiter = true;
    } MTOptions;

    // [MT_PARAMS]
    struct {
        float EngBrakePower = 0.0f;
        float EngBrakeThreshold = 0.75f;
        // Clutch _lift_ distance
        float ClutchThreshold = 0.15f;
        float StallingRPM = 0.09f;
        float StallingRate = 3.75f;
        float StallingSlip = 0.40f;
        float RPMDamage = 0.1f;
        float MisshiftDamage = 5.0f;
        float CreepIdleThrottle = 0.1f;
        float CreepIdleRPM = 0.1f;
    } MTParams;

    // [GAMEPLAY_ASSISTS]
    struct {
        bool SimpleBike = false;
        bool HillGravity = false;
        bool AutoGear1 = false;
        bool AutoLookBack = false;
        bool ThrottleStart = false;
        bool DefaultNeutral = true;
        bool DisableAutostart = false;
        bool LeaveEngineRunning = false;
    } GameAssists;

    // [DRIVING_ASSISTS]
    struct {
        struct {
            bool Enable = false;
            // true: only applied to abs-less vehicles
            bool Filter = true;
        } ABS;

        struct {
            bool Enable = false;
            // 0 Brake, 1 Throttle
            int Mode = 0;
            // tyre speed threshold, m/s
            float SlipMax = 2.5f;
        } TCS;

        struct {
            bool Enable = false;

            float OverMin = 05.0f; // deg
            float OverMax = 15.0f; // deg

            float OverMinComp = 0.0f; // brake mult
            float OverMaxComp = 1.0f; // brake mult

            float UnderMin = 5.0f; // deg
            float UnderMax = 10.0f; // deg

            float UnderMinComp = 0.0f; // brake mult
            float UnderMaxComp = 1.0f; // brake mult

        } ESP;

        struct {
            bool Enable = false;
            float Viscosity = 10.0f;
        } LSD;
    } DriveAssists;

    // [SHIFT_OPTIONS]
    struct {
        bool UpshiftCut = true;
        bool DownshiftBlip = true;
        float ClutchRateMult = 1.0f;
        float RPMTolerance = 0.2f;
    } ShiftOptions;

    // [AUTO_PARAMS]
    struct {
        // Lower = upshift later
        float UpshiftLoad = 0.12f;
        // Higher = downshift later
        float DownshiftLoad = 0.60f;
        // Don't upshift until next gears' RPM is over this value.
        float NextGearMinRPM = 0.33f;
        // Downshift when RPM drops below this value.
        float CurrGearMinRPM = 0.27f;
        // Lower = keep in low gear longer // eco - 0.33
        float EcoRate = 0.05f;
        // Timeout mult for not downshifting after an upshift
        float DownshiftTimeoutMult = 1.0f;
        // Experimental new tcu
        bool UsingATCU = false;
    } AutoParams;

    // [CUSTOM_STEERING]
    struct {
        // 0 Default, 1 Enhanced
        int Mode = 0;
        float CountersteerMult = 1.0f;
        // In Degrees
        float CountersteerLimit = 15.0f;
        float SteeringMult = 1.0f;
        float SteeringReduction = 0.9f;
        float Gamma = 1.0f;
        bool CustomRotation = true;
        float CustomRotationDegrees = 720.0f;
        float CenterTime = 0.000100f;
        float SteerTime = 0.001000f;
        bool MouseSteering = true;
        float MouseSensitivity = 0.5f;
    } CustomSteering;

    // [HUD]
    struct {
        bool Enable = true;
        bool Always = true;
        // Fonts:
        // 0 - Chalet London
        // 1 - Sign Painter
        // 2 - Slab Serif
        // 4 - Chalet Cologne
        // 7 - Pricedown
        int Font = 4;

        bool Outline = true;

        // Levels:
        // 0 - Debug
        // 1 - Info
        // 2 - Error/UI (LogLevel WARN)
        // 3 - None
        int NotifyLevel = INFO;

        struct {
            bool Enable = true;
            float XPos = 0.9525f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int TopColorR = 255;
            int TopColorG = 63;
            int TopColorB = 63;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } Gear;

        struct {
            bool Enable = true;
            float XPos = 0.935f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } ShiftMode;

        struct {
            // can be kph, mph, or ms
            std::string Speedo = "kph";
            bool ShowUnit = true;
            float XPos = 0.860f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } Speedo;

        struct {
            bool Enable = true;
            float Redline = 0.850f;

            float XPos = 0.120f;
            float YPos = 0.765f;
            float XSz = 0.140f;
            float YSz = 0.005f;

            int BgR = 0;
            int BgG = 0;
            int BgB = 0;
            int BgA = 128;

            int FgR = 255;
            int FgG = 255;
            int FgB = 255;
            int FgA = 255;

            int RedlineR = 255;
            int RedlineG = 92;
            int RedlineB = 0;
            int RedlineA = 255;

            int RevLimitR = 255;
            int RevLimitG = 0;
            int RevLimitB = 0;
            int RevLimitA = 255;
        } RPMBar;

        struct {
            bool Enable = true;
            bool Always = false;
            float ImgXPos = 0.22f;
            float ImgYPos = 0.80f;
            float ImgSize = 0.05f;

            float PedalXPos = 0.22f;
            float PedalYPos = 0.90f;
            float PedalXSz = 0.04f;
            float PedalYSz = 0.10f;
            float PedalXPad = 0.0f;
            float PedalYPad = 0.0f;
            int PedalBgA = 92;

            int PedalThrottleR = 0;
            int PedalThrottleG = 255;
            int PedalThrottleB = 0;
            int PedalThrottleA = 255;

            int PedalBrakeR = 255;
            int PedalBrakeG = 0;
            int PedalBrakeB = 0;
            int PedalBrakeA = 255;

            int PedalClutchR = 0;
            int PedalClutchG = 0;
            int PedalClutchB = 255;
            int PedalClutchA = 255;
        } Wheel;

        struct {
            bool Enable = false;
            float XPos = 0.500f;
            float YPos = 0.035f;
            float Size = 1.000f;
        } DashIndicators;

        struct {
            bool Enable = false;
            float XPos = 0.5f;
            float YPos = 0.95f;
            float XSz = 0.5f;
            float YSz = 0.05f;
            float MarkerXSz = 0.020f;

            int BgR = 0;
            int BgG = 0;
            int BgB = 0;
            int BgA = 128;

            int FgR = 255;
            int FgG = 255;
            int FgB = 255;
            int FgA = 255;
        } MouseSteering;
    } HUD;

    // [CONTROLLER]
    struct {
        int HoldTimeMs = 250;
        int MaxTapTimeMs = 200;
        float TriggerValue = 0.75f;

        bool ToggleEngine = false;

        bool BlockCarControls = true;
        bool IgnoreShiftsUI = true;
        bool BlockHShift = true;

        //long ShiftUpBlocks = -1;
        //long ShiftDownBlocks = -1;
        //long ClutchBlocks = -1;

        struct {
            bool Enable = false;
            //long ShiftUpBlocks = -1;
            //long ShiftDownBlocks = -1;
            //long ClutchBlocks = -1;
        } Native;
    } Controller;

    // [MISC]
    struct {
        bool UDPTelemetry = true;
        bool DashExtensions = true;
        bool SyncAnimations = true;
        struct {
            bool Enable = true;
            int AttachId = 0; // 0: Head, 1: Vehicle, 2: FPV Offset?
            bool RemoveHead = true;
            bool FollowMovement = true;
            float MovementMultVel = 0.750f;
            float MovementMultRot = 0.15f;
            float MovementCap = 45.0f;
            float FOV = 55.0f;
            float OffsetHeight = 0.04f;
            float OffsetForward = 0.05f;
            float OffsetSide = 0.0f;
            float Pitch = 0.0f;
            float LookTime = 0.000010f;
            float MouseLookTime = 0.000001f;
            int MouseCenterTimeout = 750;
            float MouseSensitivity = 0.3f;
            struct {
                bool Disable = false;
                int AttachId = 0; // 0: Head, 1: Vehicle, 2: FPV Offset?
                float FOV = 55.0f;
                float OffsetHeight = -0.05f;
                float OffsetForward = -0.08f;
                float OffsetSide = 0.0f;
                float Pitch = -11.0f;
            } Bike;
        } Camera;
        bool HidePlayerInFPV = false;
    } Misc;

    // [UPDATE]
    struct {
        bool EnableUpdate = true;
        std::string IgnoredVersion = "v0.0.0";
    } Update;

    // [DEBUG]
    struct {
        int LogLevel = INFO;
        bool DisplayInfo = false;
        bool DisplayGearingInfo = false;
        bool DisplayWheelInfo = false;
        bool DisplayFFBInfo = false;
        bool DisplayNPCInfo = false;
        bool DisableInputDetect = false;
        bool DisablePlayerHide = false;

        struct {
            bool EnableTimers = false;
            std::vector<TimerParams> Timers{
                { "kph", 0.0f, 100.0f, 0.1f },
                { "mph", 0.0f,  60.0f, 0.1f }
            };
            struct {
                bool Enable = false;
                float PosX = 0.075f;
                float PosY = 0.125f;
                float Size = 0.200f;
            } GForce;
        } Metrics;

        bool DisableNPCBrake = false;
        bool DisableNPCGearbox = false;
    } Debug;

    // settings_wheel.ini parts
    struct {
        // [OPTIONS]
        struct {
            bool Enable = true;
            bool SyncRotation = true;
            bool LogiLEDs = false;
            bool HPatternKeyboard = false;
            bool UseShifterForAuto = false;
        } Options;

        // [INPUT_DEVICES]
        struct {
            std::vector<GUID> RegisteredGUIDs;
        } InputDevices;

        // [FORCE_FEEDBACK]
        struct {
            bool Enable = true;
            bool Scale = true;
            int AntiDeadForce = 1600;
            float SATAmpMult = 1.25f;
            int SATMax = 10000;
            float SATFactor = 0.75f;
            int DamperMax = 100;
            int DamperMin = 40;
            float DamperMinSpeed = 6.4f; // TargetSpeed in m/s
            float DetailMult = 4.0f;
            int DetailLim = 5000;
            int DetailMAW = 3;
            float CollisionMult = 2.5f;
            float Gamma = 0.8f;
            float MaxSpeed = 80.0f;
        } FFB;

        // [STEER]
        struct {
            float AngleMax = 900.0f;
            float AngleCar = 720.0f;
            float AngleBike = 180.0f;
            float AngleBoat = 360.0f;
            float SteerMult = 1.0f;
            float Gamma = 1.0f;
            float AntiDeadZone = 0.25f;
            float DeadZone = 0.0f;
            float DeadZoneOffset = 0.0f;
            int Min = -1;
            int Max = -1;
        } Steering;

        // [THROTTLE]
        struct {
            float Gamma = 1.0f;
            float AntiDeadZone = 0.25f;
            int Min = -1;
            int Max = -1;
        } Throttle;

        // [BRAKE]
        struct {
            float Gamma = 1.0f;
            float AntiDeadZone = 0.25f;
            int Min = -1;
            int Max = -1;
        } Brake;

        // [CLUTCH]
        struct {
            int Min = -1;
            int Max = -1;
        } Clutch;

        // [HANDBRAKE_ANALOG]
        struct {
            int Min = -1;
            int Max = -1;
        } HandbrakeA;
    } Wheel;

    // Methods
    /*
     *  Checks if GUID already exists and returns device index
     *  otherwise appends GUID and returns new device index
     */
    ptrdiff_t SteeringAppendDevice(const GUID & dev_guid, const std::string& dev_name);
    int GUIDToDeviceIndex(GUID guid);

    void SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis, int minVal, int maxVal);
    void SteeringSaveButton(const std::string &confTag, ptrdiff_t index, int button);
    void KeyboardSaveKey(const std::string &confTag, const std::string &key);
    void ControllerSaveButton(const std::string &confTag, const std::string &button);
    void LControllerSaveButton(const std::string & confTag, int button);
    void SteeringAddWheelToKey(const std::string & conftag, ptrdiff_t index, int button, const std::string & keyName);
    bool SteeringClearWheelToKey(int button);

private:
    void parseSettingsGeneral();
    void parseSettingsControls(CarControls* scriptControl);
    void parseSettingsWheel(CarControls *scriptControl);

    // Just looks up which GUID corresponds with what number and returns the GUID.
    GUID DeviceIndexToGUID(int device, std::vector<GUID> guids);

    template<typename T>
    CarControls::SInput<T> parseWheelItem(CSimpleIniA& ini, const char* section, T default, const char* name = nullptr);

    CarControls::SInput<int> parseKeyboardItem(CSimpleIniA& ini, const char* key, const char* default, const char* name = nullptr);

    template<typename T>
    CarControls::SInput<T> parseControllerItem(CSimpleIniA& ini, const char* key, T default, const char* name, const char* description);

    int nDevices = 0;
    std::string settingsGeneralFile;
    std::string settingsControlsFile;
    std::string settingsWheelFile;
    std::string settingsMenuFile;
};
