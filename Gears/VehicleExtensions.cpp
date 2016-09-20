#include "VehicleExtensions.hpp"
#include "../../ScriptHookV_SDK/inc/nativeCaller.h"

uint64_t VehicleExtensions::GetAddress(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);
	return address;
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A2 : 0x792);

	return address == 0 ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint16_t VehicleExtensions::GetGearNext(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	return address == 0 ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint32_t VehicleExtensions::GetGears(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	return address == 0 ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
}

void VehicleExtensions::SetGears(Vehicle handle, uint32_t value) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearCurr(Vehicle handle, uint16_t value) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearNext(Vehicle handle, uint16_t value) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A2 : 0x792);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint32_t VehicleExtensions::GetTopGear(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A6 : 0x796);

	return address == 0 ? 0 : *reinterpret_cast<const unsigned char *>(address + offset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7D4 : 0x7C4);

	return address == 0 ? 0.0f : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7D4 : 0x7C4);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetClutch(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

	return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetClutch(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetTurbo(Vehicle handle) const {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

	return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetTurbo(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottle(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottle(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottleP(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottleP(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetBrakeP(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetBrakeP(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x768 : 0x758);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x768 : 0x758);

	*reinterpret_cast<float *>(address + offset) = value;
}

uint64_t VehicleExtensions::GetWheelsPtr(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0xAA0 : 0xA80);
	offset = (getGameVersion() > 23 ? 0xAB0 : offset);

	return *reinterpret_cast<uint64_t *>(address + offset);
}

uint64_t VehicleExtensions::GetWheelPtr(uint64_t address, int index) {

	return *reinterpret_cast<uint64_t *>(address + index * 8);
}

void VehicleExtensions::SetWheelsHealth(Vehicle handle, float health) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0xAA0 : 0xA80);
	offset = (getGameVersion() > 23 ? 0xAB0 : offset);

	uint64_t wheelPtr;
	wheelPtr = *reinterpret_cast<uint64_t *>(address + offset);

	uint64_t wheels[6] = {};

	for (int i = 0; i < 6; i++) {
		wheels[i] = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		if (wheels[i]) {
			*reinterpret_cast<float *>(wheels[i] + (getGameVersion() > 3 ? 0x1E0 : 0x1D0)) = health;
		}
	}
}

// Steering input angle, steering lock independent
float VehicleExtensions::GetSteeringInputAngle(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8A4 : 0x894);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringInputAngle(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8A4 : 0x894);

	*reinterpret_cast<float *>(address + offset) = value;
}

// Wheel angle, steering lock dependent
float VehicleExtensions::GetSteeringAngle(Vehicle handle) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8AC : 0x89C);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringAngle(Vehicle handle, float value) const {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8AC : 0x89C);

	*reinterpret_cast<float *>(address + offset) = value;
}
