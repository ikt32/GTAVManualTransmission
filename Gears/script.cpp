#include "script.h"

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
#include "Camera.h"
#include "Misc.h"
#include "StartingAnimation.h"
#include "DrivingAssists.h"
#include "ScriptHUD.h"
#include "AWD.h"
#include "LaunchControl.h"
#include "CruiseControl.h"
#include "SpeedLimiter.h"

#include "UDPTelemetry/Socket.h"
#include "UDPTelemetry/UDPTelemetry.h"

#include "Memory/MemoryPatcher.hpp"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleFlags.h"

#include "Input/CarControls.hpp"

#include "Util/ScriptUtils.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/MathExt.h"
#include "Util/Files.h"
#include "Util/UIUtils.h"
#include "Util/Timer.h"
#include "Util/ValueTimer.h"
#include "Util/GameSound.h"
#include "Util/SysUtils.h"
#include "Util/Strings.hpp"

#include <GTAVDashHook/DashHook/DashHook.h>
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

namespace fs = std::filesystem;
using VExt = VehicleExtensions;

ReleaseInfo g_releaseInfo;
std::mutex g_releaseInfoMutex;

bool g_notifyUpdate;
std::mutex g_notifyUpdateMutex;

bool g_checkUpdateDone;
std::mutex g_checkUpdateDoneMutex;

Socket g_socket;

int g_textureWheelId;
int g_textureAbsId;
int g_textureTcsId;
int g_textureEspId;
int g_textureBrkId;

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

std::vector<ValueTimer<float>> g_speedTimers;

// sadly there's no initial BONK, but the sound works.
GameSound g_gearRattle1("DAMAGED_TRUCK_IDLE", "", "");
// primitively double the volume...
GameSound g_gearRattle2("DAMAGED_TRUCK_IDLE", "", "");

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
void functionAudioFX();

void functionDash() {
    if (!g_settings.Misc.DashExtensions)
        return;

    VehicleDashboardData data{};
    DashHook_GetData(&data);

    data.ABSLight |= DashLights::AbsBulbState && DashLights::AbsNotify;

    if (g_peripherals.IgnitionState == IgnitionState::Stall) {
        // data.indicator_left = true;
        // data.indicator_right = true;
        // data.handbrakeLight = true;
        data.engineLight = true;
        data.ABSLight = true;
        data.petrolLight = true;
        data.oilLight = true;
        // data.headlights = true;
        // data.fullBeam = true;
        data.batteryLight = true;
    }

    if (g_settings().DriveAssists.AWD.Enable) {
        if (g_settings().DriveAssists.AWD.SpecialFlags & AWD::AWD_REMAP_DIAL_Y97Y_R32) {
            data.oilPressure = lerp(
                data.oilPressure,
                AWD::GetTransferValue(),
                1.0f - pow(0.0001f, MISC::GET_FRAME_TIME()));
            // https://www.gta5-mods.com/vehicles/nissan-skyline-gt-r-bnr32
            // oil pressure gauge uses data.temp
            // battery voltage uses data.temp
        }
        else if (g_settings().DriveAssists.AWD.SpecialFlags & AWD::AWD_REMAP_DIAL_WANTED188_R32) {
            // oil temperature doesn't remember value set here, so keep track of it ourselves
            // also reduce range.

            AWD::GetDisplayValue() = lerp(
                AWD::GetDisplayValue(),
                map(AWD::GetTransferValue(), 0.0f, 1.0f, 0.27f, 0.70f),
                1.0f - pow(0.0001f, MISC::GET_FRAME_TIME()));

            data.oilTemperature = AWD::GetDisplayValue();
        }
    }

    DashHook_SetData(data);
}

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
        g_gearRattle1.Stop();
        g_gearRattle2.Stop();
        updateActiveSteeringAnim(g_playerVehicle);
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
        functionDash();
    }
    if (g_playerVehicle != g_lastPlayerVehicle && vehAvail) {
        if (g_vehData.mGearTop == 1 || g_vehData.mFlags[1] & FLAG_IS_ELECTRIC)
            g_gearStates.FakeNeutral = false;
        else
            g_gearStates.FakeNeutral = g_settings.GameAssists.DefaultNeutral;
    }

    if (g_settings.Debug.Metrics.EnableTimers && vehAvail) {
        for(auto& valueTimer : g_speedTimers) {
            float speed;
            switch(joaat(valueTimer.mUnit.c_str())) {
                case (joaat("kph")):
                    speed = g_vehData.mVelocity.y * 3.6f;
                    break;
                case (joaat("mph")):
                    speed = g_vehData.mVelocity.y / 0.44704f;
                    break;
                default:
                    speed = g_vehData.mVelocity.y;
            }
            valueTimer.Update(speed);
        }
    }
    
    g_lastPlayerVehicle = g_playerVehicle;
}

