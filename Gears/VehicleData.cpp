#include "../../ScriptHookV_SDK/inc/natives.h"

#include "VehicleData.hpp"

VehicleData::VehicleData() {
	zeroSamples();
}

void VehicleData::Clear() {
	IsBike = false;
	IsTruck = false;
	Address = 0;
	Gears = 0;
	Rpm = 0.0f;
	Clutch = 1.0f;
	Throttle = 0.0f;
	Turbo = 0.0f;
	Speed = 0.0f;
	Velocity = 0.0f;
	TopGear = 1;
	LockGear = 1;
	CurrGear = 1;
	NextGear = 1;
	LockGears = 0x00010001;
	PrevGear = 1;
	LockSpeed = 0.0f;
	LockTruck = false;
	SimulatedNeutral = false;
	prevVelocities.x = 0;
	prevVelocities.y = 0;
	prevVelocities.z = 0;
	zeroSamples();
}

bool VehicleData::isBadTruck(char* name) {
	for (int i = 0; i < badModelNames.size(); i++) {
		if (strcmp(name, badModelNames[i]) == 0)
			return true;
	}
	return false;
}

bool VehicleData::noClutch(char* name) {
	for (int i = 0; i < noClutchModelNames.size(); i++) {
		if (strcmp(name, noClutchModelNames[i]) == 0)
			return true;
	}
	return false;
}

// Updates values from memory and natives
void VehicleData::UpdateValues(VehicleExtensions& ext, Vehicle vehicle) {
	Hash model = ENTITY::GET_ENTITY_MODEL(vehicle);

	Address = ext.GetAddress(vehicle);
	Gears = ext.GetGears(vehicle);
	CurrGear = ext.GetGearCurr(vehicle); //(0xFFFF0000 & Gears) >> 16;
	NextGear = ext.GetGearNext(vehicle); // 0x0000FFFF & Gears;
	Rpm = ext.GetCurrentRPM(vehicle);
	Clutch = ext.GetClutch(vehicle);
	Throttle = ext.GetThrottle(vehicle);
	Turbo = ext.GetTurbo(vehicle);
	TopGear = ext.GetTopGear(vehicle);
	Speed = ENTITY::GET_ENTITY_SPEED(vehicle);
	Velocity = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y;
	RotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle);
	IsBike = VEHICLE::IS_THIS_MODEL_A_BIKE(model) == TRUE;
	IsTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
	NoClutch = noClutch(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
	Pitch = ENTITY::GET_ENTITY_PITCH(vehicle);
	SteeringAngle = ext.GetSteeringAngle(vehicle);
	if (!IsBike) {
		WheelCompressions[0] = ext.GetWheelsCompression(vehicle).at(0);
		WheelCompressions[1] = ext.GetWheelsCompression(vehicle).at(1);
	}
}

// Only does this for the first two wheels because I'm lazy, damn it
std::array<float, 2> VehicleData::GetWheelCompressionSpeeds() {
	long long time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

	std::array<float, 2> result;
	result[0] = (WheelCompressions[0] - prevCompressions[0]) / ((time - prevCompressTime) / 1e9f);
	result[1] = (WheelCompressions[1] - prevCompressions[1]) / ((time - prevCompressTime) / 1e9f);

	prevCompressTime = time;
	prevCompressions = WheelCompressions;

	return result;
}


// Takes velocities in m/s, returns acceleration vectors in m/s^2
Vector3 VehicleData::getAccelerationVectors(Vector3 velocities) {
	long long time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

	Vector3 result;
	result.x = (velocities.x - prevVelocities.x) / ((time - prevAccelTime) / 1e9f);
	result.y = (velocities.y - prevVelocities.y) / ((time - prevAccelTime) / 1e9f);
	result.z = (velocities.z - prevVelocities.z) / ((time - prevAccelTime) / 1e9f);
	result._paddingx = 0;
	result._paddingy = 0;
	result._paddingz = 0;

	prevAccelTime = time;
	prevVelocities = velocities;

	accelSamples[averageAccelIndex] = result;
	averageAccelIndex = (averageAccelIndex + 1) % (SAMPLES - 1);

	return result;
}

Vector3 VehicleData::getAccelerationVectorsAverage() const {
	Vector3 result;
	result.x = 0;
	result.y = 0;
	result.z = 0;

	for (int i = 0; i < SAMPLES; i++) {
		result.x += accelSamples[i].x;
		result.y += accelSamples[i].y;
		result.z += accelSamples[i].z;
	}

	result.x = result.x / SAMPLES;
	result.y = result.y / SAMPLES;
	result.z = result.z / SAMPLES;
	return result;
}


void VehicleData::zeroSamples() {
	for (int i = 0; i < SAMPLES; i++) {
		accelSamples[i].x = 0.0f;
		accelSamples[i].y = 0.0f;
		accelSamples[i].z = 0.0f;
	}
}
