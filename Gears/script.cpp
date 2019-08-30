#include "script.h"

#include <string>
#include <algorithm>
#include <thread>
#include <mutex>

#include <inc/natives.h>
#include <inc/enums.h>
#include <inc/main.h>
#include <inc/types.h>

#include <menu.h>
#include <MiniPID/MiniPID.h>
#include <fmt/format.h>

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "ShiftModes.h"

#include "Memory/MemoryPatcher.hpp"
#include "Memory/NativeMemory.hpp"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleFlags.h"

#include "Input/CarControls.hpp"

#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/MathExt.h"
#include "Util/GameSound.h"
#include "Util/Files.h"
#include "Util/UIUtils.h"
#include "Util/MiscEnums.h"
#include "UpdateChecker.h"
#include "Constants.h"
#include "Compatibility.h"

ReleaseInfo g_releaseInfo;
std::mutex g_releaseInfoMutex;

bool g_notifyUpdate;
std::mutex g_notifyUpdateMutex;

bool g_checkUpdateDone;
std::mutex g_checkUpdateDoneMutex;

const std::string mtPrefix = "~b~Manual Transmission~w~~n~";

int textureWheelId;

NativeMenu::Menu menu;
CarControls carControls;
ScriptSettings settings;

Player player;
Ped playerPed;
Vehicle vehicle;
Vehicle lastVehicle;

VehiclePeripherals peripherals;
VehicleGearboxStates gearStates;
WheelPatchStates wheelPatchStates;

VehicleExtensions ext;
VehicleData vehData(ext);

int prevNotification = 0;

MiniPID pid(1.0, 0.0, 0.0);

void SetControlADZ(eControl control, float value, float adz);
void updateShifting();

//TODO: Reorganize/move?
void handleBrakePatch() {
    bool lockedUp = false;
    bool ebrk = vehData.mHandbrake;
    bool brn = VEHICLE::IS_VEHICLE_IN_BURNOUT(vehicle);
    auto lockUps = vehData.mWheelsLockedUp;
    auto susps = vehData.mSuspensionTravel;
    auto brakePressures = ext.GetWheelBrakePressure(vehicle);
    for (int i = 0; i < vehData.mWheelCount; i++) {
        if (lockUps[i] && susps[i] > 0.0f && brakePressures[i] > 0.0f)
            lockedUp = true;
    }
    if (ebrk || brn)
        lockedUp = false;
    bool absNativePresent = vehData.mHasABS && settings.ABSFilter;

    if (settings.CustomABS && lockedUp && !absNativePresent) {
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        for (uint8_t i = 0; i < lockUps.size(); i++) {
            ext.SetWheelBrakePressure(vehicle, i, ext.GetWheelBrakePressure(vehicle)[i] * 0.9f);
        }
        if (settings.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~(ABS)");
    }
    else {
        if (wheelPatchStates.EngBrakeActive || wheelPatchStates.EngLockActive) {
            if (!MemoryPatcher::BrakePatcher.Patched()) {
                MemoryPatcher::PatchBrake();
            }
        }
        else if (wheelPatchStates.InduceBurnout) {
            if (!MemoryPatcher::BrakePatcher.Patched()) {
                MemoryPatcher::PatchBrake();
            }
            if (!MemoryPatcher::ThrottlePatcher.Patched()) {
                MemoryPatcher::PatchThrottle();
            }
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (ext.IsWheelPowered(vehicle, i)) {
                    ext.SetWheelBrakePressure(vehicle, i, 0.0f);
                    ext.SetWheelPower(vehicle, i, 2.0f * ext.GetDriveForce(vehicle));
                }
                else {
                    float handlingBrakeForce = *reinterpret_cast<float *>(vehData.mHandlingPtr + hOffsets.fBrakeForce);
                    float inpBrakeForce = handlingBrakeForce * carControls.BrakeVal;
                    ext.SetWheelPower(vehicle, i, 0.0f);
                    ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
                }

            }
            fakeRev();
            wheelPatchStates.InduceBurnout = false;
        }
        else {
            if (MemoryPatcher::BrakePatcher.Patched()) {
                MemoryPatcher::RestoreBrake();
            }
            if (MemoryPatcher::ThrottlePatcher.Patched()) {
                MemoryPatcher::RestoreThrottle();
            }
        }
    }
}

/*
 * TODO: Some ideas to require rev matching for smooth driving...
 */

void checkRevMatch() {
    float expectedSpeed = vehData.mRPM * (vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr]);
    float actualSpeed = vehData.mWheelAverageDrivenTyreSpeed;
    float speedDelta = expectedSpeed - actualSpeed;
    
    if (speedDelta > 0.0f) {
        // Engine is faster than wheels, wheelspin should occur
        if (vehData.mClutch == 1.0f) {
            // TODO: Implement
        }
        else {
            // TODO: Clutch heating?
        }
    }

    // Engine is slower than wheels, slowdown/blocking should occur
    if (speedDelta < 0.0f) {
        if (vehData.mClutch == 1.0f) {
            // TODO: Implement
        }
        else {
            // TODO: Clutch heating? Engine brake?
        }
    }
}

bool isPlayerAvailable(Player player, Ped playerPed) {
    if (!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE) ||
        CUTSCENE::IS_CUTSCENE_PLAYING() ||
        !ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        ENTITY::IS_ENTITY_DEAD(playerPed)) {
        return false;
    }
    return true;
}

bool isVehicleAvailable(Vehicle vehicle, Ped playerPed) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1);
}

void update_player() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();
}

void update_vehicle() {
    vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
    // Reset vehicle stats on vehicle change (or leave)
    if (vehicle != lastVehicle) {
        peripherals = VehiclePeripherals();
        gearStates = VehicleGearboxStates();
        wheelPatchStates = WheelPatchStates();
        vehData.SetVehicle(vehicle); // assign new vehicle;
    }
    if (isVehicleAvailable(vehicle, playerPed)) {
        vehData.Update(); // Update before doing anything else
    }
    if (vehicle != lastVehicle && isVehicleAvailable(vehicle, playerPed)) {
        if (vehData.mGearTop == 1 || vehData.mFlags[1] & FLAG_IS_ELECTRIC)
            gearStates.FakeNeutral = false;
        else
            gearStates.FakeNeutral = settings.DefaultNeutral;
    }

    lastVehicle = vehicle;
}

// Read inputs
void update_inputs() {
    updateLastInputDevice();
    carControls.UpdateValues(carControls.PrevInput, false);
}

void update_hud() {
    if (!isPlayerAvailable(player, playerPed) || !isVehicleAvailable(vehicle, playerPed)) {
        return;
    }

    if (settings.DisplayInfo) {
        drawDebugInfo();
    }
    if (settings.DisplayWheelInfo) {
        drawVehicleWheelInfo();
    }
    if (settings.HUD && vehData.mDomain == VehicleDomain::Road &&
        (settings.EnableManual || settings.AlwaysHUD)) {
        drawHUD();
    }
    if (settings.HUD &&
        (vehData.mDomain == VehicleDomain::Road || vehData.mDomain == VehicleDomain::Water) &&
        (carControls.PrevInput == CarControls::Wheel || settings.AlwaysSteeringWheelInfo) &&
        settings.SteeringWheelInfo && textureWheelId != -1) {
        drawInputWheelInfo();
    }

    if (settings.DisplayFFBInfo) {
        float angle = getSteeringAngle(vehicle);
        drawSteeringLines(angle);
    }
}

void wheelControlWater() {
    checkWheelButtons();
    handlePedalsDefault(carControls.ThrottleVal, carControls.BrakeVal);
    doWheelSteering();
    playFFBWater();
}

void wheelControlRoad() {
    checkWheelButtons();
    if (settings.EnableManual && 
        !(vehData.mClass == VehicleClass::Bike && settings.SimpleBike)) {
        handlePedalsRealReverse(carControls.ThrottleVal, carControls.BrakeVal);
    }
    else {
        handlePedalsDefault(carControls.ThrottleVal, carControls.BrakeVal);
    }
    doWheelSteering();
    if (vehData.mIsAmphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle) > 0.10f) {
        playFFBWater();
    }
    else {
        playFFBGround();
    }
}

