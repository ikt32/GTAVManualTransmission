#include "VehicleExtensions.hpp"

uintptr_t VehicleExtensions::PatchClutchLow() {
	// Tested on build 350 and build 617
	// We're only interested in the first 7 bytes but we need the correct one
	// C7 43 40 CD CC CC 3D is what we're looking for, the second occurrence, at 
	// 7FF6555FE34A or GTA5.exe+ECE34A in build 617.
	uintptr_t address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx");

	if (address) {
		memset((void *)address, 0x90, 7);
	}
	return address;
}
void VehicleExtensions::RestoreClutchLow(uintptr_t address) {
	byte instrArr[7] = { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D };
	if (address) {
		for (int i = 0; i < 7; i++) {
			memset((void *)(address + i), instrArr[i], 1);
		}
	}
}

uintptr_t VehicleExtensions::PatchClutchStationary01() {
	// Also looking for C7 43 40 CD CC CC 3D
	// This pattern also works on 350 and 617
	uintptr_t address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\xE9\xF6\x04\x00\x00", "xxxxxxxx????");

	if (address) {
		memset((void *)address, 0x90, 7);
	}
	return address;
}
void VehicleExtensions::RestoreClutchStationary01(uintptr_t address) {
	byte instrArr[7] = { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D };
	if (address) {
		for (int i = 0; i < 7; i++) {
			memset((void *)(address + i), instrArr[i], 1);
		}
	}
}

uintptr_t VehicleExtensions::PatchClutchStationary04() {
	// Looking for F3 0F 11 47 40
	// This pattern works on 350 and 617
	uintptr_t address = mem.FindPattern("\xF3\x0F\x11\x47\x40\xF3\x0F\x59\x3D\xDA\x9C\x8E\x00", "xxxxxxxxx????");

	if (address) {
		memset((void *)address, 0x90, 5);
	}
	return address;
}
void VehicleExtensions::RestoreClutchStationary04(uintptr_t address) {
	byte instrArr[5] = { 0xF3, 0x0F, 0x11, 0x47, 0x40 };
	if (address) {
		for (int i = 0; i < 5; i++) {
			memset((void *)(address + i), instrArr[i], 1);
		}
	}
}

uintptr_t VehicleExtensions::PatchThrottleRedline() {
	// Looking for 44 89 71 44
	uintptr_t address = mem.FindPattern("\x44\x89\x71\x44\xF3\x0F\x11\x75\x47", "xxxxxxx??");

	if (address) {
		memset((void *)address, 0x90, 4);
	}
	return address;
}
void VehicleExtensions::RestoreThrottleRedline(uintptr_t address) {
	byte instrArr[4] = { 0x44, 0x89, 0x71, 0x44 };
	if (address) {
		for (int i = 0; i < 4; i++) {
			memset((void *)(address + i), instrArr[i], 1);
		}
	}
}


uintptr_t VehicleExtensions::PatchClutchStationaryLow() {
	// F3 0F 11 43 40 <- Looking for this
	// 0F B7 43 04    <- Next instruction
	uintptr_t address = mem.FindPattern("\xF3\x0F\x11\x43\x40\x0F\xB7\x43\x04", "xxxxxxx??");

	if (address) {
		memset((void *)address, 0x90, 5);
	}
	return address;
}
void VehicleExtensions::RestoreClutchStationaryLow(uintptr_t address) {
	byte instrArr[7] = { 0xF3, 0x0F, 0x11, 0x43, 0x40 };
	if (address) {
		for (int i = 0; i < 5; i++) {
			memset((void *)(address + i), instrArr[i], 1);
		}
	}
}

uint64_t VehicleExtensions::GetAddress(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);
	return address;
}

uint32_t VehicleExtensions::GetGears(Vehicle handle) {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	return address == 0 ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
}

void VehicleExtensions::SetGears(Vehicle handle, uint32_t value) {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint32_t VehicleExtensions::GetTopGear(Vehicle handle)
{
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7A6 : 0x796);

	return address == 0 ? 0 : *reinterpret_cast<const unsigned char *>(address + offset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle)
{
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7D4 : 0x7C4);

	return address == 0 ? 0.0f : *reinterpret_cast<const float *>(address + offset);
}
void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7D4 : 0x7C4);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetClutch(Vehicle handle) {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

	return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
}
void VehicleExtensions::SetClutch(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetTurbo(Vehicle handle) {
	const uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

	return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
}
void VehicleExtensions::SetTurbo(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetThrottle(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

	return *reinterpret_cast<float *>(address + offset);
}
void VehicleExtensions::SetThrottle(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetThrottleP(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

	return *reinterpret_cast<float *>(address + offset);
}
void VehicleExtensions::SetThrottleP(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetBrakeP(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

	return *reinterpret_cast<float *>(address + offset);
}
void VehicleExtensions::SetBrakeP(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

	*reinterpret_cast<float *>(address + offset) = value;
}
float VehicleExtensions::GetFuelLevel(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x768 : 0x758);

	return *reinterpret_cast<float *>(address + offset);
}
void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0x768 : 0x758);

	*reinterpret_cast<float *>(address + offset) = value;
}
uint64_t VehicleExtensions::GetWheelsPtr(Vehicle handle) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0xAA0 : 0xA80);

	return *reinterpret_cast<uint64_t *>(address + offset);
}
uint64_t VehicleExtensions::GetWheelPtr(uint64_t address, int index) {
		
	return *reinterpret_cast<uint64_t *>(address + index*8);
}

void VehicleExtensions::SetWheelsHealth(Vehicle handle, float health) {
	uint64_t address = mem.GetAddressOfEntity(handle);

	int offset = (getGameVersion() > 3 ? 0xAA0 : 0xA80);

	uint64_t wheelPtr;
	wheelPtr = *reinterpret_cast<uint64_t *>(address + offset);

	uint64_t wheels[6];

	for (int i = 0; i < 6; i++) {
		wheels[i] = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		if (wheels[i]) {
			*reinterpret_cast<float *>(wheels[i] + (getGameVersion() > 3 ? 0x1E0 : 0x1D0)) = health;
		}
	}
}
