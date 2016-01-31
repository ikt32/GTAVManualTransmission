#include "VehicleExtensions.hpp"

namespace VehExt {
	uint64_t VehicleExtensions::GetAddress(Vehicle handle) {
		uint64_t address = mem.GetAddressOfEntity(handle);
		return address;
	}

	uint32_t VehicleExtensions::GetGears(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7a0;

		return address == 0 ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
	}

	void VehicleExtensions::SetGears(Vehicle handle, uint32_t value)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7a0;

		*reinterpret_cast<uint32_t *>(address + offset) = value;
	}
	float VehicleExtensions::GetCurrentRPM(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7D4;

		return address == 0 ? 0.0f : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7D4;

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetClutch(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E0;

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetClutch(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E0;

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetTurbo(Vehicle handle)
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7F8;

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetTurbo(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7F8;

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetThrottle(Vehicle handle)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E4;

		return *reinterpret_cast<float *>(address + offset);
	}
	void VehicleExtensions::SetThrottle(Vehicle handle, float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E4;

		*reinterpret_cast<float *>(address + offset) = value;
	}
}
