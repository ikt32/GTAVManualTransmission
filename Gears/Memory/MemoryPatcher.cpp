#include "MemoryPatcher.hpp"

#include <iomanip>
#include <sstream>
#include <Windows.h>
#include "../Util/Logger.hpp"

namespace MemoryPatcher {
	int total = 2;
	int patched = 0;

	//int t_S_LOW = 0;
	//int p_S_LOW = 0;

	MemoryAccess mem;

	uintptr_t clutchLowAddr = 0;
	uintptr_t clutchLowTemp = 0;

	//uintptr_t clutchS01Addr = 0;
	//uintptr_t clutchS01Temp = 0;
	//
	//uintptr_t clutchS04Addr = 0;
	//uintptr_t clutchS04Temp = 0;
	//
	//uintptr_t throttleCutAddr = 0;
	//uintptr_t throttleCutTemp = 0;
	//
	//uintptr_t clutchStationaryLowAddr = 0;
	//uintptr_t clutchStationaryLowTemp = 0;

	uintptr_t gear7A0Addr = 0;
	uintptr_t gear7A0Temp = 0;

	uintptr_t RevLimiterAddr = 0;
	uintptr_t RevLimiterTemp = 0;

	//uintptr_t SteeringAddr = 0;
	//uintptr_t SteeringTemp = 0;


	bool PatchInstructions() {
		Logger logger(LOGFILE);
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

		//RevLimiterTemp = PatchRevLimiter();
		//if (RevLimiterTemp) {
		//	RevLimiterAddr = RevLimiterTemp;
		//	patched++;
		//	std::stringstream hexaddr;
		//	hexaddr << std::hex << RevLimiterAddr;
		//	logger.Write("RevLimiter @ " + hexaddr.str());
		//}
		//else {
		//	logger.Write("RevLimiter patch failed!");
		//}


		///////////////////////////////////////////////////

		//clutchS01Temp = PatchClutchStationary01();
		//if (clutchS01Temp && !clutchS01Addr) {
		//	clutchS01Addr = clutchS01Temp;
		//	patched++;
		//	std::stringstream hexaddr;
		//	hexaddr << std::hex << clutchS01Addr;
		//	logger.Write("clutchS01 @ " + hexaddr.str());
		//}
		//else {
		//	logger.Write("Clutch_Stationary_01 not patched");
		//}
		//
		//clutchS04Temp = PatchClutchStationary04();
		//if (clutchS04Temp) {
		//	clutchS04Addr = clutchS04Temp;
		//	patched++;
		//	std::stringstream hexaddr;
		//	hexaddr << std::hex << clutchS04Addr;
		//	logger.Write("clutchS04 @ " + hexaddr.str());
		//}
		//else {
		//	logger.Write("Clutch_Stationary_04 not patched");
		//}


		//throttleCutTemp = PatchThrottleRedline();
		//if (throttleCutTemp) {
		//	throttleCutAddr = throttleCutTemp;
		//	patched++;
		//	std::stringstream hexaddr;
		//	hexaddr << std::hex << throttleCutAddr;
		//	logger.Write("throttleC @ " + hexaddr.str());
		//}
		//else {
		//	logger.Write("Throttle_Redline not patched");
		//}

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

		/*SteeringTemp = PatchSteering();
		if (SteeringTemp) {
			SteeringAddr = SteeringTemp;
			patched++;
			std::stringstream hexaddr;
			hexaddr << std::hex << SteeringAddr;
			logger.Write("Steering @ " + hexaddr.str());
		} else {
			logger.Write("Steering  not patched");
		}*/

		if (patched == total) {
			logger.Write("Patching success");
			return true;
		}
		logger.Write("Patching failed");
		return false;
	}

