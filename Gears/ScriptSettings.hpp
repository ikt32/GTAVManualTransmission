#pragma once

#include "ScriptControls.hpp"

#define SETTINGSFILE "./Gears.ini"

class ScriptSettings {
public:
	ScriptSettings();

	void Read(ScriptControls* scriptControl);
	void Save() const;

	bool EnableManual;
	bool RealReverse;
	bool SimpleBike;

	bool EngDamage;
	bool EngStall;
	bool EngBrake;
	bool ClutchCatching;
	bool ClutchShifting;
	bool DefaultNeutral;
	bool UITips;
	bool Hshifter;
	bool LogiWheel;
	int WheelRange;
	bool FFEnable;
	int FFDamperStationary;
	int FFDamperMoving;
	float FFPhysics;
	float FFCenterSpring;
	bool DisableDpad;

	bool Debug;

	float UITips_X;
	float UITips_Y;
	float UITips_Size;

	float ClutchCatchpoint;
	float StallingThreshold;
	float RPMDamage;
	int MisshiftDamage;
private:
	void CheckSettings();
};
