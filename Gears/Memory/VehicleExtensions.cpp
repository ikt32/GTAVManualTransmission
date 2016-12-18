#include "VehicleExtensions.hpp"
#include <vector>
#include "NativeMemory.hpp"

void VehicleExtensions::ClearAddress() {
	currAddress = nullptr;
}

BYTE *VehicleExtensions::GetAddress(Vehicle handle) {
	if (currAddress == nullptr) {
		MemoryAccess mem;
		currAddress = reinterpret_cast<BYTE *>(mem.GetAddressOfEntity(handle));
	}
	return currAddress;
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A2 : 0x792);
	offset = (gameVersion > 25 ? 0x7C2 : offset);
	offset = (gameVersion > 27 ? 0x7E2 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint16_t VehicleExtensions::GetGearNext(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A0 : 0x790);
	offset = (gameVersion > 25 ? 0x7C0 : offset);
	offset = (gameVersion > 27 ? 0x7E0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint32_t VehicleExtensions::GetGears(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A0 : 0x790);
	offset = (gameVersion > 25 ? 0x7C0 : offset);
	offset = (gameVersion > 27 ? 0x7E0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
}

void VehicleExtensions::SetGears(Vehicle handle, uint32_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A0 : 0x790);
	offset = (gameVersion > 25 ? 0x7C0 : offset);
	offset = (gameVersion > 27 ? 0x7E0 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearCurr(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A0 : 0x790);
	offset = (gameVersion > 25 ? 0x7C0 : offset);
	offset = (gameVersion > 27 ? 0x7E0 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearNext(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A2 : 0x792);
	offset = (gameVersion > 25 ? 0x7C2 : offset);
	offset = (gameVersion > 27 ? 0x7E2 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint32_t VehicleExtensions::GetTopGear(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7A6 : 0x796);
	offset = (gameVersion > 25 ? 0x7C6 : offset);
	offset = (gameVersion > 27 ? 0x7E6 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const unsigned char *>(address + offset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7D4 : 0x7C4);
	offset = (gameVersion > 25 ? 0x7F4 : offset);
	offset = (gameVersion > 27 ? 0x814 : offset);

	return address == nullptr ? 0.0f : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7D4 : 0x7C4);
	offset = (gameVersion > 25 ? 0x7F4 : offset);
	offset = (gameVersion > 27 ? 0x814 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetClutch(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7E0 : 0x7D0);
	offset = (gameVersion > 25 ? 0x800 : offset);
	offset = (gameVersion > 27 ? 0x820 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetClutch(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7E0 : 0x7D0);
	offset = (gameVersion > 25 ? 0x800 : offset);
	offset = (gameVersion > 27 ? 0x820 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetTurbo(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7F8 : 0x7D8);
	offset = (gameVersion > 25 ? 0x818 : offset);
	offset = (gameVersion > 27 ? 0x838 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetTurbo(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7F8 : 0x7D8);
	offset = (gameVersion > 25 ? 0x818 : offset);
	offset = (gameVersion > 27 ? 0x838 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7E4 : 0x7D4);
	offset = (gameVersion > 25 ? 0x804 : offset);
	offset = (gameVersion > 27 ? 0x824 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x7E4 : 0x7D4);
	offset = (gameVersion > 25 ? 0x804 : offset);
	offset = (gameVersion > 27 ? 0x824 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottleP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8B4 : 0x8A4);
	offset = (gameVersion > 25 ? 0x8D4 : offset);
	offset = (gameVersion > 27 ? 0x8FC : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottleP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8B4 : 0x8A4);
	offset = (gameVersion > 25 ? 0x8D4 : offset);
	offset = (gameVersion > 27 ? 0x8FC : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetBrakeP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8B8 : 0x8A8);
	offset = (gameVersion > 25 ? 0x8D8 : offset);
	offset = (gameVersion > 27 ? 0x900 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetBrakeP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8B8 : 0x8A8);
	offset = (gameVersion > 25 ? 0x8D8 : offset);
	offset = (gameVersion > 27 ? 0x900 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x768 : 0x758);
	offset = (gameVersion > 25 ? 0x788 : offset);
	offset = (gameVersion > 27 ? 0x7A8 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x768 : 0x758);
	offset = (gameVersion > 25 ? 0x788 : offset);
	offset = (gameVersion > 27 ? 0x7A8 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

uint64_t VehicleExtensions::GetWheelsPtr(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0xAA0 : 0xA80);
	offset = (gameVersion > 23 ? 0xAB0 : offset);
	if (gameVersion == 06 || gameVersion == 07) {
		offset = 0xA90; // FiveM
	}
	offset = (gameVersion > 25 ? 0xAE0 : offset);
	offset = (gameVersion > 27 ? 0xB10 : offset);

	return *reinterpret_cast<uint64_t *>(address + offset);
}

void VehicleExtensions::SetWheelsHealth(Vehicle handle, float health) {
	auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers

	uint64_t wheels[6] = {};
	for (auto i = 0; i < 6; i++) {
		wheels[i] = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i); // iterate through wheels
		if (wheels[i]) {
			*reinterpret_cast<float *>(wheels[i] + (gameVersion > 3 ? 0x1E0 : 0x1D0)) = health;
		}
	}
}

// Don't use this on non-cars
std::vector<float> VehicleExtensions::GetWheelsCompression(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);

	uint64_t wheels[6] = {};
	std::vector<float> compressions;

	for (auto i = 0; i < 6; i++) {
		wheels[i] = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i); // iterate through wheels
		if (wheels[i]) {
			compressions.push_back(*reinterpret_cast<float *>(wheels[i] + (gameVersion > 3 ? 0x160 : 0x150)));
		}
	}
	return compressions;
}

// Steering input angle, steering lock independent
float VehicleExtensions::GetSteeringInputAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8A4 : 0x894);
	offset = (gameVersion > 25 ? 0x8C4 : offset);
	offset = (gameVersion > 27 ? 0x8EC : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringInputAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8A4 : 0x894);
	offset = (gameVersion > 25 ? 0x8C4 : offset);
	offset = (gameVersion > 27 ? 0x8EC : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

// Wheel angle, steering lock dependent
float VehicleExtensions::GetSteeringAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8AC : 0x89C);
	offset = (gameVersion > 25 ? 0x8CC : offset);
	offset = (gameVersion > 27 ? 0x8F4 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 3 ? 0x8AC : 0x89C);
	offset = (gameVersion > 25 ? 0x8CC : offset);
	offset = (gameVersion > 27 ? 0x8F4 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetEngineTemp(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > 27 ? 0x9AC : -1);
	if (offset == -1)
		return 0;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDirtLevel(Vehicle handle) {
	auto address = GetAddress(handle);


	auto offset = (gameVersion > 27 ? 0x938 : -1);
	if (offset == -1)
		return 0;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDashSpeed(Vehicle handle) {
	auto address = GetAddress(handle);


	auto offset = (gameVersion > 27 ? 0x9F0 : -1);
	if (offset == -1)
		return 0;

	return *reinterpret_cast<float *>(address + offset);
}
