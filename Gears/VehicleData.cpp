#include "VehicleData.hpp"

VehicleData::VehicleData() {
}


VehicleData::~VehicleData() {
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
}

bool VehicleData::isBadTruck(char *name) {
	for (int i = 0; i < badModelNames.size(); i++) {
		if (strcmp(name, badModelNames[i]) == 0)
			return true;
	}
	return false;
}

void VehicleData::ReadMemData(VehicleExtensions ext, Vehicle vehicle) {
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
	IsBike = VEHICLE::IS_THIS_MODEL_A_BIKE(model) == TRUE;
	IsTruck = isBadTruck(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model));
}


// Takes velocities in m/s, returns acceleration vectors in m/s^2
Vector3 VehicleData::getAccelerationVectors(Vector3 velocities) {
	long long time = std::chrono::system_clock::now().time_since_epoch().count();
	Vector3 result;
	result.x = (velocities.x - prevVelocities.x)*1000000.0f / (time - prevTime);
	result.y = (velocities.y - prevVelocities.y)*1000000.0f / (time - prevTime);
	result.z = (velocities.z - prevVelocities.z)*1000000.0f / (time - prevTime);
	prevTime = time;
	prevVelocities = velocities;
	return result;
}