// Apply input as controls for selected devices
void update_input_controls() {
    if (!isPlayerAvailable(player, playerPed)) {
        stopForceFeedback();
        return;
    }

    blockButtons();
    startStopEngine();

    if (!settings.EnableWheel || carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    if (!carControls.WheelAvailable())
        return;

    if (isVehicleAvailable(vehicle, playerPed)) {
        switch (vehData.mDomain) {
        case VehicleDomain::Road: {
            wheelControlRoad();
            break;
        }
        case VehicleDomain::Water: {
            // TODO: Option to disable for water
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
    else {
        stopForceFeedback();
    }
}

// don't write with VehicleExtensions and dont set clutch state
void update_misc_features() {
    if (!isPlayerAvailable(player, playerPed))
        return;

    if (isVehicleAvailable(vehicle, playerPed)) {
        if (settings.AutoLookBack && vehData.mClass != VehicleClass::Heli) {
            functionAutoLookback();
        }
    }

    functionHidePlayerInFPV();
}

// Only when mod is working or writes clutch stuff.
// Called inside manual transmission part!
// Mostly optional stuff.
void update_manual_features() {
    if (settings.HillBrakeWorkaround) {
        functionHillGravity();
    }

    if (carControls.PrevInput != CarControls::InputDevices::Wheel) {
        if (vehData.mClass == VehicleClass::Bike && settings.SimpleBike) {
            functionAutoReverse();
        }
        else {
            functionRealReverse();
        }
    }

    if (settings.EngBrake) {
        functionEngBrake();
    }
    else {
        wheelPatchStates.EngBrakeActive = false;
    }

    // Engine damage: RPM Damage
    if (settings.EngDamage && vehData.mHasClutch) {
        functionEngDamage();
    }

    if (settings.EngLock) {
        functionEngLock();
    }
    else {
        wheelPatchStates.EngLockActive = false;
    }

    // TODO: engLock change to checkRevMatch?

    if (!gearStates.FakeNeutral &&
        !(settings.SimpleBike && vehData.mClass == VehicleClass::Bike) && vehData.mHasClutch) {
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

    handleBrakePatch();
}

// Manual Transmission part of the mod
void update_manual_transmission() {
    if (!isPlayerAvailable(player, playerPed) || !isVehicleAvailable(vehicle, playerPed)) {
        return;
    }

    //updateSteeringMultiplier();

    if (carControls.ButtonJustPressed(CarControls::KeyboardControlType::Toggle) ||
        carControls.ButtonHeld(CarControls::WheelControlType::Toggle, 500) ||
        carControls.ButtonHeld(CarControls::ControllerControlType::Toggle) ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeld(CarControls::LegacyControlType::Toggle)) {
        toggleManual(!settings.EnableManual);
        return;
    }

    if (vehData.mDomain != VehicleDomain::Road || !settings.EnableManual)
        return;

    ///////////////////////////////////////////////////////////////////////////
    //          Active whenever Manual is enabled from here
    ///////////////////////////////////////////////////////////////////////////

    if (carControls.ButtonJustPressed(CarControls::KeyboardControlType::ToggleH) ||
        carControls.ButtonJustPressed(CarControls::WheelControlType::ToggleH) ||
        carControls.ButtonHeld(CarControls::ControllerControlType::ToggleH) ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeld(CarControls::LegacyControlType::ToggleH)) {
        cycleShiftMode();
    }

    if (MemoryPatcher::NumGearboxPatched != MemoryPatcher::NumGearboxPatches) {
        MemoryPatcher::ApplyGearboxPatches();
    }
    update_manual_features();

    switch(settings.ShiftMode) {
        case Sequential: {
            functionSShift();
            if (settings.AutoGear1) {
                functionAutoGear1();
            }
            break;
        }
        case HPattern: {
            if (carControls.PrevInput == CarControls::Wheel) {
                functionHShiftWheel();
            }
            if (carControls.PrevInput == CarControls::Keyboard ||
                carControls.PrevInput == CarControls::Wheel && settings.HPatternKeyboard) {
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

    functionLimiter();

    updateShifting();

    // Finally, update memory each loop
    handleRPM();
    ext.SetGearCurr(vehicle, gearStates.LockGear);
    ext.SetGearNext(vehicle, gearStates.LockGear);
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
}

void toggleManual(bool enable) {
    // Don't need to do anything
    if (settings.EnableManual == enable)
        return;
    settings.EnableManual = enable;
    settings.SaveGeneral();
    std::string message = mtPrefix;
    if (settings.EnableManual) {
        message += "Enabled";
    }
    else {
        message += "Disabled";
    }
    showNotification(message, &prevNotification);
    readSettings();
    initWheel();
    clearPatches();
}

void initWheel() {
    carControls.InitWheel();
    carControls.CheckGUIDs(settings.RegisteredGUIDs);
    logger.Write(INFO, "[Wheel] Steering wheel initialization finished");
}

void stopForceFeedback() {
    carControls.StopFFB(settings.LogiLEDs);
}

float Racer_getSteeringAngle(Vehicle v);
void PlayerRacer_UpdateControl();

void update_steering() {
    bool isCar = vehData.mClass == VehicleClass::Car;
    bool useWheel = carControls.PrevInput == CarControls::Wheel;
    bool customSteering = settings.CustomSteering.Mode > 0;

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

    if (isVehicleAvailable(vehicle, playerPed)) {
        updateSteeringMultiplier();
    }

    if (customSteering) {
        PlayerRacer_UpdateControl();
    }

    if (settings.DisplayInfo) {
        float steeringAngle = Racer_getSteeringAngle(vehicle);

        Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
        Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(vehicle, 1);
        Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, speedVector.x, speedVector.y, speedVector.z);

        float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(vehicle) * -sin(steeringAngle);
        float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(vehicle) * cos(steeringAngle);
        Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);

        //showText(0.3f, 0.15f, 0.5f, fmt::format("Angle: {}", rad2deg(steeringAngle)));

        GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelRelative.x, travelRelative.y, travelRelative.z, 0, 255, 0, 255);
        drawSphere(travelRelative, 0.25f, { 0, 255, 0, 255 });
        GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
        drawSphere(steeringWorld, 0.25f, { 255, 0, 255, 255 });
    }
}

// From CustomSteering TODO Merge into the new CustomSteering thing
// calculateReduction
void updateSteeringMultiplier() {
    float mult = 1.0f;

    if (carControls.PrevInput == CarControls::Wheel) {
        mult = mult * settings.GameSteerMultWheel;
    }
    else {
        mult = mult * settings.CustomSteering.SteeringMult;
    }
    // TODO: Consider not using and moving steering reduction/mult to input side
    ext.SetSteeringMultiplier(vehicle, mult);
}

void resetSteeringMultiplier() {
    if (vehicle != 0) {
        ext.SetSteeringMultiplier(vehicle, 1.0f);
    }
}

void updateLastInputDevice() {
    if (!settings.DisableInputDetect && 
        carControls.PrevInput != carControls.GetLastInputDevice(carControls.PrevInput,settings.EnableWheel)) {
        carControls.PrevInput = carControls.GetLastInputDevice(carControls.PrevInput, settings.EnableWheel);
        std::string message = mtPrefix + "Input: ";
        switch (carControls.PrevInput) {
            case CarControls::Keyboard:
                message += "Keyboard";
                break;
            case CarControls::Controller:
                message += "Controller";
                if (settings.ShiftMode == HPattern && settings.BlockHShift) {
                    message += "~n~Mode: Sequential";
                    setShiftMode(Sequential);
                }
                break;
            case CarControls::Wheel:
                message += "Steering wheel";
                break;
            default: break;
        }
        // Suppress notification when not in car
        if (isVehicleAvailable(vehicle, playerPed)) {
            showNotification(message, &prevNotification);
        }
    }
    if (carControls.PrevInput == CarControls::Wheel) {
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
    if (shiftMode > 2 || shiftMode < 0)
        return;

    if (isVehicleAvailable(vehicle, playerPed) && shiftMode != HPattern && vehData.mGearCurr > 1)
        gearStates.FakeNeutral = false;

    if (shiftMode == HPattern &&
        carControls.PrevInput == CarControls::Controller && 
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
        if (gearStates.Shifting)
            return;
        gearStates.NextGear = gear;
        gearStates.Shifting = true;
        gearStates.ClutchVal = 0.0f;
        gearStates.ShiftDirection = gear > gearStates.LockGear ? ShiftDirection::Up : ShiftDirection::Down;
    }
    else {
        gearStates.LockGear = gear;
    }
}
void functionHShiftTo(int i) {
    if (settings.ClutchShiftingH && vehData.mHasClutch) {
        if (carControls.ClutchVal > 1.0f - settings.ClutchThreshold) {
            shiftTo(i, false);
            gearStates.FakeNeutral = false;
        }
        else {
            gearStates.FakeNeutral = true;
            if (settings.EngDamage && vehData.mHasClutch) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    vehicle,
                    VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - settings.MisshiftDamage);
            }
        }
    }
    else {
        shiftTo(i, false);
        gearStates.FakeNeutral = false;
    }
}

void functionHShiftKeyboard() {
    int clamp = g_numGears - 1;
    if (vehData.mGearTop <= clamp) {
        clamp = vehData.mGearTop;
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (carControls.ButtonJustPressed(static_cast<CarControls::KeyboardControlType>(i))) {
            functionHShiftTo(i);
        }
    }
    if (carControls.ButtonJustPressed(CarControls::KeyboardControlType::HN) && vehData.mHasClutch) {
        gearStates.FakeNeutral = !gearStates.FakeNeutral;
    }
}

bool isHShifterJustNeutral() {
    auto minGear = CarControls::WheelControlType::HR;
    auto topGear = CarControls::WheelControlType::H10;
    for (auto gear = static_cast<uint32_t>(minGear); gear <= static_cast<uint32_t>(topGear); ++gear) {
        if (carControls.ButtonReleased(static_cast<CarControls::WheelControlType>(gear)))
            return true;
    }
    return false;
}

void functionHShiftWheel() {
    int clamp = g_numGears - 1;
    if (vehData.mGearTop <= clamp) {
        clamp = vehData.mGearTop;
    }
    for (uint8_t i = 0; i <= clamp; i++) {
        if (carControls.ButtonJustPressed(static_cast<CarControls::WheelControlType>(i))) {
            functionHShiftTo(i);
        }
    }

    if (isHShifterJustNeutral()) {
        if (settings.ClutchShiftingH &&
            settings.EngDamage && vehData.mHasClutch) {
            if (carControls.ClutchVal < 1.0 - settings.ClutchThreshold) {
            }
        }
        gearStates.FakeNeutral = vehData.mHasClutch;
    }

    if (carControls.ButtonReleased(CarControls::WheelControlType::HR)) {
        shiftTo(1, false);
        gearStates.FakeNeutral = vehData.mHasClutch;
    }
}

bool isUIActive() {
    return PED::IS_PED_RUNNING_MOBILE_PHONE_TASK(playerPed) ||
        menu.IsThisOpen() || TrainerV::Active();
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
    if (settings.DisplayInfo && !menu.IsThisOpen()) {
        showText(0.01, 0.550, 0.3, "Shifting: " + std::string(gearStates.Shifting ? "Y" : "N"));
        showText(0.01, 0.575, 0.3, "Clutch: " + std::to_string(gearStates.ClutchVal));
        showText(0.01, 0.600, 0.3, "Lock: " + std::to_string(gearStates.LockGear));
        showText(0.01, 0.625, 0.3, "Next: " + std::to_string(gearStates.NextGear));
    }

    if (!gearStates.Shifting)
        return;

    auto handlingPtr = ext.GetHandlingPtr(vehicle);
    // This is x Clutch per second? e.g. changerate 2.5 -> clutch fully (dis)engages in 1/2.5 seconds? or whole thing?
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= settings.ClutchRateMult;

    /*
     * 4.0 gives similar perf as base - probably the whole shift takes 1/rate seconds
     * with my extra disengage step, the whole thing should take also 1/rate seconds
     */
    shiftRate = shiftRate * GAMEPLAY::GET_FRAME_TIME() * 4.0f;

    // Something went wrong, abort and just shift to NextGear.
    if (gearStates.ClutchVal > 1.5f) {
        gearStates.ClutchVal = 0.0f;
        gearStates.Shifting = false;
        gearStates.LockGear = gearStates.NextGear;
        return;
    }

    if (gearStates.NextGear != gearStates.LockGear) {
        gearStates.ClutchVal += shiftRate;
    }
    if (gearStates.ClutchVal >= 1.0f && gearStates.LockGear != gearStates.NextGear) {
        gearStates.LockGear = gearStates.NextGear;
        return;
    }
    if (gearStates.NextGear == gearStates.LockGear) {
        gearStates.ClutchVal -= shiftRate;
    }

    if (gearStates.ClutchVal < 0.0f && gearStates.NextGear == gearStates.LockGear) {
        gearStates.ClutchVal = 0.0f;
        gearStates.Shifting = false;
    }
}

void functionSShift() {
    auto xcTapStateUp = carControls.ButtonTapped(CarControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = carControls.ButtonTapped(CarControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = carControls.ButtonTapped(CarControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = carControls.ButtonTapped(CarControls::LegacyControlType::ShiftDown);

    if (settings.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XInputController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = NativeController::TapState::ButtonUp;
    }

    // Shift up
    if (carControls.PrevInput == CarControls::Controller	&& xcTapStateUp == XInputController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Controller	&& ncTapStateUp == NativeController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        if (!vehData.mHasClutch) {
            if (vehData.mGearCurr < vehData.mGearTop) {
                shiftTo(gearStates.LockGear + 1, true);
            }
            gearStates.FakeNeutral = false;
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (settings.ClutchShiftingS && 
            carControls.ClutchVal < 1.0f - settings.ClutchThreshold) {
            return;
        }

        // Reverse to Neutral
        if (vehData.mGearCurr == 0 && !gearStates.FakeNeutral) {
            shiftTo(1, false);
            gearStates.FakeNeutral = true;
            return;
        }

        // Neutral to 1
        if (vehData.mGearCurr == 1 && gearStates.FakeNeutral) {
            gearStates.FakeNeutral = false;
            return;
        }

        // 1 to X
        if (vehData.mGearCurr < vehData.mGearTop) {
            shiftTo(gearStates.LockGear + 1, true);
            gearStates.FakeNeutral = false;
            return;
        }
    }

    // Shift down

    if (carControls.PrevInput == CarControls::Controller	&& xcTapStateDn == XInputController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Controller	&& ncTapStateDn == NativeController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        if (!vehData.mHasClutch) {
            if (vehData.mGearCurr > 0) {
                shiftTo(gearStates.LockGear - 1, false);
                gearStates.FakeNeutral = false;
            }
            return;
        }

        // Shift block /w clutch shifting for seq.
        if (settings.ClutchShiftingS &&
            carControls.ClutchVal < 1.0f - settings.ClutchThreshold) {
            return;
        }

        // 1 to Neutral
        if (vehData.mGearCurr == 1 && !gearStates.FakeNeutral) {
            gearStates.FakeNeutral = true;
            return;
        }

        // Neutral to R
        if (vehData.mGearCurr == 1 && gearStates.FakeNeutral) {
            shiftTo(0, false);
            gearStates.FakeNeutral = false;
            return;
        }

        // X to 1
        if (vehData.mGearCurr > 1) {
            shiftTo(gearStates.LockGear - 1, true);
            gearStates.FakeNeutral = false;
        }
        return;
    }
}

/*
 * Manual part of the automatic transmission (R<->N<->D)
 */
bool subAutoShiftSequential() {
    auto xcTapStateUp = carControls.ButtonTapped(CarControls::ControllerControlType::ShiftUp);
    auto xcTapStateDn = carControls.ButtonTapped(CarControls::ControllerControlType::ShiftDown);

    auto ncTapStateUp = carControls.ButtonTapped(CarControls::LegacyControlType::ShiftUp);
    auto ncTapStateDn = carControls.ButtonTapped(CarControls::LegacyControlType::ShiftDown);

    if (settings.IgnoreShiftsUI && isUIActive()) {
        xcTapStateUp = xcTapStateDn = XInputController::TapState::ButtonUp;
        ncTapStateUp = ncTapStateDn = NativeController::TapState::ButtonUp;
    }

    // Shift up
    if (carControls.PrevInput == CarControls::Controller	&& xcTapStateUp == XInputController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Controller	&& ncTapStateUp == NativeController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftUp) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::ShiftUp)) {
        // Reverse to Neutral
        if (vehData.mGearCurr == 0 && !gearStates.FakeNeutral) {
            shiftTo(1, false);
            gearStates.FakeNeutral = true;
            return true;
        }

        // Neutral to 1
        if (vehData.mGearCurr == 1 && gearStates.FakeNeutral) {
            gearStates.FakeNeutral = false;
            return true;
        }

        // Invalid + N to 1
        if (vehData.mGearCurr == 0 && gearStates.FakeNeutral) {
            shiftTo(1, false);
            gearStates.FakeNeutral = false;
            return true;
        }
    }

    // Shift down
    if (carControls.PrevInput == CarControls::Controller	&& xcTapStateDn == XInputController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Controller	&& ncTapStateDn == NativeController::TapState::Tapped ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::ShiftDown) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::ShiftDown)) {
        // 1 to Neutral
        if (vehData.mGearCurr == 1 && !gearStates.FakeNeutral) {
            gearStates.FakeNeutral = true;
            return true;
        }

        // Neutral to R
        if (vehData.mGearCurr == 1 && gearStates.FakeNeutral) {
            shiftTo(0, false);
            gearStates.FakeNeutral = false;
            return true;
        }

        // Invalid + N to R
        if (vehData.mGearCurr == 0 && gearStates.FakeNeutral) {
            shiftTo(0, false);
            gearStates.FakeNeutral = false;
            return true;
        }
    }
    return false;
}

/*
 * Manual part of the automatic transmission (Direct selection)
 */
bool subAutoShiftSelect() {
    if (carControls.ButtonIn(CarControls::WheelControlType::APark)) {
        if (gearStates.LockGear != 1) {
            shiftTo(1, false);
        }
        gearStates.FakeNeutral = true;
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
        return true;
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::AReverse)) {
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 5.0f) {
            shiftTo(0, false);
            gearStates.FakeNeutral = false;
        }
        return true;
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::ADrive)) {
        shiftTo(1, false);
        gearStates.FakeNeutral = false;
        return true;
    }
    // Unassigned neutral -> pop into neutral when any gear is released
    if (carControls.WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)] == -1) {
        if (carControls.ButtonReleased(CarControls::WheelControlType::APark) ||
            carControls.ButtonReleased(CarControls::WheelControlType::AReverse) ||
            carControls.ButtonReleased(CarControls::WheelControlType::ADrive)) {
            if (gearStates.LockGear != 1) {
                shiftTo(1, false);
            }
            gearStates.FakeNeutral = true;
            return true;
        }
    }
    // Assigned neutral -> handle like any other button.
    else {
        if (carControls.ButtonJustPressed(CarControls::WheelControlType::ANeutral)) {
            if (gearStates.LockGear != 1) {
                shiftTo(1, false);
            }
            gearStates.FakeNeutral = true;
            return true;
        }
    }
    return false;
}

