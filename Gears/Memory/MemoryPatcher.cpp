#include "MemoryPatcher.hpp"

#include <iomanip>
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

// Clutch disengage @ High speed rev limiting
uintptr_t PatchClutchRevLimit();
void RestoreClutchRevLimit(uintptr_t address);

// Does the same as Custom Steering by InfamousSabre
// Kept for emergency/backup purposes in the case InfamousSabre
// stops support earlier than I do. (not trying to compete here!)
uintptr_t ApplySteeringCorrectionPatch();
void RevertSteeringCorrectionPatch(uintptr_t address, byte *origInstr, int origInstrSz);

// Disable (normal) user input while wheel steering is active.
uintptr_t ApplySteerControlPatch();
void RevertSteerControlPatch(uintptr_t address, byte *origInstr, int origInstrSz);

// When disabled, brake pressure doesn't decrease.
uintptr_t ApplyBrakePatch();
void RevertBrakePatch(uintptr_t address, byte *origInstr, int origInstrSz);

int NumGearboxPatches = 3;
int TotalPatched = 0;

bool SteerCorrectPatched = false;
bool SteerControlPatched = false;
bool BrakeDecrementPatched = false;

uintptr_t clutchLowAddr = NULL;
uintptr_t clutchLowTemp = NULL;

uintptr_t clutchRevLimitAddr = NULL;
uintptr_t clutchRevLimitTemp = NULL;

uintptr_t gear7A0Addr = NULL;
uintptr_t gear7A0Temp = NULL;

uintptr_t steeringAddr = NULL;
uintptr_t steeringTemp = NULL;

uintptr_t steerControlAddr = NULL;
uintptr_t steerControlTemp = NULL;

uintptr_t brakeAddr = NULL;
uintptr_t brakeTemp = NULL;

