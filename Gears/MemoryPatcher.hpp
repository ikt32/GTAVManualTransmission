#pragma once
#include "NativeMemory.hpp"

namespace MemoryPatcher
{
	extern bool PatchInstructions();
	extern bool PatchJustS_LOW();
	extern bool RestoreInstructions();
	extern bool RestoreJustS_LOW();
	
	// Clutch disengage @ Low Speed High Gear 
	extern uintptr_t PatchClutchLow();
	extern void RestoreClutchLow(uintptr_t address);

	// Clutch 0.1 @ Stationary
	extern uintptr_t PatchClutchStationary01();
	extern void RestoreClutchStationary01(uintptr_t address);

	// Clutch 0.4 @ Stationary
	extern uintptr_t PatchClutchStationary04();
	extern void RestoreClutchStationary04(uintptr_t address);

	// Throttle disengage at redline
	extern uintptr_t PatchThrottleRedline();
	extern void RestoreThrottleRedline(uintptr_t address);

	// Clutch small value @ full press
	extern uintptr_t PatchClutchStationaryLow();
	extern void RestoreClutchStationaryLow(uintptr_t address);

	// Keep Clutch disengage at redline

	// 5 things need to be patched
	// How do I make this less crap?
	extern int total;
	extern int patched;

	extern int t_S_LOW;
	extern int p_S_LOW;

	extern MemoryAccess mem;

	extern uintptr_t clutchLowAddr;
	extern uintptr_t clutchLowTemp;

	extern uintptr_t clutchS01Addr;
	extern uintptr_t clutchS01Temp;

	extern uintptr_t clutchS04Addr;
	extern uintptr_t clutchS04Temp;

	extern uintptr_t throttleCutAddr;
	extern uintptr_t throttleCutTemp;

	extern uintptr_t clutchStationaryLowAddr;
	extern uintptr_t clutchStationaryLowTemp;
};