void functionAShift() { 
    // Manual part
    if (carControls.PrevInput == CarControls::Wheel && settings.UseShifterForAuto) {
        if (subAutoShiftSelect()) 
            return;
    }
    else {
        if (subAutoShiftSequential()) 
            return;
    }
    
    int currGear = vehData.mGearCurr;
    if (currGear == 0)
        return;
    if (gearStates.Shifting)
        return;

    if (carControls.ThrottleVal > gearStates.ThrottleHang)
        gearStates.ThrottleHang = carControls.ThrottleVal;
    else if (gearStates.ThrottleHang > 0.0f)
        gearStates.ThrottleHang -= GAMEPLAY::GET_FRAME_TIME() * settings.EcoRate;

    if (gearStates.ThrottleHang < 0.0f)
        gearStates.ThrottleHang = 0.0f;

    float currSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    float nextGearMinSpeed = 0.0f; // don't care about top gear
    if (currGear < vehData.mGearTop) {
        nextGearMinSpeed = settings.NextGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[currGear + 1];
    }

    float currGearMinSpeed = settings.CurrGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[currGear];

    float engineLoad = gearStates.ThrottleHang - map(vehData.mRPM, 0.2f, 1.0f, 0.0f, 1.0f);

    bool skidding = false;
    for (auto x : ext.GetWheelSkidSmokeEffect(vehicle)) {
        if (abs(x) > 3.5f)
            skidding = true;
    }

    // Shift up.
    if (currGear < vehData.mGearTop) {
        if (engineLoad < settings.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding) {
            shiftTo(vehData.mGearCurr + 1, true);
            gearStates.FakeNeutral = false;
            gearStates.LastUpshiftTime = GAMEPLAY::GET_GAME_TIMER();
        }
    }

    // Shift down later when ratios are far apart
    float gearRatioRatio = 1.0f;

    if (vehData.mGearTop > 1) {
        float baseGearRatio = 1.948768f / 3.333333f;
        float thisGearRatio = vehData.mGearRatios[2] / vehData.mGearRatios[1];
        gearRatioRatio = baseGearRatio / thisGearRatio;
        gearRatioRatio = map(gearRatioRatio, 1.0f, 2.0f, 1.0f, 4.0f);
    }

    float rateUp = *reinterpret_cast<float*>(vehData.mHandlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float upshiftDuration = 1.0f / (rateUp * settings.ClutchRateMult);
    bool tpPassed = GAMEPLAY::GET_GAME_TIMER() > gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * settings.DownshiftTimeoutMult);

    // Shift down
    if (currGear > 1) {
        if (tpPassed && engineLoad > settings.DownshiftLoad * gearRatioRatio || currSpeed < currGearMinSpeed) {
            shiftTo(currGear - 1, true);
            gearStates.FakeNeutral = false;
        }
    }

    if (settings.DisplayInfo && !menu.IsThisOpen()) {
        showText(0.01, 0.525, 0.3, fmt::format("Engine load: \t{:.3f}", engineLoad));
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
    const float idleThrottle = 0.24f; // TODO -> Settings?

    float clutchRatio = map(carControls.ClutchVal, settings.ClutchThreshold, 1.0f, 1.0f, 0.0f);
    clutchRatio = std::clamp(clutchRatio, 0.0f, 1.0f);

    // TODO: Check for settings.ShiftMode == Automatic if anybody complains...

    bool clutchEngaged = carControls.ClutchVal < 1.0f - settings.ClutchThreshold;
    float minSpeed = 0.2f * (vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr]);
    float expectedSpeed = vehData.mRPM * (vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr]) * clutchRatio;
    float actualSpeed = vehData.mWheelAverageDrivenTyreSpeed;
    if (abs(actualSpeed) < abs(minSpeed) &&
    	clutchEngaged) {
        float throttle = map(abs(actualSpeed), 0.0f, abs(expectedSpeed), idleThrottle, 0.0f);
    	throttle = std::clamp(throttle, 0.0f, idleThrottle);

    	bool userThrottle = abs(carControls.ThrottleVal) > idleThrottle || abs(carControls.BrakeVal) > idleThrottle;
    	if (!userThrottle) {
    		if (vehData.mGearCurr > 0)
    			SetControlADZ(ControlVehicleAccelerate, throttle, 0.25f);
    		else
    			SetControlADZ(ControlVehicleBrake, throttle, 0.25f);
    	}
    }
}

