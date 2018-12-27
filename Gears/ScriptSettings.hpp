#pragma once
#include <Windows.h>

#include <vector>
#include "simpleini/SimpleIni.h"
#include "ShiftModes.h"
#include "Util/Logger.hpp"
#include "Memory/VehicleExtensions.hpp"

class Logger;
class CarControls;

class ScriptSettings {
public:
    ScriptSettings();
    void SetFiles(const std::string &general, const std::string &wheel);
    void Read(CarControls* scriptControl);
    void SaveGeneral() const;
    void SaveController(CarControls *scriptControl) const;
    void SaveWheel(CarControls *scriptControl) const;

    // Only use this AFTER wheel settings are read.
    std::vector<GUID> GetGuids();

    // settings_general.ini parts
    // [OPTIONS]
    bool EnableManual = false;
    ShiftModes ShiftMode = Sequential; 	// 0 Seq, 1 H, 2 Auto
    bool SimpleBike = false;
    bool EngDamage = false;
    bool EngStall = false;
    bool EngStallS = false;
    bool EngBrake = false;
    bool EngLock = false;
    bool ClutchCatching = false;
    bool ClutchShiftingH = false;
    bool ClutchShiftingS = false;
    bool DefaultNeutral = false;

    float EngBrakePower = 0.0f;
    float EngBrakeThreshold = 0.75f;
    float ClutchThreshold = 0.15f;
    float StallingThreshold = 0.75f;
    float RPMDamage = 1.5f;
    int MisshiftDamage = 10;
    bool HardLimiter = false;

    bool UseShifterForAuto = false;

    // misc stuff
    bool HillBrakeWorkaround = false;
    bool AutoGear1 = false;
    bool AutoLookBack = false;
    bool ThrottleStart = false;
    bool HidePlayerInFPV = false;

    bool CrossScript = false;

    // [HUD]
    bool HUD = true;
    bool AlwaysHUD = false;
    int HUDFont = 0;

    bool GearIndicator = true;
    float GearXpos = 0.95f;
    float GearYpos = 0.95f;
    float GearSize = 1.0f;
    int GearTopColorR = 255;
    int GearTopColorG = 255;
    int GearTopColorB = 255;

    bool ShiftModeIndicator = true;
    float ShiftModeXpos = 0.925f;
    float ShiftModeYpos = 0.95f;
    float ShiftModeSize = 1.0f;

    // can be kph, mph, or ms
    std::string Speedo = "kph";
    bool SpeedoShowUnit = false;
    float SpeedoSize = 1.5f;
    float SpeedoYpos = 0.85f;
    float SpeedoXpos = 0.85f;

    bool RPMIndicator = true;
    float RPMIndicatorXpos = 0.9125f;
    float RPMIndicatorYpos = 0.965f;
    float RPMIndicatorWidth = 0.25f;
    float RPMIndicatorHeight = 0.05f;
    float RPMIndicatorRedline = 0.875f;

    int RPMIndicatorBackgroundR = 0;
    int RPMIndicatorBackgroundG = 0;
    int RPMIndicatorBackgroundB = 64;
    int RPMIndicatorBackgroundA = 128;
    int RPMIndicatorForegroundR = 255;
    int RPMIndicatorForegroundG = 255;
    int RPMIndicatorForegroundB = 255;
    int RPMIndicatorForegroundA = 255;
    int RPMIndicatorRedlineR = 255;
    int RPMIndicatorRedlineG = 128;
    int RPMIndicatorRedlineB = 128;
    int RPMIndicatorRedlineA = 255;
    int RPMIndicatorRevlimitR = 255;
    int RPMIndicatorRevlimitG = 0;
    int RPMIndicatorRevlimitB = 0;
    int RPMIndicatorRevlimitA = 255;

    bool AlwaysSteeringWheelInfo = false;
    bool SteeringWheelInfo = false;
    float SteeringWheelTextureX	 = 0.0f;
    float SteeringWheelTextureY	 = 0.0f;
    float SteeringWheelTextureSz = 0.0f;
    float PedalInfoX			 = 0.0f;
    float PedalInfoY			 = 0.0f;
    float PedalInfoH			 = 0.0f;
    float PedalInfoW			 = 0.0f;
    float PedalInfoPadX			 = 0.0f;
    float PedalInfoPadY			 = 0.0f;

    // [CONTROLLER]
    bool ToggleEngine = false;
    bool BlockCarControls = false;
    bool IgnoreShiftsUI = false;
    bool BlockHShift = true;

    // [DEBUG]
    bool DisplayInfo = false;
    bool DisplayGearingInfo = false;
    bool DisplayWheelInfo = false;
    bool DisplayFFBInfo = false;
    int LogLevel = INFO;
    bool ShowNPCInfo = false;

    std::vector<GUID> RegisteredGUIDs;

    // settings_wheel.ini parts
    // [OPTIONS]
    bool EnableWheel = false;
    bool WheelWithoutManual = true;
    bool PatchSteering = false;
    bool PatchSteeringAlways = false;
    bool PatchSteeringControl = false;
    bool LogiLEDs = false;
    bool HPatternKeyboard = false;
    float SteeringReductionWheel = 0.0f;
    float SteeringReductionOther = 0.0f;

    // [FORCE_FEEDBACK]
    bool EnableFFB = true;
    bool ScaleFFB = true;
    float SATAmpMult = 1.0f;
    int DamperMax = 50;
    int DamperMin = 10;
    float DamperMinSpeed = 1.2f; // TargetSpeed in m/s
    float DetailMult = 2.5f;
    float CollisionMult = 1.0f;

    // [STEER]
    float SteerAngleMax = 900.0f;
    float SteerAngleCar = 720.0f;
    float SteerAngleBike = 180.0f;
    float SteerAngleBoat = 360.0f;
    float GameSteerMultWheel = 1.0f;
    float GameSteerMultOther = 1.0f;

    // [THROTTLE]
    float ThrottleGamma = 1.0f;

    // [BRAKE]
    float BrakeGamma = 1.0f;

    // Methods
    /*
     *  Checks if GUID already exists and returns device index
     *  otherwise appends GUID and returns new device index
     */
    ptrdiff_t SteeringAppendDevice(const GUID & dev_guid, std::string dev_name);
    int GUIDToDeviceIndex(GUID guid);

    void SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis, int minVal, int maxVal);
    void SteeringSaveFFBAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis);
    void SteeringSaveButton(const std::string &confTag, ptrdiff_t index, int button);
    void SteeringSaveHShifter(const std::string &confTag, ptrdiff_t index, const std::vector<int>& button);
    void KeyboardSaveKey(const std::string &confTag, const std::string &key);
    void ControllerSaveButton(const std::string &confTag, const std::string &button);
    void LControllerSaveButton(const std::string & confTag, int button);
    void SteeringAddWheelToKey(const std::string & cs, ptrdiff_t index, int button, const std::string & key_name);
    bool SteeringClearWheelToKey(int button);

private:
    void parseSettingsGeneral(CarControls *scriptControl);
    void parseSettingsWheel(CarControls *scriptControl);

    // Just looks up which GUID corresponds with what number and returns the GUID.
    GUID DeviceIndexToGUID(int device, std::vector<GUID> guids);

    int nDevices = 0;
    std::string settingsGeneralFile;
    std::string settingsWheelFile;
    std::string settingsMenuFile;
};
