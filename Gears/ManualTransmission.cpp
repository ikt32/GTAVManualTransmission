#include "ManualTransmission.h"

#include "script.h"
#include "Constants.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/Strings.hpp"
#include "VehicleConfig.h"

#include "inc/natives.h"
#include "inc/types.h"

extern ScriptSettings g_settings;
extern CarControls g_controls;
extern Vehicle g_playerVehicle;
extern VehicleGearboxStates g_gearStates;
extern VehicleData g_vehData;
extern std::vector<Vehicle> g_ignoredVehicles;

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
    return g_gearStates.FakeNeutral && g_settings.MTOptions.Enable;
}

int MT_GetShiftMode() {
    return EToInt(g_settings().MTOptions.ShiftMode) + 1;
}

void MT_SetShiftMode(int mode) {
    if (mode == 1 || mode == 2 || mode == 3)
        setShiftMode(static_cast<EShiftMode>(mode - 1));
}

int MT_GetShiftIndicator() {
    if (!ENTITY::DOES_ENTITY_EXIST(g_playerVehicle)) {
        // vehData is not initialized yet, so mGearRatios can not be dereferenced.
        return 0;
    }
    float nextGearMinSpeed = 0.0f; // don't care about top gear
    if (g_gearStates.LockGear < g_vehData.mGearTop) {
        nextGearMinSpeed = g_settings().AutoParams.NextGearMinRPM * g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_gearStates.LockGear + 1];
    }
    float engineLoad = g_controls.ThrottleVal - map(g_vehData.mRPM, 0.2f, 1.0f, 0.0f, 1.0f);
    bool shiftUpLoad = g_gearStates.LockGear < g_vehData.mGearTop && 
        engineLoad < g_settings().AutoParams.UpshiftLoad && 
        g_vehData.mWheelAverageDrivenTyreSpeed > nextGearMinSpeed;

    float currGearMinSpeed = g_settings().AutoParams.CurrGearMinRPM * g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_gearStates.LockGear];
    bool shiftDownLoad = engineLoad > g_settings().AutoParams.DownshiftLoad || g_vehData.mWheelAverageDrivenTyreSpeed < currGearMinSpeed;

    if (g_gearStates.HitRPMSpeedLimiter || g_gearStates.HitRPMLimiter || shiftUpLoad) {
        return 1;
    }
    if (g_vehData.mGearCurr > 1 && shiftDownLoad) {
        return 2;
    }
    return 0;
}

float MT_GetAutoEcoRate() {
    return g_settings().AutoParams.EcoRate;
}

void MT_SetAutoEcoRate(float rate) {
    extern VehicleConfig* g_activeConfig;
    APPLY_CONFIG_VALUE(AutoParams.EcoRate, rate);
}

void MT_AddIgnoreVehicle(int vehicle) {
    if (std::find(g_ignoredVehicles.begin(), g_ignoredVehicles.end(), vehicle) == g_ignoredVehicles.end()) {
        g_ignoredVehicles.push_back(vehicle);
    }
}

void MT_DelIgnoreVehicle(int vehicle) {
    auto it = std::find(g_ignoredVehicles.begin(), g_ignoredVehicles.end(), vehicle);
    if (it != g_ignoredVehicles.end()) {
        g_ignoredVehicles.erase(it);
    }
}

void MT_ClearIgnoredVehicles() {
    g_ignoredVehicles.clear();
}

unsigned MT_NumIgnoredVehicles() {
    return static_cast<unsigned>(g_ignoredVehicles.size());
}

const int* MT_GetIgnoredVehicles() {
    return g_ignoredVehicles.data();
}

int MT_GetManagedVehicle() {
    return g_playerVehicle;
}

bool MT_LookingLeft() {
    return g_controls.ButtonIn(CarControls::WheelControlType::LookLeft);
}

bool MT_LookingRight() {
    return g_controls.ButtonIn(CarControls::WheelControlType::LookRight);
}

bool MT_LookingBack() {
    return g_controls.ButtonIn(CarControls::WheelControlType::LookBack) ||

        g_controls.ButtonIn(CarControls::WheelControlType::LookLeft) &&
        g_controls.ButtonIn(CarControls::WheelControlType::LookRight);
}
