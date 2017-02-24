#pragma once
#include "NativeMemory.hpp"

namespace MemoryPatcher {
	extern bool PatchInstructions();
	extern bool RestoreInstructions();

	// Clutch disengage @ Low Speed High Gear, low RPM
	extern uintptr_t PatchClutchLow();
	extern void RestoreClutchLow(uintptr_t address);

	// Individually: Disable "shifting down" wanted
	// 7A0 is NextGear, or what it appears like in my mod.
	extern uintptr_t PatchGear7A0();
	extern void RestoreGear7A0(uintptr_t address);

	// Does the same as Custom Steering by InfamousSabre
	// Kept for emergency/backup purposes in the case InfamousSabre
	// stops support earlier than I do. (not trying to compete here!)
	extern uintptr_t PatchSteering();
	extern void RestoreSteering(uintptr_t address);

	extern int total;
	extern int patched;

	extern MemoryAccess mem;

	extern uintptr_t clutchLowAddr;
	extern uintptr_t clutchLowTemp;

	extern uintptr_t gear7A0Addr;
	extern uintptr_t gear7A0Temp;

	extern uintptr_t SteeringAddr;
	extern uintptr_t SteeringTemp;
};