void functionEngStall() {
    const float stallRate = GAMEPLAY::GET_FRAME_TIME() * 3.33f;

    float minSpeed = settings.StallingRPM * abs(vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr]);
    float actualSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    // Closer to idle speed = less buildup for stalling
    float speedDiffRatio = map(abs(minSpeed) - abs(actualSpeed), 0.0f, abs(minSpeed), 0.0f, 1.0f);
    speedDiffRatio = std::clamp(speedDiffRatio, 0.0f, 1.0f);
    if (speedDiffRatio < 0.45f)
        speedDiffRatio = 0.0f; //ignore if we're close-ish to idle
    
    bool clutchEngaged = carControls.ClutchVal < 1.0f - settings.ClutchThreshold;
    bool stallEngaged = carControls.ClutchVal < 1.0f - settings.StallingThreshold;

    // this thing is big when the clutch isnt pressed
    float invClutch = 1.0f - carControls.ClutchVal;

    if (stallEngaged &&
        vehData.mRPM < 0.25f && //engine actually has to idle
        abs(actualSpeed) < abs(minSpeed) &&
        VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
        float change = invClutch * speedDiffRatio * stallRate;
        gearStates.StallProgress += change;
    }
    else if (gearStates.StallProgress > 0.0f) {
        float change = stallRate; // "subtract" quickly
        gearStates.StallProgress -= change;
    }

    if (gearStates.StallProgress > 1.0f) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
        }
        gearStates.StallProgress = 0.0f;
    }

    // Simulate push-start
    // We'll just assume the ignition thing is in the "on" position.
    if (actualSpeed > minSpeed && !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        stallEngaged) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, true, true);
    }

    //showText(0.1, 0.00, 0.4, fmt("Stall progress: %.02f", gearStates.StallProgress));
    //showText(0.1, 0.02, 0.4, fmt("Clutch: %d", clutchEngaged));
    //showText(0.1, 0.04, 0.4, fmt("Stall?: %d", stallEngaged));
    //showText(0.1, 0.06, 0.4, fmt("SpeedDiffRatio: %.02f", speedDiffRatio));
}

void functionEngDamage() {
    if (settings.ShiftMode == Automatic ||
        vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC || 
        vehData.mGearTop == 1) {
        return;
    }

    if (vehData.mGearCurr != vehData.mGearTop &&
        vehData.mRPM > 0.98f &&
        carControls.ThrottleVal > 0.98f) {
        VEHICLE::SET_VEHICLE_ENGINE_HEALTH(vehicle, 
                                           VEHICLE::GET_VEHICLE_ENGINE_HEALTH(vehicle) - (settings.RPMDamage));
    }
}

void functionEngLock() {
    if (settings.ShiftMode == Automatic ||
        vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC ||
        vehData.mGearTop == 1 || 
        vehData.mGearCurr == vehData.mGearTop ||
        gearStates.FakeNeutral) {
        wheelPatchStates.EngLockActive = false;
        return;
    }
    const float reverseThreshold = 2.0f;

    float dashms = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y);

    float speed = dashms;
    auto ratios = vehData.mGearRatios;
    float DriveMaxFlatVel = vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[vehData.mGearCurr];

    float inputMultiplier = (1.0f - carControls.ClutchVal);
    auto wheelsSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    bool wrongDirection = false;
    if (vehData.mGearCurr == 0 && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
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
    if ((speed > abs(maxSpeed * 1.15f) + 3.334f || wrongDirection) && inputMultiplier > settings.ClutchThreshold) {
        wheelPatchStates.EngLockActive = true;
        float lockingForce = 60.0f * inputMultiplier;
        auto wheelsToLock = vehData.mWheelsDriven;//getDrivenWheels();

        for (int i = 0; i < vehData.mWheelCount; i++) {
            if (i >= wheelsToLock.size() || wheelsToLock[i]) {
                ext.SetWheelBrakePressure(vehicle, i, lockingForce);
                ext.SetWheelSkidSmokeEffect(vehicle, i, lockingForce);
            }
            else {
                float inpBrakeForce = *reinterpret_cast<float *>(vehData.mHandlingPtr + hOffsets.fBrakeForce) * carControls.BrakeVal;
                ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
            }
        }
        fakeRev(true, 1.0f);
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
            showText(0.5, 0.80, 0.25, fmt::format("Eng block: {:.3f}", inputMultiplier));
            showText(0.5, 0.85, 0.25, fmt::format("Eng block force: {:.3f}", lockingForce));
        }
    }
    else {
        wheelPatchStates.EngLockActive = false;
    }
}

