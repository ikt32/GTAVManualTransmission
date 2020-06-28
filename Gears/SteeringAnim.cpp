#include "SteeringAnim.h"

#include "StartingAnimation.h"

#include "Constants.h"
#include "Input/CarControls.hpp"
#include "ScriptUtils.h"
#include "ScriptSettings.hpp"

#include "Util/Logger.hpp"
#include "Util/MathExt.h"
#include "Util/Timer.h"
#include "Util/Files.h"

#include <inc/natives.h>
#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern ScriptSettings g_settings;
extern CarControls g_controls;

namespace {
    std::string animFile = std::string(Constants::ModDir) + "\\animations.yml";
    enum eAnimationFlags {
        ANIM_FLAG_NORMAL = 0,
        ANIM_FLAG_REPEAT = 1,
        ANIM_FLAG_STOP_LAST_FRAME = 2,
        ANIM_FLAG_UPPERBODY = 16,
        ANIM_FLAG_ENABLE_PLAYER_CONTROL = 32,
        ANIM_FLAG_CANCELABLE = 120,
    };

    enum eTaskTypeIndex {
        // For more, see https://pastebin.com/2gFqJ3Px by CamxxCore
        CTaskCloseVehicleDoorFromInside = 164,
    };

    std::vector<SteeringAnimation::Animation> steeringAnimations;
    SteeringAnimation::Animation lastAnimation;
    size_t steeringAnimIdx = 0;
    float setAngle = 0.0f;
    bool fileProblem = false;
}

namespace YAML {
    template<>
    struct convert<SteeringAnimation::Animation> {
        static bool decode(const Node& node, SteeringAnimation::Animation& anim) {
            anim.Dictionary = node["Dictionary"].as<std::string>();
            anim.Name = node["AnimName"].as<std::string>();
            anim.Rotation = node["Rotation"].as<float>();
            anim.Layouts = node["Layouts"].as<std::vector<std::string>>();
            return true;
        }
    };
}

void playAnimTime(const SteeringAnimation::Animation& anim, float time);
void cancelAnim(const SteeringAnimation::Animation& anim);
float mapAnim(float wheelDegrees, float maxAnimAngle);

void SteeringAnimation::SetFile(const std::string& cs) {
    animFile = cs;
}

bool SteeringAnimation::FileProblem() {
    return fileProblem;
}

const std::vector<SteeringAnimation::Animation>& SteeringAnimation::GetAnimations() {
    return steeringAnimations;
}

size_t SteeringAnimation::GetAnimationIndex() {
    return steeringAnimIdx;
}

void SteeringAnimation::SetAnimationIndex(size_t index) {
    cancelAnim(lastAnimation);
    steeringAnimIdx = index;
}

void SteeringAnimation::SetRotation(float wheelDegrees) {
    // No valid thing found?
    if (steeringAnimIdx >= steeringAnimations.size())
        return;

    float maxAnimAngle = steeringAnimations[steeringAnimIdx].Rotation;

    // div 2 cuz one-way angle
    float finalSteerVal = mapAnim(wheelDegrees, maxAnimAngle / 2.0f);

    setAngle = finalSteerVal;
}

void SteeringAnimation::Update() {
    bool steeringWheelSync = g_controls.PrevInput == CarControls::Wheel && g_settings.Wheel.Options.SyncRotation;
    bool customSteeringSync = g_settings.CustomSteering.Mode > 0 && g_settings.CustomSteering.CustomRotation;

    // Not active, cancel all animations.
    if (!Util::VehicleAvailable(g_playerVehicle, g_playerPed) ||
        !(steeringWheelSync || customSteeringSync) ||
        !g_settings.Misc.SyncAnimations ||
        PAD::IS_CONTROL_PRESSED(2, eControl::ControlVehicleAim) ||
        PED::IS_PED_DOING_DRIVEBY(g_playerPed) ||
        PLAYER::IS_PLAYER_PRESSING_HORN(PLAYER::PLAYER_ID()) ||
        PAD::IS_CONTROL_PRESSED(0, eControl::ControlVehicleDuck) ||
        StartingAnimation::Playing() ||
        PED::IS_PED_RUNNING_MOBILE_PHONE_TASK(g_playerPed) ||
        TASK::GET_IS_TASK_ACTIVE(g_playerPed, eTaskTypeIndex::CTaskCloseVehicleDoorFromInside) ||
        steeringAnimIdx >= steeringAnimations.size()) {
        cancelAnim(lastAnimation);
        return;
    }

    playAnimTime(
        steeringAnimations[steeringAnimIdx],
        setAngle);
}