byte origSteerInstr[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte origSteerInstrDest[4] = {0x00, 0x00, 0x00, 0x00};

byte origSteerControlInstr[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte origBrakeInstr[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const int maxAttempts = 4;
int gearAttempts = 0;
int steerAttempts = 0;
int steerControlAttempts = 0;
int brakeAttempts = 0;

bool PatchInstructions() {
    if (gearAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "GEARBOX: Patching");

    if (NumGearboxPatches == TotalPatched) {
        logger.Write(DEBUG, "GEARBOX: Already patched");
        return true;
    }

    clutchLowTemp = PatchClutchLow();

    if (clutchLowTemp) {
        clutchLowAddr = clutchLowTemp;
        TotalPatched++;
        logger.Writef(DEBUG, "GEARBOX: Patched clutchLow @ 0x%p", clutchLowAddr);
    }
    else {
        logger.Write(ERROR, "GEARBOX: clutchLow patch failed");
    }

    clutchRevLimitTemp = PatchClutchRevLimit();

    if (clutchRevLimitTemp) {
        clutchRevLimitAddr = clutchRevLimitTemp;
        TotalPatched++;
        logger.Writef(DEBUG, "GEARBOX: Patched clutchRevLimit @ 0x%p", clutchRevLimitAddr);
    }
    else {
        logger.Write(ERROR, "GEARBOX: clutchRevLimit patch failed");
    }

    gear7A0Temp = PatchGear7A0();

    if (gear7A0Temp) {
        gear7A0Addr = gear7A0Temp;
        TotalPatched++;
        logger.Writef(DEBUG, "GEARBOX: Patched gear7A0  @ 0x%p", gear7A0Addr);
    }
    else {
        logger.Write(ERROR, "GEARBOX: gear7A0 patch failed");
    }

    if (TotalPatched == NumGearboxPatches) {
        logger.Write(DEBUG, "GEARBOX: Patch success");
        gearAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "GEARBOX: Patching failed");
    gearAttempts++;

    if (gearAttempts > maxAttempts) {
        logger.Write(ERROR, "GEARBOX: Patch attempt limit exceeded");
        logger.Write(ERROR, "GEARBOX: Patching disabled");
    }
    return false;
}

bool RestoreInstructions() {
    logger.Write(DEBUG, "GEARBOX: Restoring instructions");

    if (TotalPatched == 0) {
        logger.Write(DEBUG, "GEARBOX: Already restored/intact");
        return true;
    }

    if (clutchLowAddr) {
        RestoreClutchLow(clutchLowAddr);
        clutchLowAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "GEARBOX: clutchLow not restored");
    }

    if (clutchRevLimitAddr) {
        RestoreClutchRevLimit(clutchRevLimitAddr);
        clutchRevLimitAddr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "GEARBOX: clutchRevLimit not restored");
    }

    if (gear7A0Addr) {
        RestoreGear7A0(gear7A0Addr);
        gear7A0Addr = 0;
        TotalPatched--;
    }
    else {
        logger.Write(DEBUG, "GEARBOX: gear@7A0 not restored");
    }

    if (TotalPatched == 0) {
        logger.Write(DEBUG, "GEARBOX: Restore success");
        gearAttempts = 0;
        return true;
    }
    logger.Write(ERROR, "GEARBOX: Restore failed");
    return false;
}

std::string formatByteArray(byte *byteArray, size_t length) {
    std::string instructionBytes;
    for (int i = 0; i < length; ++i) {
        char buff[4];
        snprintf(buff, 4, "%02X ", byteArray[i]);
        instructionBytes += buff;
    }
    return instructionBytes;
}

bool PatchSteeringCorrection() {
    if (steerAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "STEERING: Patching");

    if (SteerCorrectPatched) {
        logger.Write(DEBUG, "STEERING: Already patched");
        return true;
    }

    steeringTemp = ApplySteeringCorrectionPatch();

    if (steeringTemp) {
        steeringAddr = steeringTemp;
        SteerCorrectPatched = true;

        std::string instructionBytes = formatByteArray(origSteerInstr, sizeof(origSteerInstr) / sizeof(byte));

        logger.Writef(DEBUG, "STEERING: Steering Correction @ 0x%p", steeringAddr);
        logger.Write(DEBUG, "STEERING: Patch success, original: " + instructionBytes);
        steerAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "STEERING: Patch failed");
    steerAttempts++;

    if (steerAttempts > maxAttempts) {
        logger.Write(ERROR, "STEERING: Patch attempt limit exceeded");
        logger.Write(ERROR, "STEERING: Patching disabled");
    }
    return false;
}

bool RestoreSteeringCorrection() {
    logger.Write(DEBUG, "STEERING: Restoring instructions");

    if (!SteerCorrectPatched) {
        logger.Write(DEBUG, "STEERING: Already restored/intact");
        return true;
    }

    if (steeringAddr) {
        RevertSteeringCorrectionPatch(steeringAddr, origSteerInstr, sizeof(origSteerInstr) / sizeof(byte));
        steeringAddr = 0;
        SteerCorrectPatched = false;
        logger.Write(DEBUG, "STEERING: Restore success");
        steerAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "STEERING: Restore failed");
    return false;
}

bool PatchSteeringControl() {
    if (steerControlAttempts > maxAttempts) {
        return false;
    }

    logger.Write(DEBUG, "STEERING CONTROL: Patching");

    if (SteerControlPatched) {
        logger.Write(DEBUG, "STEERING CONTROL: Already patched");
        return true;
    }

    steerControlTemp = ApplySteerControlPatch();

    if (steerControlTemp) {
        steerControlAddr = steerControlTemp;
        SteerControlPatched = true;

        std::string instructionBytes = formatByteArray(origSteerControlInstr, sizeof(origSteerControlInstr) / sizeof(byte));
        
        logger.Writef(DEBUG, "STEERING CONTROL: Steering Control @ 0x%p", steerControlAddr);
        logger.Write(DEBUG, "STEERING CONTROL: Patch success, original : " + instructionBytes);
        steerControlAttempts = 0;

        return true;
    }

    logger.Write(ERROR, "STEERING CONTROL: Patch failed");
    steerControlAttempts++;

    if (steerControlAttempts > maxAttempts) {
        logger.Write(ERROR, "STEERING CONTROL: Patch attempt limit exceeded");
        logger.Write(ERROR, "STEERING CONTROL: Patching disabled");
    }
    return false;
}

bool RestoreSteeringControl() {
    logger.Write(DEBUG, "STEERING CONTROL: Restoring instructions");

    if (!SteerControlPatched) {
        logger.Write(DEBUG, "STEERING CONTROL: Already restored/intact");
        return true;
    }

    if (steerControlAddr) {
        //byte origInstr[8] = { 0xF3, 0x0F, 0x11, 0x8B, 0xFC, 0x08, 0x00, 0x00 };
        RevertSteerControlPatch(steerControlAddr, origSteerControlInstr, sizeof(origSteerControlInstr) / sizeof(byte));
        steerControlAddr = 0;
        SteerControlPatched = false;
        logger.Write(DEBUG, "STEERING CONTROL: Restore success");
        steerControlAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "STEERING CONTROL: Restore failed");
    return false;
}

bool PatchBrakeDecrement() {
    if (brakeAttempts > maxAttempts) {
        return false;
    }

    //logger.Write("BRAKE PRESSURE: Patching");

    if (BrakeDecrementPatched) {
        //logger.Write("BRAKE PRESSURE: Already patched");
        return true;
    }

    brakeTemp = ApplyBrakePatch();

    if (brakeTemp) {
        brakeAddr = brakeTemp;
        BrakeDecrementPatched = true;

        //std::string instructionBytes = formatByteArray(origBrakeInstr, sizeof(origBrakeInstr) / sizeof(byte));

        //logger.Writef("BRAKE PRESSURE: BRAKE PRESSURE @ 0x%p", brakeAddr);
        //logger.Write("BRAKE PRESSURE: Patch success, original : " + instructionBytes);
        brakeAttempts = 0;

        return true;
    }

    logger.Write(ERROR, "BRAKE PRESSURE: Patch failed");
    brakeAttempts++;

    if (brakeAttempts > maxAttempts) {
        logger.Write(ERROR, "BRAKE PRESSURE: Patch attempt limit exceeded");
        logger.Write(ERROR, "BRAKE PRESSURE: Patching disabled");
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
        RevertBrakePatch(brakeAddr, origBrakeInstr, sizeof(origBrakeInstr) / sizeof(byte));
        brakeAddr = 0;
        BrakeDecrementPatched = false;
        //logger.Write("BRAKE PRESSURE: Restore success");
        brakeAttempts = 0;
        return true;
    }

    logger.Write(ERROR, "BRAKE PRESSURE: Restore failed");
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
        address = mem::FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D\x66\x44\x89\x43\x04", "xxxxxxxxxxxx");

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, 7);
    }
    return address;
}

