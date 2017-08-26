#pragma once
#include <Windows.h>

#include <vector>
#include "simpleini/SimpleIni.h"
#include "ShiftModes.h"

class Logger;
class ScriptControls;

static const int numGears = 8;

class ScriptSettings {
public:
	ScriptSettings();
	void SetFiles(const std::string &general, const std::string &wheel, const std::string &stick);
	void Read(ScriptControls* scriptControl);
	void SaveGeneral() const;
	void SaveController(ScriptControls *scriptControl) const;
	void SaveWheel(ScriptControls *scriptControl) const;
	void SaveStick(ScriptControls *scriptControl) const;
	//bool IsCorrectVersion() const;
	//std::string GetVersionError();

	// Only use this AFTER wheel settings are read.
	std::vector<GUID> GetGuids();

	// settings_general.ini parts
	// [OPTIONS]
	bool EnableManual = true;
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

	float EngBrakePower;
	float EngBrakeThreshold;
	float ClutchCatchpoint = 0.15f;
	float StallingThreshold = 0.75f;
	float RPMDamage = 1.5f;
	int MisshiftDamage = 10;

	bool HillBrakeWorkaround = false;
	bool AutoGear1 = false;
	bool AutoLookBack = false;
	bool ThrottleStart = false;
	bool CrossScript = false;

	// [HUD]
	bool HUD = true;
	bool AlwaysHUD = false;
	int HUDFont = 0;

	bool GearIndicator;
	float GearXpos = 0.95f;
	float GearYpos = 0.95f;
	float GearSize = 1.0f;
	int GearTopColorR = 255;
	int GearTopColorG = 255;
	int GearTopColorB = 255;

	bool ShiftModeIndicator;
	float ShiftModeXpos = 0.925f;
	float ShiftModeYpos = 0.95f;
	float ShiftModeSize = 1.0f;

	// can be kph, mph, or ms
	std::string Speedo;
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

	// [DEBUG]
	bool DisplayInfo = false;
	bool DisplayGearingInfo;
	bool DisplayWheelInfo = false;

	std::vector<GUID> reggdGuids;

	// settings_wheel.ini parts
	// [OPTIONS]
	bool EnableWheel = false;
	bool WheelWithoutManual = true;
	bool PatchSteering = false;
	bool PatchSteeringAlways = false;
	bool PatchSteeringControl = false;
	bool LogiLEDs = false;
	bool HPatternKeyboard = false;
	bool BlockCarControls = false;

	// [FORCE_FEEDBACK]
	bool EnableFFB = true;
	float FFGlobalMult = 1.0f;
	int DamperMax = 50;
	int DamperMin = 10;
	float TargetSpeed = 1.2f; // TargetSpeed in m/s
	float PhysicsStrength = 1.0f;
	float DetailStrength = 100.0f;

	// [STEER]
	float SteerAngleMax = 900.0f;
	float SteerAngleCar = 720.0f;
	float SteerAngleBike = 180.0f;
	float SteerAngleBoat = 360.0f;
	float GameSteerMult = 1.0f;



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
	void SteeringSaveHShifter(const std::string &confTag, ptrdiff_t index, int button[numGears]);
	void KeyboardSaveKey(const std::string &confTag, const std::string &key);
	void ControllerSaveButton(const std::string &confTag, const std::string &button);
	void LControllerSaveButton(const std::string & confTag, int button);
	void SteeringAddWheelToKey(const std::string & cs, ptrdiff_t index, int button, const std::string & key_name);
	bool SteeringClearWheelToKey(int button);

	void StickSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis, int minVal, int maxVal);

private:
	void parseSettingsGeneral(ScriptControls *scriptControl);
	void parseSettingsWheel(ScriptControls *scriptControl);
	void parseSettingsStick(ScriptControls *scriptControl);

	//std::string settings_general_version = "000";
	//std::string settings_wheel_version = "000";

	// Just looks up which GUID corresponds with what number and returns the GUID.
	GUID DeviceIndexToGUID(int device, std::vector<GUID> guids);

	int nDevices = 0;
	std::string settingsGeneralFile;
	std::string settingsWheelFile;
	std::string settingsStickFile;
	std::string settingsMenuFile;
};
