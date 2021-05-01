#include "SpeedLimiter.h"

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
    float limitThrottle = 0.0f;
}

bool SpeedLimiter::Update(float& throttle) {
    bool use = false;
    float targetSetpoint = g_settings().MTOptions.SpeedLimiter.Speed;
    // 5 kph min buffer
    if (g_vehData.mVelocity.y > targetSetpoint - (5.0f / 3.6f) && throttle > limitThrottle) {
        float targetAcceleration = (targetSetpoint - g_vehData.mVelocity.y);
        limitThrottle = std::clamp(limitThrottle + (targetAcceleration - g_vehData.mAcceleration.y) * MISC::GET_FRAME_TIME(), -1.0f, 1.0f);
        float newThrottle = std::clamp(throttle + limitThrottle, 0.0f, 1.0f);

        if (newThrottle < throttle) {
            throttle = newThrottle;
            use = true;
        }
    }
    else {
        limitThrottle = 0.0f;
    }
    return use;
}
