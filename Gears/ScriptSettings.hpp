#pragma once

#include <Windows.h>
#include "ScriptControls.hpp"

class ScriptSettings
{
public:
	ScriptSettings();

	void Read(ScriptControls *scriptControl);
	void Save();

	bool EnableManual;
	bool AutoGear1;
	bool AutoReverse;
	bool OldReverse;
	bool SimpleBike;

	bool EngDamage;
	int EngStall;
	bool EngBrake;
	bool ClutchCatching;
	bool DefaultNeutral;
	bool UITips;
	bool Hshifter;
	bool Debug;
};

