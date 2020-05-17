#include "AtcuLogic.h"
#include <numeric>

void AtcuLogic::Cycle() {
    // Using new TCU
    // introduce 'powertrain' based algorithm:
    // ratio between gear ratio and wheels power output are always constant, this ratio named 'powertrain ratio'
    // this ratio effected by vehicle engine, throttle and brakes
    // it also effected while vehicle in collision included bump
    // .
    // this algorithm predicts next gear 's wheels power output, and then shift up when current output drop below next gear 's wheels power output
    float economyCorrection = (g_controls.ThrottleVal * 0.8f) + 0.2f; // configurable point (economy) 0.2 - 0.8
    auto currGameTime = GAMEPLAY::GET_GAME_TIMER();
    g_gearStates.Atcu.PowertrainSecondCounter++;
    if ((g_gearStates.Atcu.PowertrainTimer + 2) < (currGameTime / 1000)) {
        if (g_gearStates.Atcu.PowertrainSecondCounter - g_gearStates.Atcu.PowertrainHistorySize > 100) g_gearStates.Atcu.PowertrainSecondCounter += 100;
        g_gearStates.Atcu.PowertrainTimer = currGameTime / 1000;
        g_gearStates.Atcu.PowertrainSecondCounter = 0;
    }
    auto wp = g_ext.GetWheelPower(g_playerVehicle);
    float currTotalPower = std::accumulate(wp.begin(), wp.end(), 0.0f);
    currTotalPower = currTotalPower / g_vehData.mWheelCount;
    float currRatio = g_vehData.mGearRatios[g_vehData.mGearCurr];
    float powertrainRatio = currTotalPower / currRatio;
    if (economyCorrection >= 0.95f && (powertrainRatio - g_gearStates.Atcu.parsePowertrainRatioThreshold()) >= -0.000001f) {
        float rateUp = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
        float upshiftDuration = 1.0f / (rateUp * g_settings().ShiftOptions.ClutchRateMult);
        bool tpPassed = currGameTime > (g_gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration) + 100);
        bool isStable = V3D(g_vehData.mAcceleration).x < 0.8f && V3D(g_vehData.mAcceleration).x > -0.8f;
        if (isStable && tpPassed && g_vehData.mRPM < 0.80f && g_controls.BrakeVal < 0.01f && g_controls.HandbrakeVal < 0.01f) {
            bool allWheelOnGround = true;
            for (uint8_t i = 0; i < g_vehData.mWheelCount; ++i) {
                if (!g_vehData.mWheelsOnGround[i]) {
                    allWheelOnGround = false;
                    break;
                }
            }
            if (allWheelOnGround && !isSkidding(3.0f) && g_gearStates.Atcu.CurrentRpm <= g_vehData.mRPM) {
                g_gearStates.Atcu.updatePowertrainRatioDistribution(powertrainRatio);
            }
            else {
                g_gearStates.LastUpshiftTime = currGameTime; // dont update powertrain data after bump or skidding
            }
        }
    }
    int currGear = g_vehData.mGearCurr;
    float currSpeed = g_vehData.mWheelAverageDrivenTyreSpeed;
    float currSpeedWorld = g_vehData.mVelocity.y;
    bool skidding = isSkidding(3.5f);
    g_gearStates.Atcu.CurrentRpm = g_vehData.mRPM;
    //shift up
    if (currGear < g_vehData.mGearTop) {
        if (skidding) {
            // use world speed instead when skiding
            if (currSpeedWorld > (getGearMaxSpeed(currGear))) {
                shiftTo(g_vehData.mGearCurr + 1, true);
                g_gearStates.FakeNeutral = false;
            }
        }
        else {
            if (g_gearStates.Atcu.isPowertrainRatioTrustworthy()) {
                if (currTotalPower <= (gearPredictStandardPower(currGear + 1) * 0.99f) && currSpeed > (getGearMinSpeed(currGear + 1) * economyCorrection)) {
                    shiftTo(g_vehData.mGearCurr + 1, true);
                    g_gearStates.FakeNeutral = false;
                }
                else if (g_vehData.mRPM > 0.98f && currSpeedWorld > getGearMaxSpeed(currGear)) {
                    shiftTo(g_vehData.mGearCurr + 1, true);
                    g_gearStates.FakeNeutral = false;
                }
            }
            else {
                auto currGearMaxSpeed = getGearMaxSpeed(currGear);
                auto prevGearMaxSpeed = getGearMaxSpeed(currGear - 1);
                if (currSpeed > (((currGearMaxSpeed - prevGearMaxSpeed) * economyCorrection) + (prevGearMaxSpeed * economyCorrection))) {
                    shiftTo(g_vehData.mGearCurr + 1, true);
                    g_gearStates.FakeNeutral = false;
                }
            }
        }
    }
    // Shift down
    if (currGear > 1) {
        float currGearMinSpeed = getGearMinSpeed(currGear);
        float speedGap = (currGearMinSpeed - getGearMinSpeed(currGear - 1)) / 5;
        if (speedGap < 8) speedGap = 8;
        if ((skidding ? currSpeedWorld : currSpeed) < ((currGearMinSpeed - speedGap) * economyCorrection)) {
            shiftTo(currGear - 1, true);
            g_gearStates.FakeNeutral = false;
        }
    }
}

float AtcuLogic::getGearMinSpeed(int gear) {
    int currGear = g_vehData.mGearCurr;
    float peakTorqueSpeed = (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[gear]) * 0.75f;
    float prevGearMaxSpeed = getGearMaxSpeed(gear - 1); // however, the previous gear ratio might not happy with 0.75f
    return gear > 1 ? (peakTorqueSpeed < prevGearMaxSpeed ? peakTorqueSpeed : prevGearMaxSpeed) : peakTorqueSpeed;
}

float AtcuLogic::getGearMaxSpeed(int gear) {
    if (gear < 1) return 0;
    return (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[gear]) * 0.90f;
    // rev further more unlikely provides good effcient or performance due to power loss
}

float AtcuLogic::gearPredictStandardPower(int gear) {
    return (g_gearStates.Atcu.parsePowertrainRatio() * g_vehData.mGearRatios[gear]);
}

float AtcuLogic::gearPredictRpm(int atGear) {
    float max = g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[atGear];
    float currSpeed = g_vehData.mWheelAverageDrivenTyreSpeed;
    return currSpeed / max;
}
