#pragma once
#include <Windows.h>

#include <vector>
#include "simpleini/SimpleIni.h"

class Logger;
class ScriptControls;

static const int numGears = 8;

class ScriptSettings {
public:
	ScriptSettings(const std::string &general, const std::string &wheel);
	void SetFiles(const std::string &general, const std::string &wheel);
	void Read(ScriptControls* scriptControl);
	void SaveGeneral() const;
	void SaveController(ScriptControls *scriptControl) const;
	void SaveWheel(ScriptControls *scriptControl) const;
	bool IsCorrectVersion() const;
	std::string GetVersionError();

	// Only use this AFTER wheel settings are read.
	std::vector<GUID> GetGuids();

	// settings_general.ini parts
	// [OPTIONS]
	bool EnableManual = true;
	int ShiftMode = 0; 	// 0 Seq, 1 H, 2 Auto
	bool SimpleBike = false;
	bool EngDamage = false;
	bool EngStall = false;
	bool EngBrake = false;
	bool ClutchCatching = false;
	bool ClutchShiftingH = false;
	bool ClutchShiftingS = false;
	bool DefaultNeutral = false;

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
	float GearXpos = 0.95f;
	float GearYpos = 0.95f;
	float GearSize = 1.0f;
	int GearTopColorR = 255;
	int GearTopColorG = 255;
	int GearTopColorB = 255;

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

	// [CONTROLLER]
	bool ToggleEngine = false; // false makes it just turn ON the engine

	// [DEBUG]
	bool DisplayInfo = false;
	bool LogCar = false;

	std::vector<GUID> reggdGuids;

	// settings_wheel.ini parts
	// [OPTIONS]
	bool EnableWheel = false;
	bool WheelWithoutManual = true;
	bool AltControls = false;
	bool PatchSteering = false;
	bool PatchSteeringAlways = false;
	bool LogiLEDs = false;
	bool HPatternKeyboard = false;

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
	float SteerAngleAlt = 180.0f;
	float GameSteerMult = 1.0f;
	bool BlockCarControls = false;
	bool DisplayWheelInfo = false;

	// Methods
	/*
	 *  Checks if GUID already exists and returns device index
	 *  otherwise appends GUID and returns new device index
	 */
	ptrdiff_t SteeringAppendDevice(const GUID & dev_guid, std::string dev_name);
	void SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis, int minVal, int maxVal);
	void SteeringSaveFFBAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis);
	void SteeringSaveButton(const std::string &confTag, ptrdiff_t index, int button);
	void SteeringSaveHShifter(const std::string &confTag, ptrdiff_t index, int button[numGears]);
	void KeyboardSaveKey(const std::string &confTag, const std::string &key);
	void ControllerSaveButton(const std::string &confTag, const std::string &button, int btnToBlock);

private:
	void parseSettingsGeneral(ScriptControls *scriptControl);
	void parseSettingsWheel(ScriptControls *scriptControl);

	std::string settings_general_version = "000";
	std::string settings_wheel_version = "000";

	// Just looks up which GUID corresponds with what number and returns the GUID.
	GUID DeviceIndexToGUID(int device, std::vector<GUID> guids);

	int nDevices = 0;
	std::string settingsGeneralFile;
	std::string settingsWheelFile;
	std::string settingsMenuFile;
};