void functionEngBrake() {
    // When you let go of the throttle at high RPMs
    float activeBrakeThreshold = settings.EngBrakeThreshold;

    if (vehData.mRPM >= activeBrakeThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > 5.0f && !gearStates.FakeNeutral) {
        float handlingBrakeForce = *reinterpret_cast<float *>(vehData.mHandlingPtr + hOffsets.fBrakeForce);
        float inpBrakeForce = handlingBrakeForce * carControls.BrakeVal;

        float throttleMultiplier = 1.0f - carControls.ThrottleVal;
        float clutchMultiplier = 1.0f - carControls.ClutchVal;
        float inputMultiplier = throttleMultiplier * clutchMultiplier;
        if (inputMultiplier > 0.0f) {
            wheelPatchStates.EngBrakeActive = true;
            float rpmMultiplier = (vehData.mRPM - activeBrakeThreshold) / (1.0f - activeBrakeThreshold);
            float engBrakeForce = settings.EngBrakePower * handlingBrakeForce * inputMultiplier * rpmMultiplier;
            auto wheelsToBrake = vehData.mWheelsDriven;// getDrivenWheels();
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (i >= wheelsToBrake.size() || wheelsToBrake[i]) {
                    ext.SetWheelBrakePressure(vehicle, i, engBrakeForce + inpBrakeForce);
                }
                else {
                    ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
                }
            }
            if (settings.DisplayInfo) {
                showText(0.85, 0.500, 0.4, fmt::format("EngBrake:\t\t{:.3f}", inputMultiplier), 4);
                showText(0.85, 0.525, 0.4, fmt::format("Pressure:\t\t{:.3f}", engBrakeForce), 4);
                showText(0.85, 0.550, 0.4, fmt::format("BrkInput:\t\t{:.3f}", inpBrakeForce), 4);
            }
        }
        
    }
    else {
        wheelPatchStates.EngBrakeActive = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

// TODO: Change ratios for additional param RPM rise speed
void fakeRev(bool customThrottle, float customThrottleVal) {
    float throttleVal = customThrottle ? customThrottleVal : carControls.ThrottleVal;
    float timeStep = GAMEPLAY::GET_FRAME_TIME();
    float accelRatio = 2.5f * timeStep;
    float rpmValTemp = vehData.mRPMPrev > vehData.mRPM ? vehData.mRPMPrev - vehData.mRPM : 0.0f;
    if (vehData.mGearCurr == 1) {			// For some reason, first gear revs slower
        rpmValTemp *= 2.0f;
    }
    float rpmVal = vehData.mRPM +			// Base value
        rpmValTemp +						// Keep it constant
        throttleVal * accelRatio;	// Addition value, depends on delta T
    //showText(0.4, 0.4, 1.0, fmt("FakeRev %d %.02f", customThrottle, rpmVal));
    ext.SetCurrentRPM(vehicle, rpmVal);
}

void handleRPM() {
    float clutch = carControls.ClutchVal;

    // Shifting is only true in Automatic and Sequential mode
    if (gearStates.Shifting) {
        if (gearStates.ClutchVal > clutch)
            clutch = gearStates.ClutchVal;

        // Only lift and blip when no clutch used
        if (carControls.ClutchVal == 0.0f) {
            if (gearStates.ShiftDirection == ShiftDirection::Up &&
                settings.UpshiftCut) {
                CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            }
            else if (settings.DownshiftBlip) {
                float expectedRPM = vehData.mWheelAverageDrivenTyreSpeed / (vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr - 1]);
                if (vehData.mRPM < expectedRPM * 0.75f)
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.66f);
            }
        }

    }

    // Ignores clutch 
    if (!gearStates.Shifting) {
        if (settings.ShiftMode == Automatic ||
            vehData.mClass == VehicleClass::Bike && settings.SimpleBike) {
            clutch = 0.0f;
        }
    }

    //bool skip = false;

    // Game wants to shift up. Triggered at high RPM, high speed.
    // Desired result: high RPM, same gear, no more accelerating
    // Result:	Is as desired. Speed may drop a bit because of game clutch.
    // Update 2017-08-12: We know the gear speeds now, consider patching
    // shiftUp completely?
    if (vehData.mGearCurr > 0 &&
        (gearStates.HitRPMSpeedLimiter && ENTITY::GET_ENTITY_SPEED(vehicle) > 2.0f)) {
        ext.SetThrottle(vehicle, 1.0f);
        fakeRev(false, 1.0f);
        CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
        float counterForce = 0.25f*-(static_cast<float>(vehData.mGearTop) - static_cast<float>(vehData.mGearCurr))/static_cast<float>(vehData.mGearTop);
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(vehicle, 1, 0.0f, counterForce, 0.0f, true, true, true, true);
        //showText(0.4, 0.1, 1.0, "REV LIM SPD");
        //showText(0.4, 0.15, 1.0, "CF: " + std::to_string(counterForce));
    }
    if (gearStates.HitRPMLimiter) {
        ext.SetCurrentRPM(vehicle, 1.0f);
        //showText(0.4, 0.1, 1.0, "REV LIM RPM");
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
    
    if (vehData.mGearCurr > 1) {
        finalClutch = map(clutch, 0.0f, 1.0f, 1.0f, 0.6f);

        // The next statement is a workaround for rolling back + brake + gear > 1 because it shouldn't rev then.
        // Also because we're checking on the game Control accel value and not the pedal position
        bool rollingback = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 0.0 && 
                           carControls.BrakeVal > 0.1f && 
                           carControls.ThrottleVal > 0.05f;

        // When pressing clutch and throttle, handle clutch and RPM
        if (clutch > 0.4f &&
            carControls.ThrottleVal > 0.05f &&
            !gearStates.FakeNeutral && 
            !rollingback &&
            (!gearStates.Shifting || carControls.ClutchVal > 0.4f)) {
            fakeRev(false, 0);
            ext.SetThrottle(vehicle, carControls.ThrottleVal);
        }
        // Don't care about clutch slippage, just handle RPM now
        if (gearStates.FakeNeutral) {
            fakeRev(false, 0);
            ext.SetThrottle(vehicle, carControls.ThrottleVal);
        }
    }

    if (gearStates.FakeNeutral || clutch >= 1.0f) {
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
    gearStates.HitRPMLimiter = vehData.mRPM > 1.0f;

    if (!settings.HardLimiter && 
        (vehData.mGearCurr == vehData.mGearTop || vehData.mGearCurr == 0)) {
        gearStates.HitRPMSpeedLimiter = false;
        return;
    }

    auto ratios = vehData.mGearRatios;
    float DriveMaxFlatVel = vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[vehData.mGearCurr];

    if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > maxSpeed && vehData.mRPM >= 1.0f) {
        gearStates.HitRPMSpeedLimiter = true;
    }
    else {
        gearStates.HitRPMSpeedLimiter = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

#pragma region region_wheel

// Anti-Deadzone.
void SetControlADZ(eControl control, float value, float adz) {
    CONTROLS::_SET_CONTROL_NORMAL(0, control, sgn(value) * adz + (1.0f - adz) * value);
}

void functionRealReverse() {
    // Forward gear
    // Desired: Only brake
    if (vehData.mGearCurr > 0) {
        // LT behavior when stopped: Just brake
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal < carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -0.5f) { // < 0.5 so reverse never triggers
                                                                    //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stop");
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetThrottleP(vehicle, 0.1f);
            ext.SetBrakeP(vehicle, 1.0f);
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
        }
        // LT behavior when rolling back: Brake
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal < carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -0.5f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollback");
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, carControls.BrakeVal);
            ext.SetThrottle(vehicle, 0.0f);
            ext.SetThrottleP(vehicle, 0.1f);
            ext.SetBrakeP(vehicle, 1.0f);
        }
        // RT behavior when rolling back: Burnout
        if (!gearStates.FakeNeutral && carControls.ThrottleVal > 0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -1.0f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
            //CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.ThrottleVal);
            if (carControls.BrakeVal < 0.1f) {
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, false);
            }
            wheelPatchStates.InduceBurnout = true;
        }
    }
    // Reverse gear
    // Desired: RT reverses, LT brakes
    if (vehData.mGearCurr == 0) {
        // Enables reverse lights
        ext.SetThrottleP(vehicle, -0.1f);
        // RT behavior
        int throttleAndSomeBrake = 0;
        if (carControls.ThrottleVal > 0.01f && carControls.ThrottleVal > carControls.BrakeVal) {
            throttleAndSomeBrake++;
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Active Reverse");

            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.ThrottleVal);
        }
        // LT behavior when reversing
        if (carControls.BrakeVal > 0.01f &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y <= -0.5f) {
            throttleAndSomeBrake++;
            //showText(0.3, 0.35, 0.5, "functionRealReverse: Brake @ Reverse");

            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, carControls.BrakeVal);
        }
        // Throttle > brake  && BrakeVal > 0.1f
        if (throttleAndSomeBrake >= 2) {
            //showText(0.3, 0.4, 0.5, "functionRealReverse: Weird combo + rev it");

            CONTROLS::ENABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, carControls.BrakeVal);
            fakeRev(false, 0);
        }

        // LT behavior when forward
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal <= carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > 0.1f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.BrakeVal);

            //CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetBrakeP(vehicle, 1.0f);
        }

        // LT behavior when still
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal <= carControls.BrakeVal &&
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

    if (vehData.mGearCurr > 0) {
        // Going forward
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are going forward");
            // Throttle Pedal normal
            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
            }
            // Brake Pedal normal
            if (wheelBrakeVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
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
                SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                ext.SetThrottleP(vehicle, 0.1f);
                ext.SetBrakeP(vehicle, 1.0f);
                brakelights = true;
            }
            
            if (!gearStates.FakeNeutral && wheelThrottleVal > 0.01f && carControls.ClutchVal < settings.ClutchThreshold && !gearStates.FakeNeutral) {
                //showText(0.3, 0.0, 1.0, "We should burnout");
                //SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
                //SetControlADZ(ControlVehicleBrake, wheelThrottleVal, carControls.ADZThrottle);
                ext.SetThrottle(vehicle, 1.0f);
                ext.SetThrottleP(vehicle, 1.0f);
                wheelPatchStates.InduceBurnout = true;
            }

            if (wheelThrottleVal > 0.01f && (carControls.ClutchVal > settings.ClutchThreshold || gearStates.FakeNeutral)) {
                if (wheelBrakeVal > 0.01f) {
                    //showText(0.3, 0.0, 1.0, "We should rev and brake");
                    //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)) );
                    SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                    ext.SetThrottleP(vehicle, 0.1f);
                    ext.SetBrakeP(vehicle, 1.0f);
                    brakelights = true;
                    fakeRev(false, 0);
                }
                else if (carControls.ClutchVal > 0.9 || gearStates.FakeNeutral) {
                    //showText(0.3, 0.0, 1.0, "We should rev and do nothing");
                    ext.SetThrottleP(vehicle, wheelThrottleVal); 
                    fakeRev(false, 0);
                } else {
                    //showText(0.3, 0.0, 1.0, "We should rev and apply throttle");
                    SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
                    ext.SetThrottleP(vehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
            }
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
        }
    }

    if (vehData.mGearCurr == 0) {
        // Enables reverse lights
        ext.SetThrottleP(vehicle, -0.1f);
        
        // We're reversing
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are reversing");
            // Throttle Pedal Reverse
            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelThrottleVal, carControls.ADZThrottle);
            }
            // Brake Pedal Reverse
            if (wheelBrakeVal > 0.01f) {
                SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                ext.SetThrottleP(vehicle, -wheelBrakeVal);
                ext.SetBrakeP(vehicle, 1.0f);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                SetControlADZ(ControlVehicleBrake, wheelThrottleVal, carControls.ADZThrottle);
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
                if (carControls.ClutchVal < settings.ClutchThreshold) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
                }
                // Brake Pedal Reverse
                if (wheelBrakeVal > 0.01f) {
                    SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
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
        SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
    }
    if (wheelBrakeVal > 0.01f) {
        SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
    }
}