	bool RestoreInstructions() {
		Logger logger(LOGFILE);
		logger.Write("Restoring instructions");

		if (clutchLowAddr) {
			RestoreClutchLow(clutchLowAddr);
			clutchLowAddr = 0;
			patched--;
		}
		else {
			logger.Write("clutchLow not restored");
		}

		//if (RevLimiterAddr) {
		//	RestoreRevLimiter(RevLimiterAddr);
		//	RevLimiterAddr = 0;
		//	patched--;
		//}
		//else {
		//	logger.Write("RevLimiter not restored");
		//}

		//if (clutchS01Addr) {
		//	RestoreClutchStationary01(clutchS01Addr);
		//	clutchS01Addr = 0;
		//	patched--;
		//}
		//else {
		//	logger.Write("0.1 not restored");
		//}
		//
		//if (clutchS04Addr) {
		//	RestoreClutchStationary04(clutchS04Addr);
		//	clutchS04Addr = 0;
		//	patched--;
		//}
		//else {
		//	logger.Write("0.4 not restored");
		//}

		//if (throttleCutAddr) {
		//	RestoreThrottleRedline(throttleCutAddr);
		//	throttleCutAddr = 0;
		//	patched--;
		//}
		//else {
		//	logger.Write("Throttle not restored");
		//}

		if (gear7A0Addr) {
			RestoreGear7A0(gear7A0Addr);
			gear7A0Addr = 0;
			patched--;
		}
		else {
			logger.Write("Gear@7A0 not restored");
		}

		//if (SteeringAddr) {
		//	RestoreSteering(SteeringAddr);
		//	SteeringAddr = 0;
		//	patched--;
		//}
		//else {
		//	logger.Write("Steering not restored");
		//} // eh



		if (patched == 0) {
			logger.Write("Restore success");
			return true;
		}
		logger.Write("Restore failed");
		return false;
	}

	/*bool PatchJustS_LOW() {
		Logger logger(LOGFILE);
		clutchStationaryLowTemp = PatchClutchStationaryLow();
		//if (clutchStationaryLowTemp) {
		//	clutchStationaryLowAddr = clutchStationaryLowTemp;
		//	p_S_LOW++;
		//	std::stringstream hexaddr;
		//	hexaddr << std::hex << clutchStationaryLowAddr;
		//	logger.Write("clutchS_L @ " + hexaddr.str());
		//}
		//else {
		//	logger.Write("Clutch_Stationary_Low not patched");
		//}
		if (p_S_LOW == t_S_LOW) {
			logger.Write("Patching ClutchSpecial success");
			return true;
		}
		else {
			logger.Write("Patching ClutchSpecial failed");
			return false;
		}
	}

	bool RestoreJustS_LOW() {
		Logger logger(LOGFILE);
		//if (clutchStationaryLowAddr) {
		//	RestoreClutchStationaryLow(clutchStationaryLowAddr);
		//	clutchStationaryLowAddr = 0;
		//	p_S_LOW--;
		//}
		//else {
		//	logger.Write("S_Low not restored");
		//}

		if (p_S_LOW == 0) {
			logger.Write("Restore ClutchSpecial success");
			return true;
		}
		else {
			logger.Write("Restore ClutchSpecial failed");
			return false;
		}
	}*/

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

	uintptr_t PatchRevLimiter() {
		// 44 89 7B 60 is prevprev
		// 89 73 5C is previous instruction
		// 66 89 13 is what we're looking for.
		
		uintptr_t address = mem.FindPattern("\x44\x89\x7B\x60\x89\x73\x5C\x66\x89\x13", "xxxxxxxxxx");

		if (address) {
			uint8_t offset = 7;
			memset(reinterpret_cast<void *>(address + offset), 0x90, 3);
			return (address + offset);
		}
		return 0;
	}

