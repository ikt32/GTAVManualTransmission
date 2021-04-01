#include "CruiseControl.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"

#include <inc/enums.h>
#include <inc/natives.h>

extern CarControls g_controls;
extern ScriptSettings g_settings;
extern Vehicle g_playerVehicle;
extern VehicleData g_vehData;

using VExt = VehicleExtensions;

namespace {
    bool active = false;
}

bool CruiseControl::GetActive() {
    return active;
}

void CruiseControl::SetActive(bool active) {
    ::active = active;
}

void CruiseControl::Update(float& throttle, float& brake, float& clutch) {
    if (throttle > 0.2f || brake > 0.01f || clutch > 0.25f) {
        active = false;
    }

    if (active) {
        float prevThrottle = VExt::GetThrottleP(g_playerVehicle);
        float prevBrake = VExt::GetBrakeP(g_playerVehicle);

        float delta = g_vehData.mVelocity.y - g_settings().DriveAssists.CruiseControl.Speed;

        // Too fast, slow down
        if (delta > 0) {
            // Lift
            if (prevThrottle > 0) {
                throttle = prevThrottle - map(delta, 0.0f, 12.0f, 0.0f, 1.0f);
            }
            // Brake
            else if (delta > 6.0f) {
                throttle = 0.0f;
                brake = map(delta, 0.0f, 12.0f, 0.0f, 1.0f);
            }
            // Coast
            else {
                throttle = prevThrottle;
            }
        }
        // Too slow, accelerate
        else {
            // increase speed
            throttle = prevThrottle + map(delta, 0.0f, -12.0f, 0.0f, 1.0f);
            brake = 0.0f;
        }

        throttle = std::clamp(throttle, 0.0f, 1.0f);
        brake = std::clamp(brake, 0.0f, 1.0f);
    }
}
