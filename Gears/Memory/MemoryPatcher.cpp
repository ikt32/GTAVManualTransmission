#include "MemoryPatcher.hpp"
#include <utility>

#include "NativeMemory.hpp"
#include "Versions.h"
#include "../Util/Logger.hpp"
#include "../Util/Util.hpp"

#include "PatternInfo.h"
#include "Patcher.h"

namespace MemoryPatcher {
// Does the same as Custom Steering by InfamousSabre
uintptr_t ApplySteeringCorrectionPatch();

// Disable (normal) user input while wheel steering is active.
uintptr_t ApplySteerControlPatch();

// When disabled, throttle doesn't decrease.
uintptr_t ApplyThrottlePatch();

int NumGearboxPatches = 4;
int TotalPatched = 0;

bool SteerCorrectPatched = false;
bool SteerControlPatched = false;
bool ThrottleDecrementPatched = false;

uintptr_t steeringAddr = NULL;
uintptr_t steeringTemp = NULL;

uintptr_t steerControlAddr = NULL;
uintptr_t steerControlTemp = NULL;

uintptr_t throttleAddr = NULL;
uintptr_t throttleTemp = NULL;

const int maxAttempts = 4;
int gearboxAttempts = 0;
int steerAttempts = 0;
int steerControlAttempts = 0;
int throttleAttempts = 0;

// When disabled, shift-up doesn't trigger.
PatternInfo shiftUp;
Patcher ShiftUpPatcher("[Gears] Shift Up", shiftUp, true);

// When disabled, shift-down doesn't trigger.
PatternInfo shiftDown;
Patcher ShiftDownPatcher("[Gears] Shift Down", shiftDown, true);

// When disabled, clutch doesn't disengage on low RPMs
// (in the RPM region where shift-down would've been triggered)
PatternInfo clutchLow;
Patcher ClutchLowRPMPatcher("[Gears] Clutch Low RPM", clutchLow, true);

// When disabled, clutch doesn't disengage on redline 
// (in the RPM region where shift-up would've been triggered)
PatternInfo clutchRevLimit;
Patcher ClutchRevLimPatcher("[Gears] Clutch Rev lim", clutchRevLimit, true);

PatternInfo steeringCorrection;
PatternInfo steeringControl;

PatternInfo throttle;
Patcher ThrottlePatcher("[Misc] Throttle", throttle);

// When disabled, brake pressure doesn't decrease.
PatternInfo brake;
Patcher BrakePatcher("[Misc] Brake", brake);

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
    steeringCorrection = PatternInfo("\x0F\x84\xD0\x01\x00\x00" "\x0F\x28\x4B\x70" "\xF3\x0F\x10\x25\x00\x00\x00\x00" "\xF3\x0F\x10\x1D\x00\x00\x00\x00" "\x0F\x28\xC1" "\x0F\x28\xD1",
        "xx????" "xxx?" "xxx?????" "xxx?????" "xx?" "xx?", 
        { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90 });
    steeringControl = PatternInfo("\xF3\x0F\x11\x8B\xFC\x08\x00\x00" "\xF3\x0F\x10\x83\x00\x09\x00\x00" "\xF3\x0F\x58\x83\xFC\x08\x00\x00" "\x41\x0F\x2F\xC3",
        "xxxx??xx" "xxxx??xx" "xxxx??xx" "xxxx", 
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

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
        //shiftUp
        //shiftDown
        //brake
        //throttle
        //clutchLow
        //clutchRevLimit
        //steeringCorrection
        //steeringControl
    }
}

bool ApplyGearboxPatches() {
    if (gearboxAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "PATCH: GEARBOX: Patching");

    if (NumGearboxPatches == TotalPatched) {
        logger.Write(DEBUG, "PATCH: GEARBOX: Already patched");
        return true;
    }

    if (ClutchLowRPMPatcher.Patch()) {
        TotalPatched++;
    }

    if (ClutchRevLimPatcher.Patch()) {
        TotalPatched++;
    }

    if (ShiftDownPatcher.Patch()) {
        TotalPatched++;
    }

    if (ShiftUpPatcher.Patch()) {
        TotalPatched++;
    }

    if (TotalPatched == NumGearboxPatches) {
        logger.Write(DEBUG, "PATCH: GEARBOX: Patch success");
        gearboxAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "PATCH: GEARBOX: Patching failed");
    gearboxAttempts++;

    if (gearboxAttempts > maxAttempts) {
        logger.Write(ERROR, "PATCH: GEARBOX: Patch attempt limit exceeded");
        logger.Write(ERROR, "PATCH: GEARBOX: Patching disabled");
    }
    return false;
}

