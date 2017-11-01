#include "VehicleExtensions.hpp"
#include <vector>
#include "NativeMemory.hpp"
#include "Versions.h"
#include "Offsets.hpp"
#include "../Util/MathExt.h"
#include <functional>
#include <map>
#include "Util/Logger.hpp"

const eGameVersion g_gameVersion = getGameVersion();

int findOffset(const std::map<int, int, std::greater<int>> &offsets) {
    return offsets.lower_bound(g_gameVersion)->second;
}

VehicleExtensions::VehicleExtensions() {
    mem::init();
}

// Offsets done by me might need revision
void VehicleExtensions::initOffsets() {
    uintptr_t addr = mem::FindPattern("\x3A\x91\x00\x00\x00\x00\x74\x00\x84\xD2", "xx????x?xx");
    rocketBoostActiveOffset = addr == 0 ? 0 : *(int*)(addr + 2);
    logger.Writef("Rocket Boost Active Offset: 0x%X", rocketBoostActiveOffset);

    addr = mem::FindPattern("\x48\x8B\x47\x00\xF3\x44\x0F\x10\x9F\x00\x00\x00\x00", "xxx?xxxxx????");
    rocketBoostChargeOffset = addr == 0 ? 0 : *(int*)(addr + 9);
    logger.Writef("Rocket Boost Charge Offset: 0x%X", rocketBoostChargeOffset);

    addr = mem::FindPattern("\x74\x26\x0F\x57\xC9", "xxxxx");
    fuelLevelOffset = addr == 0 ? 0 : *(int*)(addr + 8);
    logger.Writef("Fuel Level Offset: 0x%X", fuelLevelOffset);

    addr = mem::FindPattern("\x48\x8D\x8F\x00\x00\x00\x00\x4C\x8B\xC3\xF3\x0F\x11\x7C\x24",
        "xxx????xxxxxxxx");
    nextGearOffset = addr == 0 ? 0 : *(int*)(addr + 3);
    logger.Writef("Next Gear Offset: 0x%X", nextGearOffset);

    currentGearOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 2;
    logger.Writef("Current Gear Offset: 0x%X", currentGearOffset);

    topGearOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 6;
    logger.Writef("Top Gear Offset: 0x%X", topGearOffset);

    gearRatiosOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 8;
    logger.Writef("Gear Ratios Offset: 0x%X", gearRatiosOffset);

    driveForceOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 0x28;
    logger.Writef("Drive Force Offset: 0x%X", driveForceOffset);

    initialDriveMaxFlatVelOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 0x2C;
    logger.Writef("Initial Drive Max Flat Velocity Offset: 0x%X", initialDriveMaxFlatVelOffset);

    driveMaxFlatVelOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 0x30;
    logger.Writef("Drive Max Flat Velocity Offset: 0x%X", driveMaxFlatVelOffset);

    addr = mem::FindPattern("\xF3\x44\x0F\x10\x93\x00\x00\x00\x00\xF3\x0F\x10\x0D",
        "xxxxx????xxxx");
    currentRPMOffset = addr == 0 ? 0 : *(int*)(addr + 5);
    logger.Writef("RPM Offset: 0x%X", currentRPMOffset);

    clutchOffset = addr == 0 ? 0 : *(int*)(addr + 5) + 0xC;
    logger.Writef("Clutch Offset: 0x%X", clutchOffset);

    throttleOffset = addr == 0 ? 0 : *(int*)(addr + 5) + 0x10;
    logger.Writef("Throttle Offset: 0x%X", throttleOffset);

    addr = mem::FindPattern("\xF3\x0F\x10\x8F\x68\x08\x00\x00\x88\x4D\x8C\x0F\x2F\xCF", 
                            "xxxx????xxx???");
    turboOffset = addr == 0 ? 0 : *(int*)(addr + 4);
    logger.Writef("Turbo Offset: 0x%X", turboOffset);

    addr = mem::FindPattern("\x3C\x03\x0F\x85\x00\x00\x00\x00\x48\x8B\x41\x20\x48\x8B\x88",
        "xxxx????xxxxxxx");
    handlingOffset = addr == 0 ? 0 : *(int*)(addr + 0x16);
    logger.Writef("Handling Offset: 0x%X", handlingOffset);

    addr = mem::FindPattern("\x74\x0A\xF3\x0F\x11\xB3\x1C\x09\x00\x00\xEB\x25", "xxxxxx????xx");
    steeringAngleInputOffset = addr == 0 ? 0 : *(int*)(addr + 6);
    logger.Writef("Steering Input Offset: 0x%X", steeringAngleInputOffset);

    steeringAngleOffset = addr == 0 ? 0 : *(int*)(addr + 6) + 8;
    logger.Writef("Steering Angle Offset: 0x%X", steeringAngleOffset);

    throttlePOffset = addr == 0 ? 0 : *(int*)(addr + 6) + 0x10;
    logger.Writef("ThrottleP Offset: 0x%X", throttlePOffset);

    brakePOffset = addr == 0 ? 0 : *(int*)(addr + 6) + 0x14;
    logger.Writef("BrakeP Offset: 0x%X", brakePOffset);

    addr = mem::FindPattern("\x44\x8A\xAA\x00\x00\x00\x00\x0F\x2F\xFB", "xxx????xxx");
    handbrakeOffset = addr == 0 ? 0 : *(int*)(addr + 3);
    logger.Writef("Handbrake Offset: 0x%X", handbrakeOffset);

    const std::map<int, int, std::greater<int>> dirtLevelOffsets{
        { G_VER_1_0_335_2_STEAM, 0 },
        { G_VER_1_0_877_1_STEAM, 0x910 },
        { G_VER_1_0_944_2_STEAM, 0x938 },
        { G_VER_1_0_1103_2_STEAM, 0x948 },
        { G_VER_1_0_1180_2_STEAM, 0x968 }
    };
    dirtLevelOffset = findOffset(dirtLevelOffsets);
    logger.Writef("Dirt Level Offset: 0x%X", dirtLevelOffset);

    const std::map<int, int, std::greater<int>> engineTempOffsets{
        { G_VER_1_0_335_2_STEAM, 0 },
        { G_VER_1_0_877_1_STEAM, 0x984 },
        { G_VER_1_0_944_2_STEAM, 0x9AC },
        { G_VER_1_0_1103_2_STEAM, 0x9BC },
        { G_VER_1_0_1180_2_STEAM, 0x9DC }
    };
    engineTempOffset = findOffset(engineTempOffsets);
    logger.Writef("Engine Temperature Offset: 0x%X", engineTempOffset);

    const std::map<int, int, std::greater<int>> dashSpeedOffsets{
        { G_VER_1_0_335_2_STEAM, 0 },
        { G_VER_1_0_372_2_STEAM, 0x9A4 },
        { G_VER_1_0_877_1_STEAM, 0x9C8 },
        { G_VER_1_0_944_2_STEAM, 0x9F0 },
        { G_VER_1_0_1103_2_STEAM, 0xA00 },
        { G_VER_1_0_1180_2_STEAM, 0xA10 }
    };
    dashSpeedOffset = findOffset(dashSpeedOffsets);
    logger.Writef("Dashboard Speed Offset: 0x%X", dashSpeedOffset);

    const std::map<int, int, std::greater<int>> wheelsPtrOffsets{
        { G_VER_1_0_335_2_STEAM, 0xA80 },
        { G_VER_1_0_372_2_STEAM, 0xAA0 },
        { G_VER_1_0_791_2_STEAM, 0xAB0 },
        { G_VER_1_0_877_1_STEAM, 0xAE0 },
        { G_VER_1_0_944_2_STEAM, 0xB10 },
        { G_VER_1_0_1103_2_STEAM, 0xB20 },
        { G_VER_1_0_1180_2_STEAM, 0xB40 }
    };
    wheelsPtrOffset = findOffset(wheelsPtrOffsets);
    logger.Writef("Wheels Pointer Offset: 0x%X", wheelsPtrOffset);

    numWheelsOffset = wheelsPtrOffset == 0 ? 0 : wheelsPtrOffset + 8;
    logger.Writef("Wheel Count Offset: 0x%X", numWheelsOffset);
}