// Read inputs
void update_inputs() {
    if (g_focused != SysUtil::IsWindowFocused()) {
        // no focus -> focus
        if (!g_focused) {
            logger.Write(DEBUG, "[Wheel] Window focus gained: re-initializing FFB");
            g_wheelInitDelayTimer.Reset(100);
        }
        else {
            logger.Write(DEBUG, "[Wheel] Window focus lost");
        }
    }
    g_focused = SysUtil::IsWindowFocused();

    if (g_wheelInitDelayTimer.Expired() && g_wheelInitDelayTimer.Period() > 0) {
        initWheel();
        g_wheelInitDelayTimer.Reset(0);
    }

    updateLastInputDevice();
    g_controls.UpdateValues(g_controls.PrevInput, false);
}

void update_hud() {
    if (!Util::PlayerAvailable(g_player, g_playerPed) || !Util::VehicleAvailable(g_playerVehicle, g_playerPed)) {
        return;
    }

    MTHUD::UpdateHUD();
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
    if (g_vehData.mIsAmphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(g_playerVehicle) > 0.10f) {
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
    functionAudioFX();

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

    if (g_settings.MTOptions.EngLock) {
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
        g_controls.ButtonHeld(CarControls::ControllerControlType::Toggle) ||
        g_controls.PrevInput == CarControls::Controller	&& g_controls.ButtonHeld(CarControls::LegacyControlType::Toggle)) {
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
        g_controls.ButtonHeld(CarControls::ControllerControlType::ToggleH) ||
        g_controls.PrevInput == CarControls::Controller	&& g_controls.ButtonHeld(CarControls::LegacyControlType::ToggleH)) {
        setShiftMode(Next(g_settings().MTOptions.ShiftMode));
    }

    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::CycleAssists) || 
        g_controls.ButtonJustPressed(CarControls::WheelControlType::CycleAssists) ||
        g_controls.ButtonHeld(CarControls::ControllerControlType::CycleAssists) ||
        g_controls.PrevInput == CarControls::Controller && g_controls.ButtonHeld(CarControls::LegacyControlType::CycleAssists)) {

        uint8_t currMode = 0;
        // 3: ABS + ESC + TCS
        // 2: ABS + ESC || TCS
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
                UI::Notify(INFO, "Assists: ABS + ESC + TCS");
                g_settings().DriveAssists.ABS.Enable = true;
                g_settings().DriveAssists.ESP.Enable = true;
                g_settings().DriveAssists.TCS.Enable = true;
                break;
            case 2:
                UI::Notify(INFO, "Assists: ABS + ESC");
                g_settings().DriveAssists.ABS.Enable = true;
                g_settings().DriveAssists.ESP.Enable = true;
                g_settings().DriveAssists.TCS.Enable = false;
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
    VExt::SetSteeringMultiplier(g_playerVehicle, mult);
}

void resetSteeringMultiplier() {
    if (g_playerVehicle != 0) {
        VExt::SetSteeringMultiplier(g_playerVehicle, 1.0f);
    }
}

