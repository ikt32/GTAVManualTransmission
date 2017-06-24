#include "VehicleExtensions.hpp"
#include <vector>
#include "NativeMemory.hpp"
#include "../Util/Versions.h"
#include "Offsets.hpp"

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

Vector3 Cross(Vector3 left, Vector3 right) {
	Vector3 result;
	result.x = left.y * right.z - left.z * right.y;
	result.y = left.z * right.x - left.x * right.z;
	result.z = left.x * right.y - left.y * right.x;
	return result;
}

Vector3 operator + (Vector3 left, Vector3 right) {
	return Vector3 { 
		left.x + right.x,
		0,
		left.y + right.y,
		0,
		left.z + right.z 
	};
}

Vector3 operator * (Vector3 value, float scale) {
	return Vector3{ 
		value.x * scale, 
		0, 
		value.y * scale, 
		0, 
		value.z * scale ,
		0 
	};
}

Vector3 operator * (float scale, Vector3 vec) {
	return vec * scale;
}

Vector3 GetOffsetInWorldCoords(Vector3 position, Vector3 rotation, Vector3 forward, Vector3 offset) {
	const float deg2Rad = 0.01745329251994329576923690768489;
	float num1 = cosf(rotation.y * deg2Rad);
	float x = num1 * cosf(-rotation.z  * deg2Rad);
	float y = num1 * sinf(rotation.z  * deg2Rad);
	float z = sinf(-rotation.y * deg2Rad);
	Vector3 right = { x, 0, y, 0, z, 0 };
	Vector3 up = Cross(right, forward);
	return position + (right * offset.x) + (forward * offset.y) + (up * offset.z);
}

VehicleExtensions::VehicleExtensions() {
	mem::init();
}

BYTE *VehicleExtensions::GetAddress(Vehicle handle) {
	return reinterpret_cast<BYTE *>(mem::GetAddressOfEntity(handle));
}

uint32_t VehicleExtensions::GetGears(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint32_t *>(address + offset);
}

void VehicleExtensions::SetGears(Vehicle handle, uint32_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F0 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A2 : 0x792);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C2 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E2 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F2 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

void VehicleExtensions::SetGearCurr(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F2 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint16_t VehicleExtensions::GetGearNext(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A0 : 0x790);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E0 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F0 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const uint16_t *>(address + offset);
}

void VehicleExtensions::SetGearNext(Vehicle handle, uint16_t value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A2 : 0x792);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C2 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E2 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F2 : offset);

	*reinterpret_cast<uint32_t *>(address + offset) = value;
}

uint32_t VehicleExtensions::GetTopGear(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7A6 : 0x796);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7C6 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7E6 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7F6 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const unsigned char *>(address + offset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7D4 : 0x7C4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7F4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x814 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x824 : offset);

	return address == nullptr ? 0.0f : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7D4 : 0x7C4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x7F4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x814 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x824 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetClutch(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E0 : 0x7D0);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x800 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x820 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x830 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetClutch(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E0 : 0x7D0);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x800 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x820 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x830 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetTurbo(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7F8 : 0x7D8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x818 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x838 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x848 : offset);

	return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + offset);
}

void VehicleExtensions::SetTurbo(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7F8 : 0x7D8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x818 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x838 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x848 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E4 : 0x7D4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x804 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x824 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x834 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x7E4 : 0x7D4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x804 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x824 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x834 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetThrottleP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B4 : 0x8A4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8FC : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x90C : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetThrottleP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B4 : 0x8A4);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8FC : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x90C : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetBrakeP(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B8 : 0x8A8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D8 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x900 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x910 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetBrakeP(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8B8 : 0x8A8);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8D8 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x900 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x910 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

bool VehicleExtensions::GetHandbrake(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8BC : 0x8AC);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8DC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x904 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x914 : offset);

	return *reinterpret_cast<bool *>(address + offset);
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x768 : 0x758);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x788 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7A8 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7B8 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x768 : 0x758);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x788 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x7A8 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x7B8 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetEngineTemp(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x984 : -1);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x9AC : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x9BC : offset);

	if (offset < 0)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDirtLevel(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x910 : -1);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x938 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x948 : offset);

	if (offset < 0)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetDashSpeed(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_463_1_NOSTEAM ? 0x9A4 : -1);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x9C8 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x9F0 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0xA00 : offset);

	if (offset < 0)
		return 0.0f;

	return *reinterpret_cast<float *>(address + offset);
}

