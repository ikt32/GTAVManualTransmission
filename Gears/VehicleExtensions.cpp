#include "VehicleExtensions.hpp"

namespace VehExt {
	uint64_t VehicleExtensions::GetAddress(Vehicle handle) {
		uint64_t address = mem.GetAddressOfEntity(handle);
		return address;
	}

	uint32_t VehicleExtensions::GetGears(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7A0 : 0x790);

		return address == 0 ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
	}

	void VehicleExtensions::SetGears(Vehicle handle, uint32_t value)
	{
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
	void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7D4 : 0x7C4);

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetClutch(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetClutch(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7E0 : 0x7D0);

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetTurbo(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetTurbo(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7F8 : 0x7D8);

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetThrottle(Vehicle handle)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

		return *reinterpret_cast<float *>(address + offset);
	}
	void VehicleExtensions::SetThrottle(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x7E4 : 0x7D4);

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetThrottleP(Vehicle handle)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

		return *reinterpret_cast<float *>(address + offset);
	}
	void VehicleExtensions::SetThrottleP(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x8B4 : 0x8A4);

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetBrakeP(Vehicle handle)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

		return *reinterpret_cast<float *>(address + offset);
	}
	void VehicleExtensions::SetBrakeP(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = (getGameVersion() > 3 ? 0x8B8 : 0x8A8);

		*reinterpret_cast<float *>(address + offset) = value;
	}
}
