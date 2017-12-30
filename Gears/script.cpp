#define NOMINMAX

#ifdef _DEBUG
#include "Dump.h"
#endif

#include <string>
#include <iomanip>
#include <algorithm>

#include "../../ScriptHookV_SDK/inc/natives.h"
#include "../../ScriptHookV_SDK/inc/enums.h"
#include "../../ScriptHookV_SDK/inc/main.h"
#include "../../ScriptHookV_SDK/inc/types.h"

#include "script.h"

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"

#include "Input/ScriptControls.hpp"
#include "Memory/MemoryPatcher.hpp"
#include "Util/Logger.hpp"
#include "Util/Util.hpp"
#include "Util/Paths.h"

#include "menu.h"
#include "Memory/NativeMemory.hpp"
#include "Util/MathExt.h"
#include "ShiftModes.h"
#include "Memory/Offsets.hpp"
#include "MiniPID/MiniPID.h"

const std::string mtPrefix = "~b~Manual Transmission~w~~n~";

const char* decorCurrentGear = "mt_gear";
const char* decorShiftNotice = "mt_shift_indicator";
const char* decorFakeNeutral = "mt_neutral";
const char* decorSetShiftMode = "mt_set_shiftmode";
const char* decorGetShiftMode = "mt_get_shiftmode";

std::array<float, NUM_GEARS> upshiftSpeedsGame{};
std::array<float, NUM_GEARS> upshiftSpeedsMod{};

std::string textureWheelFile;
int textureWheelId;

GameSound gearRattle("DAMAGED_TRUCK_IDLE", nullptr);
int soundID;

std::string absoluteModPath;
std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsStickFile;
std::string settingsMenuFile;

NativeMenu::Menu menu;
ScriptControls controls;
ScriptSettings settings;

Player player;
Ped playerPed;
Vehicle vehicle;
Vehicle lastVehicle;
VehicleData vehData;
VehicleExtensions ext;

int prevNotification = 0;
int prevExtShift = 0;

int speedoIndex;
extern std::vector<std::string> speedoTypes;

MiniPID pid(1.0, 0.0, 0.0);
DWORD	vehUpdateTime;

const float g_baseStallSpeed = 0.08f;
const float g_baseCatchMinSpeed = 0.12f;
const float g_baseCatchMaxSpeed = 0.24f;

float getAngleBetween(float h1, float h2, float separation) {
    auto diff = fabs(h1 - h2);
    if (diff < separation)
        return (separation - diff) / separation;
    if (fabs(diff - 360.0f) < separation)
        return (separation - fabs(diff - 360.0f)) / separation;
    return separation;
}

void showNPCInfo(Vehicle vehicles[1024], int count) {
    showText(0.9, 0.5, 0.4, "NPC Vehs: " + std::to_string(count));
    bool lookBack = CONTROLS::IS_CONTROL_PRESSED(2, ControlVehicleLookBehind) == TRUE;
    auto vehPos = ENTITY::GET_ENTITY_COORDS(vehicle, true);
    float searchdist = 50.0f;
    float searchfov = 15.0f;
    for (int i = 0; i < count; i++) {
        if (vehicles[i] == 0) continue;
        if (vehicles[i] == vehicle) continue;
        Vector3 targetPos = ENTITY::GET_ENTITY_COORDS(vehicles[i], true);
        Vector3 direction = Normalize(targetPos - vehPos);
        float vehDirection = atan2(direction.y, direction.x) * (180.0f / 3.14159f);
        float myHeading = ENTITY::GET_ENTITY_HEADING(vehicle) + 90.0f;
        if (lookBack) myHeading += 180.0f;
        float latDist = getAngleBetween(vehDirection, myHeading, searchfov);
        if (latDist < searchfov) {
            float dist = Distance(vehPos, targetPos);
            if (dist < searchdist) {
                float meCloseness = pow(2.0f * (searchdist - dist) / searchdist, 2.0f);
                int drawAlpha = std::min((int)(255 * latDist * meCloseness), 255);
                Color bgColor = transparentGray;
                Color fgColor = solidWhite;
                if (drawAlpha < bgColor.A) bgColor.A = drawAlpha;
                if (drawAlpha < fgColor.A) fgColor.A = drawAlpha;
                auto plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicles[i]);

                Color thColor = fgColor;
                float throttle = ext.GetThrottleP(vehicles[i]);

                if (throttle >= 0.0f) {
                    thColor.R = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                    thColor.B = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                }
                else {
                    thColor.B = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                    thColor.G = (int)map(throttle, 0.0f, 1.0f, 255.0f, 0.0f);
                }

                Color brColor = fgColor;
                float brake = ext.GetBrakeP(vehicles[i]);

                brColor.B = (int)map(brake, 0.0f, 1.0f, 255.0f, 0.0f);
                brColor.G = (int)map(brake, 0.0f, 1.0f, 255.0f, 0.0f);

                Color rpmColor = fgColor;
                float rpm = ext.GetCurrentRPM(vehicles[i]);
                rpmColor.G = (int)map(rpm, 0.0f, 1.0f, 255.0f, 165.0f);
                rpmColor.B = (int)map(rpm, 0.0f, 1.0f, 255.0f, 0.0f);


                showDebugInfo3DColors(targetPos, 
                    { { UI::_GET_LABEL_TEXT(VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicles[i]))), fgColor },
                      { plate, fgColor },
                      { "Throttle: " + std::to_string(throttle), thColor },
                      { "Brake: " + std::to_string(brake), brColor },
                      { "Steer:" + std::to_string(ext.GetSteeringAngle(vehicles[i])), fgColor },
                      { "RPM: " + std::to_string(rpm), rpmColor },
                      { "Gear: " + std::to_string(ext.GetGearCurr(vehicles[i])), fgColor },
                      { "Top Gear: " + std::to_string(ext.GetTopGear(vehicles[i])), fgColor }, }, 
                    bgColor);
            }
        }
    }
}

void updateNPCVehicles(Vehicle vehicles[1024], int count) {
    if (vehUpdateTime + 200 < GetTickCount()) {
        vehUpdateTime = GetTickCount();
        for (int i = 0; i < count; i++) {
            if (vehicles[i] == 0) continue;
            if (vehicles[i] == vehicle) continue;
            if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicles[i])) continue;
            if (ext.GetTopGear(vehicles[i]) == 1) continue;

            int currGear = ext.GetGearCurr(vehicles[i]);
            if (currGear == 0) continue;

            // common
            float currSpeed = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicles[i], true).y;
            auto ratios = ext.GetGearRatios(vehicles[i]);
            float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicles[i]);
            float InitialDriveMaxFlatVel = ext.GetInitialDriveMaxFlatVel(vehicles[i]);

            // Shift up
            float maxSpeedUpShiftWindow = DriveMaxFlatVel / ratios[currGear];
            float minSpeedUpShiftWindow = InitialDriveMaxFlatVel / ratios[currGear];
            float upshiftSpeed = map(ext.GetThrottleP(vehicles[i]), 0.0f, 1.0f, minSpeedUpShiftWindow, maxSpeedUpShiftWindow);

            if (currGear < ext.GetTopGear(vehicles[i])) {
                if (currSpeed > upshiftSpeed) {
                    ext.SetGearNext(vehicles[i], ext.GetGearCurr(vehicles[i]) + 1);
                    continue;
                }
            }

            // Shift down
            float prevGearTopSpeed = DriveMaxFlatVel / ratios[currGear - 1];
            float prevGearMinSpeed = InitialDriveMaxFlatVel / ratios[currGear - 1];
            float highEndShiftSpeed = fminf(minSpeedUpShiftWindow, prevGearTopSpeed);
            float prevGearDelta = prevGearTopSpeed - prevGearMinSpeed;
            float downshiftSpeed = map(ext.GetThrottleP(vehicles[i]), 0.0f, 1.0f, prevGearMinSpeed, highEndShiftSpeed);

            if (currGear > 1) {
                if (currSpeed < downshiftSpeed - prevGearDelta) {
                    ext.SetGearNext(vehicles[i], ext.GetGearCurr(vehicles[i]) - 1);
                }
            }
        }
    }
}

void update_npc() {
    bool mtActive = MemoryPatcher::ShiftUpPatched;
    if (!ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        !PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        ENTITY::IS_ENTITY_DEAD(playerPed) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) {
        return;
    }

    if (!vehicle || !ENTITY::DOES_ENTITY_EXIST(vehicle) ||
        playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
        return;
    }

    if (!settings.ShowNPCInfo && !(settings.EnableManual && mtActive)) return;

    const int ARR_SIZE = 1024;
    Vehicle vehicles[ARR_SIZE];
    int count = worldGetAllVehicles(vehicles, ARR_SIZE);

    if (settings.ShowNPCInfo) {
        showNPCInfo(vehicles, count);
    }

    if (settings.EnableManual && mtActive) {
        updateNPCVehicles(vehicles, count);
    }
}

