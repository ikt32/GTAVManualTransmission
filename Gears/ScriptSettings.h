#pragma once

#include <Windows.h>
#include "..\..\ScriptHookV_SDK\inc\enums.h"
#include "ScriptControls.h"

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