void updateLastInputDevice() {
    if (g_settings.Debug.DisableInputDetect)
        return;

    if (!PED::IS_PED_IN_ANY_VEHICLE(g_playerPed, true))
        return;

    if (g_controls.PrevInput != g_controls.GetLastInputDevice(g_controls.PrevInput,g_settings.Wheel.Options.Enable)) {
        g_controls.PrevInput = g_controls.GetLastInputDevice(g_controls.PrevInput, g_settings.Wheel.Options.Enable);
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
        PAD::STOP_PAD_SHAKE(0);
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

bool isClutchPressed() {
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic)
        return false;
    return g_controls.ClutchVal > 1.0f - g_settings().MTParams.ClutchThreshold;
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

    // Rate multipliers for transmission tuning
    // Based on 1.0 rate with the Sultan.
    // Stock  1.00
    // Street 1.25
    // Sport  1.50
    // Race   2.00

    int modVal = VEHICLE::GET_VEHICLE_MOD(vehicle, eVehicleMod::VehicleModTransmission);

    switch (modVal) {
        case -1: shiftRate *= 1.00f; break;
        case  0: shiftRate *= 1.25f; break;
        case  1: shiftRate *= 1.50f; break;
        case  2: shiftRate *= 2.00f; break;
        default: shiftRate *= 2.00f; break;
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
    else if (isClutchPressed())
        shiftPass = true;
    else
        shiftPass = false;

    if (shiftPass) {
        shiftTo(i, false);
        g_gearStates.FakeNeutral = false;
    }
    else {
        g_gearStates.FakeNeutral = true;
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
    if (g_controls.ButtonJustPressed(CarControls::KeyboardControlType::HN) && g_vehData.mHasClutch) {
        g_gearStates.FakeNeutral = !g_gearStates.FakeNeutral;
    }
}

bool isHShifterJustNeutral() {
    auto minGear = CarControls::WheelControlType::HR;
    auto topGear = CarControls::WheelControlType::H10;
    for (auto gear = static_cast<uint32_t>(minGear); gear <= static_cast<uint32_t>(topGear); ++gear) {
        if (g_controls.ButtonReleased(static_cast<CarControls::WheelControlType>(gear)))
            return true;
    }
    return false;
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

    if (isHShifterJustNeutral()) {
        g_gearStates.FakeNeutral = g_vehData.mHasClutch;
    }

    if (g_controls.ButtonReleased(CarControls::WheelControlType::HR)) {
        shiftTo(1, false);
        g_gearStates.FakeNeutral = g_vehData.mHasClutch;
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
    float cl = g_gearStates.ShiftDirection == ShiftDirection::Up ? 0.1f : 0.9f;

    if (shiftProgress <= 0.1f) {
        g_gearStates.ClutchVal = map(shiftProgress, 0.0f, 0.1f, 0.0f, 1.0f);
        g_gearStates.ShiftState = ShiftState::PressingClutch;
    }
    else if (g_gearStates.LockGear != g_gearStates.NextGear) {
        g_gearStates.ClutchVal = 1.0f;
        g_gearStates.LockGear = g_gearStates.NextGear;
        g_gearStates.ShiftState = ShiftState::FullClutch;
    }

    if (shiftProgress > cl) {
        g_gearStates.ClutchVal = map(shiftProgress, cl, 1.0f, 1.0f, 0.0f);
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

    bool allowKeyboard = g_controls.PrevInput == CarControls::Keyboard || 
        g_controls.PrevInput == CarControls::Controller;

    // Shift up
    if (g_controls.PrevInput == CarControls::Controller	&& xcTapStateUp == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller	&& ncTapStateUp == NativeController::TapState::Tapped ||
        allowKeyboard && g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        g_controls.PrevInput == CarControls::Wheel			&& g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        if (!g_vehData.mHasClutch) {
            if (g_vehData.mGearCurr < g_vehData.mGearTop) {
                shiftTo(g_gearStates.LockGear + 1, true);
            }
            g_gearStates.FakeNeutral = false;
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (g_settings().MTOptions.ClutchShiftS && 
            !isClutchPressed()) {
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

    if (g_controls.PrevInput == CarControls::Controller	&& xcTapStateDn == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller	&& ncTapStateDn == NativeController::TapState::Tapped ||
        allowKeyboard && g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        g_controls.PrevInput == CarControls::Wheel			&& g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        if (!g_vehData.mHasClutch) {
            if (g_vehData.mGearCurr > 0) {
                shiftTo(g_gearStates.LockGear - 1, false);
                g_gearStates.FakeNeutral = false;
            }
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (g_settings().MTOptions.ClutchShiftS &&
            !isClutchPressed()) {
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

    bool allowKeyboard = g_controls.PrevInput == CarControls::Keyboard ||
        g_controls.PrevInput == CarControls::Controller;

    // Shift up
    if (g_controls.PrevInput == CarControls::Controller	&& xcTapStateUp == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller	&& ncTapStateUp == NativeController::TapState::Tapped ||
        allowKeyboard  && g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        g_controls.PrevInput == CarControls::Wheel			&& g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        // Reverse to Neutral
        if (g_vehData.mGearCurr == 0 && !g_gearStates.FakeNeutral) {
            shiftTo(1, false);
            g_gearStates.FakeNeutral = true;
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
    if (g_controls.PrevInput == CarControls::Controller	&& xcTapStateDn == XInputController::TapState::Tapped ||
        g_controls.PrevInput == CarControls::Controller	&& ncTapStateDn == NativeController::TapState::Tapped ||
        allowKeyboard && g_controls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        g_controls.PrevInput == CarControls::Wheel			&& g_controls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        // 1 to Neutral
        if (g_vehData.mGearCurr == 1 && !g_gearStates.FakeNeutral) {
            g_gearStates.FakeNeutral = true;
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
        g_gearStates.FakeNeutral = true;
        PAD::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
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
            g_gearStates.FakeNeutral = true;
            return true;
        }
    }
    // Assigned neutral -> handle like any other button.
    else {
        if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ANeutral)) {
            if (g_gearStates.LockGear != 1) {
                shiftTo(1, false);
            }
            g_gearStates.FakeNeutral = true;
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

    bool clutchEngaged = !isClutchPressed() && !g_gearStates.FakeNeutral;

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

    bool clutchEngaged = !isClutchPressed() && !g_gearStates.FakeNeutral;

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
                PAD::SET_PAD_SHAKE(0, 100, 255);
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
                                           VEHICLE::GET_VEHICLE_ENGINE_HEALTH(g_playerVehicle) - (g_settings().MTParams.RPMDamage));
    }
}

void functionEngLock() {
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic ||
        g_vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC ||
        g_vehData.mGearTop == 1 || 
        g_vehData.mGearCurr == g_vehData.mGearTop ||
        g_gearStates.FakeNeutral ||
        g_wheelPatchStates.InduceBurnout) {
        g_wheelPatchStates.EngLockActive = false;
        return;
    }
    const float reverseThreshold = 2.0f;

    float dashms = abs(g_vehData.mDiffSpeed);

    float speed = dashms;
    auto ratios = g_vehData.mGearRatios;
    float DriveMaxFlatVel = g_vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[g_vehData.mGearCurr];

    float inputMultiplier = (1.0f - g_controls.ClutchVal);
    auto wheelsSpeed = g_vehData.mDiffSpeed;

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
    if ((speed > abs(maxSpeed * 1.15f) + 3.334f || wrongDirection) && !isClutchPressed()) {
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
    // When you let go of the throttle at high RPMs
    float activeBrakeThreshold = g_settings().MTParams.EngBrakeThreshold;

    float throttleMultiplier = 1.0f - g_controls.ThrottleVal;
    float clutchMultiplier = 1.0f - g_controls.ClutchVal;

    // Always treat clutch pedal as unpressed for auto
    if (g_settings().MTOptions.ShiftMode == EShiftMode::Automatic)
        clutchMultiplier = 1.0f;

    float inputMultiplier = throttleMultiplier * clutchMultiplier;

    if (g_wheelPatchStates.EngLockActive || 
        g_wheelPatchStates.InduceBurnout || 
        g_gearStates.FakeNeutral ||
        g_vehData.mDiffSpeed < 5.0f ||
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
        }
        else {
            slipMin = tcsSlipMin;
            slipMax = tcsSlipMax;
        }
        
        tractionThrottle = map(tcsData.AverageSlip,
            slipMin, slipMax,
            g_controls.ThrottleVal, 0.0f);
        tractionThrottle = std::clamp(tractionThrottle, 0.0f, 1.0f);

        finalThrottle = tractionThrottle;

        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            if (tcsData.SlippingWheels[i]) {
                g_vehData.mWheelsTcs[i] = true;
            }
            else {
                g_vehData.mWheelsTcs[i] = false;
            }
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

            if (g_gearStates.ShiftDirection == ShiftDirection::Down &&
                g_settings().ShiftOptions.DownshiftBlip) {
                float expectedRPM = g_vehData.mDiffSpeed / (g_vehData.mDriveMaxFlatVel / g_vehData.mGearRatios[g_gearStates.NextGear]);
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
        absData.Use ||
        espData.Use ||
        tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0;

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

        // LSD is used in all assists, but this case if for when no other
        // assists are active.
        if (lsdData.Use) {
            auto brakeVals = DrivingAssists::GetLSDBrakes(lsdData);
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVals[i]);
            }
        }
        if (espData.Use) {
            auto brakeVals = DrivingAssists::GetESPBrakes(espData, lsdData);
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVals[i]);
            }
        }
        if (tcsData.Use && g_settings().DriveAssists.TCS.Mode == 0) {
            auto brakeVals = DrivingAssists::GetTCSBrakes(tcsData, lsdData);
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVals[i]);
            }
        }
        if (absData.Use) {
            auto brakeVals = DrivingAssists::GetABSBrakes(absData, lsdData);
            for (int i = 0; i < g_vehData.mWheelCount; i++) {
                VExt::SetWheelBrakePressure(g_playerVehicle, i, brakeVals[i]);
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
        for (int i = 0; i < g_vehData.mWheelCount; i++) {
            if (!tcsData.Use)
                g_vehData.mWheelsTcs[i] = false;
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
        UI::ShowText(0.60f, 0.125f, 0.25f, fmt::format("Average slip: {:.2f} m/s", tcsData.AverageSlip));
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
    g_gearStates.HitRPMLimiter = g_vehData.mRPM > 1.0f;

    if (!g_settings.MTOptions.HardLimiter && 
        (g_vehData.mGearCurr == g_vehData.mGearTop || g_vehData.mGearCurr == 0)) {
        g_gearStates.HitRPMSpeedLimiter = false;
        return;
    }

    auto ratios = g_vehData.mGearRatios;
    float DriveMaxFlatVel = g_vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[g_vehData.mGearCurr];

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
            PAD::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, g_controls.BrakeVal);
            VExt::SetThrottle(g_playerVehicle, 0.0f);
            VExt::SetThrottleP(g_playerVehicle, 0.1f);
            VExt::SetBrakeP(g_playerVehicle, 1.0f);
        }
        // RT behavior when rolling back: Burnout
        if (!g_gearStates.FakeNeutral && g_controls.ThrottleVal > 0.5f && !isClutchPressed() &&
            roadSpeed < -1.0f ) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
            //PAD::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.ThrottleVal);
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
            PAD::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, g_controls.ThrottleVal);
        }
        // LT behavior when reversing
        if (g_controls.BrakeVal > 0.01f &&
            roadSpeed <= -0.5f) {
            throttleAndSomeBrake++;
            //UI::ShowText(0.3, 0.35, 0.5, "functionRealReverse: Brake @ Reverse");

            PAD::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            PAD::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, g_controls.BrakeVal);
        }
        // Throttle > brake  && BrakeVal > 0.1f
        if (throttleAndSomeBrake >= 2) {
            //UI::ShowText(0.3, 0.4, 0.5, "functionRealReverse: Weird combo + rev it");

            PAD::ENABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            PAD::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, g_controls.BrakeVal);
            fakeRev(false, 0);
        }

        // LT behavior when forward
        if (g_controls.BrakeVal > 0.01f && g_controls.ThrottleVal <= g_controls.BrakeVal &&
            roadSpeed > 0.1f) {
            //UI::ShowText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
            PAD::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, g_controls.BrakeVal);

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
            PAD::_SET_CONTROL_NORMAL(0, blockList[i], 1.0f);
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
    bool prevKeyboard = g_controls.PrevInput == CarControls::Keyboard;
    bool prevWheel = g_controls.PrevInput == CarControls::Wheel;

    bool heldControllerXinput = g_controls.ButtonReleasedAfter(CarControls::ControllerControlType::Engine, g_settings.Controller.HoldTimeMs);
    bool heldControllerNative = g_controls.ButtonReleasedAfter(CarControls::LegacyControlType::Engine, g_settings.Controller.HoldTimeMs);

    bool pressedKeyboard = g_controls.ButtonJustPressed(CarControls::KeyboardControlType::Engine);
    bool pressedWheel = g_controls.ButtonJustPressed(CarControls::WheelControlType::Engine);

    bool controllerActive = prevController && heldControllerXinput || prevController && heldControllerNative;
    bool keyboardActive = prevKeyboard && pressedKeyboard;
    bool wheelActive = prevWheel && pressedWheel;

    bool throttleStart = false;
    if (g_settings.GameAssists.ThrottleStart) {
        bool clutchedIn = isClutchPressed();
        bool freeGear = clutchedIn || g_gearStates.FakeNeutral;
        throttleStart = g_controls.ThrottleVal > 0.75f && freeGear;
    }

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
        PAD::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
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
                g_playerVehicle, 1, 0.0f, -1 * (pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f, true, true, true, true);
        }
        if (pitch > 10.0f || clutchNeutral)
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            g_playerVehicle, 1, 0.0f, -1 * (pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f, true, true, true, true);
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

bool isHShifterInAnySlot() {
    return g_controls.ButtonIn(CarControls::WheelControlType::HR) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H1) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H2) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H3) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H4) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H5) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H6) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H7) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H8) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H9) ||
        g_controls.ButtonIn(CarControls::WheelControlType::H10);
}