void update() {
    ///////////////////////////////////////////////////////////////////////////
    //                     Are we in a supported vehicle?
    ///////////////////////////////////////////////////////////////////////////
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();

    if (!ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        !PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        ENTITY::IS_ENTITY_DEAD(playerPed) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) {
        stopForceFeedback();
        return;
    }

    vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    if (!vehicle || !ENTITY::DOES_ENTITY_EXIST(vehicle) || 
        playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
        stopForceFeedback();
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    //                       Update vehicle and inputs
    ///////////////////////////////////////////////////////////////////////////

    if (vehicle != lastVehicle) {
        initVehicle();
    }
    lastVehicle = vehicle;

    vehData.UpdateValues(ext, vehicle);

    bool ignoreClutch = false;
    if (settings.ShiftMode == Automatic ||
        vehData.Class == VehicleClass::Bike && settings.SimpleBike) {
        ignoreClutch = true;
    }

    updateLastInputDevice();
    controls.UpdateValues(controls.PrevInput, ignoreClutch, false);

    if (settings.CrossScript) {
        crossScriptUpdated();
    }

    updateSteeringMultiplier();

    ///////////////////////////////////////////////////////////////////////////
    //                                   HUD
    ///////////////////////////////////////////////////////////////////////////
    if (settings.DisplayInfo) {
        drawDebugInfo();
    }
    if (settings.DisplayWheelInfo) {
        drawVehicleWheelInfo();
    }
    if (settings.HUD && vehData.Domain == VehicleDomain::Road &&
        (settings.EnableManual || settings.AlwaysHUD)) {
        drawHUD();
    }
    if (settings.HUD &&
        (vehData.Domain == VehicleDomain::Road || vehData.Domain == VehicleDomain::Water) &&
        (controls.PrevInput == ScriptControls::Wheel || settings.AlwaysSteeringWheelInfo) &&
        settings.SteeringWheelInfo && textureWheelId != -1) {
        drawInputWheelInfo();
    }

    if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Toggle) ||
        controls.ButtonHeld(ScriptControls::WheelControlType::Toggle, 500) ||
        controls.ButtonHeld(ScriptControls::ControllerControlType::Toggle) ||
        controls.PrevInput == ScriptControls::Controller	&& controls.ButtonHeld(ScriptControls::LegacyControlType::Toggle)) {
        toggleManual();
        return;
    }

    ///////////////////////////////////////////////////////////////////////////
    //                            Alt vehicle controls
    ///////////////////////////////////////////////////////////////////////////

    if (settings.EnableWheel && controls.PrevInput == ScriptControls::Wheel) {

        if (vehData.Domain == VehicleDomain::Water) {
            handleVehicleButtons();
            handlePedalsDefault(controls.ThrottleVal, controls.BrakeVal);
            doWheelSteering();
            playFFBWater();
            return;
        }

    }
    
    ///////////////////////////////////////////////////////////////////////////
    //                          Ground vehicle controls
    ///////////////////////////////////////////////////////////////////////////

    if (!settings.EnableManual &&
        settings.EnableWheel && settings.WheelWithoutManual &&
        controls.WheelControl.IsConnected(controls.SteerGUID)) {

        handleVehicleButtons();
        handlePedalsDefault(controls.ThrottleVal, controls.BrakeVal);
        doWheelSteering();
        if (vehData.Amphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle) > 0.10f) {
            playFFBWater();
        }
        else {
            playFFBGround();
        }
    }	

    ///////////////////////////////////////////////////////////////////////////
    //          Active whenever Manual is enabled from here
    ///////////////////////////////////////////////////////////////////////////
    if (vehData.Domain != VehicleDomain::Road)
        return;

    if (!settings.EnableManual) {
        return;
    }

    if (MemoryPatcher::TotalPatched != MemoryPatcher::NumGearboxPatches) {
        MemoryPatcher::PatchInstructions();
    }
    if (!MemoryPatcher::ShiftUpPatched) {
        MemoryPatcher::PatchShiftUp();
    }
    
    handleVehicleButtons();

    if (settings.EnableWheel && controls.WheelControl.IsConnected(controls.SteerGUID)) {
        doWheelSteering();
        if (vehData.Amphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle) > 0.10f) {
            playFFBWater();
        }
        else {
            playFFBGround();
        }
    }
    

    if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ToggleH) ||
        controls.ButtonJustPressed(ScriptControls::WheelControlType::ToggleH) ||
        controls.ButtonHeld(ScriptControls::ControllerControlType::ToggleH) ||
        controls.PrevInput == ScriptControls::Controller	&& controls.ButtonHeld(ScriptControls::LegacyControlType::ToggleH)) {
        cycleShiftMode();
    }

    ///////////////////////////////////////////////////////////////////////////
    //                            Actual mod operations
    ///////////////////////////////////////////////////////////////////////////

    // Reverse behavior
    // For bikes, do this automatically.
    if (vehData.Class == VehicleClass::Bike && settings.SimpleBike) {
        if (controls.PrevInput == ScriptControls::InputDevices::Wheel) {
            handlePedalsDefault( controls.ThrottleVal, controls.BrakeVal );
        } else {
            functionAutoReverse();
        }
    }
    else {
        if (controls.PrevInput == ScriptControls::InputDevices::Wheel) {
            handlePedalsRealReverse( controls.ThrottleVal, controls.BrakeVal );
        } else {
            functionRealReverse();
        }
    }

    functionLimiter();

    if (settings.EngBrake) {
        functionEngBrake();
    }
    else {
        vehData.EngBrakeActive = false;
    }

    // Engine damage: RPM Damage
    if (settings.EngDamage &&
        !vehData.NoClutch) {
        functionEngDamage();
    }

    if (settings.EngLock) {
        functionEngLock();
    }
    else {
        vehData.EngLockActive = false;
    }

    manageBrakePatch();

    if (!vehData.FakeNeutral && 
        !(settings.SimpleBike && vehData.Class == VehicleClass::Bike) && 
        !vehData.NoClutch) {
        // Stalling
        if (settings.EngStall && settings.ShiftMode == HPattern ||
            settings.EngStallS && settings.ShiftMode == Sequential) {
            functionEngStall();
        }

        // Simulate "catch point"
        // When the clutch "grabs" and the car starts moving without input
        if (settings.ClutchCatching && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
            functionClutchCatch();
        }
    }

    // Manual shifting
    switch(settings.ShiftMode) {
        case Sequential: {
            functionSShift();
            if (settings.AutoGear1) {
                functionAutoGear1();
            }
            break;
        }
        case HPattern: {
            if (controls.PrevInput == ScriptControls::Wheel) {
                functionHShiftWheel();
            }
            if (controls.PrevInput == ScriptControls::Keyboard ||
                controls.PrevInput == ScriptControls::Wheel && settings.HPatternKeyboard) {
                functionHShiftKeyboard();
            }
            break;
        }
        case Automatic: {
            functionAShift();
            break;
        }
        default: break;
    }

    if (settings.AutoLookBack) {
        functionAutoLookback();
    }

    if (settings.HillBrakeWorkaround) {
        functionHillGravity();
    }

    if (gearRattle.Active) {
        if (controls.ClutchVal > 1.0f - settings.ClutchCatchpoint ||
            !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
            gearRattle.Stop();
        }
    }

    // Finally, update memory each loop
    handleRPM();
    ext.SetGearCurr(vehicle, vehData.LockGear);
    ext.SetGearNext(vehicle, vehData.LockGear);
}

///////////////////////////////////////////////////////////////////////////////
//                            Helper things
///////////////////////////////////////////////////////////////////////////////
void crossScriptUpdated() {
    // Current gear
    DECORATOR::DECOR_SET_INT(vehicle, (char *)decorCurrentGear, ext.GetGearCurr(vehicle));

    // Shift indicator: 0 = nothing, 1 = Shift up, 2 = Shift down
    if (vehData.HitLimiter) {
        DECORATOR::DECOR_SET_INT(vehicle, (char *)decorShiftNotice, 1);
    }
    else if (ext.GetGearCurr(vehicle) > 1 && vehData.RPM < 0.4f) {
        DECORATOR::DECOR_SET_INT(vehicle, (char *)decorShiftNotice, 2);
    }
    else if (ext.GetGearCurr(vehicle) == ext.GetGearNext(vehicle)) {
        DECORATOR::DECOR_SET_INT(vehicle, (char *)decorShiftNotice, 0);
    }

    // Simulated Neutral
    if (vehData.FakeNeutral && settings.EnableManual) {
        DECORATOR::DECOR_SET_INT(vehicle, (char *)decorFakeNeutral, 1);
    }
    else {
        DECORATOR::DECOR_SET_INT(vehicle, (char *)decorFakeNeutral, 0);
    }

    // External shifting
    int currExtShift = DECORATOR::DECOR_GET_INT(vehicle, (char *)decorSetShiftMode);
    if (prevExtShift != currExtShift && currExtShift > 0) {
        // 1 Seq, 2 H, 3 Auto
        setShiftMode(currExtShift - 1);
    }
    prevExtShift = currExtShift;

    DECORATOR::DECOR_SET_INT(vehicle, (char *)decorGetShiftMode, static_cast<int>(settings.ShiftMode) + 1);
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Mod control
///////////////////////////////////////////////////////////////////////////////

void initVehicle() {
    reset();
    std::fill(upshiftSpeedsGame.begin(), upshiftSpeedsGame.end(), 0.0f);
    std::fill(upshiftSpeedsMod.begin(), upshiftSpeedsMod.end(), 0.0f);
    vehData = VehicleData();
    vehData.UpdateValues(ext, vehicle);

    if (vehData.NoClutch) {
        vehData.FakeNeutral = false;
    }
    else {
        vehData.FakeNeutral = settings.DefaultNeutral;
    }
    shiftTo(1, false);
    ext.SetGearCurr(vehicle, 1);
    ext.SetGearNext(vehicle, 1);
    initSteeringPatches();
}

void initialize() {
    readSettings();
    initWheel();
}

void reset() {
    resetSteeringMultiplier();
    gearRattle.Stop();
    if (MemoryPatcher::TotalPatched == MemoryPatcher::NumGearboxPatches) {
        MemoryPatcher::RestoreInstructions();
    }
    if (MemoryPatcher::SteerCorrectPatched) {
        MemoryPatcher::RestoreSteeringCorrection();
    }
    if (MemoryPatcher::BrakeDecrementPatched) {
        MemoryPatcher::RestoreBrakeDecrement();
    }
    if (MemoryPatcher::ShiftUpPatched) {
        MemoryPatcher::RestoreShiftUp();
    }
    stopForceFeedback();
}

void toggleManual() {
    settings.EnableManual = !settings.EnableManual;
    settings.SaveGeneral();
    std::string message = mtPrefix;
    if (settings.EnableManual) {
        message += "Enabled";
    }
    else {
        message += "Disabled";
    }
    showNotification(message, &prevNotification);

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        VEHICLE::SET_VEHICLE_HANDBRAKE(vehicle, false);
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
    }
    if (!settings.EnableManual) {
        reset();
    }
    initialize();
    initSteeringPatches();
}

void initWheel() {
    controls.InitWheel();
    // controls.StickControl.InitDevice();
    controls.CheckGUIDs(settings.reggdGuids);
    controls.SteerGUID = controls.WheelAxesGUIDs[static_cast<int>(controls.SteerAxisType)];
    logger.Write(INFO, "WHEEL: Steering wheel initialization finished");
}

void stopForceFeedback() {
    if (controls.WheelControl.IsConnected(controls.SteerGUID)) {
        if (settings.LogiLEDs) {
            controls.WheelControl.PlayLedsDInput(controls.SteerGUID, 0.0, 0.5, 1.0);
        }
        controls.WheelControl.StopEffects();
    }
}

void initSteeringPatches() {
    if (settings.PatchSteering &&
        (controls.PrevInput == ScriptControls::Wheel || settings.PatchSteeringAlways) &&
        vehData.Class == VehicleClass::Car) {
        if (!MemoryPatcher::SteerCorrectPatched)
            MemoryPatcher::PatchSteeringCorrection();
    }
    else {
        if (MemoryPatcher::SteerCorrectPatched)
            MemoryPatcher::RestoreSteeringCorrection();
        resetSteeringMultiplier();
    }

    if (settings.PatchSteeringControl &&
        controls.PrevInput == ScriptControls::Wheel &&
        vehData.Class == VehicleClass::Car) {
        if (!MemoryPatcher::SteerControlPatched)
            MemoryPatcher::PatchSteeringControl();
    }
    else {
        if (MemoryPatcher::SteerControlPatched)
            MemoryPatcher::RestoreSteeringControl();
    }
}

void applySteeringMultiplier(float multiplier) {
    if (vehicle != 0) {
        ext.SetSteeringMultiplier(vehicle, multiplier);
    }
}

// From CustomSteering
void updateSteeringMultiplier() {
    float mult = 1;

    if (MemoryPatcher::SteerCorrectPatched) {
        Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(vehicle);
        Vector3 pos = ENTITY::GET_ENTITY_COORDS(vehicle, 1);
        Vector3 motion = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(vehicle, pos.x + vel.x, pos.y + vel.y, pos.z + vel.z);

        if (motion.y > 3) {
            mult = (0.15f + (powf((1.0f / 1.13f), (abs(motion.y) - 7.2f))));
            if (mult != 0) { mult = floorf(mult * 1000) / 1000; }
            if (mult > 1) { mult = 1; }
        }

        if (controls.PrevInput == ScriptControls::Wheel) {
            mult = (1 + (mult - 1) * settings.SteeringReductionWheel);
        }
        else {
            mult = (1 + (mult - 1) * settings.SteeringReductionOther);
        }
    }

    if (controls.PrevInput == ScriptControls::Wheel) {
        mult = mult * settings.GameSteerMultWheel;
    }
    else {
        mult = mult * settings.GameSteerMultOther;
    }

    applySteeringMultiplier(mult);
}

