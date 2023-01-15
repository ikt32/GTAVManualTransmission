#include "script.h"
#include "GitInfo.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "UpdateChecker.h"
#include "Constants.h"
#include "Compatibility.h"
#include "CustomSteering.h"
#include "WheelInput.h"
#include "SteeringAnim.h"
#include "VehicleConfig.h"
#include "AtcuLogic.h"
#include "Misc.h"
#include "StartingAnimation.h"
#include "DrivingAssists.h"
#include "ScriptHUD.h"
#include "AWD.h"
#include "LaunchControl.h"
#include "CruiseControl.h"
#include "SpeedLimiter.h"

#include "Dashboard.h"
#include "GearRattle.h"
#include "Textures.h"

#include "UDPTelemetry/Socket.h"
#include "UDPTelemetry/UDPTelemetry.h"

#include "Memory/MemoryPatcher.hpp"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleBone.h"
#include "Memory/VehicleFlags.h"

#include "Input/CarControls.hpp"

#include "Util/ScriptUtils.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "Util/Timer.h"
#include "Util/ValueTimer.h"
#include "Util/GameSound.h"
#include "Util/SysUtils.h"
#include "Util/Strings.hpp"
#include "Util/MiscEnums.h"

#include <menu.h>

#include <inc/natives.h>
#include <inc/enums.h>
#include <inc/main.h>
#include <inc/types.h>

#include <fmt/format.h>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <filesystem>
#include <numeric>
#include <fstream>

namespace fs = std::filesystem;
using VExt = VehicleExtensions;

ReleaseInfo g_releaseInfo;
std::mutex g_releaseInfoMutex;

bool g_notifyUpdate;
std::mutex g_notifyUpdateMutex;

bool g_checkUpdateDone;
std::mutex g_checkUpdateDoneMutex;

Socket g_socket;

NativeMenu::Menu g_menu;
CarControls g_controls;
ScriptSettings g_settings;

Player g_player;
Ped g_playerPed;
Vehicle g_playerVehicle;
Vehicle g_lastPlayerVehicle;

VehiclePeripherals g_peripherals;
VehicleGearboxStates g_gearStates;
WheelPatchStates g_wheelPatchStates;

VehicleData g_vehData;

std::vector<VehicleConfig> g_vehConfigs;

bool g_focused;
Timer g_wheelInitDelayTimer(0);
Timer g_lastUpdateTimer(0);

std::vector<ValueTimer<float>> g_speedTimers;

GameSound g_downshiftProtectSfx("CONFIRM_BEEP", "HUD_MINI_GAME_SOUNDSET", "");

void updateShifting();
void blockButtons();
void startStopEngine();
void functionAutoReverse();
void functionRealReverse();
void updateLastInputDevice();
void updateSteeringMultiplier();

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void functionHShiftTo(int i);
void functionHShiftKeyboard();
void functionHShiftWheel();
void functionSShift();
void functionAShift();

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch();
void functionEngStall();
void functionEngDamage();
void functionEngBrake();
void functionEngLock();

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void handleBrakePatch();
void handleRPM();
void functionLimiter();

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback();
void functionAutoGear1();
void functionHillGravity();

void UpdatePause();

void setVehicleConfig(Vehicle vehicle) {
    std::string oldName;
    if (g_settings.ConfigActive()) {
        oldName = g_settings().Name;
    }

    g_settings.SetVehicleConfig(nullptr);

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        auto currModel = ENTITY::GET_ENTITY_MODEL(vehicle);
        auto itModelMatch = std::find_if(g_vehConfigs.begin(), g_vehConfigs.end(),
            [currModel, vehicle](const VehicleConfig & config) {
                auto modelsIt = std::find_if(config.ModelNames.begin(), config.ModelNames.end(),
                    [currModel](const std::string & modelName) {
                        return joaat(modelName.c_str()) == currModel;
                    });
                if (modelsIt != config.ModelNames.end())
                    return true;
                return false;
            });

        auto itPlateMatch = std::find_if(g_vehConfigs.begin(), g_vehConfigs.end(),
            [currModel, vehicle](const VehicleConfig& config) {
                auto modelsIt = std::find_if(config.ModelNames.begin(), config.ModelNames.end(),
                    [currModel](const std::string& modelName) {
                        return joaat(modelName.c_str()) == currModel;
                    });
                auto platesIt = std::find_if(config.Plates.begin(), config.Plates.end(),
                    [vehicle](const std::string& plate) {
                        return StrUtil::toLower(plate) == StrUtil::toLower(VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicle));
                    });
                if (platesIt != config.Plates.end() && modelsIt != config.ModelNames.end())
                    return true;
                return false;
            });

        auto itMatch = itPlateMatch;
        if (itMatch == g_vehConfigs.end())
            itMatch = itModelMatch;

        if (itMatch != g_vehConfigs.end()) {
            g_settings.SetVehicleConfig(&*itMatch);
            if (itMatch->Name != oldName) {
                UI::Notify(INFO, fmt::format("Configuration [{}] loaded.", itMatch->Name));
            }
        }
    }
}

void update_player() {
    g_player = PLAYER::PLAYER_ID();
    g_playerPed = PLAYER::PLAYER_PED_ID();
}

void updateActiveSteeringAnim(Vehicle vehicle) {
    const auto& anims = SteeringAnimation::GetAnimations();
    auto layoutHash = VEHICLE::GET_VEHICLE_LAYOUT_HASH(vehicle);
    size_t animIdx = anims.size();
    for (size_t i = 0; i < anims.size(); ++i) {
        bool found = false;
        for (const std::string& layout : anims[i].Layouts) {
            if (joaat(layout.c_str()) == layoutHash) {
                animIdx = i;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }
    if (animIdx == anims.size() && layoutHash != 0) {
        std::string msg = fmt::format("Animation: No valid animation found for layout hash 0x{:08X}", layoutHash);
        logger.Write(WARN, msg);
        UI::Notify(DEBUG, msg);
    }
    SteeringAnimation::SetAnimationIndex(animIdx);
}

void update_vehicle() {
    g_playerVehicle = PED::GET_VEHICLE_PED_IS_IN(g_playerPed, false);
    bool vehAvail = Util::VehicleAvailable(g_playerVehicle, g_playerPed);
    if (!vehAvail) {
        g_playerVehicle = 0;
    }

    // Reset vehicle stats on vehicle change (or leave)
    if (g_playerVehicle != g_lastPlayerVehicle) {
        g_peripherals = VehiclePeripherals();
        g_gearStates = VehicleGearboxStates();
        g_wheelPatchStates = WheelPatchStates();
        g_vehData.SetVehicle(g_playerVehicle); // assign new vehicle;
        functionHidePlayerInFPV(true);
        setVehicleConfig(g_playerVehicle);
        GearRattle::Stop();
        updateActiveSteeringAnim(g_playerVehicle);
        
        g_controls.PlayFFBDynamics(0, 0);
        g_controls.PlayFFBCollision(0);

        if (g_playerVehicle != 0) {
            VExt::SetSteeringAngle(g_playerVehicle, 0.0f);
            VExt::SetSteeringInputAngle(g_playerVehicle, 0.0f);

            VehicleBones::RegisterMatrix(g_playerVehicle, "needle_speedo");
            VehicleBones::RegisterMatrix(g_playerVehicle, "needle_torque");
            VehicleBones::RegisterMatrix(g_playerVehicle, "steeringwheel");
        }
    }

    if (vehAvail) {
        g_vehData.Update(); // Update before doing anything else

        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
            g_peripherals.IgnitionState = IgnitionState::On;
        }
        else if (!(g_peripherals.IgnitionState == IgnitionState::Stall)) {
            // Engine off, but not stalled
            g_peripherals.IgnitionState = IgnitionState::Off;
        }
    }
    if (g_playerVehicle != g_lastPlayerVehicle && vehAvail) {
        if (g_vehData.mIsCVT)
            g_gearStates.FakeNeutral = false;
        else
            g_gearStates.FakeNeutral = g_settings.GameAssists.DefaultNeutral;
    }

    if (g_settings.Debug.Metrics.EnableTimers && vehAvail) {
        for(auto& valueTimer : g_speedTimers) {
            float speed;
            switch(joaat(valueTimer.mUnit.c_str())) {
                case joaat("kph"):
                    speed = Length(g_vehData.mVelocity) * 3.6f;
                    break;
                case joaat("mph"):
                    speed = Length(g_vehData.mVelocity) / 0.44704f;
                    break;
                default:
                    speed = Length(g_vehData.mVelocity);
            }
            valueTimer.Update(speed);
        }
    }
    
    g_lastPlayerVehicle = g_playerVehicle;
}

// Read inputs
void update_inputs() {
    if (g_lastUpdateTimer.Elapsed() > 1000) {
        // It's been >0.1 second between messages, the game probably has been resumed after pause.
        // Let's reset the wheel.
        logger.Write(DEBUG, "[Wheel] %d millis since last update, re-init FFB", g_lastUpdateTimer.Elapsed());
        g_wheelInitDelayTimer.Reset(1);
    }
    g_lastUpdateTimer.Reset();

    if (g_focused != SysUtil::IsWindowFocused()) {
        // no focus -> focus
        if (!g_focused) {
            logger.Write(DEBUG, "[Wheel] Window focus gained: re-initializing FFB");
            g_wheelInitDelayTimer.Reset(500);
        }
        else {
            logger.Write(DEBUG, "[Wheel] Window focus lost");
        }
    }
    g_focused = SysUtil::IsWindowFocused();

    if (g_wheelInitDelayTimer.Expired() && g_wheelInitDelayTimer.Period() > 0) {
        g_controls.GetWheel().Acquire();
        g_controls.PlayFFBDynamics(0, 0);
        g_controls.PlayFFBCollision(0);
        g_wheelInitDelayTimer.Reset(0);
    }

    updateLastInputDevice();
    g_controls.UpdateValues(g_controls.PrevInput, false);
}

void wheelControlWater() {
    WheelInput::CheckButtons();
    WheelInput::HandlePedalsArcade(g_controls.ThrottleVal, g_controls.BrakeVal);
    WheelInput::DoSteering();
    WheelInput::PlayFFBWater();
}

void wheelControlRoad() {
    WheelInput::CheckButtons();
    if (g_settings.MTOptions.Enable && 
        !(g_vehData.mClass == VehicleClass::Bike && g_settings.GameAssists.SimpleBike)) {
        WheelInput::HandlePedals(g_controls.ThrottleVal, g_controls.BrakeVal);
    }
    else {
        WheelInput::HandlePedalsArcade(g_controls.ThrottleVal, g_controls.BrakeVal);
    }
    WheelInput::DoSteering();
    if (g_vehData.mIsAmphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(g_playerVehicle) > 0.10f ||
        VExt::GetHoverTransformRatio(g_playerVehicle) >= 0.5f) {
        WheelInput::PlayFFBWater();
    }
    else {
        WheelInput::PlayFFBGround();
    }
}

// Apply input as controls for selected devices
void update_input_controls() {
    if (!Util::PlayerAvailable(g_player, g_playerPed)) {
        return;
    }

    blockButtons();
    startStopEngine();

    if (!g_settings.Wheel.Options.Enable || g_controls.PrevInput != CarControls::Wheel) {
        return;
    }

    if (!g_controls.WheelAvailable())
        return;

    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        switch (g_vehData.mDomain) {
        case VehicleDomain::Road: {
            wheelControlRoad();
            break;
        }
        case VehicleDomain::Water: {
            wheelControlWater();
            break;
        }
        case VehicleDomain::Air: {
            // not supported
            break;
        }
        case VehicleDomain::Bicycle: {
            // not supported
            break;
        }
        case VehicleDomain::Rail: {
            // not supported
            break;
        }
        case VehicleDomain::Unknown: {
            break;
        }
        default: { }
        }
    }
}

// don't write with VehicleExtensions and dont set clutch state
void update_misc_features() {
    if (!Util::PlayerAvailable(g_player, g_playerPed))
        return;

    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        if (g_settings.GameAssists.AutoLookBack && g_vehData.mClass != VehicleClass::Heli) {
            functionAutoLookback();
        }
    }

    functionHidePlayerInFPV(false);

    if (g_settings().DriveAssists.ABS.Flash && g_controls.BrakeVal > 0.0f && DashLights::AbsNotify) {
        // Don't flash the initial 500ms
        bool delayPassed = MISC::GET_GAME_TIMER() > DashLights::LastAbsTrigger + 500;
        if (delayPassed) {
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, DashLights::AbsBulbState);
        }
    }
}