	void RestoreRevLimiter(uintptr_t address) {
		byte instrArr[3] = {0x66, 0x89, 0x13};
		if (address) {
			for (int i = 0; i < 3; i++) {
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

	/*uintptr_t PatchClutchStationary01() {
		// Also looking for C7 43 40 CD CC CC 3D
		// This pattern also works on 350 and 617
		uintptr_t address = mem.FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\xE9\xF6\x04\x00\x00", "xxxxxxxx????");

		if (address) {
			memset((void *)address, 0x90, 7);
		}
		return address;
	}
	void RestoreClutchStationary01(uintptr_t address) {
		byte instrArr[7] = { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D };
		if (address) {
			for (int i = 0; i < 7; i++) {
				memset((void *)(address + i), instrArr[i], 1);
			}
		}
	}

	uintptr_t PatchClutchStationary04() {
		// Looking for F3 0F 11 47 40
		// This pattern works on 350 and 617
		uintptr_t address = mem.FindPattern("\xF3\x0F\x11\x47\x40\xF3\x0F\x59\x3D\xDA\x9C\x8E\x00", "xxxxxxxxx????");

		if (address) {
			memset((void *)address, 0x90, 5);
		}
		return address;
	}
	void RestoreClutchStationary04(uintptr_t address) {
		byte instrArr[5] = { 0xF3, 0x0F, 0x11, 0x47, 0x40 };
		if (address) {
			for (int i = 0; i < 5; i++) {
				memset((void *)(address + i), instrArr[i], 1);
			}
		}
	}

	uintptr_t PatchThrottleRedline() {
		// Looking for 44 89 71 44
		uintptr_t address = mem.FindPattern("\x44\x89\x71\x44\xF3\x0F\x11\x75\x47", "xxxxxxx??");
	
		if (address) {
			memset((void *)address, 0x90, 4);
		}
		return address;
	}
	void RestoreThrottleRedline(uintptr_t address) {
		byte instrArr[4] = { 0x44, 0x89, 0x71, 0x44 };
		if (address) {
			for (int i = 0; i < 4; i++) {
				memset((void *)(address + i), instrArr[i], 1);
			}
		}
	}

	uintptr_t PatchClutchStationaryLow() {
		// F3 0F 11 43 40 <- Looking for this
		// 0F B7 43 04    <- Next instruction
		uintptr_t address = mem.FindPattern("\xF3\x0F\x11\x43\x40\x0F\xB7\x43\x04", "xxxxxxx??");

		if (address) {
			memset((void *)address, 0x90, 5);
		}
		return address;
	}
	void RestoreClutchStationaryLow(uintptr_t address) {
		byte instrArr[5] = { 0xF3, 0x0F, 0x11, 0x43, 0x40 };
		if (address) {
			for (int i = 0; i < 5; i++) {
				memset((void *)(address + i), instrArr[i], 1);
			}
		}
	}*/
	//uintptr_t PatchSteering() {
	//	// in 791_2
	//	// 45 84 E4	(Prev)

	//	// 0F 84 D0 01 00 00 = JE	[some address]
	//	// becomes
	//	// E9 D1 01 00 00 90 = JMP	[some address]

	//	// 0F 28 4B 70 (Next)
	//	uintptr_t address = mem.FindPattern("\x45\x84\xE4\x0F\x84\xD0\x01\x00\x00\x0F\x28\x4B\x70", "???xxxxxxxx??");

	//	if (address) {
	//		uint8_t offset = 3;
	//		address = address + offset;
	//		byte instrArr[6] = { 0xE9, 0xD1, 0x01, 0x00, 0x00, 0x90 };
	//		for (int i = 0; i < 6; i++) {
	//			memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
	//		}
	//		return address;
	//	}
	//	return 0;
	//}

	//void RestoreSteering(uintptr_t address) {
	//	byte instrArr[6] = { 0x0F, 0x84, 0xD0, 0x01, 0x00, 0x00 };
	//	if (address) {
	//		for (int i = 0; i < 6; i++) {
	//			memset(reinterpret_cast<void *>(address + i), instrArr[i], 1);
	//		}
	//	}
	//}
}
