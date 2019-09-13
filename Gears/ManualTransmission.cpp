#include "ManualTransmission.h"

#include "script.h"
#include "Constants.h"
#include "ScriptSettings.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Input/CarControls.hpp"
#include "VehicleData.hpp"
#include "Util/MathExt.h"

#include "inc/natives.h"
#include "inc/types.h"

extern ScriptSettings settings;
extern CarControls carControls;
extern Vehicle vehicle;
extern VehicleGearboxStates gearStates;
extern VehicleData vehData;
extern std::vector<Vehicle> ignoredVehicles;

const char* MT_GetVersion() {
    return Constants::DisplayVersion;
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
    if (!ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        // vehData is not initialized yet, so mGearRatios can not be dereferenced.
        return 0;
    }
    float nextGearMinSpeed = 0.0f; // don't care about top gear
    if (gearStates.LockGear < vehData.mGearTop) {
        nextGearMinSpeed = settings.NextGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[gearStates.LockGear + 1];
    }
    float engineLoad = carControls.ThrottleVal - map(vehData.mRPM, 0.2f, 1.0f, 0.0f, 1.0f);
    bool shiftUpLoad = gearStates.LockGear < vehData.mGearTop && 
        engineLoad < settings.UpshiftLoad && 
        vehData.mWheelAverageDrivenTyreSpeed > nextGearMinSpeed;

    float currGearMinSpeed = settings.CurrGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[gearStates.LockGear];
    bool shiftDownLoad = engineLoad > settings.DownshiftLoad || vehData.mWheelAverageDrivenTyreSpeed < currGearMinSpeed;

    if (gearStates.HitRPMSpeedLimiter || gearStates.HitRPMLimiter || shiftUpLoad) {
        return 1;
    }
    if (vehData.mGearCurr > 1 && shiftDownLoad) {
        return 2;
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
