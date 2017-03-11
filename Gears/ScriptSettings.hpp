#pragma once
#include <Windows.h>
#define CORRECTVGENERAL 420
#define CORRECTVWHEEL 420
#include <string>
#include <vector>
#include "Util/Logger.hpp"
#include <algorithm>

class Logger;
class ScriptControls;

class ScriptSettings {
public:
	ScriptSettings(std::string general, std::string wheel, Logger &logger);
	void Read(ScriptControls* scriptControl);
	void Save() const;
	void IsCorrectVersion() const;

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

	bool UITips = true;
	bool UITips_OnlyNeutral = false;
	float UITips_X = 0.95f;
	float UITips_Y = 0.95f;
	float UITips_Size = 1.0f;
	int UITips_TopGearC_R = 255;
	int UITips_TopGearC_G = 255;
	int UITips_TopGearC_B = 255;
	
	bool CrossScript = false;

	// [CONTROLLER]
	bool ToggleEngine = false; // false makes it just turn ON the engine

	// [DEBUG]
	bool Debug = false;

	std::vector<GUID> reggdGuids;

	// settings_wheel.ini parts
	// [OPTIONS]
	bool WheelEnabled = false;
	bool WheelWithoutManual = true;
	bool AltControls = false;
	bool PatchSteering = false;
	bool PatchSteeringAlways = false;
	bool LogiLEDs = false;

	// [FORCE_FEEDBACK]
	bool FFEnable = true;
	float FFGlobalMult = 1.0f;
	int DamperMax = 50;
	int DamperMin = 10;
	int TargetSpeed = 10; // TargetSpeed in m/s
	float FFPhysics = 1.0f;
	float DetailStrength = 100.0f;

	// [STEER]
	float SteerAngleMax = 900.0f;
	float SteerAngleCar = 720.0f;
	float SteerAngleBike = 180.0f;
	float SteerAngleAlt = 180.0f;

	// Methods
	/*
	 *  Checks if GUID already exists and returns device index
	 *  otherwise appends GUID and returns new device index
	 */
	int SteeringAppendDevice(const GUID & dev_guid, std::string dev_name);
	void SteeringSaveAxis(std::string confTag, int index, std::string axis, int minVal, int maxVal);
	void SteeringSaveFFBAxis(std::string confTag, int index, std::string axis);
	void SteeringSaveButton(std::string confTag, int index, int button);
	void SteeringSaveHShifter(std::string confTag, int index, int button[8]);
private:
	int settings_general_version = 0;
	int settings_wheel_version = 0;
	Logger logger;
	void parseSettingsWheel(ScriptControls *scriptControl);

	// Just looks up which GUID corresponds with what number and returns the GUID.
	GUID DeviceIndexToGUID(int device, std::vector<GUID> guids);

	int nDevices = 0;
	std::string settingsGeneralFile;
	std::string settingsWheelFile;
};