void functionAutoReverse() {
    // Go forward
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y > -1.0f &&
        vehData.mGearCurr == 0) {
        shiftTo(1, false);
    }

    // Reverse
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true).y < 1.0f &&
        vehData.mGearCurr > 0) {
        gearStates.FakeNeutral = false;
        shiftTo(0, false);
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////
// Blocks some vehicle controls on tapping, tries to activate them when holding.
// TODO: Some original "tap" controls don't work.
void blockButtons() {
    if (!settings.EnableManual || !settings.BlockCarControls || vehData.mDomain != VehicleDomain::Road ||
        carControls.PrevInput != CarControls::Controller || !isVehicleAvailable(vehicle, playerPed)) {
        return;
    }
    if (settings.ShiftMode == Automatic && vehData.mGearCurr > 1) {
        return;
    }

    if (carControls.UseLegacyController) {
        for (int i = 0; i < static_cast<int>(CarControls::LegacyControlType::SIZEOF_LegacyControlType); i++) {
            if (carControls.ControlNativeBlocks[i] == -1) continue;
            if (i != (int)CarControls::LegacyControlType::ShiftUp && 
                i != (int)CarControls::LegacyControlType::ShiftDown) continue;

            if (carControls.ButtonHeldOver(static_cast<CarControls::LegacyControlType>(i), 200)) {
                CONTROLS::_SET_CONTROL_NORMAL(0, carControls.ControlNativeBlocks[i], 1.0f);
            }
            else {
                CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlNativeBlocks[i], true);
            }

            if (carControls.ButtonReleasedAfter(static_cast<CarControls::LegacyControlType>(i), 200)) {
                // todo
            }
        }
        if (carControls.ControlNativeBlocks[(int)CarControls::LegacyControlType::Clutch] != -1) {
            CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlNativeBlocks[(int)CarControls::LegacyControlType::Clutch], true);
        }
    }
    else {
        for (int i = 0; i < static_cast<int>(CarControls::ControllerControlType::SIZEOF_ControllerControlType); i++) {
            if (carControls.ControlXboxBlocks[i] == -1) continue;
            if (i != (int)CarControls::ControllerControlType::ShiftUp && 
                i != (int)CarControls::ControllerControlType::ShiftDown) continue;

            if (carControls.ButtonHeldOver(static_cast<CarControls::ControllerControlType>(i), 200)) {
                CONTROLS::_SET_CONTROL_NORMAL(0, carControls.ControlXboxBlocks[i], 1.0f);
            }
            else {
                CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlXboxBlocks[i], true);
            }

            if (carControls.ButtonReleasedAfter(static_cast<CarControls::ControllerControlType>(i), 200)) {
                // todo
            }
        }
        if (carControls.ControlXboxBlocks[(int)CarControls::ControllerControlType::Clutch] != -1) {
            CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlXboxBlocks[(int)CarControls::ControllerControlType::Clutch], true);
        }
    }
}

void startStopEngine() {
    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        (carControls.PrevInput == CarControls::Controller	&& carControls.ButtonJustPressed(CarControls::ControllerControlType::Engine) ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonJustPressed(CarControls::LegacyControlType::Engine) ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::Engine) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::Engine) ||
        settings.ThrottleStart && carControls.ThrottleVal > 0.75f && (carControls.ClutchVal > settings.ClutchThreshold || gearStates.FakeNeutral))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, true, false, true);
    }
    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) &&
        (carControls.PrevInput == CarControls::Controller	&& carControls.ButtonJustPressed(CarControls::ControllerControlType::Engine) && settings.ToggleEngine ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonJustPressed(CarControls::LegacyControlType::Engine) && settings.ToggleEngine ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::Engine) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::Engine))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(vehicle, false, true, true);
    }
}

// Set state, and activate them if hazards are off.
void setBlinkers(bool left, bool right) {
    peripherals.BlinkerLeft = left;
    peripherals.BlinkerRight = right;

    if (!peripherals.BlinkerHazard) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, right);
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, left);
    }
}

void checkIndicatorActions() {
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorHazard)) {
        if (!peripherals.BlinkerHazard) {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, true);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, true);
            peripherals.BlinkerHazard = true;
        }
        else {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 0, peripherals.BlinkerRight);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(vehicle, 1, peripherals.BlinkerLeft);
            peripherals.BlinkerHazard = false;
        }
    }

    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorLeft)) {
        if (!peripherals.BlinkerLeft) {
            peripherals.BlinkerTicks = 1;
            setBlinkers(true, false);
        }
        else {
            peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorRight)) {
        if (!peripherals.BlinkerRight) {
            peripherals.BlinkerTicks = 1;
            setBlinkers(false, true);
        }
        else {
            peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }

    float wheelCenterDeviation = (carControls.SteerVal - 0.5f) / 0.5f;

    if (peripherals.BlinkerTicks == 1 && abs(wheelCenterDeviation) > 0.2f) {
        peripherals.BlinkerTicks = 2;
    }

    if (peripherals.BlinkerTicks == 2 && abs(wheelCenterDeviation) < 0.1f) {
        peripherals.BlinkerTicks = 0;
        setBlinkers(false, false);
    }
}

void checkRadioButtons() {
    if (carControls.ButtonHeld(CarControls::WheelControlType::RadioPrev, 1000) ||
        carControls.ButtonHeld(CarControls::WheelControlType::RadioNext, 1000)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() != RadioOff) {
            peripherals.RadioStationIndex = AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
        }
        AUDIO::SET_VEH_RADIO_STATION(vehicle, "OFF");
        return;
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::RadioNext)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xFF266D1D0EB1195D(); // Next radio station
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::RadioPrev)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xDD6BCF9E94425DF9(); // Prev radio station
    }
}

void checkCameraButtons() {
    if (carControls.ButtonIn(CarControls::WheelControlType::LookBack)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }
    
    // who was first?
    if (carControls.ButtonIn(CarControls::WheelControlType::LookRight) &&
        carControls.ButtonJustPressed(CarControls::WheelControlType::LookLeft)) {
        peripherals.LookBackRShoulder = true;
    }

    if (carControls.ButtonIn(CarControls::WheelControlType::LookLeft) &&
        carControls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(peripherals.LookBackRShoulder ? -180.0f : 180.0f);
    }
    else if (carControls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(90.0f);
    }
    else if (carControls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(-90.0f);
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::LookLeft) && !carControls.ButtonIn(CarControls::WheelControlType::LookRight) ||
        carControls.ButtonReleased(CarControls::WheelControlType::LookRight) && !carControls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f);
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::LookLeft)  ||
        carControls.ButtonReleased(CarControls::WheelControlType::LookRight)) {
        peripherals.LookBackRShoulder = false;
    }

    if (carControls.ButtonJustPressed(CarControls::WheelControlType::Camera)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
    }
}

void checkVehicleInputButtons() {
    if (carControls.HandbrakeVal > 0.1f) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, carControls.HandbrakeVal);
    }
    if (carControls.ButtonIn(CarControls::WheelControlType::Handbrake)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
    }
    if (carControls.ButtonIn(CarControls::WheelControlType::Horn)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::Lights)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
    }
}

