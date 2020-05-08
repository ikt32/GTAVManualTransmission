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
// - User-definable animations
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

    // List from a63nt-5m1th @ 5mods:
    // https://forums.gta5-mods.com/topic/23755/how-do-you-change-the-character-s-driving-animation
    const std::vector<SteeringAnimation::Animation> steeringAnimations{
        { "veh@std@ds@base", "steer_no_lean", 720.0f, {
            "LAYOUT_STANDARD",
            "LAYOUT_STD_LOWROOF",
            "LAYOUT_STD_HABANERO",
            "LAYOUT_STD_HIGHWINDOW",
            "LAYOUT_STD_EXITFIXUP",
            "LAYOUT_STD_RIPLEY",
            "LAYOUT_STD_STRATUM",
            "LAYOUT_STD_STRETCH",
            "LAYOUT_STD_ZTYPE",
            "LAYOUT_RANGER",
            "LAYOUT_RANGER_SWAT", // Granger
            // Add-on vehicles
            "LAYOUT_STD_RHD",
            "LAYOUT_STD_AE86", // FreedomGundam's AE86
            "LAYOUT_STD_SIM22", // Zievs' 22B
        }},
        { "veh@low@front_ds@base", "steer_no_lean", 360.0f, {
            "LAYOUT_LOW",
            "LAYOUT_LOW_RESTRICTED",
            "LAYOUT_LOW_BFINJECTION",
            "LAYOUT_LOW_CHEETAH",
            "LAYOUT_LOW_DUNE",
            "LAYOUT_LOW_ENTITYXF",
            "LAYOUT_LOW_INFERNUS",
            "LAYOUT_LOW_SENTINEL2",
            // Add-on vehicles
            "LAYOUT_LOW_RHD",
        }},
        { "veh@truck@ds@base", "steer_no_lean", 720.0f, {
            "LAYOUT_TRUCK",
            "LAYOUT_TRUCK_BIFF",
            "LAYOUT_TRUCK_TOW",
            "LAYOUT_TRUCK_DOCKTUG",
            "LAYOUT_TRUCK_BARRACKS",
            "LAYOUT_TRUCK_MIXER",
            "LAYOUT_FIRETRUCK",
            "LAYOUT_DUMPTRUCK",
        }},
        { "veh@van@ds@base", "steer_no_lean", 720.0f, {
            "LAYOUT_VAN",
            "LAYOUT_VAN_POLICE",
            "LAYOUT_VAN_BODHI",
            "LAYOUT_VAN_CADDY",
            "LAYOUT_VAN_MULE",
            "LAYOUT_VAN_PONY",
            "LAYOUT_VAN_BOXVILLE",
            "LAYOUT_VAN_TRASH",
            "LAYOUT_VAN_JOURNEY",
            "LAYOUT_ONE_DOOR_VAN",
            "LAYOUT_RIOT_VAN",
        }},
        { "veh@bus@bus@driver@base", "steer_no_lean", 720.0f, {
            "LAYOUT_BUS",
            "LAYOUT_TOURBUS",
            "LAYOUT_PRISON_BUS",
            "LAYOUT_COACH",
        }},
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
        !g_settings.Misc.SyncAnimations ||
        PLAYER::IS_PLAYER_FREE_AIMING(PLAYER::PLAYER_ID()) ||
        PLAYER::IS_PLAYER_PRESSING_HORN(PLAYER::PLAYER_ID())) {
        cancelAnim(lastAnimation);
        return;
    }

    // No valid thing found?
    if (g_steerAnimDictIdx >= steeringAnimations.size())
        return;

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
    // No valid thing found?
    if (g_steerAnimDictIdx >= steeringAnimations.size())
        return;

    float maxAnimAngle = steeringAnimations[g_steerAnimDictIdx].Rotation;

    // div 2 cuz one-way angle
    float finalSteerVal = figureOutAnimModulation(wheelDegrees, maxAnimAngle / 2.0f);

    setAngle = finalSteerVal;
}

void cancelAnim(const SteeringAnimation::Animation& anim) {
    const char* dict = anim.Dictionary.c_str();
    const char* name = anim.Name.c_str();

    if (anim.Dictionary.empty() || anim.Name.empty()) {
        return;
    }

    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict, name, 3);

    if (playing) {
        UI::Notify(DEBUG, fmt::format("Cancelled steering animation ({})", lastAnimation.Dictionary), false);
        AI::STOP_ANIM_TASK(g_playerPed, dict, name, -8.0f);
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
                UI::Notify(ERROR, fmt::format("Failed to load animation dictionary [{}}]", dict), false);
                break;
            }
            WAIT(0);
        }

        constexpr int flag = ANIM_FLAG_UPPERBODY | ANIM_FLAG_ENABLE_PLAYER_CONTROL;
        AI::TASK_PLAY_ANIM(g_playerPed, dict, name, -8.0f, 8.0f, -1, flag, 1.0f, 0, 0, 0);
        lastAnimation = anim;
        UI::Notify(DEBUG, fmt::format("Started steering animation ({})", lastAnimation.Dictionary), false);

    }
    else {
        ENTITY::SET_ENTITY_ANIM_SPEED(g_playerPed, dict, name, 0.0f);
        ENTITY::SET_ENTITY_ANIM_CURRENT_TIME(g_playerPed, dict, name, time);
    }
}
