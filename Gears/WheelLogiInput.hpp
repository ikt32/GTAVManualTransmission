#pragma once
#include <Windows.h>
#include <Xinput.h>

#include "..\..\ScriptHookV_SDK\inc\types.h"


#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Logger.hpp"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include "..\..\LogitechSteeringWheel_SDK\Include\LogitechSteeringWheelLib.h"

class WheelLogiInput {
public:
	WheelLogiInput(int index);
	~WheelLogiInput();
	void DoWheelSteering(float steerVal) const;
	void PlayWheelEffects(
		ScriptSettings* settings,
		VehicleData* vehData,
		Vehicle vehicle) const;
	void UpdateLogiValues();
	bool InitWheel(ScriptSettings* settings, Logger* logger);
	void PlayWheelVisuals(float rpm) const;
	//bool IsActive = false;
	int GetIndex() const;
	float GetLogiWheelVal() const;
	float GetLogiThrottleVal() const;
	float GetLogiBrakeVal() const;
	float GetLogiClutchVal() const;
	bool IsActive(ScriptSettings* settings) const;

private:
	int index_ = 0;
	long logiSteeringWheelPos;
	long logiThrottlePos;
	long logiBrakePos;
	long logiClutchPos;
	float logiWheelVal;
	float logiThrottleVal;
	float logiBrakeVal;
	float logiClutchVal;
	LogiControllerPropertiesData properties;
};
