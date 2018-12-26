#include "MemoryPatcher.hpp"
#include <utility>

#include "NativeMemory.hpp"
#include "Versions.h"
#include "../Util/Logger.hpp"
#include "../Util/Util.hpp"

#include "PatternInfo.h"

namespace MemoryPatcher {
// Clutch disengage @ Low Speed High Gear, low RPM
uintptr_t ApplyClutchLowPatch();
void RevertClutchLowPatch(uintptr_t address);

// Disable "shifting down" wanted
uintptr_t ApplyShiftDownPatch();
void RevertShiftDownPatch(uintptr_t address);

// Disable "shifting up" wanted
uintptr_t ApplyShiftUpPatch();
void RevertShiftUpPatch(uintptr_t address);

// Clutch disengage @ High speed rev limiting
uintptr_t ApplyClutchRevLimitPatch();
void RevertClutchRevLimitPatch(uintptr_t address);

// Does the same as Custom Steering by InfamousSabre
uintptr_t ApplySteeringCorrectionPatch();

// Disable (normal) user input while wheel steering is active.
uintptr_t ApplySteerControlPatch();

// When disabled, brake pressure doesn't decrease.
uintptr_t ApplyBrakePatch();

// When disabled, throttle doesn't decrease.
uintptr_t ApplyThrottlePatch();

int NumGearboxPatches = 4;
int TotalPatched = 0;

bool SteerCorrectPatched = false;
bool SteerControlPatched = false;
bool BrakeDecrementPatched = false;
bool ThrottleDecrementPatched = false;

uintptr_t clutchLowAddr = NULL;
uintptr_t clutchLowTemp = NULL;

uintptr_t clutchRevLimitAddr = NULL;
uintptr_t clutchRevLimitTemp = NULL;

uintptr_t shiftDownAddr = NULL;
uintptr_t shiftDownTemp = NULL;

uintptr_t shiftUpAddr = NULL;
uintptr_t shiftUpTemp = NULL;

uintptr_t steeringAddr = NULL;
uintptr_t steeringTemp = NULL;

uintptr_t steerControlAddr = NULL;
uintptr_t steerControlTemp = NULL;

uintptr_t brakeAddr = NULL;
uintptr_t brakeTemp = NULL;

uintptr_t throttleAddr = NULL;
uintptr_t throttleTemp = NULL;

const int maxAttempts = 4;
int gearboxAttempts = 0;
int steerAttempts = 0;
int steerControlAttempts = 0;
int brakeAttempts = 0;
int throttleAttempts = 0;

PatternInfo shiftUp;
PatternInfo shiftDown;
PatternInfo brake;
PatternInfo throttle;

PatternInfo clutchLow;
PatternInfo clutchRevLimit;
PatternInfo steeringCorrection;
PatternInfo steeringControl;

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

    clutchLowTemp = ApplyClutchLowPatch();
    if (clutchLowTemp) {
        clutchLowAddr = clutchLowTemp;
        TotalPatched++;
        logger.Write(DEBUG, "PATCH: GEARBOX: Patched clutchLow @ 0x%p", clutchLowAddr);
    }
    else {
        logger.Write(ERROR, "PATCH: GEARBOX: clutchLow patch failed");
    }

    clutchRevLimitTemp = ApplyClutchRevLimitPatch();
    if (clutchRevLimitTemp) {
        clutchRevLimitAddr = clutchRevLimitTemp;
        TotalPatched++;
        logger.Write(DEBUG, "PATCH: GEARBOX: Patched clutchRevLimit @ 0x%p", clutchRevLimitAddr);
    }
    else {
        logger.Write(ERROR, "PATCH: GEARBOX: clutchRevLimit patch failed");
    }

    shiftDownTemp = ApplyShiftDownPatch();
    if (shiftDownTemp) {
        shiftDownAddr = shiftDownTemp;
        TotalPatched++;
        logger.Write(DEBUG, "PATCH: GEARBOX: Patched shiftDown  @ 0x%p", shiftDownAddr);
    }
    else {
        logger.Write(ERROR, "PATCH: GEARBOX: shiftDown patch failed");
    }

    shiftUpTemp = ApplyShiftUpPatch();
    if (shiftUpTemp) {
        shiftUpAddr = shiftUpTemp;
        TotalPatched++;
        logger.Write(DEBUG, "PATCH: GEARBOX: Patched shiftUp  @ 0x%p", shiftUpAddr);
    }
    else {
        logger.Write(ERROR, "PATCH: GEARBOX: shiftUp patch failed");
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

    if (clutchLowAddr) {
        RevertClutchLowPatch(clutchLowAddr);
        clutchLowAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "PATCH: GEARBOX: clutchLow not restored");
    }

