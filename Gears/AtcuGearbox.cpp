#include <numeric>
#include <vector>
#include <algorithm>
#include "AtcuLogic.h"
#include <map>
#include <vector>
#include "VehicleData.hpp"

extern VehicleData g_vehData;

namespace AtcuLogic {
    void Cycle();
};

float AtcuGearbox::parsePowerIntersectionRpm(int gear) {
    if (g_vehData.mGearTop == gear) return 1.0f;
    float currRatio = g_vehData.mGearRatios[gear];
    float currRetardedRatio = currRatio * 0.6f;
    float nextRatio = g_vehData.mGearRatios[gear + 1];
    if (currRetardedRatio > nextRatio) return 0.99f;
    float currGap = currRatio - currRetardedRatio;
    float nextOffset = nextRatio - currRetardedRatio;
    float goldenRatio = nextOffset / currGap;
    return 0.8f + (0.2f * (1.0f - goldenRatio));
}

float AtcuGearbox::rpmPredictSpeed(int gear, float rpm) {
    float currRatio = g_vehData.mGearRatios[gear];
    float maxSpeed = g_vehData.mDriveMaxFlatVel / currRatio;
    return maxSpeed * rpm;
}
