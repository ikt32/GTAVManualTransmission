#pragma once

#include "Input/ScriptControls.hpp"

#define SETTINGSFILE "./Gears.ini"
#define CORRECTVERSION "4.2.0"

class ScriptControls;

class ScriptSettings {
public:
	ScriptSettings();

	void Read(ScriptControls* scriptControl);
	void Save() const;
	bool IsCorrectVersion() const;

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

	bool UITips = true;
	bool UITips_OnlyNeutral = false;
	float UITips_X = 0.95f;
	float UITips_Y = 0.95f;
	float UITips_Size = 1.0f;
	int UITips_TopGearC_R = 255;
	int UITips_TopGearC_G = 255;
	int UITips_TopGearC_B = 255;
	
	bool CrossScript = false;

	// [WHEELOPTIONS]
	bool WheelEnabled = false;
	bool WheelWithoutManual = true;
	
	bool FFEnable = true;
	float FFGlobalMult = 1.0f;
	int DamperMax = 50;
	int DamperMin = 10;
	int TargetSpeed = 10; // TargetSpeed in m/s
	float FFPhysics = 1.0f;
	float DetailStrength = 100.0f;

	// [WHEELAXIS]
	float SteerAngleMax = 900.0f;
	float SteerAngleCar = 720.0f;
	float SteerAngleBike = 180.0f;

	// [DEBUG]
	bool Debug = false;
	bool AltControls = false;
	float SteerAngleAlt = 180.0f;

private:
	std::string INIver = "0.0.0";
};
