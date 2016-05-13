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
	//bool AutoGear1;
	//bool AutoReverse;
	bool RealReverse;
	bool SimpleBike;

	bool EngDamage;
	bool EngStall;
	bool EngBrake;
	bool ClutchCatching;
	bool DefaultNeutral;
	bool UITips;
	bool Hshifter;
	bool LogiWheel;

	bool Debug;

	float UITips_X;
	float UITips_Y;
	float UITips_Size;
//private:
	void Check();
};