void functionAudioFX() {
    if (!g_gearRattle1.Playing()) {
        // play on misshift in wheel
        if (g_controls.PrevInput == CarControls::Wheel && g_settings().MTOptions.ShiftMode == EShiftMode::HPattern) {
            // When the shifter is in a slot, but the gearbox is neutral.
            // Also grinds for nonexisting gears on that car, but you shouldn't be shifting in there anyway.
            if (g_gearStates.FakeNeutral && isHShifterInAnySlot() && !isClutchPressed()) {
                g_gearRattle1.Play(g_playerVehicle);
                g_gearRattle2.Play(g_playerVehicle);
            }
        }
        // No keeb support, as I'd need to mutilate the code even more to jam it in.
    }
    else {
        // no control check, since we wanna be able to cancel easily
        if (g_settings().MTOptions.ShiftMode == EShiftMode::HPattern) {
            // Stop when popping out of any gear
            // Stop when engine is off
            // Stop when clutch is pressed
            if (isHShifterJustNeutral() ||
                !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle) || 
                isClutchPressed()) {
                g_gearRattle1.Stop();
                g_gearRattle2.Stop();
            }
        }
        else {
            // Stop when switching gearboxes
            g_gearRattle1.Stop();
            g_gearRattle2.Stop();
        }
    }
}