BYTE *VehicleExtensions::GetAddress(Vehicle handle) {
    return reinterpret_cast<BYTE *>(mem::GetAddressOfEntity(handle));
}

bool VehicleExtensions::GetRocketBoostActive(Vehicle handle) {
    if (rocketBoostActiveOffset == 0) return false;
    return *reinterpret_cast<bool *>(GetAddress(handle) + rocketBoostActiveOffset);
}

void VehicleExtensions::SetRocketBoostActive(Vehicle handle, bool val) {
    if (rocketBoostActiveOffset == 0) return;
    *reinterpret_cast<bool *>(GetAddress(handle) + rocketBoostActiveOffset) = val;
}

float VehicleExtensions::GetRocketBoostCharge(Vehicle handle) {
    if (rocketBoostChargeOffset == 0) return 0.0f;
    return *reinterpret_cast<float *>(GetAddress(handle) + rocketBoostChargeOffset);
}

void VehicleExtensions::SetRocketBoostCharge(Vehicle handle, float value) {
    if (rocketBoostChargeOffset == 0) return;
    *reinterpret_cast<float *>(GetAddress(handle) + rocketBoostChargeOffset) = value;
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
    if (fuelLevelOffset == 0) return 0.0f;
    return *reinterpret_cast<float *>(GetAddress(handle) + fuelLevelOffset);
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
    if (fuelLevelOffset == 0) return;
    *reinterpret_cast<float *>(GetAddress(handle) + fuelLevelOffset) = value;
}

