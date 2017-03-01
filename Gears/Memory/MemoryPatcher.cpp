#include "MemoryPatcher.hpp"

#include <iomanip>
#include <sstream>
#include <Windows.h>
#include "../Util/Logger.hpp"
#include "NativeMemory.hpp"

namespace MemoryPatcher {

	// Clutch disengage @ Low Speed High Gear, low RPM
	uintptr_t PatchClutchLow();
	void RestoreClutchLow(uintptr_t address);

	// Individually: Disable "shifting down" wanted
	// 7A0 is NextGear, or what it appears like in my mod.
	uintptr_t PatchGear7A0();
	void RestoreGear7A0(uintptr_t address);

	// Does the same as Custom Steering by InfamousSabre
	// Kept for emergency/backup purposes in the case InfamousSabre
	// stops support earlier than I do. (not trying to compete here!)
	uintptr_t PatchSteering();
	void RestoreSteering(uintptr_t address);

	MemoryAccess mem;

	int TotalToPatch = 2;
	int TotalPatched = 0;

	bool SteeringPatched = false;

	uintptr_t clutchLowAddr = NULL;
	uintptr_t clutchLowTemp = NULL;

	uintptr_t gear7A0Addr = NULL;
	uintptr_t gear7A0Temp = NULL;

	uintptr_t SteeringAddr = NULL;
	uintptr_t SteeringTemp = NULL;


	bool PatchInstructions() {
		Logger logger(GEARSLOGPATH);
		logger.Write("GEARBOX: Patching");

		if (TotalToPatch == TotalPatched) {
			logger.Write("GEARBOX: Already patched");
			return true;
		}

		clutchLowTemp = PatchClutchLow();

		if (clutchLowTemp) {
			clutchLowAddr = clutchLowTemp;
			TotalPatched++;
			std::stringstream hexaddr;
			hexaddr << std::hex << clutchLowAddr;
			logger.Write("GEARBOX: Patched clutchLow @ " + hexaddr.str());
		}
		else {
			logger.Write("GEARBOX: clutchLow patch failed");
		}

		gear7A0Temp = PatchGear7A0();

		if (gear7A0Temp) {
			gear7A0Addr = gear7A0Temp;
			TotalPatched++;
			std::stringstream hexaddr;
			hexaddr << std::hex << gear7A0Addr;
			logger.Write("GEARBOX: Patched gear7A0  @ " + hexaddr.str());
		}
		else {
			logger.Write("GEARBOX: gear7A0 patch failed");
		}

		if (TotalPatched == TotalToPatch) {
			logger.Write("GEARBOX: Patch success");
			return true;
		}
		logger.Write("GEARBOX: Patching failed");
		return false;
	}

	bool RestoreInstructions() {
		Logger logger(GEARSLOGPATH);
		logger.Write("GEARBOX: Restoring instructions");

		if (TotalPatched == 0) {
			logger.Write("GEARBOX: Already restored/intact");
			return true;
		}

		if (clutchLowAddr) {
			RestoreClutchLow(clutchLowAddr);
			clutchLowAddr = 0;
			TotalPatched--;
		}
		else {
			logger.Write("GEARBOX: clutchLow not restored");
		}

		if (gear7A0Addr) {
			RestoreGear7A0(gear7A0Addr);
			gear7A0Addr = 0;
			TotalPatched--;
		}
		else {
			logger.Write("GEARBOX: gear@7A0 not restored");
		}

		if (TotalPatched == 0) {
			logger.Write("GEARBOX: Restore success");
			return true;
		}
		logger.Write("GEARBOX: Restore failed");
		return false;
	}

	bool PatchSteeringCorrection() {
		Logger logger(GEARSLOGPATH);
		logger.Write("STEERING: Patching");

		if (SteeringPatched) {
			logger.Write("STEERING: Already patched");
			return true;
		}

		SteeringTemp = PatchSteering();

		if (SteeringTemp) {
			SteeringAddr = SteeringTemp;
			SteeringPatched = true;
			std::stringstream hexaddr;
			hexaddr << std::hex << SteeringAddr;
			logger.Write("STEERING: Steering @ " + hexaddr.str());
			logger.Write("STEERING: Patch success");
			return true;
		}

		logger.Write("STEERING: Patch failed");
		return false;
	}

	bool RestoreSteeringCorrection() {
		Logger logger(GEARSLOGPATH);
		logger.Write("STEERING: Restoring instructions");

		if (!SteeringPatched) {
			logger.Write("STEERING: Already restored/intact");
			return true;
		}

		if (SteeringAddr) {
			RestoreSteering(SteeringAddr);
			SteeringAddr = 0;
			SteeringPatched = false;
			logger.Write("STEERING: Restore success");

			return true;
		}

		logger.Write("STEERING: Restore failed");
		return false;
	}

	uintptr_t PatchClutchLow() {
		// Tested on build 350 and build 617
		// We're only interested in the first 7 bytes but we need the correct one
		// C7 43 40 CD CC CC 3D is what we're looking for, the second occurrence, at 
		// 7FF6555FE34A or GTA5.exe+ECE34A in build 617.

		uintptr_t address = NULL;
		if (clutchLowTemp != NULL)
			address = clutchLowTemp;
		else
			address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx");

		if (address) {
			memset(reinterpret_cast<void *>(address), 0x90, 7);
		}
		return address;
	}

	void RestoreClutchLow(uintptr_t address) {
		byte instrArr[7] = {0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D};
		if (address) {
			for (int i = 0; i < 7; i++) {
				memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
			}
		}
	}

	uintptr_t PatchGear7A0() {
		// 66 89 13 <- Looking for this
		// 89 73 5C <- Next instruction
		// EB 0A    <- Next next instruction
		uintptr_t address = NULL;
		if (gear7A0Temp != 0)
			address = gear7A0Temp;
		else 
			address = mem.FindPattern("\x66\x89\x13\x89\x73\x5C", "xxxxxx");

		if (address) {
			memset(reinterpret_cast<void *>(address), 0x90, 3);
		}
		return address;
	}

	void RestoreGear7A0(uintptr_t address) {
		byte instrArr[3] = {0x66, 0x89, 0x13};
		if (address) {
			for (int i = 0; i < 3; i++) {
				memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
			}
		}
	}

	uintptr_t PatchSteering() {
		// in 791.2, 877.1 and 944.2
		// 0F 84 D0 01 00 00 = JE	[some address]
		// becomes
		// E9 D1 01 00 00 90 = JMP	[some address]

		// 0F 28 4B 70 (Next)
		// F3 0F10 25 06F09300
		// F3 0F10 1D 3ED39F00

		uintptr_t address = NULL;
		if (SteeringTemp != NULL)
			address = SteeringTemp;
		else 
			address = mem.FindPattern(
			"\x0F\x84\x00\x00\x00\x00\x0F\x28\x4B\x70\xF3\x0F\x10\x25\x00\x00\x00\x00\xF3\x0F\x10\x1D\x00\x00\x00\x00", 
			"xx????xxxxxxxx????xxxx????");

		if (address) {
			uint8_t offset = 0;
			address = address + offset;
			byte instrArr[6] = { 0xE9, 0xD1, 0x01, 0x00, 0x00, 0x90 };
			for (int i = 0; i < 6; i++) {
				memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
			}
			return address;
		}
		return 0;
	}

	void RestoreSteering(uintptr_t address) {
		byte instrArr[6] = { 0x0F, 0x84, 0xD0, 0x01, 0x00, 0x00 };
		if (address) {
			for (int i = 0; i < 6; i++) {
				memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
			}
		}
	}
}