void resetSteeringMultiplier() {
    if (vehicle != 0) {
        ext.SetSteeringMultiplier(vehicle, 1.0f);
    }
}

void updateLastInputDevice() {
    if (controls.PrevInput != controls.GetLastInputDevice(controls.PrevInput,settings.EnableWheel)) {
        controls.PrevInput = controls.GetLastInputDevice(controls.PrevInput, settings.EnableWheel);
        std::string message = mtPrefix + "Input: ";
        switch (controls.PrevInput) {
            case ScriptControls::Keyboard:
                message += "Keyboard";
                break;
            case ScriptControls::Controller:
                message += "Controller";
                if (settings.ShiftMode == HPattern && settings.BlockHShift) {
                    message += "~n~Mode: Sequential";
                    setShiftMode(Sequential);
                }
                break;
            case ScriptControls::Wheel:
                message += "Steering wheel";
                break;
            default: break;
        }
        showNotification(message, &prevNotification);

        initSteeringPatches();
    }
    if (controls.PrevInput == ScriptControls::Wheel) {
        CONTROLS::STOP_PAD_SHAKE(0);
    }
    else {
        stopForceFeedback();
    }
}

///////////////////////////////////////////////////////////////////////////////
//                           Mod functions: Shifting
///////////////////////////////////////////////////////////////////////////////

void setShiftMode(int shiftMode) {
    gearRattle.Stop();
    if (shiftMode > 2 || shiftMode < 0)
        return;

    if ((shiftMode == Automatic ||
        shiftMode == Sequential) && ext.GetGearCurr(vehicle) > 1) {
        vehData.FakeNeutral = false;
    }

    if (shiftMode == HPattern &&
        controls.PrevInput == ScriptControls::Controller && 
        settings.BlockHShift) {
        settings.ShiftMode = Automatic;
    }
    else {
        settings.ShiftMode = (ShiftModes)shiftMode;
    }

    std::string mode = mtPrefix + "Mode: ";
    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch (settings.ShiftMode) {
        case Sequential: mode += "Sequential"; break;
        case HPattern: mode += "H-Pattern"; break;
        case Automatic: mode += "Automatic"; break;
        default: break;
    }
    showNotification(mode, &prevNotification);
}

void cycleShiftMode() {
    int tempShiftMode = settings.ShiftMode + 1;
    if (tempShiftMode >= SIZEOF_ShiftModes) {
        tempShiftMode = (ShiftModes)0;
    }

    setShiftMode(tempShiftMode);
    settings.SaveGeneral();
}

void shiftTo(int gear, bool autoClutch) {
    if (autoClutch) {
        controls.ClutchVal = 1.0f;
        CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
    }
    vehData.LockGear = gear;
}
void functionHShiftTo(int i) {
    if (settings.ClutchShiftingH && !vehData.NoClutch) {
        if (controls.ClutchVal > 1.0f - settings.ClutchCatchpoint) {
            shiftTo(i, false);
            vehData.FakeNeutral = false;
            gearRattle.Stop();
        }
        else {
            gearRattle.Play(vehicle);
            vehData.FakeNeutral = true;
            if (settings.EngDamage &&
                !vehData.NoClutch) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    vehicle,
                    VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - settings.MisshiftDamage);
            }
        }
    }
    else {
        shiftTo(i, true);
        vehData.FakeNeutral = false;
        gearRattle.Stop();
    }
}

void functionHShiftKeyboard() {
    int clamp = MAX_GEAR;
    if (ext.GetTopGear(vehicle) <= clamp) {
        clamp = ext.GetTopGear(vehicle);
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (controls.ButtonJustPressed(static_cast<ScriptControls::KeyboardControlType>(i))) {
            functionHShiftTo(i);
        }
    }
    if (controls.ButtonJustPressed(ScriptControls::KeyboardControlType::HN) &&
        !vehData.NoClutch) {
        vehData.FakeNeutral = !vehData.FakeNeutral;
    }
}

void functionHShiftWheel() {
    int clamp = MAX_GEAR;
    if (ext.GetTopGear(vehicle) <= clamp) {
        clamp = ext.GetTopGear(vehicle);
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (controls.ButtonJustPressed(static_cast<ScriptControls::WheelControlType>(i))) {
            functionHShiftTo(i);
        }
    }
    // Bleh
    if (controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H1)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H2)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H3)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H4)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H5)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H6)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::H7)) ||
        controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::HR))
    ) {
        if (settings.ClutchShiftingH &&
            settings.EngDamage &&
            !vehData.NoClutch) {
            if (controls.ClutchVal < 1.0 - settings.ClutchCatchpoint) {
                gearRattle.Play(vehicle);
            }
        }
        vehData.FakeNeutral = !vehData.NoClutch;
    }

    if (controls.ButtonReleased(static_cast<ScriptControls::WheelControlType>(ScriptControls::WheelControlType::HR))) {
        shiftTo(1, true);
        vehData.FakeNeutral = !vehData.NoClutch;
    }
}

bool isUIActive() {
    if (PED::IS_PED_RUNNING_MOBILE_PHONE_TASK(playerPed) || menu.IsThisOpen())
        return true;
    return false;
}

void functionSShift() {
    auto xcTapStateUp = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = controls.ButtonTapped(ScriptControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = controls.ButtonTapped(ScriptControls::LegacyControlType::ShiftDown);

    if (settings.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XboxController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = LegacyController::TapState::ButtonUp;
    }

    // Shift up
    if (controls.PrevInput == ScriptControls::Controller	&& xcTapStateUp == XboxController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Controller	&& ncTapStateUp == LegacyController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
        if (vehData.NoClutch) {
            if (ext.GetGearCurr(vehicle) < ext.GetTopGear(vehicle)) {
                shiftTo(vehData.LockGear + 1, true);
            }
            vehData.FakeNeutral = false;
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (settings.ClutchShiftingS && 
            controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
            return;
        }

        // Reverse to Neutral
        if (ext.GetGearCurr(vehicle) == 0 && !vehData.FakeNeutral) {
            shiftTo(1, true);
            vehData.FakeNeutral = true;
            return;
        }

        // Neutral to 1
        if (ext.GetGearCurr(vehicle) == 1 && vehData.FakeNeutral) {
            vehData.FakeNeutral = false;
            return;
        }

        // 1 to X
        if (ext.GetGearCurr(vehicle) < ext.GetTopGear(vehicle)) {
            shiftTo(vehData.LockGear + 1, true);
            vehData.FakeNeutral = false;
            return;
        }
    }

    // Shift down

    if (controls.PrevInput == ScriptControls::Controller	&& xcTapStateDn == XboxController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Controller	&& ncTapStateDn == LegacyController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
        if (vehData.NoClutch) {
            if (ext.GetGearCurr(vehicle) > 0) {
                shiftTo(vehData.LockGear - 1, true);
                vehData.FakeNeutral = false;
            }
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (settings.ClutchShiftingS &&
            controls.ClutchVal < 1.0f - settings.ClutchCatchpoint) {
            return;
        }

        // 1 to Neutral
        if (ext.GetGearCurr(vehicle) == 1 && !vehData.FakeNeutral) {
            vehData.FakeNeutral = true;
            return;
        }

        // Neutral to R
        if (ext.GetGearCurr(vehicle) == 1 && vehData.FakeNeutral) {
            shiftTo(0, true);
            vehData.FakeNeutral = false;
            return;
        }

        // X to 1
        if (ext.GetGearCurr(vehicle) > 1) {
            shiftTo(vehData.LockGear - 1, true);
            vehData.FakeNeutral = false;
        }
        return;
    }
}

/*
 * Manual part of the automatic transmission (R<->N<->D)
 */
bool subAShiftManual() {
    auto xcTapStateUp = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = controls.ButtonTapped(ScriptControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = controls.ButtonTapped(ScriptControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = controls.ButtonTapped(ScriptControls::LegacyControlType::ShiftDown);

    if (settings.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XboxController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = LegacyController::TapState::ButtonUp;
    }

    // Shift up
    if (controls.PrevInput == ScriptControls::Controller	&& xcTapStateUp == XboxController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Controller	&& ncTapStateUp == LegacyController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftUp) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftUp)) {
        // Reverse to Neutral
        if (ext.GetGearCurr(vehicle) == 0 && !vehData.FakeNeutral) {
            shiftTo(1, true);
            vehData.FakeNeutral = true;
            return true;
        }

        // Neutral to 1
        if (ext.GetGearCurr(vehicle) == 1 && vehData.FakeNeutral) {
            vehData.FakeNeutral = false;
            return true;
        }
    }

    // Shift down
    if (controls.PrevInput == ScriptControls::Controller	&& xcTapStateDn == XboxController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Controller	&& ncTapStateDn == LegacyController::TapState::Tapped ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::ShiftDown) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::ShiftDown)) {
        // 1 to Neutral
        if (ext.GetGearCurr(vehicle) == 1 && !vehData.FakeNeutral) {
            vehData.FakeNeutral = true;
            return true;
        }

        // Neutral to R
        if (ext.GetGearCurr(vehicle) == 1 && vehData.FakeNeutral) {
            shiftTo(0, true);
            vehData.FakeNeutral = false;
            return true;
        }
    }
    return false;
}

void functionAShift() { 
    // Manual part
    if (subAShiftManual()) return;
    
    int currGear = ext.GetGearCurr(vehicle);
    if (currGear == 0) return;

    float currSpeed = vehData.SpeedVector.y;
    auto ratios = ext.GetGearRatios(vehicle);
    float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicle);
    float InitialDriveMaxFlatVel = ext.GetInitialDriveMaxFlatVel(vehicle);

    float maxSpeedUpShiftWindow = DriveMaxFlatVel / ratios[currGear];
    float minSpeedUpShiftWindow = InitialDriveMaxFlatVel / ratios[currGear];
    float currGearDelta = maxSpeedUpShiftWindow - minSpeedUpShiftWindow;

    float prevGearTopSpeed = DriveMaxFlatVel / ratios[currGear - 1];
    float prevGearMinSpeed = InitialDriveMaxFlatVel / ratios[currGear - 1];
    float highEndShiftSpeed = fminf(minSpeedUpShiftWindow, prevGearTopSpeed);
    float prevGearDelta = prevGearTopSpeed - prevGearMinSpeed;

    float upshiftSpeed   = map(pow(controls.ThrottleVal, 6.0f), 0.0f, 1.0f, minSpeedUpShiftWindow, maxSpeedUpShiftWindow);
    float downshiftSpeed = map(pow(controls.ThrottleVal, 6.0f), 0.0f, 1.0f, prevGearMinSpeed,      highEndShiftSpeed);

    float acceleration = vehData.GetRelativeAcceleration().y / 9.81f;
    float accelExpect = pow(controls.ThrottleVal, 2.0f) * ratios[currGear] * VEHICLE::GET_VEHICLE_ACCELERATION(vehicle);

    // Shift up.
    if (currGear < ext.GetTopGear(vehicle)) {
        bool shouldShiftUpSPD = currSpeed > upshiftSpeed;
        bool shouldShiftUpACC = acceleration < 0.33f * accelExpect && !vehData.IgnoreAccelerationUpshiftTrigger;

        if (currSpeed > minSpeedUpShiftWindow && (shouldShiftUpSPD || shouldShiftUpACC)) {
            vehData.IgnoreAccelerationUpshiftTrigger = true;
            vehData.PrevUpshiftTime = GetTickCount();
            shiftTo(ext.GetGearCurr(vehicle) + 1, true);
            vehData.FakeNeutral = false;
            upshiftSpeedsMod[currGear] = currSpeed;
        }

        if (vehData.IgnoreAccelerationUpshiftTrigger) {
            float upshiftTimeout = 1.0f / *(float*)(ext.GetHandlingPtr(vehicle) + hOffsets.fClutchChangeRateScaleUpShift);
            if (vehData.IgnoreAccelerationUpshiftTrigger && GetTickCount() > vehData.PrevUpshiftTime + (int)(upshiftTimeout*1000.0f)) {
                vehData.IgnoreAccelerationUpshiftTrigger = false;
            }
        }
    }

    // Shift down
    if (currGear > 1) {
        if (currSpeed < downshiftSpeed - prevGearDelta) {
            shiftTo(currGear - 1, true);
            vehData.FakeNeutral = false;
        }
    }

    if (settings.DisplayInfo && !menu.IsThisOpen()) {
        showText(0.01, 0.525, 0.3, "CurrentSpeed: \t" + std::to_string(currSpeed));
        showText(0.01, 0.550, 0.3, "UpshiftSpeed: \t" + std::to_string(upshiftSpeed));
        showText(0.01, 0.575, 0.3, "DnshiftSpeed: \t" + std::to_string(downshiftSpeed - prevGearDelta));
        showText(0.01, 0.600, 0.3, "PrevGearDelta: \t" + std::to_string(prevGearDelta));
        showText(0.01, 0.625, 0.3, "CurrGearDelta: \t" + std::to_string(currGearDelta));
        showText(0.01, 0.650, 0.3, "Accel(Expect): \t" + std::to_string(accelExpect));
        showText(0.01, 0.675, 0.3, "Accel(Actual): \t\t" + std::to_string(acceleration));

    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
    float lowSpeed = g_baseCatchMinSpeed * (ext.GetDriveMaxFlatVel(vehicle) / ext.GetGearRatios(vehicle)[ext.GetGearCurr(vehicle)]);
    float mehSpeed = g_baseCatchMaxSpeed * (ext.GetDriveMaxFlatVel(vehicle) / ext.GetGearRatios(vehicle)[ext.GetGearCurr(vehicle)]);

    const float idleThrottle = 0.24f;

    if (controls.ClutchVal < 1.0f - settings.ClutchCatchpoint && 
        controls.ThrottleVal < 0.25f && controls.BrakeVal < 0.95) {
        if (settings.ShiftMode != HPattern && controls.BrakeVal > 0.1f || 
            vehData.RPM > 0.25f && vehData.SpeedVector.y >= lowSpeed) {
            return;
        }

        // Forward
        if (ext.GetGearCurr(vehicle) > 0 && vehData.SpeedVector.y < mehSpeed) {
            float throttle = map(vehData.SpeedVector.y, mehSpeed, lowSpeed, 0.0f, idleThrottle);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, throttle);
            ext.SetCurrentRPM(vehicle, 0.21f);
            ext.SetThrottle(vehicle, 0.0f);
        }

        // Reverse
        if (ext.GetGearCurr(vehicle) == 0) {
            float throttle = map(abs(vehData.SpeedVector.y), abs(mehSpeed), abs(lowSpeed), 0.0f, idleThrottle);
            // due to 0.25f deadzone throttle makes reversing anims stop, so its jerky
            // TODO: needs a workaround to prevent jerky reversing anims
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, throttle); 
            ext.SetCurrentRPM(vehicle, 0.21f);
            ext.SetThrottle(vehicle, 0.0f);
        }
    }
}