uint16_t VehicleExtensions::GetGearNext(Vehicle handle) {
    if (nextGearOffset == 0) return 0;
    return *reinterpret_cast<const uint16_t *>(GetAddress(handle) + nextGearOffset);
}

void VehicleExtensions::SetGearNext(Vehicle handle, uint16_t value) {
    if (nextGearOffset == 0) return;
    *reinterpret_cast<uint16_t *>(GetAddress(handle) + nextGearOffset) = value;
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) {
    if (currentGearOffset == 0) return 0;
    return *reinterpret_cast<const uint16_t *>(GetAddress(handle) + currentGearOffset);
}

void VehicleExtensions::SetGearCurr(Vehicle handle, uint16_t value) {
    if (currentGearOffset == 0) return;
    *reinterpret_cast<uint16_t *>(GetAddress(handle) + currentGearOffset) = value;
}

unsigned char VehicleExtensions::GetTopGear(Vehicle handle) {
    if (topGearOffset == 0) return 0;
    return *reinterpret_cast<const unsigned char *>(GetAddress(handle) + topGearOffset);
}

std::vector<float> VehicleExtensions::GetGearRatios(Vehicle handle) {
    auto address = GetAddress(handle);
    std::vector<float> ratios;
    for (int gearOffset = 0; gearOffset <= 7; gearOffset++) {
        ratios.push_back(*reinterpret_cast<float *>(address + gearRatiosOffset + gearOffset * sizeof(float)));
    }
    return ratios;
}

float VehicleExtensions::GetDriveForce(Vehicle handle) {
    if (driveForceOffset == 0) return 0.0f;
    return *reinterpret_cast<float *>(GetAddress(handle) + driveForceOffset);
}

float VehicleExtensions::GetInitialDriveMaxFlatVel(Vehicle handle) {
    if (initialDriveMaxFlatVelOffset == 0) return 0.0f;
    return *reinterpret_cast<float *>(GetAddress(handle) + initialDriveMaxFlatVelOffset);
}

float VehicleExtensions::GetDriveMaxFlatVel(Vehicle handle) {
    if (driveMaxFlatVelOffset == 0) return 0.0f;
    return *reinterpret_cast<float *>(GetAddress(handle) + driveMaxFlatVelOffset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
    if (currentRPMOffset == 0) return 0.0f;
    return *reinterpret_cast<const float *>(GetAddress(handle) + currentRPMOffset);
}

void VehicleExtensions::SetCurrentRPM(Vehicle handle, float value) {
    if (currentRPMOffset == 0) return;
    *reinterpret_cast<float *>(GetAddress(handle) + currentRPMOffset) = value;
}

float VehicleExtensions::GetClutch(Vehicle handle) {
    if (clutchOffset == 0) return 0.0f;
    auto address = GetAddress(handle);
    return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + clutchOffset);
}

void VehicleExtensions::SetClutch(Vehicle handle, float value) {
    if (clutchOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + clutchOffset) = value;
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
    if (throttleOffset == 0) return 0.0f;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + throttleOffset);
}

void VehicleExtensions::SetThrottle(Vehicle handle, float value) {
    if (throttleOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + throttleOffset) = value;
}

float VehicleExtensions::GetTurbo(Vehicle handle) {
    if (turboOffset == 0) return 0.0f;
    auto address = GetAddress(handle);
    return address == nullptr ? 0 : *reinterpret_cast<const float *>(address + turboOffset);
}

