#include "WheelLogiInput.hpp"
#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"

WheelLogiInput::WheelLogiInput(int index) {
	index_ = index;
	logiSteeringWheelPos = 0;
	logiThrottlePos = 0;
	logiBrakePos = 0;
	logiClutchPos = 0;
	logiWheelVal = 0.0f;
	logiThrottleVal = 0.0f;
	logiBrakeVal = 0.0f;
	logiClutchVal = 0.0f;
	LogiGetCurrentControllerProperties(index_, properties);
}


WheelLogiInput::~WheelLogiInput() {}

///////////////////////////////////////////////////////////////////////////////
//                             Wheel functions
///////////////////////////////////////////////////////////////////////////////
void WheelLogiInput::PlayWheelVisuals(float rpm) const {
	LogiPlayLeds(index_, rpm, 0.5f, 0.95f);
}

int WheelLogiInput::GetIndex() const {
	return index_;
}

void WheelLogiInput::PlayWheelEffects(
	ScriptSettings* settings,
	VehicleData* vehData,
	Vehicle vehicle) const {
	int damperforce;
	if (settings->FFDamperStationary < settings->FFDamperMoving) {
		settings->FFDamperMoving = settings->FFDamperStationary;
	}
	int ratio = (settings->FFDamperStationary - settings->FFDamperMoving) / 10;

	damperforce = settings->FFDamperStationary - ratio * static_cast<int>(vehData->Speed);
	if (vehData->Speed > 10.0f) {
		damperforce = settings->FFDamperMoving + static_cast<int>(0.5f * (vehData->Speed - 10.0f));
	}

	if (damperforce > (settings->FFDamperStationary + settings->FFDamperMoving) / 2) {
		damperforce = (settings->FFDamperStationary + settings->FFDamperMoving) / 2;
	}
	LogiPlayDamperForce(index_, damperforce);

	Vector3 accelVals = vehData->getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true));
	LogiPlayConstantForce(index_, static_cast<int>(-settings->FFPhysics * accelVals.x));
	LogiPlaySpringForce(index_, 0, static_cast<int>(vehData->Speed), static_cast<int>(vehData->Speed));

	if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f) {
		LogiPlayCarAirborne(index_);
	}
	else if (LogiIsPlaying(index_, LOGI_FORCE_CAR_AIRBORNE)) {
		LogiStopCarAirborne(index_);
	}
}

// Updates logiWheelVal, logiThrottleVal, logiBrakeVal, logiClutchVal
void WheelLogiInput::UpdateLogiValues() {
	logiSteeringWheelPos = LogiGetState(index_)->lX;
	logiThrottlePos = LogiGetState(index_)->lY;
	logiBrakePos = LogiGetState(index_)->lRz;
	logiClutchPos = LogiGetState(index_)->rglSlider[1];
	//LogiGenerateNonLinearValues(index_, 33);
	//logiSteeringWheelPos = LogiGetNonLinearValue(index_, LogiGetState(index_)->lX);
	//  32767 @ nope | 0
	// -32768 @ full | 1
	logiWheelVal = (static_cast<float>(logiSteeringWheelPos - 32767) / -65535.0f) - 0.5f;
	logiThrottleVal = static_cast<float>(logiThrottlePos - 32767) / -65535.0f;
	logiBrakeVal = static_cast<float>(logiBrakePos - 32767) / -65535.0f;
	logiClutchVal = 1.0f + static_cast<float>(logiClutchPos - 32767) / 65535.0f;
}

void WheelLogiInput::DoWheelSteering(float steerVal) const {
	// Anti-deadzone
	//float steerVal_ = logiSteeringWheelPos / (32768.0f);

	int additionalOffset = 2560;
	float antiDeadzoned;
	antiDeadzoned = logiSteeringWheelPos / 32768.0f;
	if (//logiSteeringWheelPos > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
		logiSteeringWheelPos <= 0) {
		antiDeadzoned = (logiSteeringWheelPos - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE - additionalOffset) / (32768.0f + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset);
	}
	if (//logiSteeringWheelPos > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
		logiSteeringWheelPos > 0) {
		antiDeadzoned = (logiSteeringWheelPos + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset) / (32768.0f + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + additionalOffset);
	}
	// Gotta find a way to make this no-delay
	CONTROLS::_SET_CONTROL_NORMAL(27, ControlVehicleMoveLeftRight, antiDeadzoned);
	//VEHICLE::SET_VEHICLE_STEER_BIAS(vehicle, -steerVal_);
}

bool WheelLogiInput::InitWheel(ScriptSettings* settings, Logger* logger) {
	if (settings->LogiWheel) {
		LogiSteeringInitialize(true);
		if (LogiIsConnected(index_)) {
			logger->Write("Wheel detected");
			LogiGetCurrentControllerProperties(index_, properties);
			properties.wheelRange = settings->WheelRange;
			LogiSetPreferredControllerProperties(properties);
			return true;
		}
		else {
			logger->Write("No wheel detected");
			return false;
		}
	}
	else {
		logger->Write("Wheel disabled");
		return false;
	}
}

float WheelLogiInput::GetLogiWheelVal() const {
	return logiWheelVal;
}

float WheelLogiInput::GetLogiThrottleVal() const {
	return logiThrottleVal;
}

float WheelLogiInput::GetLogiBrakeVal() const {
	return logiBrakeVal;
}

float WheelLogiInput::GetLogiClutchVal() const {
	return logiClutchVal;
}

bool WheelLogiInput::IsActive(ScriptSettings* settings) const {
	return (settings->LogiWheel && LogiIsConnected(index_));
}