// Only when mod is working or writes clutch stuff.
// Called inside manual transmission part!
// Mostly optional stuff.
void update_manual_features() {
    if (g_settings.GameAssists.HillGravity) {
        functionHillGravity();
    }

    if (g_controls.PrevInput != CarControls::InputDevices::Wheel) {
        if (g_vehData.mClass == VehicleClass::Bike && g_settings.GameAssists.SimpleBike) {
            functionAutoReverse();
        }
        else {
            functionRealReverse();
        }
    }

    if (g_settings.MTOptions.EngBrake) {
        functionEngBrake();
    }
    else {
        g_wheelPatchStates.EngBrakeActive = false;
    }

    // Engine damage: RPM Damage
    if (g_settings.MTOptions.EngDamage && g_vehData.mHasClutch) {
        functionEngDamage();
    }

    if (g_settings.MTOptions.EngLock && g_vehData.mHasClutch) {
        functionEngLock();
    }
    else {
        g_wheelPatchStates.EngLockActive = false;
    }

    if (!(g_settings.GameAssists.SimpleBike && g_vehData.mClass == VehicleClass::Bike) && g_vehData.mHasClutch) {
        // Stalling
        if (g_settings.MTOptions.EngStallH && g_settings().MTOptions.ShiftMode == EShiftMode::HPattern ||
            g_settings.MTOptions.EngStallS && g_settings().MTOptions.ShiftMode == EShiftMode::Sequential) {
            functionEngStall();
        }

        // Simulate "catch point"
        // When the clutch "grabs" and the car starts moving without input
        if (g_settings().MTOptions.ClutchCreep && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
            functionClutchCatch();
        }
    }

    if (g_settings().DriveAssists.AWD.Enable) {
        AWD::Update();
    }

    handleBrakePatch();
}

// Manual Transmission part of the mod
void update_manual_transmission() {
    if (!Util::PlayerAvailable(g_player, g_playerPed) || !Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        return;
    }

    //updateSteeringMultiplier();

    // TODO: Move these button checks somewhere else.

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::Toggle) ||
        g_controls.ButtonHeld(CarControls::WheelControlType::Toggle, 500) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::ControllerControlType::Toggle) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::Toggle)) {
        toggleManual(!g_settings.MTOptions.Enable);
        return;
    }

    if (g_vehData.mDomain != VehicleDomain::Road || !g_settings.MTOptions.Enable)
        return;

    ///////////////////////////////////////////////////////////////////////////
    //          Active whenever Manual is enabled from here
    ///////////////////////////////////////////////////////////////////////////

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleH) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleH) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::ControllerControlType::ToggleH) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::ToggleH)) {
        setShiftMode(Next(g_settings().MTOptions.ShiftMode));
    }

    auto lightStates = VExt::GetLightStates(g_playerVehicle);

    bool indicatorRight = (lightStates & EVehicleLightState::LightStateIndicatorRight) > 0;
    bool indicatorLeft = (lightStates & EVehicleLightState::LightStateIndicatorLeft) > 0;
    bool indicatorHazards = indicatorLeft && indicatorRight;

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::IndicatorLeft)) {
        if (!indicatorHazards) {
            if (indicatorLeft) {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, false);
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, false);
            }
            else {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, false);
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, true);
            }
        }
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::IndicatorRight)) {
        if (!indicatorHazards) {
            if (indicatorRight) {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, false);
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, false);
            }
            else {
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, true);
                VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, false);
            }
        }
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::IndicatorHazard)) {
        if (!indicatorHazards) {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, true);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, true);
        }
        else {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorRight, false);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, EIndicators::IndicatorLeft, false);
        }
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::CycleAssists) || 
        g_controls.ButtonJustPressed(CarControls::WheelControlType::CycleAssists) ||
        g_controls.ButtonHeld(CarControls::ControllerControlType::CycleAssists) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::CycleAssists)) {

        uint8_t currMode = 0;
        // 3: ABS + TC + SC
        // 2: ABS + TC
        // 1: ABS
        // 0: None!

        currMode += g_settings().DriveAssists.ABS.Enable;
        currMode += g_settings().DriveAssists.ESP.Enable;
        currMode += g_settings().DriveAssists.TCS.Enable;

        if (currMode == 0) {
            currMode = 3;
        }
        else {
            currMode = currMode - 1;
        }

        switch(currMode) {
            case 3:
                UI::Notify(INFO, "Assists: ABS, ESC, TCS");
                g_settings().DriveAssists.ABS.Enable = true;
                g_settings().DriveAssists.ESP.Enable = true;
                g_settings().DriveAssists.TCS.Enable = true;
                break;
            case 2:
                UI::Notify(INFO, "Assists: ABS + TCS");
                g_settings().DriveAssists.ABS.Enable = true;
                g_settings().DriveAssists.ESP.Enable = false;
                g_settings().DriveAssists.TCS.Enable = true;
                break;
            case 1:
                UI::Notify(INFO, "Assists: ABS only");
                g_settings().DriveAssists.ABS.Enable = true;
                g_settings().DriveAssists.ESP.Enable = false;
                g_settings().DriveAssists.TCS.Enable = false;
                break;
            case 0:
                UI::Notify(INFO, "Assists: None");
                g_settings().DriveAssists.ABS.Enable = false;
                g_settings().DriveAssists.ESP.Enable = false;
                g_settings().DriveAssists.TCS.Enable = false;
                break;
            default:
                UI::Notify(ERROR, "Assists: Switched to an invalid mode?");
                break;
        }
    }

    if (g_settings().DriveAssists.AWD.Enable) {
        if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::DriveBiasFInc) ||
            g_controls.ButtonJustPressed(CarControls::WheelControlType::DriveBiasFInc)) {
            float bias = g_settings().DriveAssists.AWD.CustomBaseBias;
            float customMax = g_settings().DriveAssists.AWD.CustomMax;
            bias += 0.05f;
            bias = std::clamp(
                round(bias * 20.0f) / 20.0f,
                0.01f,
                customMax);
            g_settings().DriveAssists.AWD.CustomBaseBias = bias;
            UI::Notify(INFO, fmt::format("Drive bias: {:.0f}F/{:.0f}R", bias * 100.0f, (1.0f - bias) * 100.0f), true);
        }

        if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::DriveBiasFDec) ||
            g_controls.ButtonJustPressed(CarControls::WheelControlType::DriveBiasFDec)) {
            float bias = g_settings().DriveAssists.AWD.CustomBaseBias;
            float customMin = g_settings().DriveAssists.AWD.CustomMin;
            bias -= 0.05f;
            bias = std::clamp(
                round(bias * 20.0f) / 20.0f,
                customMin,
                0.99f);
            g_settings().DriveAssists.AWD.CustomBaseBias = bias;
            UI::Notify(INFO, fmt::format("Drive bias: {:.0f}F/{:.0f}R", bias * 100.0f, (1.0f - bias) * 100.0f), true);
        }
    }

    if (g_settings().DriveAssists.CruiseControl.Enable) {
            if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleCC) ||
            g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleCC) ||
            g_controls.ButtonHeld(CarControls::ControllerControlType::ToggleCC) ||
            g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::ToggleCC)) {
            CruiseControl::SetActive(!CruiseControl::GetActive());
        }

        if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::CCInc) ||
            g_controls.ButtonJustPressed(CarControls::WheelControlType::CCInc)) {
            float speedValMul;
            std::string speedNameUnit = GetSpeedUnitMultiplier(g_settings.HUD.Speedo.Speedo, speedValMul);
            float speed = g_settings().DriveAssists.CruiseControl.Speed;

            float speedValUnit = speed * speedValMul;
            speedValUnit = std::clamp(speedValUnit + 5.0f, 0.0f, 500.0f);
            g_settings().DriveAssists.CruiseControl.Speed = speedValUnit / speedValMul;
            UI::Notify(INFO, fmt::format("Cruise control {:.0f} {}", speedValUnit, speedNameUnit), true);
        }

        if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::CCDec) ||
            g_controls.ButtonJustPressed(CarControls::WheelControlType::CCDec)) {
            float speedValMul;
            std::string speedNameUnit = GetSpeedUnitMultiplier(g_settings.HUD.Speedo.Speedo, speedValMul);
            float speed = g_settings().DriveAssists.CruiseControl.Speed;

            float speedValUnit = speed * speedValMul;
            speedValUnit = std::clamp(speedValUnit - 5.0f, 0.0f, 500.0f);
            g_settings().DriveAssists.CruiseControl.Speed = speedValUnit / speedValMul;
            UI::Notify(INFO, fmt::format("Cruise control {:.0f} {}", speedValUnit, speedNameUnit), true);
        }
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleLC) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleLC) ||
        g_controls.ButtonHeld(CarControls::ControllerControlType::ToggleLC) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::ToggleLC)) {
        bool newValue = !g_settings().DriveAssists.LaunchControl.Enable;
        g_settings().DriveAssists.LaunchControl.Enable = newValue;
        UI::Notify(INFO, fmt::format("Launch control {}", newValue ? "~g~ON" : "~r~OFF"));
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleABS) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleABS)) {
        bool newState = !g_settings().DriveAssists.ABS.Enable;
        UI::Notify(INFO, fmt::format("ABS {}", newState ? "~g~ON" : "~r~OFF"));
        g_settings().DriveAssists.ABS.Enable = newState;
    }
    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleESC) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleESC)) {
        bool newState = !g_settings().DriveAssists.ESP.Enable;
        UI::Notify(INFO, fmt::format("ESC {}", newState ? "~g~ON" : "~r~OFF"));
        g_settings().DriveAssists.ESP.Enable = newState;
    }
    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleTCS) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleTCS)) {
        bool newState = !g_settings().DriveAssists.TCS.Enable;
        UI::Notify(INFO, fmt::format("TCS {}", newState ? "~g~ON" : "~r~OFF"));
        g_settings().DriveAssists.TCS.Enable = newState;
    }

    if (MemoryPatcher::NumGearboxPatched != MemoryPatcher::NumGearboxPatches) {
        MemoryPatcher::ApplyGearboxPatches();
    }
    update_manual_features();

    switch(g_settings().MTOptions.ShiftMode) {
        case EShiftMode::Sequential: {
            functionSShift();
            if (g_settings.GameAssists.AutoGear1) {
                functionAutoGear1();
            }
            break;
        }
        case EShiftMode::HPattern: {
            if (g_controls.PrevInput == CarControls::Wheel) {
                functionHShiftWheel();
            }
            if (g_controls.PrevInput == CarControls::Keyboard ||
                g_controls.PrevInput == CarControls::Wheel && g_settings.Wheel.Options.HPatternKeyboard) {
                functionHShiftKeyboard();
            }
            break;
        }
        case EShiftMode::Automatic: {
            functionAShift();
            break;
        }
        default: break;
    }

    functionLimiter();

    updateShifting();

    // Finally, update memory each loop
    handleRPM();
    VExt::SetGearCurr(g_playerVehicle, g_gearStates.LockGear);
    VExt::SetGearNext(g_playerVehicle, g_gearStates.LockGear);
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void clearPatches() {
    resetSteeringMultiplier();
    if (MemoryPatcher::NumGearboxPatched != 0) {
        MemoryPatcher::RevertGearboxPatches();
    }
    if (MemoryPatcher::SteeringControlPatcher.Patched()) {
        MemoryPatcher::RestoreSteeringControl();
    }
    if (MemoryPatcher::SteeringAssistPatcher.Patched()) {
        MemoryPatcher::RestoreSteeringAssist();
    }
    if (MemoryPatcher::BrakePatcher.Patched()) {
        MemoryPatcher::RestoreBrake();
    }
    if (MemoryPatcher::ThrottlePatcher.Patched()) {
        MemoryPatcher::RestoreThrottle();
    }
    if (MemoryPatcher::ThrottleControlPatcher.Patched()) {
        MemoryPatcher::ThrottleControlPatcher.Restore();
    }
}

void toggleManual(bool enable) {
    GearRattle::Stop();

    // Don't need to do anything
    if (g_settings.MTOptions.Enable == enable)
        return;
    g_settings.MTOptions.Enable = enable;
    g_settings.SaveGeneral();
    if (g_settings.MTOptions.Enable) {
        UI::Notify(INFO, "MT Enabled");
    }
    else {
        UI::Notify(INFO, "MT Disabled");
    }
    readSettings();
    initWheel();
    clearPatches();
    g_vehData.SetVehicle(g_playerVehicle);
}

void initWheel() {
    g_controls.InitWheel();
    g_controls.CheckGUIDs(g_settings.Wheel.InputDevices.RegisteredGUIDs);
    logger.Write(INFO, "[Wheel] Steering wheel initialization finished");
}