void VehicleExtensions::SetTurbo(Vehicle handle, float value) {
    if (turboOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + turboOffset) = value;
}

uint64_t VehicleExtensions::GetHandlingPtr(Vehicle handle) {
    if (handlingOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<uint64_t *>(address + handlingOffset);
}

float VehicleExtensions::GetSteeringInputAngle(Vehicle handle) {
    if (steeringAngleInputOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + steeringAngleInputOffset);
}

void VehicleExtensions::SetSteeringInputAngle(Vehicle handle, float value) {
    if (steeringAngleInputOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + steeringAngleInputOffset) = value;
}

float VehicleExtensions::GetSteeringAngle(Vehicle handle) {
    if (steeringAngleOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + steeringAngleOffset);
}

void VehicleExtensions::SetSteeringAngle(Vehicle handle, float value) {
    if (steeringAngleOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + steeringAngleOffset) = value;
}

float VehicleExtensions::GetThrottleP(Vehicle handle) {
    if (throttlePOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + throttlePOffset);
}

void VehicleExtensions::SetThrottleP(Vehicle handle, float value) {
    if (throttlePOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + throttlePOffset) = value;
}

float VehicleExtensions::GetBrakeP(Vehicle handle) {
    if (brakePOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + brakePOffset);
}

void VehicleExtensions::SetBrakeP(Vehicle handle, float value) {
    if (brakePOffset == 0) return;
    auto address = GetAddress(handle);
    *reinterpret_cast<float *>(address + brakePOffset) = value;
}

bool VehicleExtensions::GetHandbrake(Vehicle handle) {
    if (handbrakeOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<bool *>(address + handbrakeOffset);
}

float VehicleExtensions::GetDirtLevel(Vehicle handle) {
    if (dirtLevelOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + dirtLevelOffset);
}

float VehicleExtensions::GetEngineTemp(Vehicle handle) {
    if (engineTempOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + engineTempOffset);
}

float VehicleExtensions::GetDashSpeed(Vehicle handle) {
    if (dashSpeedOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<float *>(address + dashSpeedOffset);
}

uint64_t VehicleExtensions::GetWheelsPtr(Vehicle handle) {
    if (wheelsPtrOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<uint64_t *>(address + wheelsPtrOffset);
}

uint8_t VehicleExtensions::GetNumWheels(Vehicle handle) {
    if (numWheelsOffset == 0) return 0;
    auto address = GetAddress(handle);
    return *reinterpret_cast<int *>(address + numWheelsOffset);
}

float VehicleExtensions::GetDriveBiasFront(Vehicle handle) {
    auto address = GetHandlingPtr(handle);
    if (address == 0) return 0.0f;
    return *reinterpret_cast<float *>(address + hOffsets.fDriveBiasFront);
}

float VehicleExtensions::GetDriveBiasRear(Vehicle handle) {
    auto address = GetHandlingPtr(handle);
    if (address == 0) return 0.0f;
    return *reinterpret_cast<float *>(address + hOffsets.fDriveBiasRear);
}

float VehicleExtensions::GetPetrolTankVolume(Vehicle handle) {
    auto address = GetHandlingPtr(handle);
    if (address == 0) return 0.0f;
    return *reinterpret_cast<float *>(address + hOffsets.fPetrolTankVolume);
}

float VehicleExtensions::GetOilVolume(Vehicle handle) {
    auto address = GetHandlingPtr(handle);
    if (address == 0) return 0.0f;
    return *reinterpret_cast<float *>(address + hOffsets.fOilVolume);
}

Hash VehicleExtensions::GetAIHandling(Vehicle handle) {
    auto address = GetHandlingPtr(handle);
    if (address == 0) return 0;
    auto offset = 0;
    offset = gameVersion >= G_VER_1_0_1180_2_STEAM ? 0x13C : 0;
    if (offset == 0) return 0;
    return *reinterpret_cast<Hash *>(address + offset);
}

std::vector<uint64_t> VehicleExtensions::GetWheelPtrs(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers
    auto numWheels = GetNumWheels(handle);
    std::vector<uint64_t> wheelPtrs;
    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        wheelPtrs.push_back(wheelAddr);
    }
    return wheelPtrs;
}

float VehicleExtensions::GetVisualHeight(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);

    auto offset = gameVersion >= G_VER_1_0_944_2_STEAM ? 0x080 : 0;
    if (offset == 0)
        return 0.0f;

    return *reinterpret_cast<float *>(wheelPtr + offset);
}

void VehicleExtensions::SetVisualHeight(Vehicle handle, float height) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto offset = gameVersion >= G_VER_1_0_944_2_STEAM ? 0x07C : 0;

    if (offset == 0)
        return;

    *reinterpret_cast<float *>(wheelPtr + offset) = height;
}

std::vector<float> VehicleExtensions::GetWheelHealths(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1E0 : 0x1D0;

    std::vector<float> healths;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        healths.push_back(*reinterpret_cast<float *>(wheelAddr + offset));
    }
    return healths;
}