void SteeringAnimation::Load() {
    if (!FileExists(animFile)) {
        logger.Write(ERROR, fmt::format("Animation: File \"{}\" not found, skipping animations", animFile));
        fileProblem = true;
        return;
    }

    try {
        cancelAnim(lastAnimation);
        YAML::Node animRoot = YAML::LoadFile(animFile);
        auto animNodes = animRoot["Animations"];

        steeringAnimations.clear();
        steeringAnimations = animRoot["Animations"].as<std::vector<Animation>>();
        logger.Write(DEBUG, fmt::format("Animation: Loaded {} animations", steeringAnimations.size()));
        fileProblem = false;
    }
    catch (const YAML::ParserException& ex) {
        logger.Write(ERROR, fmt::format("Encountered a YAML exception (parse)"));
        logger.Write(ERROR, fmt::format("{}", ex.what()));
        logger.Write(ERROR, fmt::format("at Line {}, Column {}", ex.mark.line, ex.mark.column));
        logger.Write(ERROR, fmt::format("msg: {}", ex.msg));
    }
    catch (const std::exception& ex) {
        logger.Write(ERROR, fmt::format("Encountered a YAML exception (std)"));
        logger.Write(ERROR, fmt::format("{}", ex.what()));
    }
}

void cancelAnim(const SteeringAnimation::Animation& anim) {
    if (anim.Dictionary.empty() || anim.Name.empty()) {
        return;
    }

    const char* dict = anim.Dictionary.c_str();
    const char* name = anim.Name.c_str();

    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict, name, 3);

    if (playing) {
        UI::Notify(DEBUG, fmt::format("Cancelled steering animation ({})", lastAnimation.Dictionary), false);
        TASK::STOP_ANIM_TASK(g_playerPed, dict, name, -8.0f);
        lastAnimation = SteeringAnimation::Animation();
    }
}

void playAnimTime(const SteeringAnimation::Animation& anim, float time) {
    if (anim.Dictionary.empty() || anim.Name.empty()) {
        return;
    }

    const char* dict = anim.Dictionary.c_str();
    const char* name = anim.Name.c_str();

    const bool playing = ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, dict, name, 3);

    if (!playing) {
        cancelAnim(lastAnimation);

        Timer t(500);

        if (!STREAMING::DOES_ANIM_DICT_EXIST(dict)) {
            UI::Notify(ERROR, fmt::format("Animation: dictionary does not exist [{}]", dict), false);
            logger.Write(ERROR, fmt::format("Animation: dictionary does not exist [{}]", dict));
            // Clear dict so we don't keep loading it
            steeringAnimations[steeringAnimIdx].Dictionary = std::string();
            return;
        }

        STREAMING::REQUEST_ANIM_DICT(dict);
        while (!STREAMING::HAS_ANIM_DICT_LOADED(dict)) {
            if (t.Expired()) {
                UI::Notify(ERROR, fmt::format("Failed to load animation dictionary [{}]", dict), false);
                logger.Write(ERROR, fmt::format("Animation: Failed to load dictionary [{}]", dict));
                // Clear dict so we don't keep loading it
                steeringAnimations[steeringAnimIdx].Dictionary = std::string();
                return;
            }
            WAIT(0);
        }

        constexpr int flag = ANIM_FLAG_ENABLE_PLAYER_CONTROL;
        TASK::TASK_PLAY_ANIM(g_playerPed, dict, name, -8.0f, 8.0f, -1, flag, 1.0f, 0, 0, 0);
        lastAnimation = anim;
        UI::Notify(DEBUG, fmt::format("Started steering animation ({})", lastAnimation.Dictionary), false);
    }
    else {
        ENTITY::SET_ENTITY_ANIM_SPEED(g_playerPed, dict, name, 0.0f);
        ENTITY::SET_ENTITY_ANIM_CURRENT_TIME(g_playerPed, dict, name, time);

        // Fix for if anim is playing while the script starts
        if (lastAnimation.Dictionary.empty()) {
            lastAnimation = anim;
        }
    }
}

// map
// -450 -360 -270 -180  -90    0    90  180  270  360  450 // wheeldegrees
//  0.0  0.0  0.0  0.0  0.0   0.5  1.0  1.0  1.0  1.0  1.0 // animTime @ 180 max
//  0.0  0.0  0.0  0.0        0.5       1.0  1.0  1.0  1.0 // animTime @ 360 max
//  0.0  0.0                  0.5                 1.0  1.0 // animTime @ 720 max
float mapAnim(float wheelDegrees, float maxAnimAngle) {
    float animTime = map(wheelDegrees, -maxAnimAngle, maxAnimAngle, 0.0f, 1.0f);
    animTime = std::clamp(animTime, 0.01f, 0.99f);
    return animTime;
}