void update_steering() {
    bool isCar = g_vehData.mClass == VehicleClass::Car;
    bool useWheel = g_controls.PrevInput == CarControls::Wheel;
    bool customSteering = g_settings.CustomSteering.Mode > 0 && !useWheel;
    bool hasControl = PLAYER::IS_PLAYER_CONTROL_ON(g_player) && 
        PED::IS_PED_SITTING_IN_VEHICLE(g_playerPed, g_playerVehicle);

    if (!hasControl) {
        customSteering = false;
        useWheel = false;
    }

    if (isCar && (useWheel || customSteering)) {
        if (!MemoryPatcher::SteeringAssistPatcher.Patched())
            MemoryPatcher::PatchSteeringAssist();
    }
    else {
        if (MemoryPatcher::SteeringAssistPatcher.Patched())
            MemoryPatcher::RestoreSteeringAssist();
    }

    if (isCar && (useWheel || customSteering)) {
        if (!MemoryPatcher::SteeringControlPatcher.Patched())
            MemoryPatcher::PatchSteeringControl();
    }
    else {
        if (MemoryPatcher::SteeringControlPatcher.Patched())
            MemoryPatcher::RestoreSteeringControl();
    }

    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        updateSteeringMultiplier();
    }

    bool isRoad = g_vehData.mDomain == VehicleDomain::Road;

    if (isRoad && customSteering) {
        CustomSteering::Update();
    }
}

void updateSteeringMultiplier() {
    float mult;

    if (g_controls.PrevInput == CarControls::Wheel) {
        mult = g_settings().Steering.Wheel.SteeringMult;
    }
    else {
        mult = g_settings().Steering.CustomSteering.SteeringMult;
    }

    auto oldMults = VExt::GetWheelSteeringMultipliers(g_playerVehicle);
    auto numWheels = VExt::GetNumWheels(g_playerVehicle);
    std::vector<float> newMults(numWheels);
    for (uint32_t i = 0; i < numWheels; ++i) {
        newMults[i] = sgn(oldMults[i]) * mult;
    }
    VExt::SetWheelSteeringMultipliers(g_playerVehicle, newMults);
}

void resetSteeringMultiplier() {
    if (g_playerVehicle != 0) {
        auto oldMults = VExt::GetWheelSteeringMultipliers(g_playerVehicle);
        auto numWheels = VExt::GetNumWheels(g_playerVehicle);
        std::vector<float> newMults(numWheels);
        for (uint32_t i = 0; i < numWheels; ++i) {
            newMults[i] = sgn(oldMults[i]) * 1.0f;
        }
        VExt::SetWheelSteeringMultipliers(g_playerVehicle, newMults);
    }
}

void updateLastInputDevice() {
    if (g_settings.Debug.DisableInputDetect)
        return;

    if (!PED::IS_PED_IN_ANY_VEHICLE(g_playerPed, true))
        return;

    if (g_controls.PrevInput != g_controls.GetLastInputDevice(g_controls.PrevInput,g_settings.Wheel.Options.Enable)) {
        g_controls.PrevInput = g_controls.GetLastInputDevice(g_controls.PrevInput, g_settings.Wheel.Options.Enable);
        if (g_controls.PrevInput != CarControls::Wheel) {
            g_controls.PlayFFBDynamics(0, 0);
            g_controls.PlayFFBCollision(0);
        }

        std::string message = "Input: ";
        switch (g_controls.PrevInput) {
            case CarControls::Keyboard:
                message += "Keyboard";
                break;
            case CarControls::Controller:
                message += "Controller";
                if (g_settings().MTOptions.ShiftMode == EShiftMode::HPattern &&
                    g_settings.Controller.BlockHShift) {
                    message += "~n~Mode: Sequential";
                    setShiftMode(EShiftMode::Sequential);
                }
                break;
            case CarControls::Wheel:
                message += "Steering wheel";
                break;
            default: break;
        }
        // Suppress notification when not in car
        if (Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
            UI::Notify(INFO, message);
        }
    }
    if (g_controls.PrevInput == CarControls::Wheel) {
        PAD::STOP_CONTROL_SHAKE(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(EShiftMode shiftMode) {
    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed) && shiftMode != EShiftMode::HPattern && g_vehData.mGearCurr > 1)
        g_gearStates.FakeNeutral = false;

    EShiftMode tempMode = shiftMode;
    if (shiftMode == EShiftMode::HPattern &&
        g_controls.PrevInput == CarControls::Controller && 
        g_settings.Controller.BlockHShift) {
        tempMode = EShiftMode::Automatic;
    }

    g_settings().MTOptions.ShiftMode = tempMode;

    std::string mode = "Mode: ";
    switch (g_settings().MTOptions.ShiftMode) {
        case EShiftMode::Sequential:    mode += "Sequential";   break;
        case EShiftMode::HPattern:      mode += "H-Pattern";    break;
        case EShiftMode::Automatic:     mode += "Automatic";    break;
        default:                                                break;
    }
    UI::Notify(INFO, mode);
}

float getShiftTime(Vehicle vehicle, ShiftDirection shiftDirection) {
    auto handlingPtr = VExt::GetHandlingPtr(vehicle);

    // When a shift is initiated, the clutch drops to 0.1
    // and then rises at fClutchChangeRateScale<Up/Down>Shift per second until it's 1.0 again
    // Since it doesn't start at 0, but at 0.1, it's more like:
    // A shift takes 0.9/fClutchChangeRateScale<Up/Down>Shift seconds.
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = g_gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= g_settings().ShiftOptions.ClutchRateMult;

    int modLevel = VEHICLE::GET_VEHICLE_MOD(vehicle, eVehicleMod::VehicleModTransmission);
    int modVal = VEHICLE::GET_VEHICLE_MOD_MODIFIER_VALUE(vehicle, eVehicleMod::VehicleModTransmission, modLevel);

    if (modLevel > -1) {
        shiftRate *= map(static_cast<float>(modVal), 0.0f, 100.0f, 1.0f, 2.0f);
    }

    // In milliseconds
    return 900.0f / shiftRate;
}

void shiftTo(int gear, bool autoClutch) {
    if (autoClutch) {
        if (g_gearStates.Shifting) {
            // Already shifting, so just queue up the next gear.
            g_gearStates.LockGear = g_gearStates.NextGear;
            g_gearStates.NextGear = gear;
        }
        else {
            // New shift.
            g_gearStates.Shifting = true;
            g_gearStates.NextGear = gear;
            g_gearStates.ClutchVal = 0.0f;
        }
        g_gearStates.ShiftDirection = gear > g_gearStates.LockGear ? ShiftDirection::Up : ShiftDirection::Down;

        // Timing and shift duration
        g_gearStates.ShiftStart = MISC::GET_GAME_TIMER();
        g_gearStates.ShiftTime = getShiftTime(g_playerVehicle, g_gearStates.ShiftDirection);
    }
    else {
        g_gearStates.LockGear = gear;
    }
}

void functionHShiftTo(int i) {
    bool shiftPass;

    bool checkShift = g_settings().MTOptions.ClutchShiftH && g_vehData.mHasClutch;

    // shifting from neutral into gear is OK when rev matched
    float expectedRPM = g_vehData.mDiffSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[i]);
    float rpmTol = g_settings().ShiftOptions.RPMTolerance;
    bool rpmInRange = Math::Near(g_vehData.mRPM, expectedRPM, rpmTol);

    if (!checkShift)
        shiftPass = true;
    else if (rpmInRange)
        shiftPass = true;
    else if (g_controls.IsClutchPressed())
        shiftPass = true;
    else
        shiftPass = false;

    if (shiftPass) {
        shiftTo(i, false);
        g_gearStates.FakeNeutral = false;
    }
    else {
        g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
        if (g_settings.MTOptions.EngDamage && g_vehData.mHasClutch) {
            VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                g_playerVehicle,
                VEHICLE::GET_VEHICLE_ENGINE_HEALTH(g_playerVehicle) - g_settings().MTParams.MisshiftDamage);
        }
    }
}

void functionHShiftKeyboard() {
    int clamp = VExt::GearsAvailable() - 1;
    if (g_vehData.mGearTop <= clamp) {
        clamp = g_vehData.mGearTop;
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (g_controls.ButtonJustPressed(static_cast<CarControls::KeyboardControlType>(i))) {
            functionHShiftTo(i);
        }
    }
    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::HN) && !g_vehData.mIsCVT) {
        g_gearStates.FakeNeutral = !g_gearStates.FakeNeutral;
    }
}

void functionHShiftWheel() {
    int clamp = VExt::GearsAvailable() - 1;
    if (g_vehData.mGearTop <= clamp) {
        clamp = g_vehData.mGearTop;
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (g_controls.ButtonJustPressed(static_cast<CarControls::WheelControlType>(i))) {
            functionHShiftTo(i);
        }
    }

    if (g_controls.IsHShifterJustNeutral()) {
        g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
    }

    if (g_controls.ButtonReleased(CarControls::WheelControlType::HR)) {
        shiftTo(1, false);
        g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
    }
}

bool isUIActive() {
    return PED::IS_PED_RUNNING_MOBILE_PHONE_TASK(g_playerPed) ||
        g_menu.IsThisOpen() || TrainerV::Active();
}

/*
 * Delayed shifting with clutch. Active for automatic and sequential shifts.
 * Shifting process:
 * 1. Init shift, next gear set
 * 2. Clutch disengages
 * 3. Next gear active
 * 4. Clutch engages again
 * 5. Done shifting
 */
void updateShifting() {
    // How long it takes to fully press the clutch, as ratio of 1/shiftrate.
    const float upshiftClutchPressTiming = 0.1f; 
    // How soon it fully lifts the clutch. Keep higher than the press timing.
    const float upshiftClutchLiftTiming = 0.1f;
    // On lifting clutch, how far to initially skip ahead. Larger = faster re-engage. Vanilla uses 0.9.
    // Keep larger than 0.4 since that's where throttle is reapplied again - smaller and there's no upshift cut.
    const float upshiftClutchLiftRange = 0.8f;
    // How long it takes to fully press the clutch, as ratio of 1/shiftrate.
    const float downshiftClutchPressTiming = 0.1f;
    // How soon it fully lifts the clutch. Keep higher than the press timing.
    const float downshiftClutchLiftTiming = 0.9f;
    // On lifting clutch, how far to initially skip ahead. Larger = faster re-engage. Vanilla uses 0.9.
    // Keep larger than 0.4 since that's where throttle is reapplied again - smaller and there's no downshift blip.
    const float downshiftClutchLiftRange = 1.0f;

    if (!g_gearStates.Shifting)
        return;

    // Something went wrong, abort and just shift to NextGear.
    if (g_gearStates.ClutchVal > 1.25f) {
        g_gearStates.ClutchVal = 0.0f;
        g_gearStates.Shifting = false;
        g_gearStates.LockGear = g_gearStates.NextGear;
        g_gearStates.ShiftState = ShiftState::InGear;
        return;
    }

    float shiftProgress = static_cast<float>(MISC::GET_GAME_TIMER() - g_gearStates.ShiftStart) / g_gearStates.ShiftTime;

    // ClutchLift @ shiftProgress timing
    float clutchPressTiming = g_gearStates.ShiftDirection == ShiftDirection::Up ? upshiftClutchPressTiming : downshiftClutchPressTiming;
    float clutchLiftTiming = g_gearStates.ShiftDirection == ShiftDirection::Up ? upshiftClutchLiftTiming : downshiftClutchLiftTiming;
    float clutchLiftRange = g_gearStates.ShiftDirection == ShiftDirection::Up ? upshiftClutchLiftRange : downshiftClutchLiftRange;

    if (shiftProgress <= clutchPressTiming) {
        g_gearStates.ClutchVal = map(shiftProgress, 0.0f, clutchPressTiming, 0.0f, 1.0f);
        g_gearStates.ShiftState = ShiftState::PressingClutch;
    }
    else if (g_gearStates.LockGear != g_gearStates.NextGear) {
        g_gearStates.ClutchVal = 1.0f;
        g_gearStates.LockGear = g_gearStates.NextGear;
        g_gearStates.ShiftState = ShiftState::FullClutch;
    }

    if (shiftProgress > clutchLiftTiming) {
        // g_ClutchLiftRange = 0.9: Skip first 0.1 to match game
        g_gearStates.ClutchVal = map(shiftProgress, clutchLiftTiming, 1.0f, clutchLiftRange, 0.0f);
        g_gearStates.ShiftState = ShiftState::ReleasingClutch;
    }
    if (shiftProgress >= 1.0f) {
        g_gearStates.ClutchVal = 0.0f;
        g_gearStates.Shifting = false;
        g_gearStates.ShiftState = ShiftState::InGear;
    }
}

