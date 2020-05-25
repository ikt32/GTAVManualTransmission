#include "MemoryPatcher.hpp"

#include "Versions.h"
#include "../Util/Logger.hpp"

#include "PatternInfo.h"
#include "Patcher.h"

namespace MemoryPatcher {
const int NumGearboxPatches = 5;
int NumGearboxPatched = 0;

const int maxAttempts = 4;
int gearboxAttempts = 0;

bool Error = false;

// When disabled, shift-up doesn't trigger.
PatternInfo shiftUp;
Patcher ShiftUpPatcher("Gears: Shift Up", shiftUp, true);

// When disabled, shift-down doesn't trigger.
PatternInfo shiftDown;
Patcher ShiftDownPatcher("Gears: Shift Down", shiftDown, true);

// When disabled, clutch doesn't disengage on low RPMs
// (in the RPM region where shift-down would've been triggered)
PatternInfo clutchLow;
Patcher ClutchLowRPMPatcher("Gears: Clutch Low RPM", clutchLow, true);

// When disabled, clutch doesn't disengage on redline 
// (in the RPM region where shift-up would've been triggered)
PatternInfo clutchRevLimit;
Patcher ClutchRevLimPatcher("Gears: Clutch Rev lim", clutchRevLimit, true);

// When disabled, game doesn't lift throttle during shifts
// Also useful to set throttle in 2nd+ while in fakeNeutral/clutched in
PatternInfo throttleLift;
Patcher ThrottleLiftPatcher("Gears: Throttle lift", throttleLift, true);

// When disabled, throttle doesn't decrease.
PatternInfo throttle;
Patcher ThrottlePatcher("Misc: Throttle", throttle);

// When disabled, brake pressure doesn't decrease.
PatternInfo brake;
Patcher BrakePatcher("Misc: Brake", brake);

// Disables countersteer/steering assist
PatternInfo steeringAssist;
PatcherJmp SteeringAssistPatcher("Steer: Steering assist", steeringAssist, true);

// Disables user steering input for total script control
PatternInfo steeringControl;
Patcher SteeringControlPatcher("Steer: Steering input", steeringControl, true);

void SetPatterns(int version) {
    // Valid for 877 to 1290
    shiftUp = PatternInfo("\x66\x89\x13\xB8\x05\x00\x00\x00", "xxxxxxxx", 
        { 0x66, 0x89, 0x13 });
    shiftDown = PatternInfo("\x66\x89\x13\x89\x73\x5C", "xxxxxx", 
        { 0x66, 0x89, 0x13 });
    brake = PatternInfo("\xEB\x05\xF3\x0F\x10\x40\x78\xF3\x0F\x59\xC4\xF3", "xxxxxx?xxxxx", 
        { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }, 11);
    throttle = PatternInfo("\x0F\x28\xC3\x44\x89\x89\xC4\x01\x00\x00\x89\x81\xD0\x01\x00\x00", "xx?xxx??xxxx??xx", 
        { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }, 3);

    // Valid for 877 to 1493
    clutchLow = PatternInfo("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx", 
        { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D });
    clutchRevLimit = PatternInfo("\xC7\x43\x40\xCD\xCC\xCC\x3D\x44\x89\x7B\x60", "xxxxxxxxx?x", 
        { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D });
    steeringAssist = PatternInfo("\x0F\x84\xD0\x01\x00\x00" "\x0F\x28\x4B\x70" "\xF3\x0F\x10\x25\x00\x00\x00\x00" "\xF3\x0F\x10\x1D\x00\x00\x00\x00" "\x0F\x28\xC1" "\x0F\x28\xD1",
        "xx????" "xxx?" "xxx?????" "xxx?????" "xx?" "xx?", 
        { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90 });
    steeringControl = PatternInfo("\xF3\x0F\x11\x8B\xFC\x08\x00\x00" "\xF3\x0F\x10\x83\x00\x09\x00\x00" "\xF3\x0F\x58\x83\xFC\x08\x00\x00" "\x41\x0F\x2F\xC3",
        "xxxx??xx" "xxxx??xx" "xxxx??xx" "xxxx", 
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

    // Valid for 1604 (FiveM) to 1868+
    throttleLift = PatternInfo("\x44\x89\x77\x50\xf3\x0f\x11\x7d\x4f", "xxxxxxxx?",
        { 0x90, 0x90, 0x90, 0x90 });

    if (version >= G_VER_1_0_1365_1_STEAM) {
        shiftUp = PatternInfo("\x66\x89\x0B\x8D\x46\x04\x66\x89\x43\04", "xx?xx?xxx?", 
            { 0x66, 0x89, 0x0B });
        shiftDown = PatternInfo("\x66\x89\x13\x44\x89\x73\x5c", "xxxxxxx", 
            { 0x66, 0x89, 0x13 });
        brake = PatternInfo("\xEB\x05\xF3\x0F\x10\x40\x78\xF3\x41\x0F\x59\xC0\xF3", "xxxxx??x?x?xx", 
            { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 }, 12);
        throttle = PatternInfo("\x83\xA1\x00\x00\x00\x00\x00\x0F\x28\xC3\x89", "xx?????xxxx", 
            { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
    }
    if (version >= G_VER_1_0_1604_0_STEAM) {
        shiftDown = PatternInfo("\x66\x89\x13\x44\x89\x73\x68\xeb\x0a", "xxxxxx?xx",
            { 0x66, 0x89, 0x13 });
        clutchLow = PatternInfo("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xx?xxxxxxxxx",
            { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D });
        clutchRevLimit = PatternInfo("\xC7\x43\x40\xCD\xCC\xCC\x3D\x44\x89\x6B\x6C\x44\x89\x73\x68", "xx?xxxxxx??xx??",
            { 0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D });
        steeringAssist = PatternInfo("\x45\x84\xED\x0F\x84\xD0\x01\x00\x00\x0F\x28\x4B\x70"
            "\xF3\x0F\x10\x25\x00\x00\x00\x00\xF3\x0F\x10\x1D\x00\x00\x00\x00\x0F\x28\xC1\x0F\x28\xD1", 
            "xxxxx????xx??xxx?????xxx?????xx?xx?",
            { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90 }, 3);
    }
}

bool Test() {
    bool success = true;
    success &= 0 != ShiftUpPatcher.Test();
    success &= 0 != ShiftDownPatcher.Test();
    success &= 0 != ClutchLowRPMPatcher.Test();
    success &= 0 != ClutchRevLimPatcher.Test();
    success &= 0 != ThrottleLiftPatcher.Test();
    success &= 0 != ThrottlePatcher.Test();
    success &= 0 != BrakePatcher.Test();
    success &= 0 != SteeringAssistPatcher.Test();
    success &= 0 != SteeringControlPatcher.Test();
    return success;
}

bool ApplyGearboxPatches() {
    if (gearboxAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "[Patch] [Gears] Patching");

    if (NumGearboxPatches == NumGearboxPatched) {
        logger.Write(DEBUG, "[Patch] [Gears] Already patched");
        return true;
    }

    if (ClutchLowRPMPatcher.Patch()) {
        NumGearboxPatched++;
    }

    if (ClutchRevLimPatcher.Patch()) {
        NumGearboxPatched++;
    }

    if (ShiftDownPatcher.Patch()) {
        NumGearboxPatched++;
    }

    if (ShiftUpPatcher.Patch()) {
        NumGearboxPatched++;
    }

    if (ThrottleLiftPatcher.Patch()) {
        NumGearboxPatched++;
    }

    if (NumGearboxPatched == NumGearboxPatches) {
        logger.Write(DEBUG, "[Patch] [Gears] Patch success");
        gearboxAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "[Patch] [Gears] Patching failed");
    gearboxAttempts++;

    if (gearboxAttempts > maxAttempts) {
        logger.Write(ERROR, "[Patch] [Gears] Patch attempt limit exceeded");
        logger.Write(ERROR, "[Patch] [Gears] Patching disabled");
    }
    return false;
}

bool RevertGearboxPatches() {
    logger.Write(DEBUG, "[Patch] [Gears] Restoring instructions");

    if (NumGearboxPatched == 0) {
        logger.Write(DEBUG, "[Patch] [Gears] Already restored/intact");
        return true;
    }

    if (ClutchLowRPMPatcher.Restore()) {
        NumGearboxPatched--;
    }

    if (ClutchRevLimPatcher.Restore()) {
        NumGearboxPatched--;
    }

    if (ShiftDownPatcher.Restore()) {
        NumGearboxPatched--;
    }

    if (ShiftUpPatcher.Restore()) {
        NumGearboxPatched--;
    }

    if (ThrottleLiftPatcher.Restore()) {
        NumGearboxPatched--;
    }

    if (NumGearboxPatched == 0) {
        logger.Write(DEBUG, "[Patch] [Gears] Restore success");
        gearboxAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "[Patch] [Gears] Restore failed");
    return false;
}

bool PatchSteeringAssist() {
    return SteeringAssistPatcher.Patch();
}

bool RestoreSteeringAssist() {
    return SteeringAssistPatcher.Restore();
}

bool PatchSteeringControl() {
    return SteeringControlPatcher.Patch();
}

bool RestoreSteeringControl() {
    return SteeringControlPatcher.Restore();
}

bool PatchBrake() {
    return BrakePatcher.Patch();
}

bool RestoreBrake() {
    return BrakePatcher.Restore();
}

bool PatchThrottle() {
    return ThrottlePatcher.Patch();
}

bool RestoreThrottle() {
    return ThrottlePatcher.Restore();
}
}

// Steering assist/correction:
// GTA V (b791.2 to b1493.1)
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

// GTA V (b1604.0)
// 45 84 ED 
// 0F 84 D8 01 00 00 
// 0F 28 4F 60 
// F3 0F 10 1D A1 20 91 00 
// F3 0F 10 15 99 48 9D 00