std::vector<float> getDrivenWheelsSpeeds(std::vector<float> wheelSpeeds) {
    std::vector<float> wheelsToConsider;
    if (ext.GetDriveBiasFront(vehicle) > 0.0f && ext.GetDriveBiasFront(vehicle) < 1.0f) {
        wheelsToConsider = wheelSpeeds;
    }
    else {
        // bikes
        if (ext.GetNumWheels(vehicle) == 2) {
            wheelsToConsider.push_back(wheelSpeeds[0]);
        }
        // normal cars
        else if (ext.GetNumWheels(vehicle) == 4) {
            // fwd
            if (ext.GetDriveBiasFront(vehicle) == 1.0f) {
                wheelsToConsider.push_back(wheelSpeeds[0]);
                wheelsToConsider.push_back(wheelSpeeds[1]);
            }
            // rwd
            else if (ext.GetDriveBiasFront(vehicle) == 0.0f) {
                wheelsToConsider.push_back(wheelSpeeds[2]);
                wheelsToConsider.push_back(wheelSpeeds[3]);
            }
        }
        // offroad, trucks
        else if (ext.GetNumWheels(vehicle) == 6) {
            // fwd
            if (ext.GetDriveBiasFront(vehicle) == 1.0f) {
                wheelsToConsider.push_back(wheelSpeeds[0]);
                wheelsToConsider.push_back(wheelSpeeds[1]);
            }
            // rwd
            else if (ext.GetDriveBiasFront(vehicle) == 0.0f) {
                wheelsToConsider.push_back(wheelSpeeds[2]);
                wheelsToConsider.push_back(wheelSpeeds[3]);
                wheelsToConsider.push_back(wheelSpeeds[4]);
                wheelsToConsider.push_back(wheelSpeeds[5]);
            }
        }
        else {
            wheelsToConsider = wheelSpeeds;
        }
    }
    return wheelsToConsider;
}

void functionEngStall() {
    float stallingRateDivisor = 3500000.0f;
    float timeStep = SYSTEM::TIMESTEP() * 100.0f;
    float avgWheelSpeed = abs(avg(getDrivenWheelsSpeeds(ext.GetTyreSpeeds(vehicle))));
    float stallSpeed = g_baseStallSpeed * abs(ext.GetDriveMaxFlatVel(vehicle)/ext.GetGearRatios(vehicle)[ext.GetGearCurr(vehicle)]);

    if (controls.ClutchVal < 1.0f - settings.StallingThreshold &&
        vehData.RPM < 0.2125f &&
        avgWheelSpeed < stallSpeed &&
        VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
        vehData.StallProgress += (rand() % 1000) / (stallingRateDivisor * (controls.ThrottleVal+0.001f) * timeStep);
    }
    else if (vehData.StallProgress > 0.0f) {
        vehData.StallProgress -= (rand() % 1000) / (stallingRateDivisor * (controls.ThrottleVal + 0.001f) * timeStep);
    }

    if (vehData.StallProgress > 1.0f) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
        }
        gearRattle.Stop();
        vehData.StallProgress = 0.0f;
    }
    //showText(0.1, 0.1, 1.0, "Stall progress: " + std::to_string(stallingProgress));
    //showText(0.1, 0.2, 1.0, "Stall speed: " + std::to_string(stallSpeed));
    //showText(0.1, 0.3, 1.0, "Actual speed: " + std::to_string(avgWheelSpeed));

    // Simulate push-start
    // We'll just assume the ignition thing is in the "on" position.
    if (avgWheelSpeed > stallSpeed && !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        controls.ClutchVal < 1.0f - settings.StallingThreshold) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, true, true);
    }
}

void functionEngDamage() {
    if (settings.ShiftMode == Automatic ||
        ext.GetTopGear(vehicle) == 1) {
        return;
    }

    if (ext.GetGearCurr(vehicle) != ext.GetTopGear(vehicle) &&
        vehData.RPM > 0.98f &&
        controls.ThrottleVal > 0.98f) {
        VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, 
                                           VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (settings.RPMDamage));
    }
}

// im solving the worng problem here help
std::vector<bool> getDrivenWheels() {
    int numWheels = ext.GetNumWheels(vehicle);
    std::vector<bool> wheelsToConsider;
    if (ext.GetDriveBiasFront(vehicle) > 0.0f && ext.GetDriveBiasFront(vehicle) < 1.0f) {
        for (int i = 0; i < numWheels; i++)
            wheelsToConsider.push_back(true);
    }
    else {
        // bikes
        if (numWheels == 2) {
            // Front id = 1, rear id = 0...
            wheelsToConsider.push_back(true);
            wheelsToConsider.push_back(false);
        }
        // normal cars
        else if (numWheels == 4) {
            // fwd
            if (ext.GetDriveBiasFront(vehicle) == 1.0f) {
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
            }
            // rwd
            else if (ext.GetDriveBiasFront(vehicle) == 0.0f) {
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
            }
        }
        // offroad, trucks
        else if (numWheels == 6) {
            // fwd
            if (ext.GetDriveBiasFront(vehicle) == 1.0f) {
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
            }
            // rwd
            else if (ext.GetDriveBiasFront(vehicle) == 0.0f) {
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(false);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
                wheelsToConsider.push_back(true);
            }
        }
        else {
            for (int i = 0; i < numWheels; i++)
                wheelsToConsider.push_back(true);
        }
    }
    return wheelsToConsider;
}

void functionEngLock() {
    if (settings.ShiftMode == Automatic ||
        ext.GetTopGear(vehicle) == 1 || 
        ext.GetGearCurr(vehicle) == ext.GetTopGear(vehicle) ||
        vehData.FakeNeutral) {
        vehData.EngLockActive = false;
        gearRattle.Stop();
        return;
    }
    const float reverseThreshold = 2.0f;

    float dashms = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y);

    float speed = dashms;
    auto ratios = ext.GetGearRatios(vehicle);
    float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicle);
    float maxSpeed = DriveMaxFlatVel / ratios[ext.GetGearCurr(vehicle)];

    float inputMultiplier = (1.0f - controls.ClutchVal);
    auto wheelsSpeed = avg(getDrivenWheelsSpeeds(ext.GetTyreSpeeds(vehicle)));

    bool wrongDirection = false;
    if (ext.GetGearCurr(vehicle) == 0 && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > reverseThreshold && wheelsSpeed > reverseThreshold) {
            wrongDirection = true;
        }
    }
    else {
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -reverseThreshold && wheelsSpeed < -reverseThreshold) {
            wrongDirection = true;
        }
    }

    // Wheels are locking up due to bad (down)shifts
    if ((speed > abs(maxSpeed * 1.15f) + 3.334f || wrongDirection) && inputMultiplier > settings.ClutchCatchpoint) {
        vehData.EngLockActive = true;
        float lockingForce = 60.0f * inputMultiplier;
        auto wheelsToLock = getDrivenWheels();

        for (int i = 0; i < ext.GetNumWheels(vehicle); i++) {
            if (i >= wheelsToLock.size() || wheelsToLock[i]) {
                ext.SetWheelBrakePressure(vehicle, i, lockingForce);
                ext.SetWheelSkidSmokeEffect(vehicle, i, lockingForce);
            }
            else {
                float inpBrakeForce = *reinterpret_cast<float *>(ext.GetHandlingPtr(vehicle) + hOffsets.fBrakeForce) * controls.BrakeVal;
                ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
            }
        }
        fakeRev(true, 1.0f);
        gearRattle.Play(vehicle);
        float oldEngineHealth = VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle);
        float damageToApply = settings.MisshiftDamage * inputMultiplier;
        if (settings.EngDamage) {
            if (oldEngineHealth >= damageToApply) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    vehicle,
                    oldEngineHealth - damageToApply);
            }
            else {
                VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
            }
        }
        if (settings.DisplayInfo) {
            showText(0.5, 0.80, 0.25, "Eng block @ " + std::to_string(static_cast<int>(inputMultiplier * 100.0f)) + "%");
            showText(0.5, 0.85, 0.25, "Eng block @ " + std::to_string(lockingForce));
        }
    }
    else {
        vehData.EngLockActive = false;
        gearRattle.Stop();
    }
}