void checkWheelButtons() {
    if (carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    checkVehicleInputButtons();
    checkCameraButtons();
    checkRadioButtons();
    checkIndicatorActions();
}

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

#pragma region region_ffb

void doWheelSteering() {
    if (carControls.PrevInput != CarControls::Wheel)
        return;

    float steerMult;
    if (vehData.mClass == VehicleClass::Bike || vehData.mClass == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.mClass == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }

    float steerValL = map(carControls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
    float steerValR = map(carControls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
    float steerValGammaL = pow(steerValL, settings.SteerGamma);
    float steerValGammaR = pow(steerValR, settings.SteerGamma);
    float steerValGamma = carControls.SteerVal < 0.5f ? -steerValGammaL : steerValGammaR;
    float effSteer = steerMult * steerValGamma;

    /*
     * Patched steering is direct without any processing, and super direct.
     * _SET_CONTROL_NORMAL is with game processing and could have a bit of assist
     * Both should work without any deadzone, with a note that the second one
     * does need a specified anti-deadzone (recommended: 24-25%)
     */
    if (vehData.mClass == VehicleClass::Car) {
        ext.SetSteeringInputAngle(vehicle, -effSteer);
    }
    else {
        SetControlADZ(ControlVehicleMoveLeftRight, effSteer, carControls.ADZSteer);
    }
}

int calculateDamper(float gain, float wheelsOffGroundRatio) {
    Vector3 accelValsAvg = vehData.mAcceleration; //TODO: Avg/dampen later
    
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
    
    if (vehData.mClass == VehicleClass::Car || vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            damperForce /= 2.0f;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 1, true)) {
            damperForce /= 2.0f;
        }
    }
    else if (vehData.mClass == VehicleClass::Bike) {
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
    auto compSpeed = vehData.mSuspensionTravelSpeeds;

    // More than 2 wheels! Trikes should be ok, etc.
    if (compSpeed.size() > 2) {
        // left should pull left, right should pull right
        compSpeedTotal = -compSpeed[0] + compSpeed[1];
    }

    return static_cast<int>(1000.0f * settings.DetailMult * compSpeedTotal);
}

void calculateSoftLock(int &totalForce) {
    float steerMult;
    if (vehData.mClass == VehicleClass::Bike || vehData.mClass == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.mClass == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }
    float effSteer = steerMult * 2.0f * (carControls.SteerVal - 0.5f);
    if (effSteer > 1.0f) {
        totalForce = (int)map(effSteer, 1.0f, steerMult, (float)totalForce, 40000.0f);
    } else if (effSteer < -1.0f) {
        totalForce = (int)map(effSteer, -1.0f, -steerMult, (float)totalForce, -40000.0f);
    }
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
    //float turnRelativeNormY = (travelRelative.y + turnRelative.y) / 2.0f;
    //Vector3 turnWorldNorm = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(vehicle, turnRelativeNormX, turnRelativeNormY, 0.0f);

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
            satForce = (int)((float)satForce / std::max(1.0f, 3.3f * understeer + 1.0f));
            understeering = true;
        }
    }

    satForce = (int)((float)satForce * (1.0f - wheelsOffGroundRatio));

    if (vehData.mClass == VehicleClass::Car || vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            satForce = satForce / 4;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 1, true)) {
            satForce = satForce / 4;
        }
    }
    else if (vehData.mClass == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(vehicle, 0, true)) {
            satForce = satForce / 10;
        }
    }

    if (settings.DisplayInfo) {
        showText(0.85, 0.175, 0.4, fmt::format("RelSteer:\t{:.3f}", steeringRelative.x), 4);
        showText(0.85, 0.200, 0.4, fmt::format("SetPoint:\t{:.3f}", travelRelative.x), 4);
        showText(0.85, 0.225, 0.4, fmt::format("Error:\t\t{:.3f}" , error), 4);
        showText(0.85, 0.250, 0.4, fmt::format("{}Under:\t\t{:.3f}~w~", understeering ? "~b~" : "~w~", understeer), 4);
    }

    return satForce;
}

// Despite being scientifically inaccurate, "self-aligning torque" is the best description.
void drawSteeringLines(float steeringAngle) {
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

    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelWorld.x, travelWorld.y, travelWorld.z, 0, 255, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, turnWorldNorm.x, turnWorldNorm.y, turnWorldNorm.z, 255, 0, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
}

float getSteeringAngle(Vehicle v) {
    auto angles = vehData.mWheelSteeringAngles;
    float wheelsSteered = 0.0f;
    float avgAngle = 0.0f;

    for (int i = 0; i < vehData.mWheelCount; i++) {
        if (i < 3 && angles[i] != 0.0f) {
            wheelsSteered += 1.0f;
            avgAngle += angles[i];
        }
    }

    if (wheelsSteered > 0.5f && wheelsSteered < 2.5f) { // bikes, cars, quads
        avgAngle /= wheelsSteered;
    }
    else {
        avgAngle = vehData.mSteeringAngle * vehData.mSteeringMult; // tank, forklift
    }
    return avgAngle;
}



float getFloatingSteeredWheelsRatio(Vehicle v) {
    auto suspensionStates = vehData.mWheelsOnGround;
    auto angles = vehData.mWheelSteeringAngles;

    float wheelsOffGroundRatio = 0.0f;
    float wheelsInAir = 0.0f;
    float wheelsSteered = 0.0f;

    for (int i = 0; i < vehData.mWheelCount; i++) {
        if (angles[i] != 0.0f) {
            wheelsSteered += 1.0f;
            if (suspensionStates[i] == false) {
                wheelsInAir += 1.0f;
            }
        }
    }
    if (wheelsSteered != 0.0f) {
        wheelsOffGroundRatio = wheelsInAir / wheelsSteered;
    }
    return wheelsOffGroundRatio;
}

float getSteeringLock() {
    switch (vehData.mClass) {
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
        carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        carControls.PlayLEDs(vehData.mRPM, 0.45f, 0.95f);
    }

    float avgAngle = getSteeringAngle(vehicle);
    float wheelsOffGroundRatio = getFloatingSteeredWheelsRatio(vehicle);

    int detailForce = calculateDetail();
    int satForce = calculateSat(2500, avgAngle, wheelsOffGroundRatio, vehData.mClass == VehicleClass::Car);
    int damperForce = calculateDamper(50.0f, wheelsOffGroundRatio);

    // Decrease damper if sat rises, so constantForce doesn't fight against damper
    float damperMult = 1.0f - std::min(fabs((float)satForce), 10000.0f) / 10000.0f;
    damperForce = (int)(damperMult * (float)damperForce);

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce);
    carControls.PlayFFBDynamics(totalForce, damperForce);

    const float minGforce = 5.0f;
    const float maxGforce = 50.0f;
    const float minForce = 500.0f;
    const float maxForce = 10000.0f;
    float gForce = abs(vehData.mAcceleration.y) / 9.81f; // TODO: Average/dampen later
    bool collision = gForce > minGforce;
    int res = static_cast<int>(map(gForce, minGforce, maxGforce, minForce, maxForce) * settings.CollisionMult);
    if (collision) {
        carControls.PlayFFBCollision(res);
    }

    if (settings.DisplayInfo && collision) {
        showNotification(fmt::format("{}Collision @ ~r~{:.3f}G~w~~n~"
                             "FFB: {}",mtPrefix, gForce, res), &prevNotification);
    }
    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > 10000 ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t\t{}", damperForce), 4);
        showText(0.85, 0.350, 0.4, fmt::format("Detail:\t\t{}", detailForce), 4);
    }
}

void playFFBWater() {
    if (!settings.EnableFFB ||
        carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        carControls.PlayLEDs(vehData.mRPM, 0.45f, 0.95f);
    }

    auto suspensionStates = vehData.mWheelsOnGround;
    auto angles = vehData.mWheelSteeringAngles;

    bool isInWater = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle) > 0.10f;
    int damperForce = calculateDamper(50.0f, isInWater ? 0.25f : 1.0f);
    int detailForce = calculateDetail();
    int satForce = calculateSat(750, ENTITY::GET_ENTITY_ROTATION_VELOCITY(vehicle).z, 1.0f, false);

    if (!isInWater) {
        satForce = 0;
    }

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce);
    carControls.PlayFFBDynamics(totalForce, damperForce);

    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > 10000 ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t{}", damperForce), 4);
    }
}

#pragma endregion region_ffb

#pragma endregion region_wheel

///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback() {
    if (vehData.mGearTop > 0 && vehData.mGearCurr == 0) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }
}

void functionAutoGear1() {
    if (vehData.mThrottle < 0.1f && ENTITY::GET_ENTITY_SPEED(vehicle) < 0.1f && vehData.mGearCurr > 1) {
        shiftTo(1, false);
    }
}

void functionHillGravity() {
    if (!carControls.BrakeVal
        && ENTITY::GET_ENTITY_SPEED(vehicle) < 2.0f &&
        VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle)) {
        float pitch = ENTITY::GET_ENTITY_PITCH(vehicle);;

        float clutchNeutral = gearStates.FakeNeutral ? 1.0f : carControls.ClutchVal;
        if (pitch < 0 || clutchNeutral) {
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
                vehicle, 1, 0.0f, -1 * (pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f, true, true, true, true);
        }
        if (pitch > 10.0f || clutchNeutral)
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            vehicle, 1, 0.0f, -1 * (pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f, true, true, true, true);
    }
}