float VehicleExtensions::GetSteeringInputAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8A4 : 0x894);
	offset = (gameVersion > G_VER_1_0_678_1_NOSTEAM ? 0x89C : offset);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8C4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8EC : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x8FC : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringInputAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8A4 : 0x894);
	offset = (gameVersion > G_VER_1_0_678_1_NOSTEAM ? 0x89C : offset);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8C4 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8EC : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x8FC : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

float VehicleExtensions::GetSteeringAngle(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8AC : 0x89C);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8CC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8F4 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x904 : offset);

	return *reinterpret_cast<float *>(address + offset);
}

void VehicleExtensions::SetSteeringAngle(Vehicle handle, float value) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x8AC : 0x89C);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x8CC : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x8F4 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x904 : offset);

	*reinterpret_cast<float *>(address + offset) = value;
}

uint64_t VehicleExtensions::GetHandlingPtr(Vehicle handle) {
	auto address = GetAddress(handle);
	
	int offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0x850 : 0x830);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x878 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0x888 : offset);

	return *reinterpret_cast<uint64_t *>(address + offset);
}

float VehicleExtensions::GetDriveBiasFront(Vehicle handle) {
	auto address = GetHandlingPtr(handle);
	return *reinterpret_cast<float *>(address + hOffsets.fDriveBiasFront);
}

float VehicleExtensions::GetDriveBiasRear(Vehicle handle) {
	auto address = GetHandlingPtr(handle);
	return *reinterpret_cast<float *>(address + hOffsets.fDriveBiasRear);
}

float VehicleExtensions::GetPetrolTankVolume(Vehicle handle) {
	auto address = GetHandlingPtr(handle);
	return *reinterpret_cast<float *>(address + hOffsets.fPetrolTankVolume);
}

float VehicleExtensions::GetOilVolume(Vehicle handle) {
	auto address = GetHandlingPtr(handle);
	return *reinterpret_cast<float *>(address + hOffsets.fOilVolume);
}

uint8_t VehicleExtensions::GetNumWheels(Vehicle handle) {
	auto address = GetAddress(handle);

	auto offset = (gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0xAA0 : 0xA80);
	// FiveM should report 1.0.505.2 now :)
	offset = (gameVersion > G_VER_1_0_463_1_NOSTEAM ? 0xA90 : offset);
	offset = (gameVersion > G_VER_1_0_757_4_NOSTEAM ? 0xAB0 : offset);
	offset = (gameVersion > G_VER_1_0_791_2_NOSTEAM ? 0xAE0 : offset);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0xB10 : offset);
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0xB20 : offset);

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
	offset = (gameVersion > G_VER_1_0_1032_1_NOSTEAM ? 0xB20 : offset);

	return *reinterpret_cast<uint64_t *>(address + offset);
}

std::vector<uint64_t> VehicleExtensions::GetWheelPtrs(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers
	auto numWheels = GetNumWheels(handle);
	std::vector<uint64_t> wheelPtrs;
	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		wheelPtrs.push_back(wheelAddr);
	}
	return wheelPtrs;
}

float VehicleExtensions::GetVisualHeight(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);

	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x080 : -1);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x080 : offset);
	if (offset == -1)
		return 0.0f;

	return *reinterpret_cast<float *>(wheelPtr + offset);
}

void VehicleExtensions::SetVisualHeight(Vehicle handle, float height) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x07C : -1);
	offset = (gameVersion > G_VER_1_0_877_1_NOSTEAM ? 0x07C : -1);

	if (offset == -1)
		return;

	*reinterpret_cast<float *>(wheelPtr + offset) = height;
}

std::vector<float> VehicleExtensions::GetWheelHealths(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x1E0 : 0x1D0;

	std::vector<float> healths;

	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		healths.push_back(*reinterpret_cast<float *>(wheelAddr + offset));
	}
	return healths;
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

float VehicleExtensions::GetSteeringMultiplier(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x138 : 0x128;

	if (numWheels > 1) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * 1);
		return abs(*reinterpret_cast<float*>(wheelAddr + offset));
	}
	return 1.0f;
}

void VehicleExtensions::SetSteeringMultiplier(Vehicle handle, float value) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x138 : 0x128;

	for (int i = 0; i<numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		int sign = sgn(*reinterpret_cast<float*>(wheelAddr + offset));
		*reinterpret_cast<float*>(wheelAddr + offset) = value * sign;
	}
}

