#include "StartingAnimation.h"

#include "ScriptUtils.h"
#include "SteeringAnim.h"
#include "inc/main.h"
#include "inc/natives.h"
#include "Memory/VehicleExtensions.hpp"
#include "Util/Timer.h"

#include <fmt/format.h>

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;

namespace {
    uint8_t lastEngineState = 0;
    bool forcePlay = false;
    SteeringAnimation::Animation currentAnim;
}

void StartingAnimation::Update() {
    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        uint8_t engineState = (VehicleExtensions::GetLightStates(g_playerVehicle) & 0x00F00000) >> 20;

        // We're starting the engine, so kick off the animation.
        if (engineState == 2 && lastEngineState != engineState || 
            forcePlay) {
            //showText(0.5f, 0.5f, 0.5f, "Starting");

            auto steeringAnimIdx = SteeringAnimation::GetAnimationIndex();
            auto steeringAnimations = SteeringAnimation::GetAnimations();

            if (steeringAnimIdx >= steeringAnimations.size())
                return;

            currentAnim = steeringAnimations[steeringAnimIdx];
            if (currentAnim.Dictionary.empty() || currentAnim.Name.empty())
                return;

            Timer t(500);

            if (STREAMING::DOES_ANIM_DICT_EXIST(currentAnim.Dictionary.c_str())) {
                STREAMING::REQUEST_ANIM_DICT(currentAnim.Dictionary.c_str());
                while (!STREAMING::HAS_ANIM_DICT_LOADED(currentAnim.Dictionary.c_str())) {
                    if (t.Expired()) {
                        currentAnim.Dictionary = std::string();
                        currentAnim.Name = std::string();
                        forcePlay = false;
                        return;
                    }
                    WAIT(0);
                }
                currentAnim.Name = "start_engine";
                constexpr int flag = 32;
                TASK::TASK_PLAY_ANIM(g_playerPed, currentAnim.Dictionary.c_str(), "start_engine", -8.0f, 8.0f, -1, flag, 0.2f, 0, 0, 0);
                forcePlay = false;
                //UI::Notify(INFO, "Starting");
            }
        }

        lastEngineState = engineState;
    }
}

bool StartingAnimation::Playing() {
    return !currentAnim.Dictionary.empty() &&
        ENTITY::IS_ENTITY_PLAYING_ANIM(g_playerPed, currentAnim.Dictionary.c_str(), "start_engine", 3);
}

void StartingAnimation::PlayManual() {
    forcePlay = true;
}