void functionHidePlayerInFPV() {
    bool visible = ENTITY::IS_ENTITY_VISIBLE(playerPed);
    bool shouldHide = false;

    if (settings.HidePlayerInFPV && CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() == 4 && isVehicleAvailable(vehicle, playerPed)) {
        shouldHide = true;
    }

    if (visible && shouldHide) {
        ENTITY::SET_ENTITY_VISIBLE(playerPed, false, false);
    }
    if (!visible && !shouldHide) {
        ENTITY::SET_ENTITY_VISIBLE(playerPed, true, false);
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
bool setupGlobals() {
    auto addr = mem::FindPattern("\x40\x53\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x8B\xDA\x75\x29",
                                 "xxxxxxxx????xxxxx");
    if (!addr) {
        logger.Write(ERROR, "Couldn't find pattern for bIsDecorRegisterLockedPtr");
        return false;
    }

    BYTE* g_bIsDecorRegisterLockedPtr = 
        reinterpret_cast<BYTE*>(addr + *reinterpret_cast<int*>(addr + 8) + 13);

    logger.Write(DEBUG, "bIsDecorRegisterLockedPtr @ 0x%p", g_bIsDecorRegisterLockedPtr);

    bool scriptDidUnlock = false;

    // only unlock if not unlocked yet
    if (*g_bIsDecorRegisterLockedPtr == 1) {
        *g_bIsDecorRegisterLockedPtr = 0;
        scriptDidUnlock = true;
    }

    //// New decorators! :)
    //registerDecorator(decorCurrentGear, DECOR_TYPE_INT);
    //registerDecorator(decorShiftNotice, DECOR_TYPE_INT);
    //registerDecorator(decorFakeNeutral, DECOR_TYPE_INT);
    //registerDecorator(decorSetShiftMode, DECOR_TYPE_INT);
    //registerDecorator(decorGetShiftMode, DECOR_TYPE_INT);

    //// Camera mods interoperability
    //registerDecorator(decorLookingLeft, DECOR_TYPE_BOOL);
    //registerDecorator(decorLookingRight, DECOR_TYPE_BOOL);
    //registerDecorator(decorLookingBack, DECOR_TYPE_BOOL);

    // only re-lock if it was locked before
    if (scriptDidUnlock) {
        *g_bIsDecorRegisterLockedPtr = 1;
    }
    return true;
}

void readSettings() {
    settings.Read(&carControls);
    if (settings.LogLevel > 4) settings.LogLevel = 1;
    logger.SetMinLevel(static_cast<LogLevel>(settings.LogLevel));

    gearStates.FakeNeutral = settings.DefaultNeutral;
    menu.ReadSettings();
    logger.Write(INFO, "Settings read");
}

void threadCheckUpdate() {
    std::thread([]() {
        std::lock_guard releaseInfoLock(g_releaseInfoMutex);
        std::lock_guard checkUpdateLock(g_checkUpdateDoneMutex);
        std::lock_guard notifyUpdateLock(g_notifyUpdateMutex);

        bool newAvailable = CheckUpdate(g_releaseInfo);

        if (newAvailable && settings.IgnoredVersion != g_releaseInfo.Version) {
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
                showNotification(fmt::format("Manual Transmission: Update available, new version: {}.",
                    g_releaseInfo.Version));
            }
            else if (g_releaseInfo.Version.empty()) {
                showNotification("Manual Transmission: Failed to check for update.");
            }
            else {
                showNotification(fmt::format("Manual Transmission: No update available, latest version: {}.",
                    g_releaseInfo.Version));
            }
        }
    }
}

// TODO: Tidy up these Racer:: things
float Racer_getSteeringAngle(Vehicle v) {
    float largestAngle = 0.0f;
    auto angles = ext.GetWheelSteeringAngles(v);

    for (auto angle : angles) {
        if (abs(angle) > abs(largestAngle)) {
            largestAngle = angle;
        }
    }
    return largestAngle;
}

// TODO: Move to control class or something
float mSteerPrev = 0.0f;

float Racer_calculateReduction() {
    Vehicle mVehicle = vehicle;
    float mult = 1;
    Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(mVehicle);
    Vector3 pos = ENTITY::GET_ENTITY_COORDS(mVehicle, 1);
    Vector3 motion = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(mVehicle, pos.x + vel.x, pos.y + vel.y,
        pos.z + vel.z);
    if (motion.y > 3) {
        mult = 0.15f + powf(0.9f, abs(motion.y) - 7.2f);
        if (mult != 0) { mult = floorf(mult * 1000) / 1000; }
        if (mult > 1) { mult = 1; }
    }
    mult = (1 + (mult - 1) * settings.CustomSteering.SteeringReduction);
    return mult;
}

// TODO: Re-use for steering wheel!
// Returns in radians
float Racer_calculateDesiredHeading(float steeringMax, float desiredHeading, float reduction) {
    // Scale input with both reduction and steering limit
    desiredHeading = desiredHeading * reduction * steeringMax;
    float correction = desiredHeading;

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(vehicle, true);
    if (abs(speedVector.y) > 3.0f) {
        Vector3 target = Normalize(speedVector);
        float travelDir = atan2(target.y, target.x) - static_cast<float>(M_PI) / 2.0f;
        if (travelDir > static_cast<float>(M_PI) / 2.0f) {
            travelDir -= static_cast<float>(M_PI);
        }
        if (travelDir < -static_cast<float>(M_PI) / 2.0f) {
            travelDir += static_cast<float>(M_PI);
        }
        // Correct for reverse
        travelDir *= sgn(speedVector.y);

        // Custom user multiplier. Division by steeringmult needed to neutralize over-correction.
        travelDir *= settings.CustomSteering.CountersteerMult / settings.CustomSteering.SteeringMult;

        // clamp auto correction to countersteer limit
        travelDir = std::clamp(travelDir, deg2rad(-settings.CustomSteering.CountersteerLimit), deg2rad(settings.CustomSteering.CountersteerLimit));

        correction = travelDir + desiredHeading;
    }

    return std::clamp(correction, -steeringMax, steeringMax);
}

void PlayerRacer_UpdateControl() {
    if (!isVehicleAvailable(vehicle, playerPed))
        return;

    float limitRadians = ext.GetMaxSteeringAngle(vehicle);
    float reduction = Racer_calculateReduction();

    float steer = -CONTROLS::GET_DISABLED_CONTROL_NORMAL(1, ControlMoveLeftRight);

    float steerCurr;

    float steerValGammaL = pow(-steer, settings.SteerGamma);
    float steerValGammaR = pow(steer, settings.SteerGamma);
    float steerValGamma = steer < 0.0f ? -steerValGammaL : steerValGammaR;

    // TODO: Other approach to smoothing input. Config transform speed.
    if (steer == 0.0f) {
        steerCurr = lerp(
            mSteerPrev,
            steerValGamma,
            1.0f - pow(0.000001f, GAMEPLAY::GET_FRAME_TIME()));
    }
    else {
        steerCurr = lerp(
            mSteerPrev,
            steerValGamma,
            1.0f - pow(0.0001f, GAMEPLAY::GET_FRAME_TIME()));
    }
    mSteerPrev = steerCurr;

    float desiredHeading = Racer_calculateDesiredHeading(limitRadians, steerCurr, reduction);

    if (vehData.mClass == VehicleClass::Car) {
        // TODO: Deluxo and more funky cars?
        if (VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle) && (ENTITY::GET_ENTITY_MODEL(vehicle) == GAMEPLAY::GET_HASH_KEY("­TOWTRUCK2") || ENTITY::GET_ENTITY_MODEL(vehicle) == GAMEPLAY::GET_HASH_KEY("­TOWTRUCK"))) {
            // noop
        }
        else {
            CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveUpDown, true);
        }
    }
    // TODO: Re-enable for airborne && deluxo
    // Disable steering and aircrobatics controls

    // Note: This breaks mouse steering, but is needed to get rid of wonky physics...
    CONTROLS::DISABLE_CONTROL_ACTION(1, ControlVehicleMoveLeftRight, true);

    // Both need to be set, top one with radian limit
    ext.SetSteeringAngle(vehicle,       desiredHeading);
    ext.SetSteeringInputAngle(vehicle,  desiredHeading * (1.0f / limitRadians));
}

void main() {
    logger.Write(INFO, "Script started");
    std::string absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + mtDir;
    std::string settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    std::string settingsWheelFile = absoluteModPath + "\\settings_wheel.ini";
    std::string settingsStickFile = absoluteModPath + "\\settings_stick.ini";
    std::string settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    std::string textureWheelFile = absoluteModPath + "\\texture_wheel.png";

    settings.SetFiles(settingsGeneralFile, settingsWheelFile);
    menu.RegisterOnMain([] { onMenuInit(); });
    menu.RegisterOnExit([] { onMenuClose(); });
    menu.SetFiles(settingsMenuFile);
    menu.Initialize();
    readSettings();

    if (settings.EnableUpdate) {
        threadCheckUpdate();
    }

    ext.initOffsets();
    if (!MemoryPatcher::Test()) {
        logger.Write(ERROR, "Patchability test failed!");
        MemoryPatcher::Error = true;
    }

    setupCompatibility();

    initWheel();

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
        update_player();
        update_vehicle();
        update_inputs();
        update_steering();
        update_hud();
        update_input_controls();
        update_manual_transmission();
        update_misc_features();
        update_menu();
        update_update_notification();
        WAIT(0);
    }
}

void ScriptMain() {
    srand(GetTickCount());
    main();
}