void functionSShift() {
    auto xcTapStateUp = g_controls.ButtonTapped(CarControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = g_controls.ButtonTapped(CarControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = g_controls.ButtonTapped(CarControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = g_controls.ButtonTapped(CarControls::LegacyControlType::ShiftDown);

    if (g_settings.Controller.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XInputController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = NativeController::TapState::ButtonUp;
    }

    // Reset warning this tick
    g_gearStates.DownshiftProtection = false;

    // Shift up
    if (g_controls.PrevInput == CarControls::Controller && xcTapStateUp == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller && ncTapStateUp == NativeController::TapState::Tapped ||
        g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        if (g_vehData.mIsCVT) {
            if (g_vehData.mGearCurr < g_vehData.mGearTop) {
                shiftTo(g_gearStates.LockGear + 1, true);
            }
            g_gearStates.FakeNeutral = false;
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (g_settings().MTOptions.ClutchShiftS && 
            !g_controls.IsClutchPressed()) {
            return;
        }

        // Reverse to Neutral
        if (g_vehData.mGearCurr == 0 && !g_gearStates.FakeNeutral) {
            shiftTo(1, false);
            g_gearStates.FakeNeutral = true;
            return;
        }

        // Neutral to 1
        if (g_vehData.mGearCurr == 1 && g_gearStates.FakeNeutral) {
            g_gearStates.FakeNeutral = false;
            return;
        }

        // 1 to X
        if (g_vehData.mGearCurr < g_vehData.mGearTop) {
            shiftTo(g_gearStates.LockGear + 1, true);
            g_gearStates.FakeNeutral = false;
            return;
        }
    }

    // Shift down

    if (g_controls.PrevInput == CarControls::Controller && xcTapStateDn == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller && ncTapStateDn == NativeController::TapState::Tapped ||
        g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        if (g_vehData.mIsCVT) {
            if (g_vehData.mGearCurr > 0) {
                shiftTo(g_gearStates.LockGear - 1, false);
                g_gearStates.FakeNeutral = false;
            }
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (g_settings().MTOptions.ClutchShiftS &&
            !g_controls.IsClutchPressed()) {
            return;
        }

        // 1 to Neutral
        if (g_vehData.mGearCurr == 1 && !g_gearStates.FakeNeutral) {
            g_gearStates.FakeNeutral = true;
            return;
        }

        // Neutral to R
        if (g_vehData.mGearCurr == 1 && g_gearStates.FakeNeutral) {
            shiftTo(0, false);
            g_gearStates.FakeNeutral = false;
            return;
        }

        float expectedRPM = g_vehData.mEstimatedSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_gearStates.LockGear - 1]);
        if (g_settings().ShiftOptions.DownshiftProtect &&
            expectedRPM > 1.0f) {
            g_gearStates.DownshiftProtection = true;

            if (g_settings.HUD.DsProt.Enable)
                g_downshiftProtectSfx.Play();
            return;
        }

        // X to 1
        if (g_vehData.mGearCurr > 1) {
            shiftTo(g_gearStates.LockGear - 1, true);
            g_gearStates.FakeNeutral = false;
        }
        return;
    }
}

/*
 * Manual part of the automatic transmission (R<->N<->D)
 */
bool subAutoShiftSequential() {
    auto xcTapStateUp = g_controls.ButtonTapped(CarControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = g_controls.ButtonTapped(CarControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = g_controls.ButtonTapped(CarControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = g_controls.ButtonTapped(CarControls::LegacyControlType::ShiftDown);

    if (g_settings.Controller.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XInputController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = NativeController::TapState::ButtonUp;
    }

    // Shift up
    if (g_controls.PrevInput == CarControls::Controller && xcTapStateUp == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller && ncTapStateUp == NativeController::TapState::Tapped ||
        g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        if (g_vehData.mIsCVT) {
            if (g_vehData.mGearCurr < g_vehData.mGearTop) {
                shiftTo(g_gearStates.LockGear + 1, true);
            }
            g_gearStates.FakeNeutral = false;
            return true;
        }
        
        // Reverse to Neutral
        if (g_vehData.mGearCurr == 0 && !g_gearStates.FakeNeutral) {
            shiftTo(1, false);
            g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
            return true;
        }

        // Neutral to 1
        if (g_vehData.mGearCurr == 1 && g_gearStates.FakeNeutral) {
            g_gearStates.FakeNeutral = false;
            return true;
        }

        // Invalid + N to 1
        if (g_vehData.mGearCurr == 0 && g_gearStates.FakeNeutral) {
            shiftTo(1, false);
            g_gearStates.FakeNeutral = false;
            return true;
        }
    }

    // Shift down
    if (g_controls.PrevInput == CarControls::Controller && xcTapStateDn == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller && ncTapStateDn == NativeController::TapState::Tapped ||
        g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        if (g_vehData.mIsCVT) {
            if (g_vehData.mGearCurr > 0) {
                shiftTo(g_gearStates.LockGear - 1, false);
                g_gearStates.FakeNeutral = false;
            }
            return true;
        }

        // 1 to Neutral
        if (g_vehData.mGearCurr == 1 && !g_gearStates.FakeNeutral) {
            g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
            return true;
        }

        // Neutral to R
        if (g_vehData.mGearCurr == 1 && g_gearStates.FakeNeutral) {
            shiftTo(0, false);
            g_gearStates.FakeNeutral = false;
            return true;
        }

        // Invalid + N to R
        if (g_vehData.mGearCurr == 0 && g_gearStates.FakeNeutral) {
            shiftTo(0, false);
            g_gearStates.FakeNeutral = false;
            return true;
        }
    }
    return false;
}

/*
 * Manual part of the automatic transmission (Direct selection)
 */
bool subAutoShiftSelect() {
    if (g_controls.ButtonIn(CarControls::WheelControlType::APark)) {
        if (g_gearStates.LockGear != 1) {
            shiftTo(1, false);
        }
        g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
        PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleHandbrake, 1.0f);
        return true;
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::AReverse)) {
        if (g_vehData.mDiffSpeed < 5.0f) {
            shiftTo(0, false);
            g_gearStates.FakeNeutral = false;
        }
        return true;
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ADrive)) {
        shiftTo(1, false);
        g_gearStates.FakeNeutral = false;
        return true;
    }
    // Unassigned neutral -> pop into neutral when any gear is released
    if (g_controls.WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)].Control == -1) {
        if (g_controls.ButtonReleased(CarControls::WheelControlType::APark) ||
            g_controls.ButtonReleased(CarControls::WheelControlType::AReverse) ||
            g_controls.ButtonReleased(CarControls::WheelControlType::ADrive)) {
            if (g_gearStates.LockGear != 1) {
                shiftTo(1, false);
            }
            g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
            return true;
        }
    }
    // Assigned neutral -> handle like any other button.
    else {
        if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ANeutral)) {
            if (g_gearStates.LockGear != 1) {
                shiftTo(1, false);
            }
            g_gearStates.FakeNeutral = !g_vehData.mIsCVT;
            return true;
        }
    }
    return false;
}

void functionAShift() {
    // float vOff = 0.025f;
    // float scl = 0.01f;
    // 
    // float spd = ENTITY::GET_ENTITY_SPEED(g_playerVehicle);
    // float vDash = VExt::GetDashSpeed(g_playerVehicle);
    // 
    // UI::ShowText(0.35f, 0.00f, 0.5f, fmt::format("vec.y: {:.2f}", g_vehData.mVelocity.y));
    // UI::DrawBar(0.65f, vOff, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, g_vehData.mVelocity.y * scl);
    // 
    // UI::ShowText(0.35f, 0.05f, 0.5f, fmt::format("spd: {:.2f}", spd));
    // UI::DrawBar(0.65f, vOff + 0.05f, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, spd * scl);
    // 
    // UI::ShowText(0.35f, 0.10f, 0.5f, fmt::format("diff: {:.2f}", g_vehData.mDiffSpeed));
    // UI::DrawBar(0.65f, vOff + 0.10f, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, g_vehData.mDiffSpeed * scl);
    // 
    // UI::ShowText(0.35f, 0.15f, 0.5f, fmt::format("dash: {:.2f}", vDash));
    // UI::DrawBar(0.65f, vOff + 0.15f, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, vDash * scl);
    // 
    // UI::ShowText(0.35f, 0.20f, 0.5f, fmt::format("nonlock: {:.2f}", g_vehData.mNonLockSpeed));
    // UI::DrawBar(0.65f, vOff + 0.20f, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, g_vehData.mNonLockSpeed * scl);
    // 
    // UI::ShowText(0.35f, 0.25f, 0.5f, fmt::format("est spd: {:.2f}", g_vehData.mEstimatedSpeed));
    // UI::DrawBar(0.65f, vOff + 0.25f, 0.25f, 0.025f, Util::ColorsI::SolidWhite, Util::ColorsI::TransparentGray, g_vehData.mEstimatedSpeed * scl);

    // Manual part
    if (g_controls.PrevInput == CarControls::Wheel && g_settings.Wheel.Options.UseShifterForAuto) {
        if (subAutoShiftSelect())
            return;
    }
    else {
        if (subAutoShiftSequential())
            return;
    }

    int currGear = g_vehData.mGearCurr;
    if (currGear == 0)
        return;
    if (g_gearStates.Shifting)
        return;

    if (g_controls.ThrottleVal >= g_gearStates.ThrottleHang)
        g_gearStates.ThrottleHang = g_controls.ThrottleVal;
    else if (g_gearStates.ThrottleHang > 0.0f)
        g_gearStates.ThrottleHang -= MISC::GET_FRAME_TIME() * g_settings().AutoParams.EcoRate;

    if (g_gearStates.ThrottleHang < 0.0f)
        g_gearStates.ThrottleHang = 0.0f;

    if (g_settings().AutoParams.UsingATCU) {
        AtcuLogic::Cycle();
    }
    else {
        float currSpeed = g_vehData.mVelocity.y;
        bool skidding = isSkidding(2.4f);

        //UI::ShowText(0.5f, 0.20f, 0.5f, fmt::format("We are {}skidding", skidding ? "" : "not "));

        float nextGearMinSpeed = 0.0f; // don't care about top gear
        if (currGear < g_vehData.mGearTop) {
            nextGearMinSpeed = g_settings().AutoParams.NextGearMinRPM * g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[currGear + 1];
        }
        float currGearMinSpeed = g_settings().AutoParams.CurrGearMinRPM * g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[currGear];
        float engineLoad = g_gearStates.ThrottleHang - map(g_vehData.mRPM, 0.2f, 1.0f, 0.0f, 1.0f);
        g_gearStates.EngineLoad = engineLoad;
        g_gearStates.UpshiftLoad = g_settings().AutoParams.UpshiftLoad;


        float rateUp = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
        float upshiftDuration = 1.0f / (rateUp * g_settings().ShiftOptions.ClutchRateMult);
        bool tpPassedUp = MISC::GET_GAME_TIMER() > g_gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * g_settings().AutoParams.UpshiftTimeoutMult);
        bool tpPassedDn = MISC::GET_GAME_TIMER() > g_gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * g_settings().AutoParams.DownshiftTimeoutMult);

        // Shift up.
        if (currGear < g_vehData.mGearTop) {
            // Clutch still slipping
            float expectedRPM = g_vehData.mDiffSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[currGear]);
            if (tpPassedUp && engineLoad < g_gearStates.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding
                && g_vehData.mRPM < expectedRPM + 0.05) {
                shiftTo(g_vehData.mGearCurr + 1, true);
                g_gearStates.FakeNeutral = false;
                g_gearStates.LastUpshiftTime = MISC::GET_GAME_TIMER();
            }
        }

        // Shift down later when ratios are far apart
        float gearRatioRatio = 1.0f;

        if (g_vehData.mGearTop > 1 && currGear > 1) {
            gearRatioRatio = g_vehData.mGearRatios[currGear - 1] / g_vehData.mGearRatios[currGear];
        }

        g_gearStates.DownshiftLoad = g_settings().AutoParams.DownshiftLoad * gearRatioRatio;

        // Shift down
        if (currGear > 1) {
            if (tpPassedDn && engineLoad > g_gearStates.DownshiftLoad || currSpeed < currGearMinSpeed) {
                // TargetGear: Find the lowest gear where engineLoad(gear) < downshiftLoad(gear)
                int targetGear = currGear - 1;

                for (auto gear = 1; gear < currGear - 1; ++gear) {
                    float expectedRPM = g_vehData.mDiffSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[gear]);
                    float engineLoadForGear = g_gearStates.ThrottleHang - map(expectedRPM, 0.2f, 1.0f, 0.0f, 1.0f);

                    if (engineLoadForGear < g_gearStates.UpshiftLoad * 0.9f || expectedRPM > 0.9f)
                        continue;

                    targetGear = gear;
                    break;
                }

                shiftTo(targetGear, true);
                g_gearStates.FakeNeutral = false;
            }
        }
    }
}

