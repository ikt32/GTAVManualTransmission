#pragma once
#include "NativeMemory.hpp"

namespace MemoryPatcher {
	extern bool PatchInstructions();
	//extern bool PatchJustS_LOW();
	extern bool RestoreInstructions();
	//extern bool RestoreJustS_LOW();

	// Clutch disengage @ Low Speed High Gear, low RPM
	extern uintptr_t PatchClutchLow();
	extern void RestoreClutchLow(uintptr_t address);

	//// Clutch 0.1 @ Stationary - Don't do this
	//extern uintptr_t PatchClutchStationary01();
	//extern void RestoreClutchStationary01(uintptr_t address);
	//
	//// Clutch 0.4 @ Stationary - Don't do this
	//extern uintptr_t PatchClutchStationary04();
	//extern void RestoreClutchStationary04(uintptr_t address);
	//
	//// Throttle disengage at redline - Don't do this
	//extern uintptr_t PatchThrottleRedline();
	//extern void RestoreThrottleRedline(uintptr_t address);
	//
	//// Clutch small value @ full press
	//// Affects other vehicles! - Don't do this!!!
	//extern uintptr_t PatchClutchStationaryLow();
	//extern void RestoreClutchStationaryLow(uintptr_t address);

	// Individually: Disable "shifting down" wanted
	// 7A0 is NextGear, or what it appears like in my mod.
	extern uintptr_t PatchGear7A0();
	extern void RestoreGear7A0(uintptr_t address);

	// Disable 1st gear limiter. This will probably bite me in the ass.
	// Changes 7A0.
	// Also apparently it stops the gear from changing.
	// We don't want this at all
	extern uintptr_t PatchRevLimiter();
	extern void RestoreRevLimiter(uintptr_t address);

	extern int total;
	extern int patched;

	extern int t_S_LOW;
	extern int p_S_LOW;

	extern MemoryAccess mem;

	extern uintptr_t clutchLowAddr;
	extern uintptr_t clutchLowTemp;

	//extern uintptr_t clutchS01Addr;
	//extern uintptr_t clutchS01Temp;
	//
	//extern uintptr_t clutchS04Addr;
	//extern uintptr_t clutchS04Temp;
	//
	//extern uintptr_t throttleCutAddr;
	//extern uintptr_t throttleCutTemp;
	//
	//extern uintptr_t clutchStationaryLowAddr;
	//extern uintptr_t clutchStationaryLowTemp;

	extern uintptr_t gear7A0Addr;
	extern uintptr_t gear7A0Temp;

	extern uintptr_t RevLimiterAddr;
	extern uintptr_t RevLimiterTemp;
};
