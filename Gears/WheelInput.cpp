#include "WheelInput.hpp"

WheelInput::WheelInput(int index) {
	index_ = index;
}


WheelInput::~WheelInput()
{
}

///////////////////////////////////////////////////////////////////////////////
//                             Wheel functions
///////////////////////////////////////////////////////////////////////////////
void WheelInput::PlayWheelVisuals(float rpm) {
	LogiPlayLeds(index_, rpm, 0.5f, 0.95f);
}

int WheelInput::GetIndex()
{
	return index_;
}

void WheelInput::PlayWheelEffects(
	ScriptSettings settings,
	VehicleData vehData,
	Vehicle vehicle) {
	int damperforce = 0;
	if (settings.FFDamperStationary < settings.FFDamperMoving) {
		settings.FFDamperMoving = settings.FFDamperStationary;
	}
	int ratio = (settings.FFDamperStationary - settings.FFDamperMoving) / 10;

	damperforce = settings.FFDamperStationary - ratio * (int)(vehData.Speed);
	if (vehData.Speed > 10.0f) {
		damperforce = settings.FFDamperMoving + (int)(0.5f * (vehData.Speed - 10.0f));
	}

	if (damperforce > (settings.FFDamperStationary + settings.FFDamperMoving) / 2) {
		damperforce = (settings.FFDamperStationary + settings.FFDamperMoving) / 2;
	}
	LogiPlayDamperForce(index_, damperforce);

	Vector3 accelVals = vehData.getAccelerationVectors(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true));
	LogiPlayConstantForce(index_, (int)(-settings.FFPhysics*accelVals.x));
	LogiPlaySpringForce(index_, 0, (int)vehData.Speed, (int)vehData.Speed);

	if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(vehicle) > 1.25f) {
		LogiPlayCarAirborne(index_);
	}
	else if (LogiIsPlaying(index_, LOGI_FORCE_CAR_AIRBORNE)) {
		LogiStopCarAirborne(index_);
	}
}

// Updates logiWheelVal, logiThrottleVal, logiBrakeVal, logiClutchVal
void WheelInput::UpdateLogiValues() {
	logiSteeringWheelPos = LogiGetState(index_)->lX;
	logiThrottlePos = LogiGetState(index_)->lY;
	logiBrakePos = LogiGetState(index_)->lRz;
	logiClutchPos = LogiGetState(index_)->rglSlider[1];
	//LogiGenerateNonLinearValues(index_, 33);
	//logiSteeringWheelPos = LogiGetNonLinearValue(index_, LogiGetState(index_)->lX);
	//  32767 @ nope | 0
	// -32768 @ full | 1
	logiWheelVal = ((float)(logiSteeringWheelPos - 32767) / -65535.0f) - 0.5f;
	logiThrottleVal = (float)(logiThrottlePos - 32767) / -65535.0f;
	logiBrakeVal = (float)(logiBrakePos - 32767) / -65535.0f;
	logiClutchVal = 1.0f + (float)(logiClutchPos - 32767) / 65535.0f;
}

void WheelInput::DoWheelSteering() {
	// Anti-deadzone
	int additionalOffset = 2560;
	float antiDeadzoned = 0.0f;
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
}

bool WheelInput::InitWheel(ScriptSettings settings, Logger logger) {
	if (settings.LogiWheel) {
		LogiSteeringInitialize(TRUE);
		if (LogiUpdate() && LogiIsConnected(index_)) {
			logger.Write("Wheel detected");
			return true;
		}
		else {
			logger.Write("No wheel detected");
			return false;
		}
	}
	else {
		logger.Write("Wheel disabled");
		return false;
	}
}

float WheelInput::GetLogiWheelVal() {
	return logiWheelVal;
}
float WheelInput::GetLogiThrottleVal() {
	return logiThrottleVal;
}
float WheelInput::GetLogiBrakeVal() {
	return logiBrakeVal;
}
float WheelInput::GetLogiClutchVal() {
	return logiClutchVal;
}