bool isSkidding(float threshold) {
    float currSpeed = g_vehData.mNonLockSpeed;
    float currSpeedWorld = g_vehData.mVelocity.y;
    bool skidding = abs(currSpeed - currSpeedWorld) > threshold;
    if (!skidding) {
        auto skids = VExt::GetWheelTractionVectorLength(g_playerVehicle);
        for (uint8_t i = 0; i < g_vehData.mWheelCount; ++i) {
            if (abs(skids[i]) > threshold && VExt::IsWheelPowered(g_playerVehicle, i)) {
                skidding = true;
                break;
            }
        }
    }
    return skidding;
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
    const float idleThrottle = g_settings().MTParams.CreepIdleThrottle;
    const float idleRPM = g_settings().MTParams.CreepIdleRPM;

    float clutchRatio = map(g_controls.ClutchVal, 1.0f - g_settings().MTParams.ClutchThreshold, 0.0f, 0.0f, 1.0f);
    clutchRatio = std::clamp(clutchRatio, 0.0f, 1.0f);

    bool clutchEngaged = !g_controls.IsClutchPressed() && !g_gearStates.FakeNeutral;

    // Always do the thing for automatic cars
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic) {
        clutchRatio = 1.0f;
        clutchEngaged = !g_gearStates.FakeNeutral;
    }

    float minSpeed = idleRPM * (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr]);
    float expectedSpeed = g_vehData.mRPM * (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr]) * clutchRatio;
    float actualSpeed = g_vehData.mDiffSpeed;

    if (abs(actualSpeed) < abs(minSpeed) &&
        clutchEngaged && !g_vehData.mHandbrake) {
        float throttle = map(abs(actualSpeed), 0.0f, abs(expectedSpeed), idleThrottle, 0.0f);
        throttle = std::clamp(throttle, 0.0f, idleThrottle);

        float inputThrottle = g_controls.ThrottleVal;

        // Controller has 0.25 deadzone, take it into account (important for stalling)
        if (g_controls.PrevInput == CarControls::Controller) {
            inputThrottle = std::clamp(map(inputThrottle, 0.25f, 1.0f, 0.0f, 1.0f), 0.0f, 1.0f);
        }

        bool userThrottle = inputThrottle > idleThrottle || abs(g_controls.BrakeVal) > idleThrottle;

        bool allWheelsOnGround = true;
        for (uint8_t i = 0; i < g_vehData.mWheelCount; ++i) {
            if (g_vehData.mWheelsDriven[i]) {
                allWheelsOnGround &= g_vehData.mWheelsOnGround[i];
            }
        }
        if (!userThrottle && allWheelsOnGround) {
            if (g_vehData.mGearCurr > 0)
                Controls::SetControlADZ(ControlVehicleAccelerate, throttle, 0.25f);
            else
                Controls::SetControlADZ(ControlVehicleBrake, throttle, 0.25f);
        }
        else if (!userThrottle && !allWheelsOnGround) {
            auto wheelDims = VExt::GetWheelDimensions(g_playerVehicle);
            for (uint8_t i = 0; i < g_vehData.mWheelCount; ++i) {
                if (g_vehData.mWheelsDriven[i]) {
                    VExt::SetWheelRotationSpeed(g_playerVehicle, i, -minSpeed / wheelDims[i].TyreRadius);
                }
            }
        }
    }
}

void functionEngStall() {
    const float stallRate = MISC::GET_FRAME_TIME() * g_settings().MTParams.StallingRate;
    const float stallSlip = g_settings().MTParams.StallingSlip;

    float minSpeed = g_settings().MTParams.StallingRPM * abs(g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr]);
    float actualSpeed = g_vehData.mDiffSpeed;

    // Closer to idle speed = less buildup for stalling
    float speedDiffRatio = map(abs(minSpeed) - abs(actualSpeed), 0.0f, abs(minSpeed), 0.0f, 1.0f);
    speedDiffRatio = std::clamp(speedDiffRatio, 0.0f, 1.0f);

    bool clutchEngaged = !g_controls.IsClutchPressed() && !g_gearStates.FakeNeutral;

    float clutchRatio = map(g_controls.ClutchVal, 1.0f - g_settings().MTParams.ClutchThreshold, 0.0f, 0.0f, 1.0f);

    if (clutchEngaged &&
        g_vehData.mRPM <= 0.201f && //engine actually has to idle
        abs(actualSpeed) < abs(minSpeed) &&
        VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
        float finalClutchRatio = map(clutchRatio, stallSlip, 1.0f, 0.0f, 1.0f);
        float change = finalClutchRatio * speedDiffRatio * stallRate;
        g_gearStates.StallProgress += change;
    }
    else if (g_gearStates.StallProgress > 0.0f) {
        float change = stallRate; // "subtract" quickly
        g_gearStates.StallProgress -= change;
    }

    if (g_gearStates.StallProgress > 1.0f) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, false, true, true);
            g_peripherals.IgnitionState = IgnitionState::Stall;

            if (g_controls.PrevInput == CarControls::Wheel)
                g_controls.PlayFFBCollision(g_settings.Wheel.FFB.DetailLim / 2);
            else if (g_controls.PrevInput == CarControls::Controller)
                PAD::SET_CONTROL_SHAKE(0, 100, 255);
        }
        g_gearStates.StallProgress = 0.0f;
    }
    if (g_gearStates.StallProgress < 0.0f) {
        g_gearStates.StallProgress = 0.0f;
    }

    // Simulate push-start
    // We'll just assume the ignition thing is in the "on" position.
    if (actualSpeed > minSpeed && !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle) &&
        clutchEngaged) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, true, true, true);
    }

    //UI::ShowText(0.1, 0.00, 0.4, fmt::format("Stall progress: {:.2f}", g_gearStates.StallProgress));
    //UI::ShowText(0.1, 0.02, 0.4, fmt::format("Clutch: {}", clutchEngaged));
    //UI::ShowText(0.1, 0.06, 0.4, fmt::format("SpeedDiffRatio: {:.2f}", speedDiffRatio));
}

void functionEngDamage() {
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic ||
        g_vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC || 
        g_vehData.mGearTop == 1) {
        return;
    }

    if (g_vehData.mGearCurr != g_vehData.mGearTop &&
        g_vehData.mRPM > 0.98f &&
        g_controls.ThrottleVal > 0.98f) {
        VEHICLE::SET_VEHICLE_ENGINE_HEALTH(g_playerVehicle, 
                                           VEHICLE::GET_VEHICLE_ENGINE_HEALTH(g_playerVehicle) - g_settings().MTParams.RPMDamage);
    }
}

void functionEngLock() {
    // Checks enough suspension compression and sensible speeds
    bool use = true;
    float minSpeed = g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr];

    for (uint32_t i = 0; i < g_vehData.mWheelCount; ++i) {
        if (g_vehData.mSuspensionTravel[i] == 0.0f &&
            VExt::IsWheelPowered(g_playerVehicle, i)) {
            use = false;
            break;
        }
        if (g_vehData.mWheelTyreSpeeds[i] < minSpeed) {
            use = false;
            break;
        }
    }

    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic ||
        g_vehData.mGearCurr == g_vehData.mGearTop ||
        g_gearStates.FakeNeutral ||
        g_wheelPatchStates.InduceBurnout ||
        !use) {
        g_wheelPatchStates.EngLockActive = false;
        return;
    }
    const float reverseThreshold = 2.0f;

    float dashms = abs(g_vehData.mDiffSpeed);

    float speed = dashms;
    auto ratios = g_vehData.mGearRatios;
    float DriveMaxFlatVel = g_vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[g_vehData.mGearCurr];

    float inputMultiplier = 1.0f - g_controls.ClutchVal;
    auto wheelsSpeed      = g_vehData.mDiffSpeed;

    bool wrongDirection = false;
    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
        if (g_vehData.mGearCurr == 0) {
            if (g_vehData.mVelocity.y > reverseThreshold && wheelsSpeed > reverseThreshold) {
                wrongDirection = true;
            }
        }
        else {
            if (g_vehData.mVelocity.y < -reverseThreshold && wheelsSpeed < -reverseThreshold) {
                wrongDirection = true;
            }
        }
    }

    // Wheels are locking up due to bad (down)shifts
    if ((speed > abs(maxSpeed * 1.15f) + 3.334f || wrongDirection) && !g_controls.IsClutchPressed()) {
        g_wheelPatchStates.EngLockActive = true;
        float lockingForce = 60.0f * inputMultiplier;
        auto wheelsToLock = g_vehData.mWheelsDriven;//getDrivenWheels();

        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            if (i >= wheelsToLock.size() || wheelsToLock[i]) {
                VExt::SetWheelPower(g_playerVehicle, i, -lockingForce * sgn(g_vehData.mVelocity.y));
                VExt::SetWheelTractionVectorLength(g_playerVehicle, i, lockingForce);
            }
            else {
                float inpBrakeForce = *reinterpret_cast<float *>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce) * g_controls.BrakeVal;
                //ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
            }
        }
        if (wrongDirection) {
            VExt::SetCurrentRPM(g_playerVehicle, 0.0f);
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, false);
        }
        else {
            fakeRev(true, 1.0f);
        }

        float oldEngineHealth = VEHICLE::GET_VEHICLE_ENGINE_HEALTH(g_playerVehicle);
        float damageToApply = g_settings().MTParams.MisshiftDamage * inputMultiplier;
        if (g_settings.MTOptions.EngDamage) {
            if (oldEngineHealth >= damageToApply) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    g_playerVehicle,
                    oldEngineHealth - damageToApply);
            }
            else {
                VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, false, true, true);
            }
        }
        if (g_settings.Debug.DisplayInfo) {
            UI::ShowText(0.5, 0.80, 0.25, fmt::format("Eng block: {:.3f}", inputMultiplier));
            UI::ShowText(0.5, 0.85, 0.25, fmt::format("Eng block force: {:.3f}", lockingForce));
        }
    }
    else {
        g_wheelPatchStates.EngLockActive = false;
    }
}

void functionEngBrake() {
    const float activeBrakeThreshold = g_settings().MTParams.EngBrakeThreshold;

    // Checks enough suspension compression and sensible speeds
    bool use = true;
    float minSpeed = g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr] * activeBrakeThreshold;

    for (uint32_t i = 0; i < g_vehData.mWheelCount; ++i) {
        if (g_vehData.mSuspensionTravel[i] == 0.0f &&
            VExt::IsWheelPowered(g_playerVehicle, i)){
            use = false;
            break;
        }
        if (g_vehData.mWheelTyreSpeeds[i] < minSpeed) {
            use = false;
            break;
        }
    }

    float throttleMultiplier = 1.0f - g_controls.ThrottleVal;
    float clutchMultiplier = 1.0f - g_controls.ClutchVal;

    // Always treat clutch pedal as unpressed for auto
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic)
        clutchMultiplier = 1.0f;

    float inputMultiplier = throttleMultiplier * clutchMultiplier;

    if (g_wheelPatchStates.EngLockActive || 
        g_wheelPatchStates.InduceBurnout || 
        g_gearStates.FakeNeutral ||
        !use ||
        g_vehData.mRPM < activeBrakeThreshold || 
        inputMultiplier < 0.05f) {
        g_wheelPatchStates.EngBrakeActive = false;
        return;
    }

    g_wheelPatchStates.EngBrakeActive = true;
    float rpmMultiplier = (g_vehData.mRPM - activeBrakeThreshold) / (1.0f - activeBrakeThreshold);
    float engBrakeForce = g_settings().MTParams.EngBrakePower * inputMultiplier * rpmMultiplier;
    auto wheelsToBrake = g_vehData.mWheelsDriven;
    for (int i = 0; i < g_vehData.mWheelCount; i++) {
        if (wheelsToBrake[i]) {
            VExt::SetWheelPower(g_playerVehicle, i, -engBrakeForce);
        }
    }
    if (g_settings.Debug.DisplayInfo) {
        UI::ShowText(0.85, 0.500, 0.4, fmt::format("EngBrake:\t\t{:.3f}", inputMultiplier), 4);
        UI::ShowText(0.85, 0.525, 0.4, fmt::format("Pressure:\t\t{:.3f}", engBrakeForce), 4);
        UI::ShowText(0.45, 0.75, 1.0, "~r~EngBrake");
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

// TODO: Move somewhere else, some day...
std::vector<float> GetHandBrakeVals(float handbrakeValInput){
    std::vector<float> brakeVals(g_vehData.mWheelCount);
    const auto offsets = VExt::GetWheelOffsets(g_playerVehicle);

    const float handlingHandbrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fHandBrakeForce);

    float inpBrakeForce = handlingHandbrakeForce * handbrakeValInput;

    for (uint8_t i = 0; i < g_vehData.mWheelCount; i++) {
        if (offsets[i].y > 0.0f) {
            // front
            brakeVals[i] = 0.0f;
        }
        else {
            // rear
            brakeVals[i] = inpBrakeForce;
        }
    }
    return brakeVals;
}