void RestoreClutchLow(uintptr_t address) {
    byte instrArr[7] = {0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D};
    if (address) {
        memcpy(reinterpret_cast<void*>(address), instrArr, 7);
    }
}

uintptr_t PatchClutchRevLimit() {
    // Tested on build 1103

    uintptr_t address;
    if (clutchRevLimitTemp != NULL)
        address = clutchRevLimitTemp;
    else
        address = mem::FindPattern("\xC7\x43\x40\xCD\xCC\xCC\x3D"
                                   "\x44\x89\x7B\x60"
                                   "\x89\x73\x5C"
                                   "\x66\x89\x13",
                                   "xxxxxxx"
                                   "xxxx"
                                   "xxx"
                                   "xxx");

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, 7);
    }
    return address;
}

void RestoreClutchRevLimit(uintptr_t address) {
    byte instrArr[7] = {0xC7, 0x43, 0x40, 0xCD, 0xCC, 0xCC, 0x3D};
    if (address) {
        memcpy(reinterpret_cast<void*>(address), instrArr, 7);
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
        address = mem::FindPattern("\x66\x89\x13\x89\x73\x5C", "xxxxxx");

    if (address) {
        memset(reinterpret_cast<void *>(address), 0x90, 3);
    }
    return address;
}

void RestoreGear7A0(uintptr_t address) {
    byte instrArr[3] = {0x66, 0x89, 0x13};
    if (address) {
        memcpy(reinterpret_cast<void*>(address), instrArr, 3);
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
        address = mem::FindPattern(
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
        byte instrArr[6] = {0xE9, 0xD1, 0x01, 0x00, 0x00, 0x90}; // make preliminary instruction: JMP to <adrr>

        memcpy(origSteerInstr, (void*)address, 6); // save whole orig instruction
        memcpy(origSteerInstrDest, (void*)(address + 2), 4); // save the address it writes to
        origSteerInstrDest[0] += 1; // Increment first address by 1
        memcpy(instrArr + 1, origSteerInstrDest, 4); // use saved address in new instruction
        memcpy((void*)address, instrArr, 6); // patch with new fixed instruction

        return address;
    }
    return 0;
}

void RevertSteeringCorrectionPatch(uintptr_t address, byte *origInstr, int origInstrSz) {
    if (address) {
        memcpy((void*)address, origInstr, origInstrSz);
    }
}

uintptr_t ApplySteerControlPatch() {
    // b1032.2
    uintptr_t address;
    if (steerControlTemp != NULL)
        address = steerControlTemp;
    else
        address = mem::FindPattern("\xF3\x0F\x11\x8B\xFC\x08\x00\x00"
                                   "\xF3\x0F\x10\x83\x00\x09\x00\x00"
                                   "\xF3\x0F\x58\x83\xFC\x08\x00\x00"
                                   "\x41\x0F\x2F\xC3",
                                   "xxxx??xx"
                                   "xxxx??xx"
                                   "xxxx??xx"
                                   "xxxx");

    if (address) {
        memcpy(origSteerControlInstr, (void*)address, 8);
        memset(reinterpret_cast<void *>(address), 0x90, 8);
    }
    return address;
}

void RevertSteerControlPatch(uintptr_t address, byte *origInstr, int origInstrSz) {
    if (address) {
        memcpy((void*)address, origInstr, origInstrSz);
    }
}

uintptr_t ApplyBrakePatch() {
    // b1032.2
    uintptr_t address;
    if (brakeTemp != NULL)
        address = brakeTemp;
    else
        address = mem::FindPattern(
            "\xEB\x05"
            "\xF3\x0F\x10\x40\x78"
            "\xF3\x0F\x59\xC4"
            "\xF3\x0F\x11\x81\xC8\x01\x00\x00" // this
            "\xC3"
            ,
            "xx"
            "xxxx?"
            "xxxx"
            "xxxx??xx"
            "x") + 11;

    if (address) {
        memcpy(origBrakeInstr, (void*)address, 8);
        memset(reinterpret_cast<void *>(address), 0x90, 8);
    }
    return address;
}

void RevertBrakePatch(uintptr_t address, byte *origInstr, int origInstrSz) {
    if (address) {
        memcpy((void*)address, origInstr, origInstrSz);
    }
}
}