void VehicleExtensions::SetWheelsHealth(Vehicle handle, float health) {
    auto wheelPtr = GetWheelsPtr(handle);  // pointer to wheel pointers
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1E0 : 0x1D0;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        *reinterpret_cast<float *>(wheelAddr + offset) = health;
    }
}

float VehicleExtensions::GetSteeringMultiplier(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x138 : 0x128;

    if (numWheels > 1) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * 1);
        return abs(*reinterpret_cast<float*>(wheelAddr + offset));
    }
    return 1.0f;
}

void VehicleExtensions::SetSteeringMultiplier(Vehicle handle, float value) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x138 : 0x128;

    for (int i = 0; i<numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        int sign = sgn(*reinterpret_cast<float*>(wheelAddr + offset));
        *reinterpret_cast<float*>(wheelAddr + offset) = value * sign;
    }
}

std::vector<Vector3> VehicleExtensions::GetWheelOffsets(Vehicle handle) {
    auto wheels = GetWheelPtrs(handle);
    std::vector<Vector3> positions;

    int offPosX = 0x20;
    int offPosY = 0x24;
    int offPosZ = 0x28;

    for (auto wheelAddr : wheels) {
        if (!wheelAddr) continue;

        Vector3 wheelPos;
        wheelPos.x = *reinterpret_cast<float *>(wheelAddr + offPosX);
        wheelPos.y = *reinterpret_cast<float *>(wheelAddr + offPosY);
        wheelPos.z = *reinterpret_cast<float *>(wheelAddr + offPosZ);
        positions.push_back(wheelPos);
    }
    return positions;
}

std::vector<Vector3> VehicleExtensions::GetWheelCoords(Vehicle handle, Vector3 position, Vector3 rotation, Vector3 direction) {
    std::vector<Vector3> worldCoords;
    std::vector<Vector3> positions = GetWheelOffsets(handle);

    for (Vector3 wheelPos : positions) {
        Vector3 absPos = GetOffsetInWorldCoords(position, rotation, direction, wheelPos);
        worldCoords.push_back(absPos);
    }
    return worldCoords;
}

std::vector<Vector3> VehicleExtensions::GetWheelLastContactCoords(Vehicle handle) {
    auto wheels = GetWheelPtrs(handle);
    std::vector<Vector3> positions;
    // 0x40: Last wheel contact coordinates
    // 0x50: Last wheel contact coordinates but centered on the wheel width
    // 0x60: Next probable wheel position? Seems to flutter around a lot, while
    //       position is entirely lost (0.0) when contact is lost. Wheels that
    //       steer emphasise this further, though acceleration/deceleration
    //       will also influence it.

    int offPosX = 0x40;
    int offPosY = 0x44;
    int offPosZ = 0x48;

    for (auto wheelAddr : wheels) {
        if (!wheelAddr) continue;

        Vector3 wheelPos;
        wheelPos.x = *reinterpret_cast<float *>(wheelAddr + offPosX);
        wheelPos.y = *reinterpret_cast<float *>(wheelAddr + offPosY);
        wheelPos.z = *reinterpret_cast<float *>(wheelAddr + offPosZ);
        positions.push_back(wheelPos);
    }
    return positions;
}

std::vector<float> VehicleExtensions::GetWheelCompressions(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x160 : 0x150;

    std::vector<float> compressions;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        compressions.push_back(*reinterpret_cast<float *>(wheelAddr + offset));
    }
    return compressions;
}