void functionEngBrake() {
    // When you let go of the throttle at high RPMs
    float activeBrakeThreshold = settings.EngBrakeThreshold;

    if (vehData.RPM >= activeBrakeThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > 5.0f && !vehData.FakeNeutral) {
        float handlingBrakeForce = *reinterpret_cast<float *>(ext.GetHandlingPtr(vehicle) + hOffsets.fBrakeForce);
        float inpBrakeForce = handlingBrakeForce * controls.BrakeVal;

        float throttleMultiplier = 1.0f - controls.ThrottleVal;
        float clutchMultiplier = 1.0f - controls.ClutchVal;
        float inputMultiplier = throttleMultiplier * clutchMultiplier;
        if (inputMultiplier > 0.0f) {
            vehData.EngBrakeActive = true;
            float rpmMultiplier = (vehData.RPM - activeBrakeThreshold) / (1.0f - activeBrakeThreshold);
            float engBrakeForce = settings.EngBrakePower * handlingBrakeForce * inputMultiplier * rpmMultiplier;
            auto wheelsToBrake = getDrivenWheels();
            for (int i = 0; i < ext.GetNumWheels(vehicle); i++) {
                if (i >= wheelsToBrake.size() || wheelsToBrake[i]) {
                    ext.SetWheelBrakePressure(vehicle, i, engBrakeForce + inpBrakeForce);
                }
                else {
                    ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
                }
            }
            if (settings.DisplayInfo) {
                showText(0.5, 0.55, 0.5, "Eng brake @ " + std::to_string(static_cast<int>(inputMultiplier * 100.0f)) + "%");
                showText(0.5, 0.60, 0.5, "Pressure: " + std::to_string(engBrakeForce));
                showText(0.5, 0.65, 0.5, "Brk. Inp: " + std::to_string(inpBrakeForce));
            }
        }
        
    }
    else {
        vehData.EngBrakeActive = false;
    }
}

void manageBrakePatch() {
    if (vehData.EngBrakeActive || vehData.EngLockActive) {
        if (!MemoryPatcher::BrakeDecrementPatched) {
            MemoryPatcher::PatchBrakeDecrement();
        }
    }
    else {
        if (MemoryPatcher::BrakeDecrementPatched) {
            MemoryPatcher::RestoreBrakeDecrement();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void fakeRev(bool customThrottle, float customThrottleVal) {
    float throttleVal = customThrottle ? customThrottleVal : controls.ThrottleVal;
    float timeStep = SYSTEM::TIMESTEP();
    float accelRatio = 2.5f * timeStep;
    float rpmValTemp = vehData.PrevRPM > vehData.RPM ? vehData.PrevRPM - vehData.RPM : 0.0f;
    if (ext.GetGearCurr(vehicle) == 1) {			// For some reason, first gear revs slower
        rpmValTemp *= 2.0f;
    }
    float rpmVal = vehData.RPM +			// Base value
        rpmValTemp +						// Keep it constant
        throttleVal * accelRatio;	// Addition value, depends on delta T
    //showText(0.4, 0.4, 2.0, "FakeRev");
    ext.SetCurrentRPM(vehicle, rpmVal);
}

void handleRPM() {
    //bool skip = false;

    // Game wants to shift up. Triggered at high RPM, high speed.
    // Desired result: high RPM, same gear, no more accelerating
    // Result:	Is as desired. Speed may drop a bit because of game clutch.
    // Update 2017-08-12: We know the gear speeds now, consider patching
    // shiftUp completely?
    if (ext.GetGearCurr(vehicle) > 0 &&
        (vehData.HitLimiter && ENTITY::GET_ENTITY_SPEED(vehicle) > 2.0f)) {
        ext.SetThrottle(vehicle, 1.0f);
        fakeRev(false, 0);
        CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
        float counterForce = 0.25f*-(static_cast<float>(ext.GetTopGear(vehicle)) - static_cast<float>(ext.GetGearCurr(vehicle)))/static_cast<float>(ext.GetTopGear(vehicle));
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(vehicle, 1, 0.0f, counterForce, 0.0f, true, true, true, true);
        //showText(0.4, 0.1, 1.0, "REV LIM");
        //showText(0.4, 0.15, 1.0, "CF: " + std::to_string(counterForce));
    }

    /*
        Game doesn't rev on disengaged clutch in any gear but 1
        This workaround tries to emulate this
        Default: ext.GetClutch(vehicle) >= 0.6: Normal
        Default: ext.GetClutch(vehicle) < 0.6: Nothing happens
        Fix: Map 0.0-1.0 to 0.6-1.0 (clutchdata)
        Fix: Map 0.0-1.0 to 1.0-0.6 (control)
    */
    if (ext.GetGearCurr(vehicle) > 1) {
        // When pressing clutch and throttle, handle clutch and RPM
        if (controls.ClutchVal > 0.4f && 
            controls.ThrottleVal > 0.05f &&
            !vehData.FakeNeutral && 
            // The next statement is a workaround for rolling back + brake + gear > 1 because it shouldn't rev then.
            // Also because we're checking on the game Control accel value and not the pedal position
            !(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 0.0 && controls.BrakeVal > 0.1f && controls.ThrottleVal > 0.05f)) {
            fakeRev(false, 0);
            ext.SetThrottle(vehicle, controls.ThrottleVal);
        }
        // Don't care about clutch slippage, just handle RPM now
        if (vehData.FakeNeutral) {
            fakeRev(false, 0);
            ext.SetThrottle(vehicle, controls.ThrottleVal);
        }
    }

    float finalClutch = 1.0f - controls.ClutchVal;

    if (vehData.FakeNeutral || controls.ClutchVal >= 1.0f) {
        if (ENTITY::GET_ENTITY_SPEED(vehicle) < 1.0f) {
            finalClutch = -5.0f;
        }
        else {
            finalClutch = -0.5f;
        }
    }

    ext.SetClutch(vehicle, finalClutch);
}

/*
 * Custom limiter thing
 */
void functionLimiter() {
    if (ext.GetGearCurr(vehicle) == ext.GetTopGear(vehicle) || ext.GetGearCurr(vehicle) == 0) {
        vehData.HitLimiter = false;
        return;
    }

    auto ratios = ext.GetGearRatios(vehicle);
    float DriveMaxFlatVel = ext.GetDriveMaxFlatVel(vehicle);
    float maxSpeed = DriveMaxFlatVel / ratios[ext.GetGearCurr(vehicle)];

    if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > maxSpeed) {
        vehData.HitLimiter = true;
    }
    else {
        vehData.HitLimiter = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

// Anti-Deadzone.
void SetControlADZ(eControl control, float value, float adz) {
    CONTROLS::_SET_CONTROL_NORMAL(0, control, sgn(value)*adz+(1.0f-adz)*value);
}

void functionRealReverse() {
    // Forward gear
    // Desired: Only brake
    if (ext.GetGearCurr(vehicle) > 0) {
        // LT behavior when stopped: Just brake
        if (controls.BrakeVal > 0.01f && controls.ThrottleVal < controls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -0.5f) { // < 0.5 so reverse never triggers
                                                                    //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stop");
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetThrottleP(vehicle, 0.1f);
            ext.SetBrakeP(vehicle, 1.0f);
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
        }
        // LT behavior when rolling back: Brake
        if (controls.BrakeVal > 0.01f && controls.ThrottleVal < controls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -0.5f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollback");
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
            ext.SetThrottle(vehicle, 0.0f);
            ext.SetThrottleP(vehicle, 0.1f);
            ext.SetBrakeP(vehicle, 1.0f);
        }
        // RT behavior when rolling back: Burnout
        if (controls.ThrottleVal > 0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -1.0f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
            if (controls.BrakeVal < 0.1f) {
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
            }
        }
    }
    // Reverse gear
    // Desired: RT reverses, LT brakes
    if (ext.GetGearCurr(vehicle) == 0) {
        // Enables reverse lights
        ext.SetThrottleP(vehicle, -0.1f);
        // RT behavior
        int throttleAndSomeBrake = 0;
        if (controls.ThrottleVal > 0.01f && controls.ThrottleVal > controls.BrakeVal) {
            throttleAndSomeBrake++;
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Active Reverse");

            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.ThrottleVal);
        }
        // LT behavior when reversing
        if (controls.BrakeVal > 0.01f &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y <= -0.5f) {
            throttleAndSomeBrake++;
            //showText(0.3, 0.35, 0.5, "functionRealReverse: Brake @ Reverse");

            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
        }
        // Throttle > brake  && BrakeVal > 0.1f
        if (throttleAndSomeBrake >= 2) {
            //showText(0.3, 0.4, 0.5, "functionRealReverse: Weird combo + rev it");

            CONTROLS::ENABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, controls.BrakeVal);
            fakeRev(false, 0);
        }

        // LT behavior when forward
        if (controls.BrakeVal > 0.01f && controls.ThrottleVal <= controls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > 0.1f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, controls.BrakeVal);

            //CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetBrakeP(vehicle, 1.0f);
        }

        // LT behavior when still
        if (controls.BrakeVal > 0.01f && controls.ThrottleVal <= controls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > -0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y <= 0.1f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stopped");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetBrakeP(vehicle, 1.0f);
        }
    }
}

// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)
void handlePedalsRealReverse(float wheelThrottleVal, float wheelBrakeVal) {
    wheelThrottleVal = pow(wheelThrottleVal, settings.ThrottleGamma);
    wheelBrakeVal = pow(wheelBrakeVal, settings.BrakeGamma);

    float speedThreshold = 0.5f;
    const float reverseThreshold = 2.0f;

    if (ext.GetGearCurr(vehicle) > 0) {
        // Going forward
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are going forward");
            // Throttle Pedal normal
            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, controls.ADZThrottle);
            }
            // Brake Pedal normal
            if (wheelBrakeVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelBrakeVal, controls.ADZBrake);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, controls.ADZThrottle);
            }

            if (wheelBrakeVal > 0.01f) {
                ext.SetThrottleP(vehicle, 0.1f);
                ext.SetBrakeP(vehicle, 1.0f);
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            }
        }

        // Rolling back
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -speedThreshold) {
            bool brakelights = false;
            // Just brake
            if (wheelThrottleVal <= 0.01f && wheelBrakeVal > 0.01f) {
                //showText(0.3, 0.0, 1.0, "We should brake");
                //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)));
                SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, controls.ADZBrake);
                ext.SetThrottleP(vehicle, 0.1f);
                ext.SetBrakeP(vehicle, 1.0f);
                brakelights = true;
            }
            
            if (wheelThrottleVal > 0.01f && controls.ClutchVal < settings.ClutchCatchpoint && !vehData.FakeNeutral) {
                //showText(0.3, 0.0, 1.0, "We should burnout");
                SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, controls.ADZThrottle);
                SetControlADZ(ControlVehicleBrake, wheelThrottleVal, controls.ADZThrottle);
            }

            if (wheelThrottleVal > 0.01f && (controls.ClutchVal > settings.ClutchCatchpoint || vehData.FakeNeutral)) {
                if (wheelBrakeVal > 0.01f) {
                    //showText(0.3, 0.0, 1.0, "We should rev and brake");
                    //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)) );
                    SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, controls.ADZBrake);
                    ext.SetThrottleP(vehicle, 0.1f);
                    ext.SetBrakeP(vehicle, 1.0f);
                    brakelights = true;
                    fakeRev(false, 0);
                }
                else if (controls.ClutchVal > 0.9 || vehData.FakeNeutral) {
                    //showText(0.3, 0.0, 1.0, "We should rev and do nothing");
                    ext.SetThrottleP(vehicle, wheelThrottleVal); 
                    fakeRev(false, 0);
                } else {
                    //showText(0.3, 0.0, 1.0, "We should rev and apply throttle");
                    SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, controls.ADZThrottle);
                    ext.SetThrottleP(vehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
            }
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
        }
    }

    if (ext.GetGearCurr(vehicle) == 0) {
        // Enables reverse lights
        ext.SetThrottleP(vehicle, -0.1f);
        
        // We're reversing
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are reversing");
            // Throttle Pedal Reverse
            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelThrottleVal, controls.ADZThrottle);
            }
            // Brake Pedal Reverse
            if (wheelBrakeVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, controls.ADZBrake);
                ext.SetThrottleP(vehicle, -wheelBrakeVal);
                ext.SetBrakeP(vehicle, 1.0f);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelThrottleVal, controls.ADZThrottle);
            }

            if (wheelBrakeVal > 0.01f) {
                ext.SetThrottleP(vehicle, -wheelBrakeVal);
                ext.SetBrakeP(vehicle, 1.0f);
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            }
        }

        // We're rolling forwards
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are rolling forwards");
            //bool brakelights = false;

            if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > reverseThreshold) {
                if (controls.ClutchVal < settings.ClutchCatchpoint) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
                }
                // Brake Pedal Reverse
                if (wheelBrakeVal > 0.01f) {
                    SetControlADZ(ControlVehicleBrake, wheelBrakeVal, controls.ADZBrake);
                    ext.SetThrottleP(vehicle, -wheelBrakeVal);
                    ext.SetBrakeP(vehicle, 1.0f);
                }
            }

            //VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
        }
    }
}

