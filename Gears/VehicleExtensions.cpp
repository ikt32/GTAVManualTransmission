#include "VehicleExtensions.hpp"

namespace VehExt {
	uintptr_t VehicleExtensions::PatchClutchAddress() {
		// Tested on build 350 and build 617
		// We're only interested in the first 7 bytes but we need the correct one
		// C7 43 40 CD CC CC 3D is what we're looking for, the second occurrence, at 
		// 7FF6555FE34A or GTA5.exe+ECE34A in build 617.
		uintptr_t address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx");

		if (address) {
			for (int i = 0; i < 7; i++) {
				*reinterpret_cast<byte *>(address + i) = 0x90;
			}
		}
		return address;
	}

	void VehicleExtensions::RestoreClutchInstr(uintptr_t address) {
		byte instrArr[7] = { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D };
		if (address) {
			for (int i = 0; i < 7; i++) {
				*reinterpret_cast<byte *>(address + i) = instrArr[i];
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

		int offset = (getGameVersion() > 3 ? 0xAA0 : 0xA90);

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
}
