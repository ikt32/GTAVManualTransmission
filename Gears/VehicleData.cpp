#include "VehicleData.hpp"

VehicleData::VehicleData() {
}


VehicleData::~VehicleData() {
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

