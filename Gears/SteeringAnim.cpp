#include "SteeringAnim.h"

#include "ScriptUtils.h"
#include "ScriptSettings.hpp"
#include "Input/CarControls.hpp"

#include "Util/Logger.hpp"
#include "Util/MathExt.h"
#include "Util/Timer.h"
#include "Util/UIUtils.h"

#include <inc/natives.h>
#include <fmt/format.h>

#include <vector>
#include <string>
#include <algorithm>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern CarControls g_controls;
extern ScriptSettings g_settings;

// TODO: Ew.
bool g_customAnim = true;
float g_tempSteerOffsetAnim = 0.0f;
int g_steerAnimDictIdx = 0;

std::vector<std::string> g_steerAnimDicts{
    "veh@std@ds@base", // 720
    "veh@low@front_ds@base", // 360 :(
    "veh@truck@ds@base", // 720
    "veh@van@ds@base", // 720
    "anim@veh@lowrider@std@ds@arm@base", // 360?
    "veh@bus@bus@driver@base", // 720
    "veh@low@dune@streamed_base", // 360 :(
};

//std::vector<std::pair<float, float>> g_compensation{
//    { 0, 0 }, // degrees
//    {  }
//};

//enum eAnimationFlags
//{
//    ANIM_FLAG_NORMAL = 0,
//    ANIM_FLAG_REPEAT = 1,
//    ANIM_FLAG_STOP_LAST_FRAME = 2,
//    ANIM_FLAG_UPPERBODY = 16,
//    ANIM_FLAG_ENABLE_PLAYER_CONTROL = 32,
//    ANIM_FLAG_CANCELABLE = 120,
//};


// map
// -450 -360 -270 -180  -90    0    90  180  270  360  450 // wheeldegrees
//  0.0  0.0  0.0  0.0  0.0   0.5  1.0  1.0  1.0  1.0  1.0 // animTime @ 180 max
//  0.0  0.0  0.0  0.0        0.5       1.0  1.0  1.0  1.0 // animTime @ 360 max
//  0.0  0.0                  0.5                 1.0  1.0 // animTime @ 720 max
float figureOutAnimModulation(float wheelDegrees, float maxAnimAngle, float maxWheelAngle) {    
    float animTime = map(wheelDegrees, -maxAnimAngle, maxAnimAngle, 0.0f, 1.0f);
    animTime = std::clamp(animTime, 0.01f, 0.99f);
    return animTime;
}

void UpdateSteeringAnimations(float wheelDegrees, float maxWheelAngle) {
    const std::string& dict = g_steerAnimDicts[g_steerAnimDictIdx];
    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict.c_str(), "steer_no_lean", 3);
    if (!g_customAnim) {
        if (playing)
            AI::STOP_ANIM_TASK(g_playerPed, dict.c_str(), "steer_no_lean", 8.0f);
        return;
    }

    float maxAnimAngle = 720.0f;
    if (dict == "veh@low@front_ds@base" ||
        dict == "veh@low@dune@streamed_base") {
        maxAnimAngle = 360.0f;
    }

    // div 2 cuz one-way angle
    float finalSteerVal = figureOutAnimModulation(wheelDegrees, maxAnimAngle / 2.0f, maxWheelAngle / 2.0f);

    showText(0.1f, 0.00f, 0.5f, fmt::format("maxAnimAngle: {}", maxAnimAngle));
    showText(0.1f, 0.05f, 0.5f, fmt::format("maxWheelAngle: {}", maxWheelAngle));
    showText(0.1f, 0.10f, 0.5f, fmt::format("wheelDegs: {}", wheelDegrees));
    showText(0.1f, 0.15f, 0.5f, fmt::format("finalSteerVal: {}", finalSteerVal));


    if (!playing) {
        // TODO: Handle errors
        Timer t(500);

        STREAMING::REQUEST_ANIM_DICT(dict.c_str());
        while (!STREAMING::HAS_ANIM_DICT_LOADED(dict.c_str())) {
            if (t.Expired()) {
                UI::Notify(ERROR, "Frick");
                break;
            }
            WAIT(0);
        }

        AI::TASK_PLAY_ANIM(g_playerPed, dict.c_str(), "steer_no_lean", -8.0f, 8.0f, -1, 48, finalSteerVal, 0, 0, 0);
    }
    else {
        ENTITY::SET_ENTITY_ANIM_CURRENT_TIME(g_playerPed, dict.c_str(), "steer_no_lean", finalSteerVal);
    }
}
