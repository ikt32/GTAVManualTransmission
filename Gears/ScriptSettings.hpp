#pragma once

#include "ScriptControls.hpp"

#define SETTINGSFILE "./Gears.ini"

class ScriptControls;

class ScriptSettings {
public:
	ScriptSettings();

	void Read(ScriptControls* scriptControl);
	void Save() const;

	bool EnableManual;
	//bool RealReverse;
	bool SimpleBike;

	bool EngDamage;
	bool EngStall;
	bool EngBrake;
	bool ClutchCatching;
	bool ClutchShifting;
	bool DefaultNeutral;
	bool UITips;
	// 0 Seq, 1 H, 2 Auto
	int ShiftMode;
	bool WheelEnabled;
	//int WheelRange;
	bool FFEnable;
	int DamperMax;
	int DamperMin;
	float FFPhysics;
	float CenterStrength;

	bool CrossScript;
	bool Debug;

	float UITips_X;
	float UITips_Y;
	float UITips_Size;

	float ClutchCatchpoint;
	float StallingThreshold;
	float RPMDamage;
	int MisshiftDamage;
	bool WheelWithoutManual;

	// TargetSpeed in m/s
	int TargetSpeed;
	bool HillBrakeWorkaround;
	float DetailStrength;
private:
	void CheckSettings();
};