// Pedals behave like RT/LT
void handlePedalsDefault(float wheelThrottleVal, float wheelBrakeVal) {
    wheelThrottleVal = pow(wheelThrottleVal, settings.ThrottleGamma);
    wheelBrakeVal = pow(wheelBrakeVal, settings.BrakeGamma);
    if (wheelThrottleVal > 0.01f) {
        SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, controls.ADZThrottle);
    }
    if (wheelBrakeVal > 0.01f) {
        SetControlADZ(ControlVehicleBrake, wheelBrakeVal, controls.ADZBrake);
    }
}

void functionAutoReverse() {
    // Go forward
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > -1.0f &&
        ext.GetGearCurr(vehicle) == 0) {
        vehData.LockGear = 1;
    }

    // Reverse
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 1.0f &&
        ext.GetGearCurr(vehicle) > 0) {
        vehData.FakeNeutral = false;
        vehData.LockGear = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////
// Blocks some vehicle controls on tapping, tries to activate them when holding.
// TODO: Some original "tap" controls don't work.
void blockButtons() {
    if (settings.BlockCarControls && settings.EnableManual && controls.PrevInput == ScriptControls::Controller &&
        !(settings.ShiftMode == Automatic && ext.GetGearCurr(vehicle) > 1)) {

        if (controls.UseLegacyController) {
            for (int i = 0; i < static_cast<int>(ScriptControls::LegacyControlType::SIZEOF_LegacyControlType); i++) {
                if (controls.ControlNativeBlocks[i] == -1) continue;
                if (i != (int)ScriptControls::LegacyControlType::ShiftUp && 
                    i != (int)ScriptControls::LegacyControlType::ShiftDown) continue;

                if (controls.ButtonHeldOver(static_cast<ScriptControls::LegacyControlType>(i), 200)) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, controls.ControlNativeBlocks[i], 1.0f);
                }
                else {
                    CONTROLS::DISABLE_CONTROL_ACTION(0, controls.ControlNativeBlocks[i], true);
                }

                if (controls.ButtonReleasedAfter(static_cast<ScriptControls::LegacyControlType>(i), 200)) {
                    // todo
                }
            }
            if (controls.ControlNativeBlocks[(int)ScriptControls::LegacyControlType::Clutch] != -1) {
                CONTROLS::DISABLE_CONTROL_ACTION(0, controls.ControlNativeBlocks[(int)ScriptControls::LegacyControlType::Clutch], true);
            }
        }
        else {
            for (int i = 0; i < static_cast<int>(ScriptControls::ControllerControlType::SIZEOF_ControllerControlType); i++) {
                if (controls.ControlXboxBlocks[i] == -1) continue;
                if (i != (int)ScriptControls::ControllerControlType::ShiftUp && 
                    i != (int)ScriptControls::ControllerControlType::ShiftDown) continue;

                if (controls.ButtonHeldOver(static_cast<ScriptControls::ControllerControlType>(i), 200)) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, controls.ControlXboxBlocks[i], 1.0f);
                }
                else {
                    CONTROLS::DISABLE_CONTROL_ACTION(0, controls.ControlXboxBlocks[i], true);
                }

                if (controls.ButtonReleasedAfter(static_cast<ScriptControls::ControllerControlType>(i), 200)) {
                    // todo
                }
            }
            if (controls.ControlXboxBlocks[(int)ScriptControls::ControllerControlType::Clutch] != -1) {
                CONTROLS::DISABLE_CONTROL_ACTION(0, controls.ControlXboxBlocks[(int)ScriptControls::ControllerControlType::Clutch], true);
            }
        }
    }
}

void startStopEngine() {
    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        (controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) ||
        controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::Engine) ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine) ||
        settings.ThrottleStart && controls.ThrottleVal > 0.75f && (controls.ClutchVal > settings.ClutchCatchpoint || vehData.FakeNeutral))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
    }
    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        ((controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::ControllerControlType::Engine) && settings.ToggleEngine) ||
        (controls.PrevInput == ScriptControls::Controller	&& controls.ButtonJustPressed(ScriptControls::LegacyControlType::Engine) && settings.ToggleEngine) ||
        controls.PrevInput == ScriptControls::Keyboard		&& controls.ButtonJustPressed(ScriptControls::KeyboardControlType::Engine) ||
        controls.PrevInput == ScriptControls::Wheel			&& controls.ButtonJustPressed(ScriptControls::WheelControlType::Engine))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
    }
}

void handleVehicleButtons() {
    blockButtons();
    startStopEngine();

    if (!controls.WheelControl.IsConnected(controls.SteerGUID) ||
        controls.PrevInput != ScriptControls::Wheel) {
        return;
    }

    if (controls.HandbrakeVal > 0.1f) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, controls.HandbrakeVal);
    }
    if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
    }
    if (controls.ButtonIn(ScriptControls::WheelControlType::Horn)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
    }
    if (controls.ButtonJustPressed(ScriptControls::WheelControlType::Lights)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
    }
    if (controls.ButtonIn(ScriptControls::WheelControlType::LookBack)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }
    
    // who was first?
    if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight) &&
        controls.ButtonJustPressed(ScriptControls::WheelControlType::LookLeft)) {
        vehData.LookBackRShoulder = true;
    }

    if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft) &&
        controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(vehData.LookBackRShoulder ? -180.0f : 180.0f);
    }
    else if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(90.0f);
    }
    else if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(-90);
    }
    if (controls.ButtonReleased(ScriptControls::WheelControlType::LookLeft) && !(controls.ButtonIn(ScriptControls::WheelControlType::LookRight)) ||
        controls.ButtonReleased(ScriptControls::WheelControlType::LookRight) && !(controls.ButtonIn(ScriptControls::WheelControlType::LookLeft))) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0);
    }
    if (controls.ButtonReleased(ScriptControls::WheelControlType::LookLeft)  ||
        controls.ButtonReleased(ScriptControls::WheelControlType::LookRight)) {
        vehData.LookBackRShoulder = false;
    }

    if (controls.ButtonJustPressed(ScriptControls::WheelControlType::Camera)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
    }


    if (controls.ButtonHeld(ScriptControls::WheelControlType::RadioPrev, 1000) ||
        controls.ButtonHeld(ScriptControls::WheelControlType::RadioNext, 1000)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() != RadioOff) {
            vehData.RadioStationIndex = AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
        }
        AUDIO::SET_VEH_RADIO_STATION(vehicle, "OFF");
        return;
    }
    if (controls.ButtonReleased(ScriptControls::WheelControlType::RadioNext)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(vehData.RadioStationIndex);
            return;
        }
        AUDIO::_0xFF266D1D0EB1195D(); // Next radio station
    }
    if (controls.ButtonReleased(ScriptControls::WheelControlType::RadioPrev)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(vehData.RadioStationIndex);
            return;
        }
        AUDIO::_0xDD6BCF9E94425DF9(); // Prev radio station
    }

    if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorLeft)) {
        if (!vehData.BlinkerLeft) {
            vehData.BlinkerTicks = 1;
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false); // L
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
            vehData.BlinkerLeft = true;
            vehData.BlinkerRight = false;
            vehData.BlinkerHazard = false;
        }
        else {
            vehData.BlinkerTicks = 0;
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
            vehData.BlinkerLeft = false;
            vehData.BlinkerRight = false;
            vehData.BlinkerHazard = false;
        }
    }
    if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorRight)) {
        if (!vehData.BlinkerRight) {
            vehData.BlinkerTicks = 1;
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false); // R
            vehData.BlinkerLeft = false;
            vehData.BlinkerRight = true;
            vehData.BlinkerHazard = false;
        }
        else {
            vehData.BlinkerTicks = 0;
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
            vehData.BlinkerLeft = false;
            vehData.BlinkerRight = false;
            vehData.BlinkerHazard = false;
        }
    }

    float wheelCenterDeviation = ( controls.SteerVal - 0.5f) / 0.5f;

    if (vehData.BlinkerTicks == 1 && abs(wheelCenterDeviation) > 0.2f)
    {
        vehData.BlinkerTicks = 2;
    }

    if (vehData.BlinkerTicks == 2 && abs(wheelCenterDeviation) < 0.1f)
    {
        vehData.BlinkerTicks = 0;
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
        vehData.BlinkerLeft = false;
        vehData.BlinkerRight = false;
        vehData.BlinkerHazard = false;
    }

    if (controls.ButtonJustPressed(ScriptControls::WheelControlType::IndicatorHazard)) {
        if (!vehData.BlinkerHazard) {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true); // L
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true); // R
            vehData.BlinkerLeft = false;
            vehData.BlinkerRight = false;
            vehData.BlinkerHazard = true;
        }
        else {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, false);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, false);
            vehData.BlinkerLeft = false;
            vehData.BlinkerRight = false;
            vehData.BlinkerHazard = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void doWheelSteering() {
    if (controls.PrevInput != ScriptControls::Wheel)
        return;

    float steerMult;
    if (vehData.Class == VehicleClass::Bike || vehData.Class == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.Class == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }

    float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);

    /*
     * Patched steering is direct without any processing, and super direct.
     * _SET_CONTROL_NORMAL is with game processing and could have a bit of delay
     * Both should work without any deadzone, with a note that the second one
     * does need a specified anti-deadzone (recommended: 24-25%)
     * 
     */
    if (vehData.Class == VehicleClass::Car && settings.PatchSteeringControl) {
        ext.SetSteeringInputAngle(vehicle, -effSteer);
    }
    else {
        SetControlADZ(ControlVehicleMoveLeftRight, effSteer, controls.ADZSteer);
    }
}