void StartUDPTelemetry() {
    if (g_settings.Misc.UDPTelemetry)
        if (!g_socket.Started())
            g_socket.Start(20777);
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
    const std::string absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + Constants::ModDir;
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

void readSettings() {
    g_settings.Read(&g_controls);
    if (g_settings.Debug.LogLevel > 4)
        g_settings.Debug.LogLevel = 1;
    logger.SetMinLevel(static_cast<LogLevel>(g_settings.Debug.LogLevel));

    g_gearStates.FakeNeutral = g_settings.GameAssists.DefaultNeutral;
    g_menu.ReadSettings();
    initTimers();

    SteeringAnimation::Load();

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
    std::string absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + Constants::ModDir;
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

    SteeringAnimation::SetFile(animationsFile);

    readSettings();
    loadConfigs();

    if (g_settings.Update.EnableUpdate) {
        threadCheckUpdate(10000);
    }

    FPVCam::InitOffsets();
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

void InitTextures() {
    std::string absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + Constants::ModDir;
    std::string textureWheelFile = absoluteModPath + "\\texture_wheel.png";
    std::string textureABSFile = absoluteModPath + "\\texture_abs.png";
    std::string textureTCSFile = absoluteModPath + "\\texture_tcs.png";
    std::string textureESPFile = absoluteModPath + "\\texture_esp.png";
    std::string textureBRKFile = absoluteModPath + "\\texture_handbrake.png";

    if (FileExists(textureWheelFile)) {
        g_textureWheelId = createTexture(textureWheelFile.c_str());
    }
    else {
        logger.Write(ERROR, textureWheelFile + " does not exist.");
        g_textureWheelId = -1;
    }

    if (FileExists(textureABSFile)) {
        g_textureAbsId = createTexture(textureABSFile.c_str());
    }
    else {
        logger.Write(ERROR, textureABSFile + " does not exist.");
        g_textureAbsId = -1;
    }

    if (FileExists(textureTCSFile)) {
        g_textureTcsId = createTexture(textureTCSFile.c_str());
    }
    else {
        logger.Write(ERROR, textureTCSFile + " does not exist.");
        g_textureTcsId = -1;
    }

    if (FileExists(textureESPFile)) {
        g_textureEspId = createTexture(textureESPFile.c_str());
    }
    else {
        logger.Write(ERROR, textureESPFile + " does not exist.");
        g_textureEspId = -1;
    }

    if (FileExists(textureBRKFile)) {
        g_textureBrkId = createTexture(textureBRKFile.c_str());
    }
    else {
        logger.Write(ERROR, textureBRKFile + " does not exist.");
        g_textureBrkId = -1;
    }
}

void ScriptTick() {
    while (true) {
        update_player();
        update_vehicle();
        Misc::UpdateEngineOnOff();
        update_inputs();
        update_steering();
        update_hud();
        update_input_controls();
        update_manual_transmission();
        update_misc_features();
        update_menu();
        update_update_notification();
        update_UDPTelemetry();
        SteeringAnimation::Update();
        StartingAnimation::Update();
        FPVCam::Update();
        WAIT(0);
    }
}

bool initialized = false;

void ScriptMain() {
    if (!initialized) {
        logger.Write(INFO, "Script started");
        ScriptInit();
        initialized = true;
    }
    else {
        logger.Write(INFO, "Script restarted");
    }
    InitTextures();
    ScriptTick();
}
