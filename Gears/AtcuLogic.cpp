#include "AtcuLogic.h"

#include "script.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Memory/Offsets.hpp"
#include <inc\types.h>
#include <inc\natives.h>
#include <numeric>

extern ScriptSettings g_settings;
extern CarControls g_controls;
extern Vehicle g_playerVehicle;
extern VehicleGearboxStates g_gearStates;
extern VehicleData g_vehData;
extern VehicleExtensions g_ext;

namespace AtcuLogic {
    void Cycle();
};

void AtcuLogic::Cycle() {
    // Using new TCU
    float economyCorrection = (g_controls.ThrottleVal * 0.7f) + 0.3f;

    int currGear = g_vehData.mGearCurr;
    float currRpm = g_vehData.mRPM;
    float currSpeed = g_vehData.mWheelAverageDrivenTyreSpeed;
    float currSpeedWorld = g_vehData.mVelocity.y;
    bool skidding = isSkidding(3.5f);
    float currPowerIntersection = g_gearStates.Atcu.parsePowerIntersectionRpm(currGear);

    //shift up
    if (currGear < g_vehData.mGearTop) {
        if (skidding) {
            g_gearStates.Atcu.upshiftingIndex = currSpeedWorld / (g_gearStates.Atcu.rpmPredictSpeed(currGear, (currPowerIntersection * economyCorrection)));
            if (g_gearStates.Atcu.upshiftingIndex > 0.9999f) {
                shiftTo(g_vehData.mGearCurr + 1, true);
                g_gearStates.FakeNeutral = false;
                g_gearStates.Atcu.upshiftingIndex = 0.0f;
            }
        }
        else {
            g_gearStates.Atcu.upshiftingIndex = currRpm / (currPowerIntersection * economyCorrection);
            if (g_gearStates.Atcu.upshiftingIndex > 1.01f) g_gearStates.Atcu.upshiftingIndex = 1.01f;
            float ndIndex = currSpeed / g_gearStates.Atcu.rpmPredictSpeed(currGear, (currPowerIntersection * economyCorrection));
            if (ndIndex > 1.01f) ndIndex = 1.01f;
            g_gearStates.Atcu.upshiftingIndex += ndIndex;
            g_gearStates.Atcu.upshiftingIndex = g_gearStates.Atcu.upshiftingIndex / 2.0f;
            if (g_gearStates.Atcu.upshiftingIndex > 1.0f) {
                shiftTo(g_vehData.mGearCurr + 1, true);
                g_gearStates.FakeNeutral = false;
                g_gearStates.Atcu.upshiftingIndex = 0.0f;
            }
        }
    }
    // Shift down
    if (currGear > 1) {
        float intersectedSpeed = g_gearStates.Atcu.rpmPredictSpeed(currGear - 1, (g_gearStates.Atcu.parsePowerIntersectionRpm(currGear - 1) * economyCorrection));
        float minSpeed = g_gearStates.Atcu.rpmPredictSpeed(currGear - 1, (g_gearStates.Atcu.parsePowerIntersectionRpm(currGear - 1) * economyCorrection) - 0.1f);
        if ((intersectedSpeed - minSpeed) < 2.5f) minSpeed = intersectedSpeed - 2.5f;
        g_gearStates.Atcu.downshiftingIndex = minSpeed / (skidding ? currSpeedWorld : currSpeed);
        if (g_gearStates.Atcu.downshiftingIndex > 1.0f || currSpeedWorld < 1.0f) {
            shiftTo(currGear - 1, true);
            g_gearStates.FakeNeutral = false;
            g_gearStates.Atcu.downshiftingIndex = 0.0f;
        }
    }
}
