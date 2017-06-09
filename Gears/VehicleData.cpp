#include "../../ScriptHookV_SDK/inc/natives.h"

#include "VehicleData.hpp"
#include "../../ScriptHookV_SDK/inc/enums.h"

VehicleData::VehicleData() {
	zeroSamples();
}

void VehicleData::Clear() {
	Class = VehicleClass::Car;
	IsTruck = false;
	Address = nullptr;
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
	WheelCompressions.clear();
	prevCompressions.clear();
	std::fill(LockSpeeds.begin(), LockSpeeds.end(), 0.0f);
	HasSpeedo = false;
	zeroSamples();
}

void VehicleData::UpdateRpm() {
	PrevRpm = Rpm;

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
	CurrGear = ext.GetGearCurr(vehicle);
	NextGear = ext.GetGearNext(vehicle);
	Rpm = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) ? ext.GetCurrentRPM(vehicle) : 0.01f;
	Clutch = ext.GetClutch(vehicle);
	Throttle = ext.GetThrottle(vehicle);
	Turbo = ext.GetTurbo(vehicle);
	TopGear = ext.GetTopGear(vehicle);
	Speed = ENTITY::GET_ENTITY_SPEED(vehicle);
	V3Velocities = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
	Velocity = V3Velocities.y;
	RotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle);
	Class = findClass(model);
	IsTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
	NoClutch = noClutch(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
	Pitch = ENTITY::GET_ENTITY_PITCH(vehicle);
	SteeringAngle = ext.GetSteeringAngle(vehicle);
	WheelCompressions = ext.GetWheelCompressions(vehicle);
	if (prevCompressions.size() != WheelCompressions.size()) {
		prevCompressions = WheelCompressions;
	}

	ControlAccelerate = (CONTROLS::GET_CONTROL_VALUE(0, ControlVehicleAccelerate) - 127) / 127.0f;
}

VehicleData::VehicleClass VehicleData::findClass(Hash model) {
	if (VEHICLE::IS_THIS_MODEL_A_CAR(model))
		return VehicleClass::Car;
	if (VEHICLE::IS_THIS_MODEL_A_BICYCLE(model))	// >bicycle is a bike. wth R*?
		return VehicleClass::Bicycle;
	if (VEHICLE::IS_THIS_MODEL_A_BIKE(model))
		return VehicleClass::Bike;
	if (VEHICLE::IS_THIS_MODEL_A_BOAT(model))
		return VehicleClass::Boat;
	if (VEHICLE::IS_THIS_MODEL_A_PLANE(model))
		return VehicleClass::Plane;
	if (VEHICLE::IS_THIS_MODEL_A_HELI(model))
		return VehicleClass::Heli;
	if (VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))
		return VehicleClass::Quad;

	return VehicleClass::Unknown;
}

// Only does this for the first two wheels because I'm lazy, damn it
std::vector<float> VehicleData::GetWheelCompressionSpeeds() {
	long long time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns

	std::vector<float> result;
	for (int i = 0; i < WheelCompressions.size(); i++) {
		result.push_back((WheelCompressions[i] - prevCompressions[i]) / ((time - prevCompressTime) / 1e9f));
	}
	
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
	Vector3 result = {};

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
