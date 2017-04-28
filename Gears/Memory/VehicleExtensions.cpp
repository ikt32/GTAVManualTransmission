#include "VehicleExtensions.hpp"
#include <vector>
#include "NativeMemory.hpp"
#include "../Util/Versions.h"

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

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A2 : 0x792);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C2 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E2 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint16_t VehicleExtensions::GetGearNext(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

uint32_t VehicleExtensions::GetGears(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
}

void VehicleExtensions::SetGears(Vehicle handle, uint32_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearCurr(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

void VehicleExtensions::SetGearNext(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A2 : 0x792);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C2 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E2 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint32_t VehicleExtensions::GetTopGear(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A6 : 0x796);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C6 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E6 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const unsigned char *>(address + offset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7D4 : 0x7C4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7F4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x814 : offset);

	return address == nullptr ? 0.0f : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7D4 : 0x7C4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7F4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x814 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetClutch(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E0 : 0x7D0);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x800 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x820 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetClutch(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E0 : 0x7D0);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x800 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x820 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetTurbo(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7F8 : 0x7D8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x818 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x838 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetTurbo(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7F8 : 0x7D8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x818 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x838 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E4 : 0x7D4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x804 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x824 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E4 : 0x7D4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x804 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x824 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottleP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B4 : 0x8A4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8FC : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottleP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B4 : 0x8A4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8FC : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetBrakeP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B8 : 0x8A8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D8 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x900 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetBrakeP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B8 : 0x8A8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D8 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x900 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

bool VehicleExtensions::GetHandbrake(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8BC : 0x8AC);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8DC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x904 : offset);

	return *reinterpret_cast<bool *>(address + offset);
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x768 : 0x758);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x788 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7A8 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x768 : 0x758);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x788 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7A8 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

uint8_t VehicleExtensions::GetNumWheels(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0xAA0 : 0xA80);
	// FiveM should report 1.0.505.2 now :)
	offset = (gameVersion > G_VER_1_0_463_1_NOSTEAM ? 0xA90 : offset);
	offset = (gameVersion > G_VER_1_0_757_4_NOSTEAM ? 0xAB0 : offset);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0xAE0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0xB10 : offset);
	offset += 8;
	
	return *reinterpret_cast<int *>(address + offset);
}

uint64_t VehicleExtensions::GetWheelsPtr(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0xAA0 : 0xA80);
	// FiveM should report 1.0.505.2 now :)
	offset = (gameVersion > G_VER_1_0_463_1_NOSTEAM ? 0xA90 : offset); 
	offset = (gameVersion > G_VER_1_0_757_4_NOSTEAM ? 0xAB0 : offset);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0xAE0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0xB10 : offset);

	return *reinterpret_cast<uint64_t *>(address + offset);
}


float VehicleExtensions::GetVisualHeight(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? -1 : 0x080);
	if (offset == -1)
		return 0.0f;

	return *reinterpret_cast<float *>(wheelPtr + offset);
}

// 0 is default. Pos is lowered, Neg = change height. Used by LSC. Set once.
// Physics are NOT affected, including hitbox.
void VehicleExtensions::SetVisualHeight(Vehicle handle, float height) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? -1 : 0x07c);

	if (offset == -1)
		return;

	*reinterpret_cast<float *>(wheelPtr + offset) = height;
}

void VehicleExtensions::SetWheelsHealth(Vehicle handle, float health) {
	auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x1E0 : 0x1D0;

	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		*reinterpret_cast<float *>(wheelAddr + offset) = health;
	}
}

std::vector<float> VehicleExtensions::GetWheelsCompression(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x160 : 0x150;

	std::vector<float> compressions;

	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		compressions.push_back(*reinterpret_cast<float *>(wheelAddr + offset));
	}
	return compressions;
}

// Steering input angle, steering lock independent
float VehicleExtensions::GetSteeringInputAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8A4 : 0x894);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8C4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8EC : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringInputAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8A4 : 0x894);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8C4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8EC : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

// Wheel angle, steering lock dependent
float VehicleExtensions::GetSteeringAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8AC : 0x89C);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8CC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8F4 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8AC : 0x89C);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8CC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8F4 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetEngineTemp(Vehicle handle) {
	if (gameVersion <= G_VER_1_0_877_1_NOSTEAM) {
		return 0.0f;
	}

	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x9AC : -1);
	if (offset == -1)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDirtLevel(Vehicle handle) {
	if (gameVersion <= G_VER_1_0_877_1_NOSTEAM) {
		return 0.0f;
	}

	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x938 : -1);
	if (offset == -1)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDashSpeed(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_463_1_NOSTEAM ? 0x9A4 : -1);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x9F0 : offset);

	if (offset == -1)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}
