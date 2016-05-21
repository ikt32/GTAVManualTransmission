#pragma once

#include <Windows.h>
#include "ScriptControls.hpp"

#define SETTINGSFILE "./Gears.ini"

class ScriptSettings
{
public:
	ScriptSettings();

	void Read(ScriptControls *scriptControl);
	void Save();

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
	int FFDamperStationary;
	int FFDamperMoving;
	int FFPhysics;
	float FFCenterSpring;

	bool Debug;
	bool DisableFullClutch;

	float UITips_X;
	float UITips_Y;
	float UITips_Size;

	float ClutchCatchpoint;
	float StallingThreshold;
	float RPMDamage;
	int MisshiftDamage;
//private:
	void CheckSettings();
};

