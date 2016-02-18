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
	bool EngDamage;
	bool EngStall;
	bool EngBrake;
	bool Hshifter;
	bool Debug;
};

