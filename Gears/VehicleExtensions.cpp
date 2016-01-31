#include "VehicleExtensions.hpp"

namespace VehExt {
	VehicleExtensions::VehicleExtensions(Vehicle vehicle) {
		handle = vehicle;
	}

	// am i going to get murdered for this hacky thing i stole from scripthookvdotnet????
	uint64_t VehicleExtensions::GetAddress() {
		uint64_t address = mem.GetAddressOfEntity(handle);
		return address;
	}

	uint32_t VehicleExtensions::GetGears()
	{
		unsigned char *const address = reinterpret_cast<unsigned char *>(mem.GetAddressOfEntity(handle));

		int offset = 0x7a0;

		return address == 0 ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
	}

	void VehicleExtensions::SetGears(uint32_t value)
	{
		unsigned char *const address = reinterpret_cast<unsigned char *>(mem.GetAddressOfEntity(handle));

		int offset = 0x7a0;

		*(address + offset) = value;
	}
	float VehicleExtensions::GetCurrentRPM()
	{
		const uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 2004;

		return address == 0 ? 0.0f : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetCurrentRPM(float rpm)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7D4;

		*reinterpret_cast<float *>(address + offset) = rpm;
	}
	float VehicleExtensions::GetClutch()
	{
		unsigned char *const address = reinterpret_cast<unsigned char *>(mem.GetAddressOfEntity(handle));

		int offset = 0x7E0;

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetClutch(float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E0;

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetTurbo()
	{
		unsigned char *const address = reinterpret_cast<unsigned char *>(mem.GetAddressOfEntity(handle));

		int offset = 0x7F8;

		return address == 0 ? 0 : *reinterpret_cast<const float *>(address + offset);
	}
	void VehicleExtensions::SetTurbo(float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7F8;

		*reinterpret_cast<float *>(address + offset) = value;
	}
	float VehicleExtensions::GetThrottle()
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E4;

		return *reinterpret_cast<float *>(address + offset);
	}
	void VehicleExtensions::SetThrottle(float value)
	{
		uint64_t address = mem.GetAddressOfEntity(handle);

		int offset = 0x7E4;

		*reinterpret_cast<float *>(address + offset) = value;
	}
}