// TODO: I should really rework how brake values are mixed with the assists...
// This just gets the correct value for all wheels during normal braking
// TODO: Also probably refactor so NPC vehicles use this
std::vector<float> GetInputBrakes() {
    std::vector<float> brakeVals(g_vehData.mWheelCount);
    const auto offsets = VExt::GetWheelOffsets(g_playerVehicle);

    const float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
    const float bbalF = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasFront);
    const float bbalR = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeBiasRear);

    float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;

    for (int i = 0; i < g_vehData.mWheelCount; i++) {
        float bbal = offsets[i].y > 0.0f ? bbalF : bbalR;
        brakeVals[i] = inpBrakeForce * bbal;
    }

    return brakeVals;
}

void handleBrakePatch() {
    auto absData = DrivingAssists::GetABS();
    auto tcsData = DrivingAssists::GetTCS();
    auto espData = DrivingAssists::GetESP();
    auto lsdData = DrivingAssists::GetLSD();

    // tcs & lc
    bool tcsThrottle = tcsData.Use && g_settings().DriveAssists.TCS.Mode == 1;
    bool lcThrottle = g_settings().DriveAssists.LaunchControl.Enable &&
        LaunchControl::GetState() == LaunchControl::ELCState::Controlling;

    float finalThrottle = 0.0f;

    float tractionThrottle = 0.0f;
    if (tcsThrottle ||
        lcThrottle) {
        float tcsSlipMin = g_settings().DriveAssists.TCS.SlipMin;
        float tcsSlipMax = g_settings().DriveAssists.TCS.SlipMax;

        float lcSlipMin = g_settings().DriveAssists.LaunchControl.SlipMin;
        float lcSlipMax = g_settings().DriveAssists.LaunchControl.SlipMax;

        float slipMin;
        float slipMax;

        // Launch control overrides traction control
        if (lcThrottle) {
            slipMin = lcSlipMin;
            slipMax = lcSlipMax;
            tcsData.Use = false;
        }
        else {
            slipMin = tcsSlipMin;
            slipMax = tcsSlipMax;
        }
        
        tractionThrottle = map(tcsData.AverageSlipRatio,
            slipMin, slipMax,
            g_controls.ThrottleVal, 0.0f);
        tractionThrottle = std::clamp(tractionThrottle, 0.0f, g_controls.ThrottleVal);

        finalThrottle = tractionThrottle;
    }

    if (tcsData.Use) {
        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            g_vehData.mWheelsTcs[i] = tcsData.LinearSlipRatio[i] > g_settings().DriveAssists.TCS.SlipMin &&
                g_vehData.mWheelTyreSpeeds[i] > 0.0f;
        }
    }
    else {
        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            g_vehData.mWheelsTcs[i] = false;
        }
    }

    // Cruise control
    bool ccThrottle = false;
    float throttle = g_controls.ThrottleVal;
    float brake = g_controls.BrakeVal;
    float clutch = g_controls.ClutchVal;

    if (CruiseControl::GetActive() && (throttle > 0.2f || brake > 0.01f || clutch > 0.25f || g_gearStates.FakeNeutral)) {
        CruiseControl::SetActive(false);
    }

    CruiseControl::Update(throttle, brake, clutch);
    if (CruiseControl::GetActive() && !tcsThrottle && !lcThrottle) {
        Controls::SetControlADZ(eControl::ControlVehicleAccelerate, throttle, 0.25f);
        Controls::SetControlADZ(eControl::ControlVehicleBrake, brake, 0.25f);
        // g_controls is read later by auto trans, so safe to update here.
        g_controls.ThrottleVal = throttle;
        g_controls.BrakeVal = brake;
        
        finalThrottle = throttle;
        ccThrottle = true;
    }

    bool speedLimThrottle = false;
    if (g_settings().MTOptions.SpeedLimiter.Enable) {
        speedLimThrottle = SpeedLimiter::Update(throttle);
        if (speedLimThrottle) {
            g_controls.ThrottleVal = throttle;
            finalThrottle = throttle;
        }
    }

    // Shift throttle blip/cut
    bool blipThrottle = false;
    bool cutThrottle = false;
    // Shifting is only true in Automatic and Sequential mode
    if (g_gearStates.Shifting) {
        float clutchInput = g_controls.ClutchVal;

        // Always treat clutch pedal as unpressed for auto
        if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic) {
            clutchInput = 0.0f;
        }

        // Only lift and blip when no clutch used
        if (clutchInput == 0.0f) {
            // Hmm, it doesn't really wanna let me rev the shit out of it.
            if (g_gearStates.ShiftDirection == ShiftDirection::Up &&
                g_settings().ShiftOptions.UpshiftCut) {
                float cutThrottleVal = 0.0f;

                if (g_gearStates.ShiftState == ShiftState::PressingClutch) {
                    cutThrottleVal = map(g_gearStates.ClutchVal, 0.0f, 0.4f, g_controls.ThrottleVal, 0.0f);
                    cutThrottleVal = std::clamp(cutThrottleVal, 0.0f, g_controls.ThrottleVal);
                    fakeRev(true, cutThrottleVal);
                }

                if (g_gearStates.ShiftState == ShiftState::ReleasingClutch) {
                    if (g_gearStates.ClutchVal < 0.4f) {
                        cutThrottleVal = g_controls.ThrottleVal;
                    }
                }

                cutThrottleVal = std::clamp(cutThrottleVal, 0.0f, 1.0f);

                cutThrottle = true;
                finalThrottle = cutThrottleVal;
            }
            // Anti-cut:
            // Somehow it doesn't free-rev on auto-clutch in HandleRPM
            else if (g_gearStates.ShiftDirection == ShiftDirection::Up &&
                !g_settings().ShiftOptions.UpshiftCut) {
                fakeRev(true, g_controls.ThrottleVal);
            }

            float expectedRPM = g_vehData.mDiffSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_gearStates.NextGear]);
            if (g_gearStates.ShiftDirection == ShiftDirection::Down &&
                g_settings().ShiftOptions.DownshiftBlip) {
                bool clutchOK = g_gearStates.ShiftState == ShiftState::FullClutch || g_gearStates.ShiftState == ShiftState::ReleasingClutch;
                if (g_vehData.mRPM < expectedRPM * 1.15f && clutchOK) {
                    float blipThrottleVal;
                    if (g_gearStates.ShiftState == ShiftState::FullClutch) {
                        blipThrottleVal = 1.0f;
                    }
                    else {
                        blipThrottleVal = map(g_gearStates.ClutchVal, 0.9f, 1.0f, 0.0f, 1.0f);
                        blipThrottleVal = std::clamp(blipThrottleVal, 0.0f, 1.0f);
                    }

                    blipThrottle = true;
                    finalThrottle = blipThrottleVal;
                    fakeRev(true, blipThrottleVal);
                }
            }
            // Anti-cut:
            // Somehow it doesn't free-rev on auto-clutch in HandleRPM
            else if (g_gearStates.ShiftDirection == ShiftDirection::Down &&
                !g_settings().ShiftOptions.DownshiftBlip) {
                fakeRev(true, g_controls.ThrottleVal);
            }
        }
    }

    bool patchThrottleControl =
        tcsThrottle ||
        lcThrottle ||
        ccThrottle ||
        speedLimThrottle ||
        blipThrottle ||
        cutThrottle;

    bool patchThrottle =
        g_wheelPatchStates.EngLockActive ||
        g_wheelPatchStates.EngBrakeActive;

    bool patchBrake =
        g_controls.UseAnalogHandbrake && g_controls.HandbrakeVal > 0.01f ||
        absData.Use ||
        espData.Use ||
        tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0;

    bool patchAbs = g_vehData.mHasABS &&
        g_controls.UseAnalogHandbrake && g_controls.HandbrakeVal > 0.01f;

    // LSD actively conflicts with brakes (applies negative brake)
    // So override LSD with the assist.
    if (patchBrake) {
        lsdData.Use = false;
        lsdData.BrakeLR = 0.0f;
        lsdData.BrakeRR = 0.0f;
        lsdData.BrakeLF = 0.0f;
        lsdData.BrakeRF = 0.0f;
    }
    else if (lsdData.Use) {
        patchBrake = true;
    }

    if (g_wheelPatchStates.InduceBurnout) {
        patchThrottle = true;
        patchBrake = true;
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        if (!MemoryPatcher::ThrottlePatcher.Patched()) {
            MemoryPatcher::PatchThrottle();
        }
        if (g_settings.Debug.DisplayInfo)
            UI::ShowText(0.45, 0.75, 1.0, "~r~Burnout");
    }
    else {
        if (patchThrottle) {
            if (!MemoryPatcher::ThrottlePatcher.Patched()) {
                MemoryPatcher::PatchThrottle();
            }
        }

        if (patchBrake) {
            if (!MemoryPatcher::BrakePatcher.Patched()) {
                MemoryPatcher::PatchBrake();
            }
        }

        if (patchAbs) {
            if (!MemoryPatcher::AbsPatcher.Patched()) {
                MemoryPatcher::AbsPatcher.Patch();
            }
        }

        // Only use LSD if no other assists are working, as LSD would just reduce brake force.
        if (lsdData.Use && !espData.Use && !tcsData.Use && !absData.Use) {
            auto brakeVals = DrivingAssists::GetLSDBrakes(lsdData);
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVals[i]);
            }
        }

        if (g_controls.UseAnalogHandbrake && g_controls.HandbrakeVal > 0.01f || 
            espData.Use ||
            tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0 ||
            absData.Use) {
            auto hbVals = GetHandBrakeVals(g_controls.HandbrakeVal);
            auto espBrakeVals = DrivingAssists::GetESPBrakes(espData);
            auto tcsBrakeVals = DrivingAssists::GetTCSBrakes(tcsData);
            auto absBrakeVals = DrivingAssists::GetABSBrakes(absData);

            auto normalBrakeVals = GetInputBrakes();

            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                float brakeVal = 0.0f;
                if (espData.Use) {
                    brakeVal = espBrakeVals[i];
                }
                if (tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0) {
                    brakeVal = std::max(brakeVal, tcsBrakeVals[i]);
                }
                if (absData.Use && g_vehData.mWheelsAbs[i]) {
                    brakeVal = std::min(brakeVal, absBrakeVals[i]);
                }
                if (hbVals[i] > 0.0f) {
                    brakeVal = brakeVal + hbVals[i];
                    VExt::SetIsABSActive(g_playerVehicle, i, false);
                }
                // aka only use analog handbrake
                if (!espData.Use &&
                    !(tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0) &&
                    !(absData.Use && g_vehData.mWheelsAbs[i])) {
                    brakeVal += normalBrakeVals[i];
                }

                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVal);
            }
        }
    }
    if (!patchBrake) {
        if (MemoryPatcher::BrakePatcher.Patched()) {
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                if (g_controls.BrakeVal == 0.0f) {
                    VExt::SetWheelBrakePressure(g_playerVehicle, i, 0.0f);
                }
            }
            MemoryPatcher::RestoreBrake();
        }
        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            g_vehData.mWheelsAbs[i] = false;
            g_vehData.mWheelsEspO[i] = false;
            g_vehData.mWheelsEspU[i] = false;
        }
    }
    if (!patchThrottle) {
        if (MemoryPatcher::ThrottlePatcher.Patched()) {
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                if (g_controls.ThrottleVal == 0.0f) {
                    VExt::SetWheelPower(g_playerVehicle, i, 0.0f);
                }
            }
            MemoryPatcher::RestoreThrottle();
        }
    }
    if (!patchAbs) {
        if (MemoryPatcher::AbsPatcher.Patched()) {
            MemoryPatcher::AbsPatcher.Restore();
        }
    }

    if (patchThrottleControl) {
        if (!MemoryPatcher::ThrottleControlPatcher.Patched()) {
            MemoryPatcher::ThrottleControlPatcher.Patch();
        }
        VExt::SetThrottle(g_playerVehicle, finalThrottle);
        VExt::SetThrottleP(g_playerVehicle, finalThrottle);
    }
    else {
        if (MemoryPatcher::ThrottleControlPatcher.Patched()) {
            MemoryPatcher::ThrottleControlPatcher.Restore();
        }
    }

    if (g_settings.Debug.DisplayInfo) {
        UI::ShowText(0.60f, 0.100f, 0.25f, fmt::format("{}TCS~s~ / {}LC",
            tcsThrottle ? "~g~" : "", lcThrottle ? "~g~" : ""));
        std::string controlledThrottle = tcsThrottle || lcThrottle ? fmt::format("{:.2f}", tractionThrottle) : "N/A";
        UI::ShowText(0.60f, 0.125f, 0.25f, fmt::format("Average slip: {:.2f}", tcsData.AverageSlipRatio));
        UI::ShowText(0.60f, 0.150f, 0.25f, fmt::format("Throttle: {}", controlledThrottle));

        if (cutThrottle) {
            UI::ShowText(0.5f, 0.5f, 0.5f, fmt::format("Cut @ {:.2f}", finalThrottle));
        }
        if (blipThrottle) {
            UI::ShowText(0.5f, 0.5f, 0.5f, fmt::format("Blip @ {:.2f}", finalThrottle));
        }
    
    }
}