void doStickControlAir() {
    SetControlADZ(ControlVehicleFlyThrottleUp, controls.PlaneThrottle, 0.25f);
    SetControlADZ(ControlVehicleFlyPitchUpDown, controls.PlanePitch, 0.25f);
    SetControlADZ(ControlVehicleFlyRollLeftRight, controls.PlaneRoll, 0.25f);
    SetControlADZ(ControlVehicleFlyYawLeft, controls.PlaneRudderL, 0.25f);
    SetControlADZ(ControlVehicleFlyYawRight, controls.PlaneRudderR, 0.25f);
}

void playFFBAir() {
    if (!settings.EnableFFB) {
        return;
    }
    // Stick ffb?
}

int calculateDamper(float gain, float wheelsOffGroundRatio) {
    Vector3 accelValsAvg = vehData.GetRelativeAccelerationAverage();
    
    // Just to use floats everywhere here
    float damperMax = static_cast<float>(settings.DamperMax);
    float damperMin = static_cast<float>(settings.DamperMin);
    float damperMinSpeed = static_cast<float>(settings.DamperMinSpeed);

    float absVehicleSpeed = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y);
    float damperFactorSpeed = map(absVehicleSpeed, 0.0f, damperMinSpeed, damperMax, damperMin);
    damperFactorSpeed = fmaxf(damperFactorSpeed, damperMin);
    // already clamped on the upper bound by abs(vel) in map()

    // 2G of deceleration causes addition of damperMin. Heavier feeling steer when braking!
    float damperFactorAccel = map(-accelValsAvg.y / 9.81f, -2.0f, 2.0f, -damperMin, damperMin);
    damperFactorAccel = fmaxf(damperFactorAccel, -damperMin);
    damperFactorAccel = fminf(damperFactorAccel, damperMin);

    float damperForce = damperFactorSpeed + damperFactorAccel;

    damperForce = damperForce * (1.0f - wheelsOffGroundRatio);
    
    if (vehData.Class == VehicleClass::Car || vehData.Class == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            damperForce /= 2.0f;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 1, true)) {
            damperForce /= 2.0f;
        }
    }
    else if (vehData.Class == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            damperForce /= 4.0f;
        }
    }

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
        damperForce *= 2.0f;
    }

    // clamp
    damperForce = fmaxf(damperForce, damperMin / 4.0f);
    damperForce = fminf(damperForce, damperMax * 2.0f);

    damperForce *= gain;

    return static_cast<int>(damperForce);
}

int calculateDetail() {
    // Detail feel / suspension compression based
    float compSpeedTotal = 0.0f;
    auto compSpeed = vehData.GetWheelCompressionSpeeds();

    // More than 2 wheels! Trikes should be ok, etc.
    if (ext.GetNumWheels(vehicle) > 2) {
        // left should pull left, right should pull right
        compSpeedTotal = -compSpeed[0] + compSpeed[1];
    }

    return static_cast<int>(1000.0f * settings.DetailMult * compSpeedTotal);
}

int calculateSoftLock(int totalForce) {
    float steerMult;
    if (vehData.Class == VehicleClass::Bike || vehData.Class == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.Class == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }
    float effSteer = steerMult * 2.0f * (controls.SteerVal - 0.5f);

    if (effSteer > 1.0f) {
        totalForce = static_cast<int>((effSteer - 1.0f) * 100000) + totalForce;
        if (effSteer > 1.1f) {
            totalForce = 10000;
        }
    } else if (effSteer < -1.0f) {
        totalForce = static_cast<int>((-effSteer - 1.0f) * -100000) + totalForce;
        if (effSteer < -1.1f) {
            totalForce = -10000;
        }
    }
    return totalForce;
}

// Despite being scientifically inaccurate, "self-aligning torque" is the best description.
int calculateSat(int defaultGain, float steeringAngle, float wheelsOffGroundRatio, bool isCar) {
    Vector3 velocityWorld = ENTITY::GET_ENTITY_VELOCITY(vehicle);
    Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(vehicle, 1);
    Vector3 travelWorld = velocityWorld + positionWorld;
    Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(vehicle, travelWorld.x, travelWorld.y, travelWorld.z);

    Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle);
    Vector3 turnWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, ENTITY::GET_ENTITY_SPEED(vehicle)*-sin(rotationVelocity.z), ENTITY::GET_ENTITY_SPEED(vehicle)*cos(rotationVelocity.z), 0.0f);
    Vector3 turnRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(vehicle, turnWorld.x, turnWorld.y, turnWorld.z);
    float turnRelativeNormX = (travelRelative.x + turnRelative.x) / 2.0f;
    float turnRelativeNormY = (travelRelative.y + turnRelative.y) / 2.0f;
    Vector3 turnWorldNorm = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, turnRelativeNormX, turnRelativeNormY, 0.0f);

    float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(vehicle)*-sin(steeringAngle);
    float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(vehicle)*cos(steeringAngle);
    Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);
    Vector3 steeringRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(vehicle, steeringWorld.x, steeringWorld.y, steeringWorld.z);

    float setpoint = travelRelative.x;

    // This can be tuned but it feels pretty nice right now with Kp = 1.0, Ki, Kd = 0.0.
    double error = pid.getOutput(steeringRelative.x, setpoint);

    int satForce = static_cast<int>(settings.SATAmpMult * defaultGain * -error);

    // "Reduction" effects - those that affect already calculated things
    bool understeering = false;
    float understeer = 0.0f;
    if (isCar) {
        understeer = sgn(travelRelative.x - steeringAngleRelX) * (turnRelativeNormX - steeringAngleRelX);
        if (steeringAngleRelX > turnRelativeNormX && turnRelativeNormX > travelRelative.x ||
            steeringAngleRelX < turnRelativeNormX && turnRelativeNormX < travelRelative.x) {
            satForce = (int)((float)satForce / std::max(1.0f, understeer + 1.0f));
            understeering = true;
        }
    }

    satForce = (int)((float)satForce * (1.0f - wheelsOffGroundRatio));

    if (vehData.Class == VehicleClass::Car || vehData.Class == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            satForce = satForce / 4;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 1, true)) {
            satForce = satForce / 4;
        }
    }
    else if (vehData.Class == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            satForce = satForce / 10;
        }
    }

    if (settings.DisplayFFBInfo) {
        GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelWorld.x, travelWorld.y, travelWorld.z, 0, 255, 0, 255);
        GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, turnWorldNorm.x, turnWorldNorm.y, turnWorldNorm.z, 255, 0, 0, 255);
        GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
    }
    if (settings.DisplayInfo) {
        showText(0.85, 0.175, 0.4, "RelSteer:\t" + std::to_string(steeringRelative.x), 4);
        showText(0.85, 0.200, 0.4, "SetPoint:\t" + std::to_string(travelRelative.x), 4);
        showText(0.85, 0.225, 0.4, "Error:\t\t" + std::to_string(error), 4);
        showText(0.85, 0.250, 0.4, std::string(understeering ? "~b~" : "~w~") + "Under:\t\t" + std::to_string(understeer) + "~w~", 4);
    }

    return satForce;
}

float getSteeringLock() {
    switch (vehData.Class) {
        case VehicleClass::Bike:
        case VehicleClass::Quad:
        case VehicleClass::Bicycle:
            return settings.SteerAngleBike;
        case VehicleClass::Car:
            return settings.SteerAngleCar;
        case VehicleClass::Boat:
            return settings.SteerAngleBoat;
        case VehicleClass::Plane:
        case VehicleClass::Heli:
        case VehicleClass::Train:
        case VehicleClass::Unknown:
        default: return settings.SteerAngleCar;
    }
}

void playFFBGround() {
    if (!settings.EnableFFB ||
        controls.PrevInput != ScriptControls::Wheel ||
        !controls.WheelControl.IsConnected(controls.SteerGUID)) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        controls.WheelControl.PlayLedsDInput(controls.SteerGUID, vehData.RPM, 0.45, 0.95);
    }

    auto suspensionStates = ext.GetWheelsOnGround(vehicle);
    auto angles = ext.GetWheelSteeringAngles(vehicle);

    // These are discrete numbers, but division is needed so floats!
    float steeredWheels = 0.0f;
    float steeredWheelsFree = 0.0f;

    for (int i = 0; i < ext.GetNumWheels(vehicle); i++) {
        if (angles[i] != 0.0f) {
            steeredWheels += 1.0f;
            if (suspensionStates[i] == false) {
                steeredWheelsFree += 1.0f;
            }
        }
    }
    float wheelsOffGroundRatio = 0.0f;
    if (steeredWheels != 0.0f) {
        wheelsOffGroundRatio = steeredWheelsFree / steeredWheels;
    }

    float steeringAngle;
    if (vehData.Class == VehicleClass::Car || steeredWheels == 0.0f) {
        steeringAngle = ext.GetSteeringAngle(vehicle)*ext.GetSteeringMultiplier(vehicle);
    }
    else {
        float allAngles = 0.0f;
        for (auto angle : angles) {
            if (angle != 0.0f) {
                allAngles += angle;
            }
        }
        steeringAngle = allAngles / steeredWheels;
    }

    int detailForce = calculateDetail();
    int satForce = calculateSat(2500, steeringAngle, wheelsOffGroundRatio, vehData.Class == VehicleClass::Car);
    int damperForce = calculateDamper(50.0f, wheelsOffGroundRatio);

    // Decrease damper if sat rises, so constantForce doesn't fight against damper
    float damperMult = 1.0f - std::min(fabs((float)satForce), 10000.0f) / 10000.0f;
    damperForce = (int)(damperMult * (float)damperForce);

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    totalForce = calculateSoftLock(totalForce);

    auto ffAxis = controls.WheelControl.StringToAxis(controls.WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::ForceFeedback)]);
    controls.WheelControl.SetConstantForce(controls.SteerGUID, ffAxis, totalForce);
    controls.WheelControl.SetDamper(controls.SteerGUID, ffAxis, damperForce);

    float gforce = abs(vehData.GetRelativeAccelerationAverage().y) / 9.81f;
    const float minGforce = 5.0f;
    const float maxGforce = 50.0f;
    if (gforce > minGforce) {
        float res = map(gforce, minGforce, maxGforce, 500.0f, 10000.0f) * settings.CollisionMult;
        controls.WheelControl.SetCollision(controls.SteerGUID, ffAxis, (int)res);
        if (settings.DisplayInfo) {
            std::string info = "Collision @ ~r~" + std::to_string(gforce) + "G~w~~n~";
            std::string info2 = "FFB: " + std::to_string((int)res);
            showNotification(mtPrefix + info + info2, &prevNotification);
        }
    }
    
    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, std::string(abs(satForce) > 10000 ? "~r~" : "~w~") + "FFBSat:\t\t" + std::to_string(satForce) + "~w~", 4);
        showText(0.85, 0.300, 0.4, std::string(abs(totalForce) > 10000 ? "~r~" : "~w~") + "FFBFin:\t\t" + std::to_string(totalForce) + "~w~", 4);
        showText(0.5, 0.05, 0.5, std::string("Damper: ") + std::to_string(damperForce).c_str());
    }
}