    if (clutchRevLimitAddr) {
        RevertClutchRevLimitPatch(clutchRevLimitAddr);
        clutchRevLimitAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "PATCH: GEARBOX: clutchRevLimit not restored");
    }

    if (shiftDownAddr) {
        RevertShiftDownPatch(shiftDownAddr);
        shiftDownAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "PATCH: GEARBOX: shiftDown not restored");
    }

    if (shiftUpAddr) {
        RevertShiftUpPatch(shiftUpAddr);
        shiftUpAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(ERROR, "PATCH: GEARBOX: shiftUp not restored");
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
        //byte origInstr[8] = { 0xF3, 0x0F, 0x11, 0x8B, 0xFC, 0x08, 0x00, 0x00 };
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
    if (brakeAttempts > maxAttempts) {
        return false;
    }

    if (BrakeDecrementPatched) {
        return true;
    }

    brakeTemp = ApplyBrakePatch();

    if (brakeTemp) {
        brakeAddr = brakeTemp;
        BrakeDecrementPatched = true;

        //std::string instructionBytes = formatByteArray(origBrakeInstr, sizeof(origBrakeInstr) / sizeof(byte));
        //logger.Write("BRAKE PRESSURE: Patch success, original : " + instructionBytes);
        brakeAttempts = 0;

        return true;
    }

    logger.Write(ERROR, "PATCH: Brake pressure Patch failed");
    brakeAttempts++;

    if (brakeAttempts > maxAttempts) {
        logger.Write(ERROR, "PATCH: Brake pressure Patch attempt limit exceeded");
        logger.Write(ERROR, "PATCH: Brake pressure Patching disabled");
    }
    return false;
}

bool RestoreBrakeDecrement() {
    //logger.Write("BRAKE PRESSURE: Restoring instructions");

    if (!BrakeDecrementPatched) {
        //logger.Write("BRAKE PRESSURE: Already restored/intact");
        return true;
    }

    if (brakeAddr) {
        memcpy((void*)brakeAddr, brake.Data.data(), brake.Data.size());
        brakeAddr = 0;
        BrakeDecrementPatched = false;
        //logger.Write("BRAKE PRESSURE: Restore success");
        brakeAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "PATCH: Brake pressure restore failed");
    return false;
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


uintptr_t ApplyClutchLowPatch() {
    uintptr_t address;
    if (clutchLowTemp != NULL)
        address = clutchLowTemp;
    else
        address = mem::FindPattern(clutchLow.Pattern, clutchLow.Mask);

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, clutchLow.Data.size());
    }
    return address;
}

void RevertClutchLowPatch(uintptr_t address) {
    if (address) {
        memcpy(reinterpret_cast<void*>(address), clutchLow.Data.data(), clutchLow.Data.size());
    }
}

uintptr_t ApplyClutchRevLimitPatch() {
    uintptr_t address;
    if (clutchRevLimitTemp != NULL)
        address = clutchRevLimitTemp;
    else
        address = mem::FindPattern(clutchRevLimit.Pattern, clutchRevLimit.Mask);

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, clutchRevLimit.Data.size());
    }
    return address;
}

void RevertClutchRevLimitPatch(uintptr_t address) {
    if (address) {
        memcpy(reinterpret_cast<void*>(address), clutchRevLimit.Data.data(), clutchRevLimit.Data.size());
    }
}

uintptr_t ApplyShiftDownPatch() {
    uintptr_t address;
    if (shiftDownTemp != 0)
        address = shiftDownTemp;
    else
        address = mem::FindPattern(shiftDown.Pattern, shiftDown.Mask);

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, shiftDown.Data.size());
    }
    return address;
}

void RevertShiftDownPatch(uintptr_t address) {
    if (address) {
        memcpy(reinterpret_cast<void*>(address), shiftDown.Data.data(), shiftDown.Data.size());
    }
}

uintptr_t ApplyShiftUpPatch() {
    uintptr_t address;
    if (shiftUpTemp != 0)
        address = shiftUpTemp;
    else
        address = mem::FindPattern(shiftUp.Pattern, shiftUp.Mask);

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, shiftUp.Data.size());
    }
    return address;
}

void RevertShiftUpPatch(uintptr_t address) {
    if (address) {
        memcpy(reinterpret_cast<void*>(address), shiftUp.Data.data(), shiftUp.Data.size());
    }
}

uintptr_t ApplySteeringCorrectionPatch() {
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
    // b1032.2
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

uintptr_t ApplyBrakePatch() {
    // b1032.2
    uintptr_t address;
    if (brakeTemp != NULL) {
        address = brakeTemp;
    }
    else {
        address = mem::FindPattern(brake.Pattern, brake.Mask);
        if (address) address += brake.Offset;
        logger.Write(DEBUG, "PATCH: Brake patch @ 0x%p", address);
    }
    
    if (address) {
        memcpy(brake.Data.data(), (void*)address, brake.Data.size());
        memset(reinterpret_cast<void *>(address), 0x90, brake.Data.size());
    }
    return address;
}

uintptr_t ApplyThrottlePatch() {
    // b1290
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
