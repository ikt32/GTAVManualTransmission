#include "ManualTransmission.h"

#include <inc/types.h>
#include "script.h"
#include "Constants.h"
#include "ScriptSettings.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Input/CarControls.hpp"
#include "VehicleData.hpp"

extern ScriptSettings settings;
extern CarControls carControls;
extern Vehicle vehicle;
extern VehicleGearboxStates gearStates;
extern VehicleData vehData;
extern std::vector<Vehicle> ignoredVehicles;

const char* MT_GetVersion() {
    return DISPLAY_VERSION;
}

bool MT_IsActive() {
    return MemoryPatcher::NumGearboxPatched > 0;
}

void MT_SetActive(bool active) {
    toggleManual(active);
}

bool MT_NeutralGear() {
    return gearStates.FakeNeutral && settings.EnableManual;
}

int MT_GetShiftMode() {
    return static_cast<int>(settings.ShiftMode) + 1;
}

void MT_SetShiftMode(int mode) {
    if (mode == 1 || mode == 2 || mode == 3)
        setShiftMode(mode - 1);
}

int MT_GetShiftIndicator() {
    if (gearStates.HitRPMSpeedLimiter || gearStates.HitRPMLimiter) {
        return 1;
    }
    else if (vehData.mGearCurr > 1 && vehData.mRPM < 0.4f) {
        return 2;
    }
    else if (vehData.mGearCurr == vehData.mGearNext) {
        return 0;
    }
    return 0;
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
    return static_cast<unsigned>(ignoredVehicles.size());
}

const int* MT_GetIgnoredVehicles() {
    return ignoredVehicles.data();
}

int MT_GetManagedVehicle() {
    return vehicle;
}

bool MT_LookingLeft() {
    return carControls.ButtonIn(CarControls::WheelControlType::LookLeft);
}

bool MT_LookingRight() {
    return carControls.ButtonIn(CarControls::WheelControlType::LookRight);
}

bool MT_LookingBack() {
    return carControls.ButtonIn(CarControls::WheelControlType::LookBack) ||

        carControls.ButtonIn(CarControls::WheelControlType::LookLeft) &&
        carControls.ButtonIn(CarControls::WheelControlType::LookRight);
}