bool RevertGearboxPatches() {
    logger.Write(DEBUG, "PATCH: GEARBOX: Restoring instructions");

    if (TotalPatched == 0) {
        logger.Write(DEBUG, "PATCH: GEARBOX: Already restored/intact");
        return true;
    }

    if (ClutchLowRPMPatcher.Restore()) {
        TotalPatched--;
    }

    if (ClutchRevLimPatcher.Restore()) {
        TotalPatched--;
    }

    if (ShiftDownPatcher.Restore()) {
        TotalPatched--;
    }

    if (ShiftUpPatcher.Restore()) {
        TotalPatched--;
    }

    if (TotalPatched == 0) {
        logger.Write(DEBUG, "PATCH: GEARBOX: Restore success");
        gearboxAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "PATCH: GEARBOX: Restore failed");
    return false;
}

bool PatchSteeringCorrection() {
    if (steerAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "PATCH: STEERING: Patching");

    if (SteerCorrectPatched) {
        logger.Write(DEBUG, "PATCH: STEERING: Already patched");
        return true;
    }

    steeringTemp = ApplySteeringCorrectionPatch();

    if (steeringTemp) {
        steeringAddr = steeringTemp;
        SteerCorrectPatched = true;

        std::string instructionBytes = ByteArrayToString(steeringCorrection.Data.data(), steeringCorrection.Data.size());

        logger.Write(DEBUG, "PATCH: STEERING: Steering Correction @ 0x%p", steeringAddr);
        logger.Write(DEBUG, "PATCH: STEERING: Patch success, original: " + instructionBytes);
        steerAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: STEERING: Patch failed");
    steerAttempts++;

    if (steerAttempts > maxAttempts) {
        logger.Write(ERROR, "PATCH: STEERING: Patch attempt limit exceeded");
        logger.Write(ERROR, "PATCH: STEERING: Patching disabled");
    }
    return false;
}

bool RestoreSteeringCorrection() {
    logger.Write(DEBUG, "PATCH: STEERING: Restoring instructions");

    if (!SteerCorrectPatched) {
        logger.Write(DEBUG, "PATCH: STEERING: Already restored/intact");
        return true;
    }

    if (steeringAddr) {
        memcpy((void*)steeringAddr, steeringCorrection.Data.data(), steeringCorrection.Data.size());
        steeringAddr = 0;
        SteerCorrectPatched = false;
        logger.Write(DEBUG, "PATCH: STEERING: Restore success");
        steerAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: STEERING: Restore failed");
    return false;
}

bool PatchSteeringControl() {
    if (steerControlAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "PATCH: STEERING CONTROL: Patching");

    if (SteerControlPatched) {
        logger.Write(DEBUG, "PATCH: STEERING CONTROL: Already patched");
        return true;
    }

    steerControlTemp = ApplySteerControlPatch();

    if (steerControlTemp) {
        steerControlAddr = steerControlTemp;
        SteerControlPatched = true;

        std::string instructionBytes = ByteArrayToString(steeringControl.Data.data(), steeringControl.Data.size());
        
        logger.Write(DEBUG, "PATCH: STEERING CONTROL: Steering Control @ 0x%p", steerControlAddr);
        logger.Write(DEBUG, "PATCH: STEERING CONTROL: Patch success, original : " + instructionBytes);
        steerControlAttempts = 0;

        return true;
    }

    logger.Write(ERROR, "PATCH: STEERING CONTROL: Patch failed");
    steerControlAttempts++;

    if (steerControlAttempts > maxAttempts) {
        logger.Write(ERROR, "PATCH: STEERING CONTROL: Patch attempt limit exceeded");
        logger.Write(ERROR, "PATCH: STEERING CONTROL: Patching disabled");
    }
    return false;
}

bool RestoreSteeringControl() {
    logger.Write(DEBUG, "PATCH: STEERING CONTROL: Restoring instructions");

    if (!SteerControlPatched) {
        logger.Write(DEBUG, "PATCH: STEERING CONTROL: Already restored/intact");
        return true;
    }

    if (steerControlAddr) {
        memcpy((void*)steerControlAddr, steeringControl.Data.data(), steeringControl.Data.size());
        steerControlAddr = 0;
        SteerControlPatched = false;
        logger.Write(DEBUG, "PATCH: STEERING CONTROL: Restore success");
        steerControlAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: STEERING CONTROL: Restore failed");
    return false;
}

bool PatchBrakeDecrement() {
    return BrakePatcher.Patch();
}

bool RestoreBrakeDecrement() {
    return BrakePatcher.Restore();
}

bool PatchThrottleDecrement() {
    if (throttleAttempts > maxAttempts) {
        return false;
    }

    if (ThrottleDecrementPatched) {
        return true;
    }

    throttleTemp = ApplyThrottlePatch();

    if (throttleTemp) {
        throttleAddr = throttleTemp;
        ThrottleDecrementPatched = true;
        throttleAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: Throttle pressure Patch failed");
    throttleAttempts++;

    if (throttleAttempts > maxAttempts) {
        logger.Write(ERROR, "PATCH: Throttle pressure Patch attempt limit exceeded");
        logger.Write(ERROR, "PATCH: Throttle pressure Patching disabled");
    }
    return false;
}

bool RestoreThrottleDecrement() {
    if (!ThrottleDecrementPatched) {
        return true;
    }

    if (throttleAddr) {
        memcpy((void*)throttleAddr, throttle.Data.data(), throttle.Data.size());
        throttleAddr = 0;
        ThrottleDecrementPatched = false;
        throttleAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: Throttle pressure restore failed");
    return false;
}

uintptr_t ApplySteeringCorrectionPatch() {
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
    // ???


    uintptr_t address;
    if (steeringTemp != NULL) {
        address = steeringTemp;
    }
    else {
        address = mem::FindPattern(steeringCorrection.Pattern, steeringCorrection.Mask);
        logger.Write(DEBUG, "PATCH: Steering correction patch @ 0x%p", address);
    }

    if (address) {
        uint8_t instrArr[6] =
            {0xE9, 0x00, 0x00, 0x00, 0x00, 0x90};               // make preliminary instruction: JMP to <adrr>
        uint8_t origSteerInstrDest[4] = {0x00, 0x00, 0x00, 0x00};

        memcpy(steeringCorrection.Data.data(), (void*)address, 6); // save whole orig instruction
        memcpy(origSteerInstrDest, (void*)(address + 2), 4);    // save the address it writes to
        origSteerInstrDest[0] += 1;                             // Increment first byte by 1
        memcpy(instrArr + 1, origSteerInstrDest, 4);            // use saved address in new instruction
        memcpy((void*)address, instrArr, 6);                    // patch with new fixed instruction

        return address;
    }
    return 0;
}

uintptr_t ApplySteerControlPatch() {
    uintptr_t address;
    if (steerControlTemp != NULL) {
        address = steerControlTemp;
    }
    else {
        address = mem::FindPattern(steeringControl.Pattern, steeringControl.Mask);
        logger.Write(DEBUG, "PATCH: Steer control patch @ 0x%p", address);
    }


    if (address) {
        memcpy(steeringControl.Data.data(), (void*)address, 8);
        memset(reinterpret_cast<void *>(address), 0x90, 8);
    }
    return address;
}

uintptr_t ApplyThrottlePatch() {
    uintptr_t address;
    if (throttleTemp != NULL) {
        address = throttleTemp;
    }
    else {
        address = mem::FindPattern(throttle.Pattern, throttle.Mask);
        if (address) address += throttle.Offset;
        logger.Write(DEBUG, "PATCH: Throttle patch @ 0x%p", address);
    }

    if (address) {
        memcpy(throttle.Data.data(), (void*)address, throttle.Data.size());
        memset(reinterpret_cast<void *>(address), 0x90, throttle.Data.size());
    }
    return address;
}

}
