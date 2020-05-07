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
extern ScriptSettings g_settings;

// TODO: Ew.
int g_steerAnimDictIdx = 0;

// TODO:
// - "Action" animation pass-through (Cancel current animation?)
// - Animation dictionary detection
// - Reorder whatever mess I made in this file

namespace {
    enum eAnimationFlags {
        ANIM_FLAG_NORMAL = 0,
        ANIM_FLAG_REPEAT = 1,
        ANIM_FLAG_STOP_LAST_FRAME = 2,
        ANIM_FLAG_UPPERBODY = 16,
        ANIM_FLAG_ENABLE_PLAYER_CONTROL = 32,
        ANIM_FLAG_CANCELABLE = 120,
    };

    const std::vector<SteeringAnimation::Animation> steeringAnimations{
        { "veh@std@ds@base", "steer_no_lean", 720.0f },
        { "veh@low@front_ds@base", "steer_no_lean", 360.0f },
        { "veh@truck@ds@base", "steer_no_lean", 720.0f },
        { "veh@van@ds@base", "steer_no_lean", 720.0f },
        { "anim@veh@lowrider@std@ds@arm@base", "steer_no_lean", 360.0f },
        { "veh@bus@bus@driver@base", "steer_no_lean", 720.0f },
    };

    SteeringAnimation::Animation lastAnimation;

    float setAngle = 0.0f;
}

void playAnimTime(const SteeringAnimation::Animation& anim, float time);
void cancelAnim(const SteeringAnimation::Animation& anim);

const std::vector<SteeringAnimation::Animation>& SteeringAnimation::GetAnimations() {
    return steeringAnimations;
}

void SteeringAnimation::Update() {
    // Not active, cancel all animations.
    if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed) || 
        !g_settings.Misc.SyncAnimations) {
        cancelAnim(lastAnimation);
        return;
    }

    playAnimTime(
        steeringAnimations[g_steerAnimDictIdx],
        setAngle);
}

// map
// -450 -360 -270 -180  -90    0    90  180  270  360  450 // wheeldegrees
//  0.0  0.0  0.0  0.0  0.0   0.5  1.0  1.0  1.0  1.0  1.0 // animTime @ 180 max
//  0.0  0.0  0.0  0.0        0.5       1.0  1.0  1.0  1.0 // animTime @ 360 max
//  0.0  0.0                  0.5                 1.0  1.0 // animTime @ 720 max
float figureOutAnimModulation(float wheelDegrees, float maxAnimAngle) {    
    float animTime = map(wheelDegrees, -maxAnimAngle, maxAnimAngle, 0.0f, 1.0f);
    animTime = std::clamp(animTime, 0.01f, 0.99f);
    return animTime;
}

void SteeringAnimation::SetRotation(float wheelDegrees) {
    float maxAnimAngle = steeringAnimations[g_steerAnimDictIdx].Rotation;

    // div 2 cuz one-way angle
    float finalSteerVal = figureOutAnimModulation(wheelDegrees, maxAnimAngle / 2.0f);

    setAngle = finalSteerVal;
}

void cancelAnim(const SteeringAnimation::Animation& anim) {
    const char* dict = anim.Dictionary.c_str();
    const char* name = anim.Name.c_str();

    if (anim.Dictionary.empty() || anim.Name.empty()) {
        lastAnimation = SteeringAnimation::Animation();
        return;
    }

    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict, name, 3);

    if (playing) {
        // Why do the top 3 not work? No idea!
        //ENTITY::STOP_ENTITY_ANIM(g_playerPed, dict, name, 8.0f);
        //AI::STOP_ANIM_TASK(g_playerPed, dict, name, 8.0f);
        //AI::CLEAR_PED_TASKS_IMMEDIATELY(g_playerPed);
        AI::CLEAR_PED_SECONDARY_TASK(g_playerPed);
        lastAnimation = SteeringAnimation::Animation();
    }
}

void playAnimTime(const SteeringAnimation::Animation& anim, float time) {
    const char* dict = anim.Dictionary.c_str();
    const char* name = anim.Name.c_str();

    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict, name, 3);

    if (!playing) {
        cancelAnim(lastAnimation);

        Timer t(500);

        STREAMING::REQUEST_ANIM_DICT(dict);
        while (!STREAMING::HAS_ANIM_DICT_LOADED(dict)) {
            if (t.Expired()) {
                UI::Notify(ERROR, "Failed to load animation dictionary [%s]", dict);
                break;
            }
            WAIT(0);
        }

        constexpr int flag = ANIM_FLAG_UPPERBODY | ANIM_FLAG_ENABLE_PLAYER_CONTROL;
        AI::TASK_PLAY_ANIM(g_playerPed, dict, name, -8.0f, 8.0f, -1, flag, 1.0f, 0, 0, 0);
        lastAnimation = anim;
    }
    else {
        ENTITY::SET_ENTITY_ANIM_SPEED(g_playerPed, dict, name, 0.0f);
        ENTITY::SET_ENTITY_ANIM_CURRENT_TIME(g_playerPed, dict, name, time);
    }
}
