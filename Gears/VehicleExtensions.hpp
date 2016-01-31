#pragma once

#include <cstdint>
#include "..\..\inc\types.h"
#include "NativeMemory.hpp"

namespace VehExt {
	class VehicleExtensions {
	public:
		VehicleExtensions(Vehicle vehicle);
		
		uint64_t GetAddress();
		uint32_t GetGears();
		void SetGears(uint32_t value);
		float GetCurrentRPM();
		void SetCurrentRPM(float value);
		float GetClutch();
		void SetClutch(float value);
		float GetTurbo();
		void SetTurbo(float value);
		float GetThrottle();
		void SetThrottle(float value);

	private:
		MemoryAccess mem;
		Vehicle handle;

	};
}
