#pragma once

#include "Input/ScriptControls.hpp"

#define SETTINGSFILE "./Gears.ini"

class ScriptControls;

class ScriptSettings {
public:
	ScriptSettings();

	void Read(ScriptControls* scriptControl);
	void Save() const;

	bool EnableManual = true;
	int ShiftMode = 0; 	// 0 Seq, 1 H, 2 Auto
	bool SimpleBike = false;
	bool CrossScript = false;
	bool HillBrakeWorkaround = false;

	bool EngDamage = false;
	bool EngStall = false;
	bool EngBrake = false;
	float RPMDamage = 1.5f;
	int MisshiftDamage = 10;

	bool ClutchCatching = false;
	bool ClutchShifting = false;
	float ClutchCatchpoint = 0.15f;
	float StallingThreshold = 0.75f;
	bool DefaultNeutral = false;

	
	bool UITips = true;
	bool UITips_OnlyNeutral = false;
	float UITips_X = 0.95f;
	float UITips_Y = 0.95f;
	float UITips_Size = 1.0f;
	int UITips_TopGearC_R = 255;
	int UITips_TopGearC_G = 255;
	int UITips_TopGearC_B = 255;

	bool Debug = false;

	bool WheelEnabled = false;
	bool WheelWithoutManual = true;

	//int WheelRange;
	bool FFEnable = true;
	int DamperMax = 50;
	int DamperMin = 10;
	// TargetSpeed in m/s
	int TargetSpeed = 10;
	float FFPhysics = 1.0f;
	float CenterStrength = 1.0f;
	float DetailStrength = 100.0f;
	bool AutoLookBack = false;
	float SteerAngleMax = 900.0f;
	float SteerAngleCar = 720.0f;
	float SteerAngleBike = 180.0f;
	bool AltControls = false;
	float SteerAngleAlt = 180.0f;
	bool AutoGear1 = false;
private:
	void CheckSettings();
};
