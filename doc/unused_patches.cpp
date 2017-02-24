/*
 * This pseudo-code file is part of the Manual Transmission mod.
 * It is used to dump unused patches, patches which ended up being
 * unnecessary. For research purposes and the sole reason that it's
 * just a shame to throw away, this will be their graveyard.
 * 
 * Also it really makes MemoryPatcher.cpp less horrendous! :)
 * 
 * The original structure will be kept.
 */
 
#include "MemoryPatcher.hpp"

#include <iomanip>
#include <sstream>
#include <Windows.h>
#include "../Util/Logger.hpp"

namespace MemoryPatcher {

	/*
	 * Disable 1st gear limiter. This will probably bite me in the ass.
	 * Changes 7A0.
	 * Also apparently it stops the gear from changing.
	 * We don't want this at all
	 */
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

	/*
	 * When stopped, the clutch is @ 0.1. This disables that behavior.
	 */
	uintptr_t PatchClutchStationary01() {
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

	/*
	 * When stopped with the 0.1 clutch patch applied, the clutch will be 0.4. This disables that behavior.
	 */
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

	/*
	 * When redlining, the the acceleration stops. This disables that behavior.
	 */
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

	/*
	 * Uh, I can't remember what this does exactly.
	 * I do remember that it fucks up the clutch of all AI vehicles when active.
	 * It probably has to do with the clutch not being at 0 when it is set so, which
	 * this patch probably disables. Looks good in numbers and for control but fucks upper_bound
	 * the rest of the game.
	 */
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
	}
}