void fakeRev(bool customThrottle, float customThrottleVal) {
    const float driveInertia = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fDriveInertia);
    float throttleVal = customThrottle ? customThrottleVal : g_controls.ThrottleVal;
    float timeStep = MISC::GET_FRAME_TIME();
    float accelRatio = 2.0f * driveInertia * timeStep;
    float rpmValTemp = g_vehData.mRPMPrev > g_vehData.mRPM ? g_vehData.mRPMPrev - g_vehData.mRPM : 0.0f;
    if (g_vehData.mGearCurr == 1) {			// For some reason, first gear revs slower
        rpmValTemp *= 2.0f;
    }
    float rpmVal = g_vehData.mRPM +			// Base value
        rpmValTemp +						// Keep it constant
        throttleVal * accelRatio;	// Addition value, depends on delta T
    //UI::ShowText(0.4, 0.4, 1.0, fmt::format("FakeRev {} {:.2f}", customThrottle, rpmVal));
    VExt::SetCurrentRPM(g_playerVehicle, std::clamp(rpmVal, 0.0f, 1.0f));
}

void handleRPM() {
    float clutchInput = g_controls.ClutchVal;
    float clutch = g_controls.ClutchVal;

    // Always treat clutch pedal as unpressed for auto
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic) {
        clutch = 0.0f;
        clutchInput = 0.0f;
    }

    // Shifting is only true in Automatic and Sequential mode
    if (g_gearStates.Shifting) {
        if (g_gearStates.ClutchVal > clutch)
            clutch = g_gearStates.ClutchVal;
    }

    // Ignores clutch 
    if (!g_gearStates.Shifting) {
        if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic ||
            g_vehData.mClass == VehicleClass::Bike && g_settings.GameAssists.SimpleBike) {
            clutch = 0.0f;
        }
    }

    // Game wants to shift up. Triggered at high RPM, high speed.
    // Desired result: high RPM, same gear, no more accelerating
    // Result:	Is as desired. Speed may drop a bit because of game clutch.
    // Update 2017-08-12: We know the gear speeds now, consider patching
    // shiftUp completely?
    if (g_vehData.mGearCurr > 0 &&
        (g_gearStates.HitRPMSpeedLimiter && abs(g_vehData.mVelocity.y) > 2.0f)) {
        PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
        VExt::SetThrottle(g_playerVehicle, 0.0f);
        VExt::SetThrottleP(g_playerVehicle, 0.0f);
        fakeRev(false, 1.0f);
        //UI::ShowText(0.4, 0.1, 1.0, "REV LIM SPD");
    }
    if (g_gearStates.HitRPMLimiter) {
        VExt::SetCurrentRPM(g_playerVehicle, 1.0f);
        //UI::ShowText(0.4, 0.1, 1.0, "REV LIM RPM");
    }
    
    /*
        Game doesn't rev on disengaged clutch in any gear but 1
        This workaround tries to emulate this
        Default: vehData.mClutch >= 0.6: Normal
        Default: vehData.mClutch < 0.6: Nothing happens
        Fix: Map 0.0-1.0 to 0.6-1.0 (clutchdata)
        Fix: Map 0.0-1.0 to 1.0-0.6 (control)
    */
    float finalClutch = 1.0f - clutch;
    
    if (g_vehData.mGearCurr > 1) {
        if (g_gearStates.Shifting) {
            finalClutch = map(clutch, 0.0f, 1.0f, 1.0f, 0.0f);
        }
        else {
            finalClutch = map(clutch, 0.0f, 1.0f, 1.0f, 0.6f);
        }
   
        // Don't care about clutch slippage, just handle RPM now
        if (g_gearStates.FakeNeutral) {
            fakeRev(false, 0);
            VExt::SetThrottle(g_playerVehicle, 1.0f);
            VExt::SetThrottleP(g_playerVehicle, g_controls.ThrottleVal);
        }
        // When pressing clutch and throttle, handle clutch and RPM
        else if (clutch > 0.4f &&
            g_controls.ThrottleVal > 0.0f &&
            (!g_gearStates.Shifting || clutchInput > 0.4f)) {
            fakeRev(false, 0);
            VExt::SetThrottle(g_playerVehicle, 1.0f);
            VExt::SetThrottleP(g_playerVehicle, g_controls.ThrottleVal);
        }
    }

    if (g_gearStates.FakeNeutral || clutch >= 1.0f) {
        if (g_vehData.mDiffSpeed < 1.0f) {
            finalClutch = -5.0f;
        }
        else {
            finalClutch = -0.5f;
        }
    }

    // >= 1.0 RPM custom rev limit: oscillates RPM, and off-throttle triggers exhaust pops
    if (g_gearStates.FakeNeutral || clutch >= 1.0f || VExt::GetHandbrake(g_playerVehicle)) {
        if (VExt::GetCurrentRPM(g_playerVehicle) >= 1.0f && g_gearStates.LastRedline == 0) {
            g_gearStates.LastRedline = MISC::GET_GAME_TIMER();
        }

        if (g_gearStates.LastRedline > 0 && MISC::GET_GAME_TIMER() >= g_gearStates.LastRedline + 25 &&
            VExt::GetCurrentRPM(g_playerVehicle) >= 1.0f && VExt::GetThrottleP(g_playerVehicle) > 0.0f) {
            VExt::SetCurrentRPM(g_playerVehicle, 0.975f);
            VExt::SetThrottle(g_playerVehicle, 0.0f);
            VExt::SetThrottleP(g_playerVehicle, 0.0f);
            g_gearStates.LastRedline = 0;
        }
    }

    // Sets finalClutch to 0 when limiting RPM
    LaunchControl::Update(finalClutch);

    VExt::SetClutch(g_playerVehicle, finalClutch);
}

/*
 * Custom limiter thing
 */
void functionLimiter() {
    if (g_settings.Debug.DisableRPMLimit)
        return;

    if (g_vehData.mGearCurr == g_vehData.mGearTop &&
        !g_settings.MTOptions.FinalGearRPMLimit)
        return;

    if (!g_vehData.mHasClutch)
        return;

    g_gearStates.HitRPMLimiter = g_vehData.mRPM > 1.0f;

    float maxSpeed = g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_vehData.mGearCurr];

    if (g_vehData.mEstimatedSpeed > maxSpeed && g_vehData.mRPM >= 1.0f) {
        g_gearStates.HitRPMSpeedLimiter = true;
    }
    else {
        g_gearStates.HitRPMSpeedLimiter = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Control override handling
///////////////////////////////////////////////////////////////////////////////

void functionRealReverse() {
    float roadSpeed = g_vehData.mVelocity.y;

    // Forward gear
    // Desired: Only brake
    if (g_vehData.mGearCurr > 0) {
        // LT behavior when stopped: Just brake
        if (g_controls.BrakeVal > 0.01f && g_controls.ThrottleVal < g_controls.BrakeVal &&
            roadSpeed < 0.5f && roadSpeed >= -0.5f) { // < 0.5 so reverse never triggers
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stop");
            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            VExt::SetThrottleP(g_playerVehicle, 0.1f);
            VExt::SetBrakeP(g_playerVehicle, 1.0f);
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
        }
        // LT behavior when rolling back: Brake
        if (g_controls.BrakeVal > 0.01f && g_controls.ThrottleVal < g_controls.BrakeVal &&
            roadSpeed < -0.5f) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollback");
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleAccelerate, g_controls.BrakeVal);
            VExt::SetThrottle(g_playerVehicle, 0.0f);
            VExt::SetThrottleP(g_playerVehicle, 0.1f);
            VExt::SetBrakeP(g_playerVehicle, 1.0f);
        }
        // RT behavior when rolling back: Burnout
        if (!g_gearStates.FakeNeutral && g_controls.ThrottleVal > 0.5f && !g_controls.IsClutchPressed() &&
            roadSpeed < -1.0f ) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
            //PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleBrake, carControls.ThrottleVal);
            if (g_controls.BrakeVal < 0.1f) {
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, false);
            }
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                if (VExt::IsWheelPowered(g_playerVehicle, i)) {
                    VExt::SetWheelBrakePressure(g_playerVehicle, i, 0.0f);
                    VExt::SetWheelPower(g_playerVehicle, i, 2.0f * VExt::GetDriveForce(g_playerVehicle));
                }
                else {
                    float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
                    float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;
                    VExt::SetWheelPower(g_playerVehicle, i, 0.0f);
                    VExt::SetWheelBrakePressure(g_playerVehicle, i, inpBrakeForce);
                }
            }
            fakeRev();
            VExt::SetThrottle(g_playerVehicle, g_controls.ThrottleVal);
            VExt::SetThrottleP(g_playerVehicle, g_controls.ThrottleVal);
            g_wheelPatchStates.InduceBurnout = true;
        }
        else {
            g_wheelPatchStates.InduceBurnout = false;
        }
    }
    // Reverse gear
    // Desired: RT reverses, LT brakes
    if (g_vehData.mGearCurr == 0) {
        // Enables reverse lights
        VExt::SetThrottleP(g_playerVehicle, -0.1f);
        // RT behavior
        int throttleAndSomeBrake = 0;
        if (g_controls.ThrottleVal > 0.01f && g_controls.ThrottleVal > g_controls.BrakeVal) {
            throttleAndSomeBrake++;
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Active Reverse");

            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleBrake, g_controls.ThrottleVal);
        }
        // LT behavior when reversing
        if (g_controls.BrakeVal > 0.01f &&
            roadSpeed <= -0.5f) {
            throttleAndSomeBrake++;
            //UI::ShowText(0.3, 0.35, 0.5, "functionRealReverse: Brake @ Reverse");

            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleAccelerate, g_controls.BrakeVal);
        }
        // Throttle > brake  && BrakeVal > 0.1f
        if (throttleAndSomeBrake >= 2) {
            //UI::ShowText(0.3, 0.4, 0.5, "functionRealReverse: Weird combo + rev it");

            PAD::ENABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleAccelerate, g_controls.BrakeVal);
            fakeRev(false, 0);
        }

        // LT behavior when forward
        if (g_controls.BrakeVal > 0.01f && g_controls.ThrottleVal <= g_controls.BrakeVal &&
            roadSpeed > 0.1f) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleBrake, g_controls.BrakeVal);

            //PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            VExt::SetBrakeP(g_playerVehicle, 1.0f);
        }

        // LT behavior when still
        if (g_controls.BrakeVal > 0.01f && g_controls.ThrottleVal <= g_controls.BrakeVal &&
            roadSpeed > -0.5f && roadSpeed <= 0.1f) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stopped");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            VExt::SetBrakeP(g_playerVehicle, 1.0f);
        }
    }
}

void functionAutoReverse() {
    float roadSpeed = g_vehData.mVelocity.y;

    // Go forward
    if (PAD::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && 
        !PAD::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
        roadSpeed > -0.5f &&
        g_vehData.mGearCurr == 0) {
        shiftTo(1, false);
    }

    // Reverse
    if (PAD::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && 
        !PAD::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
        roadSpeed < 0.5f &&
        g_vehData.mGearCurr > 0) {
        g_gearStates.FakeNeutral = false;
        shiftTo(0, false);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////
// Blocks some vehicle controls on tapping, tries to activate them when holding.
// TODO: Some original "tap" controls don't work.

template <typename CtrlType>
void blockButtons(const std::array<int, static_cast<int>(CtrlType::SIZEOF)>& blockList) {
    for (int i = 0; i < static_cast<int>(CtrlType::SIZEOF); i++) {
        if (blockList[i] == -1) continue;
        if (i != static_cast<int>(CtrlType::ShiftUp) &&
            i != static_cast<int>(CtrlType::ShiftDown)) continue;

        bool releasedThisFrame = g_controls.ButtonReleasedAfter(static_cast<CtrlType>(i), g_settings.Controller.HoldTimeMs);

        if (g_controls.ButtonHeldOver(static_cast<CtrlType>(i), g_settings.Controller.HoldTimeMs)) {
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, blockList[i], 1.0f);
        }
        else {
            PAD::DISABLE_CONTROL_ACTION(0, blockList[i], true);
        }
    }
    if (blockList[static_cast<int>(CtrlType::Clutch)] != -1) {
        PAD::DISABLE_CONTROL_ACTION(0, blockList[static_cast<int>(CtrlType::Clutch)], true);
    }
}

void blockButtons() {
    if (!g_settings.MTOptions.Enable || !g_settings.Controller.BlockCarControls || g_vehData.mDomain != VehicleDomain::Road ||
        g_controls.PrevInput != CarControls::Controller || !Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        return;
    }
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic && g_vehData.mGearCurr > 1) {
        return;
    }

    if (g_settings.Controller.Native.Enable) {
        blockButtons<CarControls::LegacyControlType>(g_controls.ControlNativeBlocks);
    }
    else {
        blockButtons<CarControls::ControllerControlType>(g_controls.ControlXboxBlocks);
    }
}

