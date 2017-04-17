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
	void RestoreSteering(uintptr_t address, byte *origInstr, int origInstrSz);

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

	byte origSteerInstr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	byte origSteerInstrDest[4] = {0x00, 0x00, 0x00, 0x00};

	const int maxTries = 4;
	int gearTries = 0;
	int steerTries = 0;

	bool PatchInstructions() {
		if (gearTries > maxTries) {
			return false;
		}

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
			gearTries = 0;
			return true;
		}
		logger.Write("GEARBOX: Patching failed");
		gearTries++;

		if (gearTries > maxTries) {
			logger.Write("GEARBOX: Patch attempt limit exceeded");
			logger.Write("GEARBOX: Patching disabled");
		}
		return false;
	}

	bool RestoreInstructions() {
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
			gearTries = 0;
			return true;
		}
		logger.Write("GEARBOX: Restore failed");
		return false;
	}

	bool PatchSteeringCorrection() {
		if (steerTries > maxTries) {
			return false;
		}

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
			std::stringstream instructs;
			for (int i = 0; i < sizeof(origSteerInstr) / sizeof(byte); ++i)
				instructs << std::hex << std::setfill('0') << std::setw(2) << (int)origSteerInstr[i] << " ";
			logger.Write("STEERING: Steering @ " + hexaddr.str());
			logger.Write("STEERING: Patch success, orig: " + instructs.str());
			steerTries = 0;
			return true;
		}

		logger.Write("STEERING: Patch failed");
		steerTries++;

		if (steerTries > maxTries) {
			logger.Write("STEERING: Patch attempt limit exceeded");
			logger.Write("STEERING: Patching disabled");
		}
		return false;
	}

	bool RestoreSteeringCorrection() {
		logger.Write("STEERING: Restoring instructions");

		if (!SteeringPatched) {
			logger.Write("STEERING: Already restored/intact");
			return true;
		}

		if (SteeringAddr) {
			RestoreSteering(SteeringAddr,origSteerInstr, sizeof(origSteerInstr) / sizeof(byte));
			SteeringAddr = 0;
			SteeringPatched = false;
			logger.Write("STEERING: Restore success");
			steerTries = 0;
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

		uintptr_t address;
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
		uintptr_t address;
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
		// GTA V (b791.2 to b1011.1)
		// F3 44 0F10 15 03319400
		// 45 84 ED				// test		r13l,r13l
		// 0F84 C8010000		// je		GTA5.exe+EXE53E
		// 0F28 4B 70 (Next)	// movaps	xmm1,[rbx+70]
		// F3 0F10 25 06F09300
		// F3 0F10 1D 3ED39F00
		
		// in 791.2, 877.1 and 944.2
		// 0F 84 D0010000 = JE	<address>
		// becomes
		// E9 D1 01000090 = JMP	<address> NOP

		// InfamousSabre
		// "\x0F\x84\x00\x00\x00\x00\x0F\x28\x4B\x70\xF3\x0F\x10\x25\x00\x00\x00\x00\xF3\x0F\x10\x1D\x00\x00\x00\x00" is what I scan for.

		// FiveM (b505.2)
		// F3 44 0F10 15 03319400
		// 45 84 ED             
		// 0F84 D0010000        // <- This one
		// 0F28 4B 70           
		// F3 0F10 25 92309400  
		// F3 0F10 1D 6A309400  

		uintptr_t address;
		if (SteeringTemp != NULL) {
			address = SteeringTemp;

		}
		else {
			address = mem.FindPattern(
				"\x0F\x84\xD0\x01\x00\x00" // <- This one
				"\x0F\x28\x4B\x70"
				"\xF3\x0F\x10\x25\x00\x00\x00\x00"
				"\xF3\x0F\x10\x1D\x00\x00\x00\x00"
				"\x0F\x28\xC1"
				"\x0F\x28\xD1",
				"xx????" // <- This one
				"xx??"
				"xxxx????"
				"xxxx????"
				"xx?"
				"xx?");
		}

		if (address) {
			byte instrArr[6] = { 0xE9, 0xD1, 0x01, 0x00, 0x00, 0x90 };	// make preliminary instruction: JMP to <adrr>

			memcpy(origSteerInstr, (void*)address, 6);					// save whole orig instruction
			memcpy(origSteerInstrDest, (void*)(address + 2), 4);		// save the address it writes to
			origSteerInstrDest[0] += 1;									// Increment first address by 1
			memcpy(instrArr + 1, origSteerInstrDest, 4);				// use saved address in new instruction
			memcpy((void*)address, instrArr, 6);						// patch with new fixed instruction

			return address;
		}
		return 0;
	}

	void RestoreSteering(uintptr_t address, byte *origInstr, int origInstrSz) {
		if (address) {
			memcpy((void*)address, origInstr, origInstrSz);
		}
	}
}