std::vector<float> VehicleExtensions::GetWheelSteeringAngles(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1C4 : 0x1B4;

    std::vector<float> angles;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        angles.push_back(*reinterpret_cast<float *>(wheelAddr + offset));
    }
    return angles;
}

std::vector<bool> VehicleExtensions::GetWheelsOnGround(Vehicle handle) {
    std::vector<bool> onGround;
    for (auto comp : GetWheelCompressions(handle)) {
        onGround.push_back(comp != 0.0f);
    }
    return onGround;
}

std::vector<WheelDimensions> VehicleExtensions::GetWheelDimensions(Vehicle handle) {
    auto wheels = GetWheelPtrs(handle);

    std::vector<WheelDimensions> dimensionsSet;
    int offTyreRadius = 0x110;
    int offRimRadius = 0x114;
    int offTyreWidth = 0x118;

    for (auto wheelAddr : wheels) {
        if (!wheelAddr) continue;

        WheelDimensions dimensions;
        dimensions.TyreRadius = *reinterpret_cast<float *>(wheelAddr + offTyreRadius);
        dimensions.RimRadius = *reinterpret_cast<float *>(wheelAddr + offRimRadius);
        dimensions.TyreWidth = *reinterpret_cast<float *>(wheelAddr + offTyreWidth);
        dimensionsSet.push_back(dimensions);
    }
    return dimensionsSet;
}

std::vector<float> VehicleExtensions::GetWheelRotationSpeeds(Vehicle handle) {
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);

    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x168 : 0x158;

    std::vector<float> speeds;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        speeds.push_back(-*reinterpret_cast<float *>(wheelAddr + offset));
    }
    return speeds;
}

std::vector<float> VehicleExtensions::GetTyreSpeeds(Vehicle handle) {
    std::vector<float> wheelSpeeds;
    int numWheels = GetNumWheels(handle);
    std::vector<float> rotationSpeed = GetWheelRotationSpeeds(handle);
    std::vector<WheelDimensions> dimensionsSet = GetWheelDimensions(handle);
    for (int i = 0; i < numWheels; i++) {
        wheelSpeeds.push_back(rotationSpeed[i] * dimensionsSet[i].TyreRadius);
    }
    return wheelSpeeds;
}

void VehicleExtensions::SetWheelSkidSmokeEffect(Vehicle handle, uint8_t index, float value) {
    if (index > GetNumWheels(handle)) {
        return;
    }
    auto wheelPtr = GetWheelsPtr(handle);
    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1B0 : 0x1A0;
    auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * index);
    *reinterpret_cast<float *>(wheelAddr + offset) = value;
}

std::vector<float> VehicleExtensions::GetWheelSkidSmokeEffect(Vehicle handle) {
    std::vector<float> values;
    
    auto wheelPtr = GetWheelsPtr(handle);
    auto numWheels = GetNumWheels(handle);
    
    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1B0 : 0x1A0;

    for (auto i = 0; i < numWheels; i++) {
        auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * i);
        values.push_back(-*reinterpret_cast<float *>(wheelAddr + offset));
    }
    return values;
}

void VehicleExtensions::SetWheelBrakePressure(Vehicle handle, uint8_t index, float value) {
    if (index > GetNumWheels(handle)) {
        return;
    }
    auto wheelPtr = GetWheelsPtr(handle);
    auto offset = gameVersion >= G_VER_1_0_372_2_STEAM ? 0x1C8 : 0x1B8;
    auto wheelAddr = *reinterpret_cast<uint64_t *>(wheelPtr + 0x008 * index);
    *reinterpret_cast<float *>(wheelAddr + offset) = value;
}

// These apply to b1103
// 0x7f8 to 0x814 are gear ratios!
// 0x7f8 - Reverse
// 0x7fc - 1
// 0x800 - 2
// 0x804 - 3
// 0x808 - 4
// 0x80c - 5
// 0x810 - 6
// 0x814 - 7

// 0x818: fDriveForce
// Affected by engine upgrade
// 

// 0x81C: fInitialDriveMaxFlatVel (m/s)
// Affected by no tuning options
// Doesn't influence anything?

// 0x820: final drive speed
// divide this by gear ratio and there's the top speed for the gear

// b1180
// 0x838 - power? Drive Force?
// 0x9E4 - has spoiler? (2nd bit)
// Couldn't find anything torque-related

// wheel+0x1ec - power/brake flags? abs turned off for e-brake with [wheel+0x1ec] & 0xFFFF3FFF