void playFFBWater() {
    if (!settings.EnableFFB ||
        controls.PrevInput != ScriptControls::Wheel ||
        !controls.WheelControl.IsConnected(controls.SteerGUID)) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        controls.WheelControl.PlayLedsDInput(controls.SteerGUID, vehData.RPM, 0.45, 0.95);
    }

    auto suspensionStates = ext.GetWheelsOnGround(vehicle);
    auto angles = ext.GetWheelSteeringAngles(vehicle);

    bool isInWater = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle) > 0.10f;
    int damperForce = calculateDamper(50.0f, isInWater ? 0.25f : 1.0f);
    int detailForce = calculateDetail();
    int satForce = calculateSat(750, ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle).z, 1.0f, false);

    if (!isInWater) {
        satForce = 0;
    }

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    totalForce = calculateSoftLock(totalForce);

    auto ffAxis = controls.WheelControl.StringToAxis(controls.WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::ForceFeedback)]);
    controls.WheelControl.SetConstantForce(controls.SteerGUID, ffAxis, totalForce);
    controls.WheelControl.SetDamper(controls.SteerGUID, ffAxis, damperForce);

    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, std::string(abs(satForce) > 10000 ? "~r~" : "~w~") + "FFBSat:\t\t" + std::to_string(satForce) + "~w~", 4);
        showText(0.85, 0.300, 0.4, std::string(abs(totalForce) > 10000 ? "~r~" : "~w~") + "FFBFin:\t\t" + std::to_string(totalForce) + "~w~", 4);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback() {
    if (ext.GetGearCurr(vehicle) == 0) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }
}

void functionAutoGear1() {
    if (ext.GetThrottle(vehicle) < 0.1f && ENTITY::GET_ENTITY_SPEED(vehicle) < 0.1f && ext.GetGearCurr(vehicle) > 1) {
        vehData.LockGear = 1;
    }
}

void functionHillGravity() {
    if (!controls.BrakeVal
        && ENTITY::GET_ENTITY_SPEED(vehicle) < 2.0f &&
        VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
        float pitch = ENTITY::GET_ENTITY_PITCH(vehicle);;

        float clutchNeutral = vehData.FakeNeutral ? 1.0f : controls.ClutchVal;
        if (pitch < 0 || clutchNeutral) {
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
                vehicle, 1, 0.0f, -1 * (pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f, true, true, true, true);
        }
        if (pitch > 10.0f || clutchNeutral)
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            vehicle, 1, 0.0f, -1 * (pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f, true, true, true, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                              Script entry
///////////////////////////////////////////////////////////////////////////////

void registerDecorator(const char *thing, eDecorType type) {
    std::string strType = "?????";
    switch (type) {
    case DECOR_TYPE_FLOAT: strType = "float"; break;
    case DECOR_TYPE_BOOL: strType = "bool"; break;
    case DECOR_TYPE_INT: strType = "int"; break;
    case DECOR_TYPE_UNK: strType = "unknown"; break;
    case DECOR_TYPE_TIME: strType = "time"; break;
    }

    if (!DECORATOR::DECOR_IS_REGISTERED_AS_TYPE((char*)thing, type)) {
        DECORATOR::DECOR_REGISTER((char*)thing, type);
        logger.Write(DEBUG, "DECOR: Registered \"%s\" as %s", thing, strType.c_str());
    }
}

// @Unknown Modder
BYTE* g_bIsDecorRegisterLockedPtr = nullptr;
bool setupGlobals() {
    auto addr = mem::FindPattern("\x40\x53\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x8B\xDA\x75\x29",
                                 "xxxxxxxx????xxxxx");
    if (!addr) {
        logger.Write(ERROR, "Couldn't find pattern for bIsDecorRegisterLockedPtr");
        return false;
    }

    g_bIsDecorRegisterLockedPtr = (BYTE*)(addr + *(int*)(addr + 8) + 13);

    logger.Write(DEBUG, "bIsDecorRegisterLockedPtr @ 0x%p", g_bIsDecorRegisterLockedPtr);

    *g_bIsDecorRegisterLockedPtr = 0;

    // New decorators! :)
    registerDecorator(decorCurrentGear, DECOR_TYPE_INT);
    registerDecorator(decorShiftNotice, DECOR_TYPE_INT);
    registerDecorator(decorFakeNeutral, DECOR_TYPE_INT);
    registerDecorator(decorSetShiftMode, DECOR_TYPE_INT);
    registerDecorator(decorGetShiftMode, DECOR_TYPE_INT);

    *g_bIsDecorRegisterLockedPtr = 1;
    return true;
}

bool isPlayerInVehicle() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();

    if (!ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        !PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        ENTITY::IS_ENTITY_DEAD(playerPed) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) {
        return false;
    }

    vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    if (!vehicle || !ENTITY::DOES_ENTITY_EXIST(vehicle) ||
        playerPed != VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1)) {
        return false;
    }
    return true;
}

std::string getInputDevice() {
    switch (controls.PrevInput) {
        case ScriptControls::Keyboard: return "Keyboard";
        case ScriptControls::Controller: return "Controller";
        case ScriptControls::Wheel: return "Wheel";
        default: return "Unknown";
    }
}

#ifdef _DEBUG
/*
 * 0: __try __except
 * 1: try catch
 */
void cleanup(int type) {
    showNotification(mtPrefix + "Script crashed! Check Gears.log");
    logger.Write(FATAL, "CRASH: Init shutdown: %s", type == 0 ? "__except" : "catch");
    bool successI = MemoryPatcher::RestoreInstructions();
    bool successS = MemoryPatcher::RestoreSteeringCorrection();
    bool successSC = MemoryPatcher::RestoreSteeringControl();
    bool successB = MemoryPatcher::RestoreBrakeDecrement();
    resetSteeringMultiplier();
    if (successI && successS && successSC && successB) {
        logger.Write(FATAL, "CRASH: Shut down script cleanly");
    }
    else {
        if (!successI)
            logger.Write(WARN, "Shut down script with instructions not restored");
        if (!successS)
            logger.Write(WARN, "Shut down script with steer correction not restored");
        if (!successSC)
            logger.Write(WARN, "Shut down script with steer control not restored");
        if (!successB)
            logger.Write(WARN, "Shut down script with brake decrement not restored");
    }
}

//https://blogs.msdn.microsoft.com/zhanli/2010/06/25/structured-exception-handling-seh-and-c-exception-handling/
//https://msdn.microsoft.com/en-us/library/5z4bw5h5.aspx
void trans_func(unsigned int, EXCEPTION_POINTERS*);

class SE_Exception {
private:
    unsigned int nSE;
public:
    SE_Exception() {}
    SE_Exception(unsigned int n) : nSE(n) {}
    ~SE_Exception() {}
    unsigned int getSeNumber() { return nSE; }
};

void trans_func(unsigned int u, EXCEPTION_POINTERS* pExp) {
    logger.Write(FATAL, "Translator function");
    DumpStackTrace(pExp);
    throw SE_Exception();
}

bool cppMain() {
    try {
        update();
        update_menu();
        WAIT(0);
    }
    catch (const std::exception &e) {
        logger.Write(FATAL, "std::exception: %s", e.what());
        cleanup(1);
        return true;
    }
    catch (const int ex) {
        logger.Write(FATAL, "int exception: %d", ex);
        cleanup(1);
        return true;
    }
    catch (const long ex) {
        logger.Write(FATAL, "long exception: %d", ex);
        cleanup(1);
        return true;
    }
    catch (const char * ex) {
        logger.Write(FATAL, "string exception: %d", ex);
        cleanup(1);
        return true;
    }
    catch (const char ex) {
        logger.Write(FATAL, "char exception: %c", ex);
        cleanup(1);
        return true;
    }
    catch(...) {
        auto currExp = std::current_exception();
        cleanup(1);
        return true;
    }
    return false;
}

bool cMain() {
    __try {
        return cppMain();
    }
    __except (DumpStackTrace(GetExceptionInformation())) {
        cleanup(0);
        return true;
    }
}
#endif

void readSettings() {
    settings.Read(&controls);
    if (settings.LogLevel > 4) settings.LogLevel = 1;
    logger.SetMinLevel((LogLevel)settings.LogLevel);

    speedoIndex = static_cast<int>(std::find(speedoTypes.begin(), speedoTypes.end(), settings.Speedo) - speedoTypes.begin());
    if (speedoIndex >= speedoTypes.size()) {
        speedoIndex = 0;
    }
    vehData.FakeNeutral = settings.DefaultNeutral;
    menu.ReadSettings();
    logger.Write(INFO, "Settings read");
}

void main() {
    logger.Write(INFO, "Script started");
    absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + mtDir;
    settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    settingsWheelFile = absoluteModPath + "\\settings_wheel.ini";
    settingsStickFile = absoluteModPath + "\\settings_stick.ini";
    settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    textureWheelFile = absoluteModPath + "\\texture_wheel.png";

    settings.SetFiles(settingsGeneralFile, settingsWheelFile, settingsStickFile);
    menu.RegisterOnMain(std::bind(menuInit));
    menu.RegisterOnExit(std::bind(menuClose));
    menu.SetFiles(settingsMenuFile);
    readSettings();
    ext.initOffsets();

    logger.Write(INFO, "Setting up globals");
    if (!setupGlobals()) {
        logger.Write(ERROR, "Global setup failed!");
    }

    if (!controls.WheelControl.PreInit()) {
        logger.Write(ERROR, "WHEEL: DirectInput failed to initialize");
    }
    initWheel();

    //if (!controls.StickControl.PreInit()) {
    //	logger.Write("STICK: DirectInput failed to initialize");
    //}

    if (FileExists(textureWheelFile)) {
        textureWheelId = createTexture(textureWheelFile.c_str());
    }
    else {
        logger.Write(ERROR, textureWheelFile + " does not exist.");
        textureWheelId = -1;
    }

    logger.Write(DEBUG, "START: Starting with MT:  %s", settings.EnableManual ? "ON" : "OFF");
    logger.Write(INFO, "START: Initialization finished");

    while (true) {
#ifdef _DEBUG
        if (cMain()) return;
#else
        update();
        update_npc();
        update_menu();
        WAIT(0);
#endif
    }
}

void ScriptMain() {
#ifdef _DEBUG
    _set_se_translator(trans_func);
#endif
    srand(GetTickCount());
    main();
}