void startStopEngine() {
    bool prevController = g_controls.PrevInput == CarControls::Controller;

    bool heldControllerXinput = g_controls.ButtonReleasedAfter(CarControls::ControllerControlType::Engine, g_settings.Controller.HoldTimeMs);
    bool heldControllerNative = g_controls.ButtonReleasedAfter(CarControls::LegacyControlType::Engine, g_settings.Controller.HoldTimeMs);

    bool pressedKeyboard = g_controls.ButtonJustPressed(CarControls::KeyboardControlType::Engine);
    bool pressedWheel = g_controls.ButtonJustPressed(CarControls::WheelControlType::Engine);

    bool controllerActive = prevController && heldControllerXinput || prevController && heldControllerNative;
    bool keyboardActive = pressedKeyboard;
    bool wheelActive = pressedWheel;

    bool throttleStart = g_settings.GameAssists.ThrottleStart &&
        g_controls.ThrottleVal > 0.90f &&
        g_controls.ClutchVal > 1.0f - g_settings().MTParams.ClutchThreshold;

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle) &&
        (controllerActive || keyboardActive || wheelActive || throttleStart)) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, true, false, true);
    }

    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle) &&
        (controllerActive && g_settings.Controller.ToggleEngine || keyboardActive || wheelActive)) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(g_playerVehicle, false, true, true);
        StartingAnimation::PlayManual();
    }
}



///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback() {
    if (g_vehData.mGearTop > 0 && g_vehData.mGearCurr == 0) {
        PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, ControlVehicleLookBehind, 1.0f);
    }
}

void functionAutoGear1() {
    if (g_vehData.mThrottle < 0.1f && ENTITY::GET_ENTITY_SPEED(g_playerVehicle) < 0.1f && g_vehData.mGearCurr > 1) {
        shiftTo(1, false);
    }
}

void functionHillGravity() {
    // TODO: Needs improvement/proper fix
    if (g_controls.HandbrakeVal < 0.1f && 
        g_controls.BrakeVal < 0.1f &&
        ENTITY::GET_ENTITY_SPEED(g_playerVehicle) < 2.0f &&
        VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(g_playerVehicle)) {
        float pitch = ENTITY::GET_ENTITY_PITCH(g_playerVehicle);;

        float clutchNeutral;
        if (g_gearStates.FakeNeutral)
            clutchNeutral = 1.0f;
        else if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic)
            clutchNeutral = 0.0f;
        else
            clutchNeutral = g_controls.ClutchVal;

        if (pitch < 0 || clutchNeutral) {
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
                g_playerVehicle, 1, { 0.0f, -1 * (pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f }, true, true, true, true);
        }
        if (pitch > 10.0f || clutchNeutral)
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
                g_playerVehicle, 1, { 0.0f, -1 * (pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f }, true, true, true, true);
    }
}

int prevCameraMode;

void functionHidePlayerInFPV(bool optionToggled) {
    bool shouldRun = false;

    int cameraMode = CAM::GET_FOLLOW_PED_CAM_VIEW_MODE();
    if (prevCameraMode != cameraMode) {
        shouldRun = true;
    }
    prevCameraMode = cameraMode;

    if (optionToggled) {
        shouldRun = true;
    }

    if (g_settings.Debug.DisablePlayerHide) {
        shouldRun = false;
    }

    if (!shouldRun)
        return;

    bool visible = ENTITY::IS_ENTITY_VISIBLE(g_playerPed);
    bool shouldHide = false;

    if (g_settings.Misc.HidePlayerInFPV && CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() == 4 && Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        shouldHide = true;
    }

    if (visible && shouldHide) {
        ENTITY::SET_ENTITY_VISIBLE(g_playerPed, false, false);
    }
    if (!visible && !shouldHide) {
        ENTITY::SET_ENTITY_VISIBLE(g_playerPed, true, false);
    }
}

void StartUDPTelemetry() {
    if (g_settings.Misc.UDPTelemetry)
        if (!g_socket.Started())
            g_socket.Start(g_settings.Misc.UDPAddress, g_settings.Misc.UDPPort);
}

void update_UDPTelemetry() {
    if (Util::VehicleAvailable(g_playerVehicle, g_playerPed) && g_settings.Misc.UDPTelemetry) {
        UDPTelemetry::UpdatePacket(g_socket, g_playerVehicle, g_vehData, g_controls);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void loadConfigs() {
    logger.Write(DEBUG, "Clearing and reloading vehicle configs...");
    g_vehConfigs.clear();
    const std::string absoluteModPath = Paths::GetModPath();
    const std::string vehConfigsPath = absoluteModPath + "\\Vehicles";

    if (!(fs::exists(fs::path(vehConfigsPath)) && fs::is_directory(fs::path(vehConfigsPath)))) {
        logger.Write(WARN, "Directory [%s] not found!", vehConfigsPath.c_str());
        setVehicleConfig(g_playerVehicle);
        return;
    }

    for (auto& file : fs::directory_iterator(vehConfigsPath)) {
        if (StrUtil::toLower(fs::path(file).extension().string()) != ".ini")
            continue;

        if (StrUtil::toLower(fs::path(file).stem().string()) == "basevehicleconfig")
            continue;

        VehicleConfig config;
        config.SetFiles(g_settings.BaseConfig(), file.path().string());
        config.LoadSettings();
        if (config.ModelNames.empty() && config.Plates.empty()) {
            logger.Write(WARN,
                "Vehicle settings file [%s] contained no model names or plates, skipping...",
                file.path().string().c_str());
            continue;
        }
        g_vehConfigs.push_back(config);
        logger.Write(DEBUG, "Loaded vehicle config [%s]", config.Name.c_str());
    }
    logger.Write(INFO, "Configs loaded: %d", g_vehConfigs.size());
    setVehicleConfig(g_playerVehicle);
}

// Always call *after* settings have been (re)loaded
void initTimers() {
    g_speedTimers.clear();
    if (g_settings.Debug.Metrics.EnableTimers) {
        for (const auto& params : g_settings.Debug.Metrics.Timers) {
            g_speedTimers.emplace_back(params.Unit,
                [](const std::string& msg) { UI::Notify(INFO, msg, false); },
                params.LimA, params.LimB, params.Tolerance);
        }
    }
}

void loadLut(const std::string& lutPath) {
    const std::string absoluteModPath = Paths::GetModPath();
    auto fullLutPath = fmt::format("{}/{}", absoluteModPath, lutPath);
    std::ifstream lut(fullLutPath);

    if (!lut.is_open()) {
        logger.Write(ERROR, "[Wheel] Failed to open LUT file '%s'", fullLutPath.c_str());
        return;
    }

    std::map<float, float> lutMap;
    std::string line;
    while (lut >> line) {
        float in, out;
        auto scanned = sscanf_s(line.c_str(), "%f|%f", &in, &out);
        if (scanned != 2) {
            logger.Write(ERROR, "[Wheel] Failed to read line in LUT file '%s'", line.c_str());
            continue;
        }
        lutMap.emplace(in, out);
    }

    g_controls.GetWheel().AssignLut(lutMap);
}

#include "ShakeData.hpp"

std::shared_ptr<CShakeData> mShakeData;

void readSettings() {
    g_settings.Read(&g_controls);
    if (g_settings.Debug.LogLevel > 4)
        g_settings.Debug.LogLevel = 1;

    auto logLevel = static_cast<LogLevel>(g_settings.Debug.LogLevel);
    if (!std::string(GIT_DIFF).empty())
        logLevel = LogLevel::DEBUG;

    logger.SetMinLevel(logLevel);

    g_gearStates.FakeNeutral = g_settings.GameAssists.DefaultNeutral;
    g_menu.ReadSettings();
    initTimers();

    SteeringAnimation::Load();

    if (!g_settings.Wheel.FFB.LUTFile.empty()) {
        logger.Write(INFO, "[Wheel] Using LUT file for FFB: '%s'",
            g_settings.Wheel.FFB.LUTFile.c_str());
        loadLut(g_settings.Wheel.FFB.LUTFile);
    }
    else {
        g_controls.GetWheel().ClearLut();
    }

    mShakeData->Load();


    logger.Write(INFO, "Settings read");
}

void threadCheckUpdate(unsigned milliseconds) {
    std::thread([milliseconds]() {
        std::lock_guard releaseInfoLock(g_releaseInfoMutex);
        std::lock_guard checkUpdateLock(g_checkUpdateDoneMutex);
        std::lock_guard notifyUpdateLock(g_notifyUpdateMutex);

        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));

        bool newAvailable = CheckUpdate(g_releaseInfo);

        if (newAvailable && g_settings.Update.IgnoredVersion != g_releaseInfo.Version) {
            g_notifyUpdate = true;
        }
        g_checkUpdateDone = true;
    }).detach();
}

void update_update_notification() {
    std::unique_lock releaseInfoLock(g_releaseInfoMutex, std::try_to_lock);
    std::unique_lock checkUpdateLock(g_checkUpdateDoneMutex, std::try_to_lock);
    std::unique_lock notifyUpdateLock(g_notifyUpdateMutex, std::try_to_lock);

    if (checkUpdateLock.owns_lock() && releaseInfoLock.owns_lock() && notifyUpdateLock.owns_lock()) {
        if (g_checkUpdateDone) {
            g_checkUpdateDone = false;
            if (g_notifyUpdate) {
                UI::Notify(INFO, fmt::format("Manual Transmission: Update available, new version: {}.",
                                         g_releaseInfo.Version), false);
            }
            else if (g_releaseInfo.Version.empty()) {
                UI::Notify(INFO, "Manual Transmission: Failed to check for update.", false);
            }
            else {
                UI::Notify(INFO, fmt::format("Manual Transmission: No update available, latest version: {}.",
                                         g_releaseInfo.Version), false);
            }
        }
    }
}

void ScriptInit() {
    std::string absoluteModPath = Paths::GetModPath();
    std::string settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    std::string settingsControlsFile = absoluteModPath + "\\settings_controls.ini";
    std::string settingsWheelFile = absoluteModPath + "\\settings_wheel.ini";
    std::string settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    std::string animationsFile = absoluteModPath + "\\animations.yml";

    g_settings.SetFiles(settingsGeneralFile, settingsControlsFile, settingsWheelFile);
    g_menu.RegisterOnMain([] { onMenuInit(); });
    g_menu.RegisterOnExit([] { onMenuClose(); });
    g_menu.SetFiles(settingsMenuFile);
    g_menu.Initialize();

    const auto shakeDataPath = Paths::GetModPath() + "\\shake.ini";

    mShakeData = std::make_shared<CShakeData>(shakeDataPath);
    SteeringAnimation::SetFile(animationsFile);

    readSettings();
    loadConfigs();

    if (g_settings.Update.EnableUpdate) {
        threadCheckUpdate(10000);
    }

    VExt::Init();
    if (!MemoryPatcher::Test()) {
        logger.Write(ERROR, "Patchability test failed!");
        MemoryPatcher::Error = true;
    }

    setupCompatibility();

    initWheel();

    g_focused = SysUtil::IsWindowFocused();

    logger.Write(DEBUG, "START: Starting with MT:  %s", g_settings.MTOptions.Enable ? "ON" : "OFF");
    logger.Write(INFO, "START: Initialization finished");

    StartUDPTelemetry();
}

void ScriptTick() {
    while (true) {
        update_player();
        update_vehicle();
        Dashboard::Update();
        Misc::UpdateEngineOnOff();
        update_inputs();
        update_steering();
        MTHUD::UpdateHUD();
        update_input_controls();
        update_manual_transmission();
        update_misc_features();
        GearRattle::Update();
        update_menu();
        update_update_notification();
        update_UDPTelemetry();
        SteeringAnimation::Update();
        StartingAnimation::Update();
        UpdatePause();
        WAIT(0);
    }
}

bool initialized = false;

uint64_t lastPauseTrigger = 0;
void UpdatePause() {
    if (PAD::IS_CONTROL_PRESSED(0, eControl::ControlFrontendPause) ||
        PAD::IS_CONTROL_PRESSED(0, eControl::ControlFrontendPauseAlternate)) {
        lastPauseTrigger = GetTickCount64();
    }

    // Probably unpaused
    if (lastPauseTrigger + 750 > GetTickCount64()) {
        g_controls.PlayFFBCollision(0);
        g_controls.PlayFFBDynamics(0, 0);
    }
}

void ScriptMain() {
    if (!initialized) {
        logger.Write(INFO, "Script started");
        ScriptInit();
        initialized = true;
    }
    else {
        logger.Write(INFO, "Script restarted");
    }
    Textures::Init();

    ScriptTick();
}
