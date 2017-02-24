#include "MemoryPatcher.hpp"

#include <iomanip>
#include <sstream>
#include <Windows.h>
#include "../Util/Logger.hpp"

namespace MemoryPatcher {
	MemoryAccess mem;

	/* 
	 * "total" refers to the "package" of patches needed to get the gearbox
	 * and clutch stuff working.
	 */
	int total = 2;
	int patched = 0;

	bool steeringPatched = false;

	uintptr_t clutchLowAddr = 0;
	uintptr_t clutchLowTemp = 0;

	uintptr_t gear7A0Addr = 0;
	uintptr_t gear7A0Temp = 0;

	uintptr_t RevLimiterAddr = 0;
	uintptr_t RevLimiterTemp = 0;

	uintptr_t SteeringAddr = 0;
	uintptr_t SteeringTemp = 0;


	bool PatchInstructions() {
		Logger logger(GEARSLOGPATH);
		logger.Write("Patching instructions");

		clutchLowTemp = PatchClutchLow();
		if (clutchLowTemp) {
			clutchLowAddr = clutchLowTemp;
			patched++;
			std::stringstream hexaddr;
			hexaddr << std::hex << clutchLowAddr;
			logger.Write("clutchLow @ " + hexaddr.str());
		}
		else {
			logger.Write("clutchLow not patched");
		}

		gear7A0Temp = PatchGear7A0();
		if (gear7A0Temp) {
			gear7A0Addr = gear7A0Temp;
			patched++;
			std::stringstream hexaddr;
			hexaddr << std::hex << gear7A0Addr;
			logger.Write("gear 7A0  @ " + hexaddr.str());
		}
		else {
			logger.Write("gear 7A0  not patched");
		}

		if (patched == total) {
			logger.Write("Patching success");
			return true;
		}
		logger.Write("Patching failed");
		return false;
	}

	bool RestoreInstructions() {
		Logger logger(GEARSLOGPATH);
		logger.Write("Restoring instructions");

		if (clutchLowAddr) {
			RestoreClutchLow(clutchLowAddr);
			clutchLowAddr = 0;
			patched--;
		}
		else {
			logger.Write("clutchLow not restored");
		}

		if (gear7A0Addr) {
			RestoreGear7A0(gear7A0Addr);
			gear7A0Addr = 0;
			patched--;
		}
		else {
			logger.Write("Gear@7A0 not restored");
		}

		if (patched == 0) {
			logger.Write("Restore success");
			return true;
		}
		logger.Write("Restore failed");
		return false;
	}

	bool PatchSteeringCorrection() {
		Logger logger(GEARSLOGPATH);
		logger.Write("Patching instructions");

		SteeringTemp = PatchSteering();
		if (SteeringTemp) {
			SteeringAddr = SteeringTemp;
			steeringPatched = true;
			std::stringstream hexaddr;
			hexaddr << std::hex << SteeringAddr;
			logger.Write("Steering @ " + hexaddr.str());
			return true;
		}

		logger.Write("Steering not patched");
		return false;
	}

	bool RestoreSteeringCorrection() {
		Logger logger(GEARSLOGPATH);
		logger.Write("Restoring instructions");

		if (SteeringAddr) {
			RestoreSteering(SteeringAddr);
			SteeringAddr = 0;
			steeringPatched = false;
			return true;
		}

		logger.Write("Steering not restored");
		return false;
	}

	uintptr_t PatchClutchLow() {
		// Tested on build 350 and build 617
		// We're only interested in the first 7 bytes but we need the correct one
		// C7 43 40 CD CC CC 3D is what we're looking for, the second occurrence, at 
		// 7FF6555FE34A or GTA5.exe+ECE34A in build 617.
		uintptr_t address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx");

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
		uintptr_t address = mem.FindPattern("\x66\x89\x13\x89\x73\x5C", "xxxxxx");

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

		uintptr_t address = mem.FindPattern(
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
