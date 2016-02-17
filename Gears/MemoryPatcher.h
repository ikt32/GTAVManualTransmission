#pragma once

#include "NativeMemory.hpp"
#include "Logger.h"

class MemoryPatcher
{
public:
	static bool PatchInstructions();
	static bool RestoreInstructions();
	
private:
	// Clutch disengage @ Low Speed High Gear 
	static uintptr_t PatchClutchLow();
	static void RestoreClutchLow(uintptr_t address);

	// Clutch 0.1 @ Stationary
	static uintptr_t PatchClutchStationary01();
	static void RestoreClutchStationary01(uintptr_t address);

	// Clutch 0.4 @ Stationary
	static uintptr_t PatchClutchStationary04();
	static void RestoreClutchStationary04(uintptr_t address);

	// Throttle disengage at redline
	static uintptr_t PatchThrottleRedline();
	static void RestoreThrottleRedline(uintptr_t address);

	// Clutch small value @ full press
	static uintptr_t PatchClutchStationaryLow();
	static void RestoreClutchStationaryLow(uintptr_t address);

	// Keep Clutch disengage at redline

	// 5 things need to be patched
	// How do I make this less crap?
	static int total;
	static int patched;

	static MemoryAccess mem;

	static uintptr_t clutchLowAddr;
	static uintptr_t clutchLowTemp;

	static uintptr_t clutchS01Addr;
	static uintptr_t clutchS01Temp;

	static uintptr_t clutchS04Addr;
	static uintptr_t clutchS04Temp;

	static uintptr_t throttleCutAddr;
	static uintptr_t throttleCutTemp;

	static uintptr_t clutchStationaryLowAddr;
	static uintptr_t clutchStationaryLowTemp;
};

