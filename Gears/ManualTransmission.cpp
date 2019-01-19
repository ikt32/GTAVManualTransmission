#include "ManualTransmission.h"

#include <inc/types.h>
#include "Memory/MemoryPatcher.hpp"
#include "Constants.h"

extern std::vector<Vehicle> ignoredVehicles;

const char* MT_GetVersion() {
    return DISPLAY_VERSION;
}

bool MT_IsActive() {
    return MemoryPatcher::NumGearboxPatched > 0;
}

void MT_SetActive(bool active) {
    // TODO?
}

void MT_AddIgnoreVehicle(int vehicle) {
    if (std::find(ignoredVehicles.begin(), ignoredVehicles.end(), vehicle) == ignoredVehicles.end()) {
        ignoredVehicles.push_back(vehicle);
    }
}

void MT_DelIgnoreVehicle(int vehicle) {
    auto it = std::find(ignoredVehicles.begin(), ignoredVehicles.end(), vehicle);
    if (it != ignoredVehicles.end()) {
        ignoredVehicles.erase(it);
    }
}

void MT_ClearIgnoredVehicles() {
    ignoredVehicles.clear();
}

unsigned MT_NumIgnoredVehicles() {
    return ignoredVehicles.size();
}