std::vector<Vector3> VehicleExtensions::GetWheelOffsets(Vehicle handle) {
	auto wheels = GetWheelPtrs(handle);
	std::vector<Vector3> positions;

	int offPosX = 0x20;
	int offPosY = 0x24;
	int offPosZ = 0x28;

	for (auto wheelAddr : wheels) {
		if (!wheelAddr) continue;

		Vector3 wheelPos;
		wheelPos.x = *reinterpret_cast<float *>(wheelAddr + offPosX);
		wheelPos.y = *reinterpret_cast<float *>(wheelAddr + offPosY);
		wheelPos.z = *reinterpret_cast<float *>(wheelAddr + offPosZ);
		positions.push_back(wheelPos);
	}
	return positions;
}

std::vector<Vector3> VehicleExtensions::GetWheelCoords(Vehicle handle, Vector3 position, Vector3 rotation, Vector3 direction) {
	std::vector<Vector3> worldCoords;
	std::vector<Vector3> positions = GetWheelOffsets(handle);

	for (Vector3 wheelPos : positions) {
		Vector3 absPos = GetOffsetInWorldCoords(position, rotation, direction, wheelPos);
		worldCoords.push_back(absPos);
	}
	return worldCoords;
}

std::vector<Vector3> VehicleExtensions::GetWheelLastContactCoords(Vehicle handle) {
	auto wheels = GetWheelPtrs(handle);
	std::vector<Vector3> positions;
	// 0x40: Last wheel contact coordinates
	// 0x50: Last wheel contact coordinates but centered on the wheel width
	// 0x60: Next probable wheel position? Seems to flutter around a lot, while
	//       position is entirely lost (0.0) when contact is lost. Wheels that
	//       steer emphasise this further, though acceleration/deceleration
	//       will also influence it.

	int offPosX = 0x40;
	int offPosY = 0x44;
	int offPosZ = 0x48;

	for (auto wheelAddr : wheels) {
		if (!wheelAddr) continue;

		Vector3 wheelPos;
		wheelPos.x = *reinterpret_cast<float *>(wheelAddr + offPosX);
		wheelPos.y = *reinterpret_cast<float *>(wheelAddr + offPosY);
		wheelPos.z = *reinterpret_cast<float *>(wheelAddr + offPosZ);
		positions.push_back(wheelPos);
	}
	return positions;
}

std::vector<float> VehicleExtensions::GetWheelCompressions(Vehicle handle) {
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

std::vector<bool> VehicleExtensions::GetWheelsOnGround(Vehicle handle) {
	std::vector<bool> onGround;
	for (auto comp : GetWheelCompressions(handle)) {
		onGround.push_back(comp != 0.0f);
	}
	return onGround;
}

std::vector<WheelDimensions> VehicleExtensions::GetWheelDimensions(Vehicle handle) {
	auto wheels = GetWheelPtrs(handle);

	std::vector<WheelDimensions> dimensionsSet;
	int offTyreRadius = 0x110;
	int offRimRadius = 0x114;
	int offTyreWidth = 0x118;

	for (auto wheelAddr : wheels) {
		if (!wheelAddr) continue;

		WheelDimensions dimensions;
		dimensions.TyreRadius = *reinterpret_cast<float *>(wheelAddr + offTyreRadius);
		dimensions.RimRadius = *reinterpret_cast<float *>(wheelAddr + offRimRadius);
		dimensions.TyreWidth = *reinterpret_cast<float *>(wheelAddr + offTyreWidth);
		dimensionsSet.push_back(dimensions);
	}
	return dimensionsSet;
}

std::vector<float> VehicleExtensions::GetWheelRotationSpeeds(Vehicle handle) {
	auto wheelPtr = GetWheelsPtr(handle);
	auto numWheels = GetNumWheels(handle);

	auto offset = gameVersion > G_VER_1_0_350_2_NOSTEAM ? 0x168 : 0x158;

	std::vector<float> speeds;

	for (auto i = 0; i < numWheels; i++) {
		auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
		speeds.push_back(-*reinterpret_cast<float *>(wheelAddr + offset));
	}
	return speeds;
}

std::vector<float> VehicleExtensions::GetTyreSpeeds(Vehicle handle) {
	std::vector<float> wheelSpeeds;
	int numWheels = GetNumWheels(handle);
	std::vector<float> rotationSpeed = GetWheelRotationSpeeds(handle);
	std::vector<WheelDimensions> dimensionsSet = GetWheelDimensions(handle);
	for (int i = 0; i < numWheels; i++) {
		wheelSpeeds.push_back(rotationSpeed[i] * dimensionsSet[i].TyreRadius);
	}
	return wheelSpeeds;
}
