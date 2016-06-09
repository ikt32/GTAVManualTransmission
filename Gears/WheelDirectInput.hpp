#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "..\..\ScriptHookV_SDK\inc\types.h"
#include "ScriptSettings.hpp"

enum class InputType {
	Wheel,
	Throttle,
	Brake,
	Clutch,
	Buttons,
	POVHat
};

class WheelDirectInput {
public:
	WheelDirectInput();
	bool Init(ScriptSettings &settings);
	bool Update();

	void Steer(float steerVal);
	void PlayEffects(
		Vector3 &accelVals,
		ScriptSettings &settings);
	float GetValue(InputType type);
private:


};