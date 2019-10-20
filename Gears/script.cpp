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
#include <fmt/format.h>

#include "ScriptSettings.hpp"
#include "VehicleData.hpp"

#include "Memory/MemoryPatcher.hpp"
#include "Memory/Offsets.hpp"
#include "Memory/VehicleFlags.h"

#include "Input/CarControls.hpp"

#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/MathExt.h"
#include "Util/Files.h"
#include "Util/UIUtils.h"
#include "UpdateChecker.h"
#include "Constants.h"
#include "Compatibility.h"
#include "CustomSteering.h"
#include "WheelInput.h"
#include "ScriptUtils.h"

ReleaseInfo g_releaseInfo;
std::mutex g_releaseInfoMutex;

bool g_notifyUpdate;
std::mutex g_notifyUpdateMutex;

bool g_checkUpdateDone;
std::mutex g_checkUpdateDoneMutex;

int textureWheelId;

NativeMenu::Menu menu;
CarControls carControls;
ScriptSettings settings;

Player player;
Ped playerPed;
Vehicle playerVehicle;
Vehicle lastVehicle;

VehiclePeripherals peripherals;
VehicleGearboxStates gearStates;
WheelPatchStates wheelPatchStates;

VehicleExtensions ext;
VehicleData vehData(ext);

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

void cycleShiftMode();
void shiftTo(int gear, bool autoClutch);
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

void update_player() {
    player = PLAYER::PLAYER_ID();
    playerPed = PLAYER::PLAYER_PED_ID();
}

void update_vehicle() {
    playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);
    // Reset vehicle stats on vehicle change (or leave)
    if (playerVehicle != lastVehicle) {
        peripherals = VehiclePeripherals();
        gearStates = VehicleGearboxStates();
        wheelPatchStates = WheelPatchStates();
        vehData.SetVehicle(playerVehicle); // assign new vehicle;
        functionHidePlayerInFPV(true);
    }
    if (Util::VehicleAvailable(playerVehicle, playerPed)) {
        vehData.Update(); // Update before doing anything else
    }
    if (playerVehicle != lastVehicle && Util::VehicleAvailable(playerVehicle, playerPed)) {
        if (vehData.mGearTop == 1 || vehData.mFlags[1] & FLAG_IS_ELECTRIC)
            gearStates.FakeNeutral = false;
        else
            gearStates.FakeNeutral = settings.GameAssists.DefaultNeutral;
    }

    lastVehicle = playerVehicle;
}

// Read inputs
void update_inputs() {
    updateLastInputDevice();
    carControls.UpdateValues(carControls.PrevInput, false);
}

void update_hud() {
    if (!Util::PlayerAvailable(player, playerPed) || !Util::VehicleAvailable(playerVehicle, playerPed)) {
        return;
    }

    if (settings.Debug.DisplayInfo) {
        drawDebugInfo();
    }
    if (settings.Debug.DisplayWheelInfo) {
        drawVehicleWheelInfo();
    }
    if (settings.HUD.HUD && vehData.mDomain == VehicleDomain::Road &&
        (settings.MTOptions.Enable || settings.HUD.AlwaysHUD)) {
        drawHUD();
    }
    if (settings.HUD.HUD &&
        (vehData.mDomain == VehicleDomain::Road || vehData.mDomain == VehicleDomain::Water) &&
        (carControls.PrevInput == CarControls::Wheel || settings.HUD.Wheel.Always) &&
        settings.HUD.Wheel.Enable && textureWheelId != -1) {
        drawInputWheelInfo();
    }

    if (settings.Debug.DisplayFFBInfo) {
        WheelInput::DrawDebugLines();
    }
}

void wheelControlWater() {
    WheelInput::CheckButtons();
    WheelInput::HandlePedalsArcade(carControls.ThrottleVal, carControls.BrakeVal);
    WheelInput::DoSteering();
    WheelInput::PlayFFBWater();
}

void wheelControlRoad() {
    WheelInput::CheckButtons();
    if (settings.MTOptions.Enable && 
        !(vehData.mClass == VehicleClass::Bike && settings.GameAssists.SimpleBike)) {
        WheelInput::HandlePedals(carControls.ThrottleVal, carControls.BrakeVal);
    }
    else {
        WheelInput::HandlePedalsArcade(carControls.ThrottleVal, carControls.BrakeVal);
    }
    WheelInput::DoSteering();
    if (vehData.mIsAmphibious && ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerVehicle) > 0.10f) {
        WheelInput::PlayFFBWater();
    }
    else {
        WheelInput::PlayFFBGround();
    }
}

// Apply input as controls for selected devices
void update_input_controls() {
    if (!Util::PlayerAvailable(player, playerPed)) {
        stopForceFeedback();
        return;
    }

    blockButtons();
    startStopEngine();

    if (!settings.Wheel.Options.Enable || carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    if (!carControls.WheelAvailable())
        return;

    if (Util::VehicleAvailable(playerVehicle, playerPed)) {
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
    if (!Util::PlayerAvailable(player, playerPed))
        return;

    if (Util::VehicleAvailable(playerVehicle, playerPed)) {
        if (settings.GameAssists.AutoLookBack && vehData.mClass != VehicleClass::Heli) {
            functionAutoLookback();
        }
    }

    functionHidePlayerInFPV(false);
}

// Only when mod is working or writes clutch stuff.
// Called inside manual transmission part!
// Mostly optional stuff.
void update_manual_features() {
    if (settings.GameAssists.HillGravity) {
        functionHillGravity();
    }

    if (carControls.PrevInput != CarControls::InputDevices::Wheel) {
        if (vehData.mClass == VehicleClass::Bike && settings.GameAssists.SimpleBike) {
            functionAutoReverse();
        }
        else {
            functionRealReverse();
        }
    }

    if (settings.MTOptions.EngBrake) {
        functionEngBrake();
    }
    else {
        wheelPatchStates.EngBrakeActive = false;
    }

    // Engine damage: RPM Damage
    if (settings.MTOptions.EngDamage && vehData.mHasClutch) {
        functionEngDamage();
    }

    if (settings.MTOptions.EngLock) {
        functionEngLock();
    }
    else {
        wheelPatchStates.EngLockActive = false;
    }

    if (!gearStates.FakeNeutral &&
        !(settings.GameAssists.SimpleBike && vehData.mClass == VehicleClass::Bike) && vehData.mHasClutch) {
        // Stalling
        if (settings.MTOptions.EngStallH && settings.MTOptions.ShiftMode == EShiftMode::HPattern ||
            settings.MTOptions.EngStallS && settings.MTOptions.ShiftMode == EShiftMode::Sequential) {
            functionEngStall();
        }

        // Simulate "catch point"
        // When the clutch "grabs" and the car starts moving without input
        if (settings.MTOptions.ClutchCreep && VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle)) {
            functionClutchCatch();
        }
    }

    handleBrakePatch();
}

// Manual Transmission part of the mod
void update_manual_transmission() {
    if (!Util::PlayerAvailable(player, playerPed) || !Util::VehicleAvailable(playerVehicle, playerPed)) {
        return;
    }

    //updateSteeringMultiplier();

    if (carControls.ButtonJustPressed(CarControls::KeyboardControlType::Toggle) ||
        carControls.ButtonHeld(CarControls::WheelControlType::Toggle, 500) ||
        carControls.ButtonHeld(CarControls::ControllerControlType::Toggle) ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeld(CarControls::LegacyControlType::Toggle)) {
        toggleManual(!settings.MTOptions.Enable);
        return;
    }

    if (vehData.mDomain != VehicleDomain::Road || !settings.MTOptions.Enable)
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

    switch(settings.MTOptions.ShiftMode) {
        case EShiftMode::Sequential: {
            functionSShift();
            if (settings.GameAssists.AutoGear1) {
                functionAutoGear1();
            }
            break;
        }
        case EShiftMode::HPattern: {
            if (carControls.PrevInput == CarControls::Wheel) {
                functionHShiftWheel();
            }
            if (carControls.PrevInput == CarControls::Keyboard ||
                carControls.PrevInput == CarControls::Wheel && settings.Wheel.Options.HPatternKeyboard) {
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
    ext.SetGearCurr(playerVehicle, gearStates.LockGear);
    ext.SetGearNext(playerVehicle, gearStates.LockGear);
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
    if (settings.MTOptions.Enable == enable)
        return;
    settings.MTOptions.Enable = enable;
    settings.SaveGeneral();
    if (settings.MTOptions.Enable) {
        UI::Notify("MT Enabled");
    }
    else {
        UI::Notify("MT Disabled");
    }
    readSettings();
    initWheel();
    clearPatches();
}

void initWheel() {
    carControls.InitWheel();
    carControls.CheckGUIDs(settings.Wheel.InputDevices.RegisteredGUIDs);
    logger.Write(INFO, "[Wheel] Steering wheel initialization finished");
}

void stopForceFeedback() {
    carControls.StopFFB(settings.Wheel.Options.LogiLEDs);
}

void update_steering() {
    bool isCar = vehData.mClass == VehicleClass::Car;
    bool useWheel = carControls.PrevInput == CarControls::Wheel;
    bool customSteering = settings.CustomSteering.Mode > 0 && !useWheel;

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

    if (Util::VehicleAvailable(playerVehicle, playerPed)) {
        updateSteeringMultiplier();
    }

    if (customSteering) {
        CustomSteering::Update();
    }
}

void updateSteeringMultiplier() {
    float mult;

    if (carControls.PrevInput == CarControls::Wheel) {
        mult = settings.Wheel.Steering.SteerMult;
    }
    else {
        mult = settings.CustomSteering.SteeringMult;
    }
    ext.SetSteeringMultiplier(playerVehicle, mult);
}

void resetSteeringMultiplier() {
    if (playerVehicle != 0) {
        ext.SetSteeringMultiplier(playerVehicle, 1.0f);
    }
}

void updateLastInputDevice() {
    if (!settings.Debug.DisableInputDetect && 
        carControls.PrevInput != carControls.GetLastInputDevice(carControls.PrevInput,settings.Wheel.Options.Enable)) {
        carControls.PrevInput = carControls.GetLastInputDevice(carControls.PrevInput, settings.Wheel.Options.Enable);
        std::string message = "Input: ";
        switch (carControls.PrevInput) {
            case CarControls::Keyboard:
                message += "Keyboard";
                break;
            case CarControls::Controller:
                message += "Controller";
                if (settings.MTOptions.ShiftMode == EShiftMode::HPattern &&
                    settings.Controller.BlockHShift) {
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
        if (Util::VehicleAvailable(playerVehicle, playerPed)) {
            UI::Notify(message);
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

void setShiftMode(EShiftMode shiftMode) {
    if (Util::VehicleAvailable(playerVehicle, playerPed) && shiftMode != EShiftMode::HPattern && vehData.mGearCurr > 1)
        gearStates.FakeNeutral = false;

    if (shiftMode == EShiftMode::HPattern &&
        carControls.PrevInput == CarControls::Controller && 
        settings.Controller.BlockHShift) {
        settings.MTOptions.ShiftMode = EShiftMode::Automatic;
    }
    else {
        settings.MTOptions.ShiftMode = shiftMode;
    }

    std::string mode = "Mode: ";
    // ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
    switch (settings.MTOptions.ShiftMode) {
        case EShiftMode::Sequential:    mode += "Sequential";   break;
        case EShiftMode::HPattern:      mode += "H-Pattern";    break;
        case EShiftMode::Automatic:     mode += "Automatic";    break;
        default:                                                break;
    }
    UI::Notify(mode);
}

void cycleShiftMode() {
    setShiftMode(Next(settings.MTOptions.ShiftMode));
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
    if (settings.MTOptions.ClutchShiftH && vehData.mHasClutch) {
        if (carControls.ClutchVal > 1.0f - settings.MTParams.ClutchThreshold) {
            shiftTo(i, false);
            gearStates.FakeNeutral = false;
        }
        else {
            gearStates.FakeNeutral = true;
            if (settings.MTOptions.EngDamage && vehData.mHasClutch) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    playerVehicle,
                    VEHICLE::GET_VEHICLE_ENGINE_HEALTH(playerVehicle) - settings.MTParams.MisshiftDamage);
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
        if (settings.MTOptions.ClutchShiftH &&
            settings.MTOptions.EngDamage && vehData.mHasClutch) {
            if (carControls.ClutchVal < 1.0 - settings.MTParams.ClutchThreshold) {
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
    if (!gearStates.Shifting)
        return;

    auto handlingPtr = ext.GetHandlingPtr(playerVehicle);
    // This is x Clutch per second? e.g. changerate 2.5 -> clutch fully (dis)engages in 1/2.5 seconds? or whole thing?
    float rateUp = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float rateDown = *reinterpret_cast<float*>(handlingPtr + hOffsets.fClutchChangeRateScaleDownShift);

    float shiftRate = gearStates.ShiftDirection == ShiftDirection::Up ? rateUp : rateDown;
    shiftRate *= settings.ShiftOptions.ClutchRateMult;

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

    if (settings.Controller.IgnoreShiftsUI && isUIActive()) {
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
        if (settings.MTOptions.ClutchShiftS && 
            carControls.ClutchVal < 1.0f - settings.MTParams.ClutchThreshold) {
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
        if (settings.MTOptions.ClutchShiftS &&
            carControls.ClutchVal < 1.0f - settings.MTParams.ClutchThreshold) {
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

    if (settings.Controller.IgnoreShiftsUI && isUIActive()) {
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
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < 5.0f) {
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
    if (carControls.PrevInput == CarControls::Wheel && settings.Wheel.Options.UseShifterForAuto) {
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

    if (carControls.ThrottleVal >= gearStates.ThrottleHang)
        gearStates.ThrottleHang = carControls.ThrottleVal;
    else if (gearStates.ThrottleHang > 0.0f)
        gearStates.ThrottleHang -= GAMEPLAY::GET_FRAME_TIME() * settings.AutoParams.EcoRate;

    if (gearStates.ThrottleHang < 0.0f)
        gearStates.ThrottleHang = 0.0f;

    float currSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    float nextGearMinSpeed = 0.0f; // don't care about top gear
    if (currGear < vehData.mGearTop) {
        nextGearMinSpeed = settings.AutoParams.NextGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[currGear + 1];
    }

    float currGearMinSpeed = settings.AutoParams.CurrGearMinRPM * vehData.mDriveMaxFlatVel / vehData.mGearRatios[currGear];

    float engineLoad = gearStates.ThrottleHang - map(vehData.mRPM, 0.2f, 1.0f, 0.0f, 1.0f);
    gearStates.EngineLoad = engineLoad;
    gearStates.UpshiftLoad = settings.AutoParams.UpshiftLoad;

    bool skidding = false;
    for (auto x : ext.GetWheelSkidSmokeEffect(playerVehicle)) {
        if (abs(x) > 3.5f)
            skidding = true;
    }

    // Shift up.
    if (currGear < vehData.mGearTop) {
        if (engineLoad < settings.AutoParams.UpshiftLoad && currSpeed > nextGearMinSpeed && !skidding) {
            shiftTo(vehData.mGearCurr + 1, true);
            gearStates.FakeNeutral = false;
            gearStates.LastUpshiftTime = GAMEPLAY::GET_GAME_TIMER();
        }
    }

    // Shift down later when ratios are far apart
    float gearRatioRatio = 1.0f;

    if (vehData.mGearTop > 1 && currGear < vehData.mGearTop) {
        float thisGearRatio = vehData.mGearRatios[currGear] / vehData.mGearRatios[currGear + 1];
        gearRatioRatio = thisGearRatio;
    }

    float rateUp = *reinterpret_cast<float*>(vehData.mHandlingPtr + hOffsets.fClutchChangeRateScaleUpShift);
    float upshiftDuration = 1.0f / (rateUp * settings.ShiftOptions.ClutchRateMult);
    bool tpPassed = GAMEPLAY::GET_GAME_TIMER() > gearStates.LastUpshiftTime + static_cast<int>(1000.0f * upshiftDuration * settings.AutoParams.DownshiftTimeoutMult);
    gearStates.DownshiftLoad = settings.AutoParams.DownshiftLoad * gearRatioRatio;

    // Shift down
    if (currGear > 1) {
        if (tpPassed && engineLoad > settings.AutoParams.DownshiftLoad * gearRatioRatio || currSpeed < currGearMinSpeed) {
            shiftTo(currGear - 1, true);
            gearStates.FakeNeutral = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Gearbox features
///////////////////////////////////////////////////////////////////////////////

void functionClutchCatch() {
    // TODO: Make more subtle / use throttle?
    const float idleThrottle = 0.24f; // TODO -> Settings?

    float clutchRatio = map(carControls.ClutchVal, settings.MTParams.ClutchThreshold, 1.0f, 1.0f, 0.0f);
    clutchRatio = std::clamp(clutchRatio, 0.0f, 1.0f);

    // TODO: Check for settings.MTOptions.ShiftMode == Automatic if anybody complains...

    bool clutchEngaged = carControls.ClutchVal < 1.0f - settings.MTParams.ClutchThreshold;
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
                Controls::SetControlADZ(ControlVehicleAccelerate, throttle, 0.25f);
    		else
                Controls::SetControlADZ(ControlVehicleBrake, throttle, 0.25f);
    	}
    }
}

void functionEngStall() {
    const float stallRate = GAMEPLAY::GET_FRAME_TIME() * 3.33f;

    float minSpeed = settings.MTParams.StallingRPM * abs(vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr]);
    float actualSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    // Closer to idle speed = less buildup for stalling
    float speedDiffRatio = map(abs(minSpeed) - abs(actualSpeed), 0.0f, abs(minSpeed), 0.0f, 1.0f);
    speedDiffRatio = std::clamp(speedDiffRatio, 0.0f, 1.0f);
    if (speedDiffRatio < 0.45f)
        speedDiffRatio = 0.0f; //ignore if we're close-ish to idle
    
    bool clutchEngaged = carControls.ClutchVal < 1.0f - settings.MTParams.ClutchThreshold;
    bool stallEngaged = carControls.ClutchVal < 1.0f - settings.MTParams.StallingThreshold;

    // this thing is big when the clutch isnt pressed
    float invClutch = 1.0f - carControls.ClutchVal;

    if (stallEngaged &&
        vehData.mRPM < 0.25f && //engine actually has to idle
        abs(actualSpeed) < abs(minSpeed) &&
        VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle)) {
        float change = invClutch * speedDiffRatio * stallRate;
        gearStates.StallProgress += change;
    }
    else if (gearStates.StallProgress > 0.0f) {
        float change = stallRate; // "subtract" quickly
        gearStates.StallProgress -= change;
    }

    if (gearStates.StallProgress > 1.0f) {
        if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle)) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(playerVehicle, false, true, true);
        }
        gearStates.StallProgress = 0.0f;
    }

    // Simulate push-start
    // We'll just assume the ignition thing is in the "on" position.
    if (actualSpeed > minSpeed && !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle) &&
        stallEngaged) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(playerVehicle, true, true, true);
    }

    //showText(0.1, 0.00, 0.4, fmt("Stall progress: %.02f", gearStates.StallProgress));
    //showText(0.1, 0.02, 0.4, fmt("Clutch: %d", clutchEngaged));
    //showText(0.1, 0.04, 0.4, fmt("Stall?: %d", stallEngaged));
    //showText(0.1, 0.06, 0.4, fmt("SpeedDiffRatio: %.02f", speedDiffRatio));
}

void functionEngDamage() {
    if (settings.MTOptions.ShiftMode == EShiftMode::Automatic ||
        vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC || 
        vehData.mGearTop == 1) {
        return;
    }

    if (vehData.mGearCurr != vehData.mGearTop &&
        vehData.mRPM > 0.98f &&
        carControls.ThrottleVal > 0.98f) {
        VEHICLE::SET_VEHICLE_ENGINE_HEALTH(playerVehicle, 
                                           VEHICLE::GET_VEHICLE_ENGINE_HEALTH(playerVehicle) - (settings.MTParams.RPMDamage));
    }
}

void functionEngLock() {
    if (settings.MTOptions.ShiftMode == EShiftMode::Automatic ||
        vehData.mFlags[1] & eVehicleFlag2::FLAG_IS_ELECTRIC ||
        vehData.mGearTop == 1 || 
        vehData.mGearCurr == vehData.mGearTop ||
        gearStates.FakeNeutral) {
        wheelPatchStates.EngLockActive = false;
        return;
    }
    const float reverseThreshold = 2.0f;

    float dashms = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y);

    float speed = dashms;
    auto ratios = vehData.mGearRatios;
    float DriveMaxFlatVel = vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[vehData.mGearCurr];

    float inputMultiplier = (1.0f - carControls.ClutchVal);
    auto wheelsSpeed = vehData.mWheelAverageDrivenTyreSpeed;

    bool wrongDirection = false;
    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle)) {
        if (vehData.mGearCurr == 0) {
            if (vehData.mVelocity.y > reverseThreshold && wheelsSpeed > reverseThreshold) {
                wrongDirection = true;
            }
        }
        else {
            if (vehData.mVelocity.y < -reverseThreshold && wheelsSpeed < -reverseThreshold) {
                wrongDirection = true;
            }
        }
    }

    // Wheels are locking up due to bad (down)shifts
    if ((speed > abs(maxSpeed * 1.15f) + 3.334f || wrongDirection) && inputMultiplier > settings.MTParams.ClutchThreshold) {
        wheelPatchStates.EngLockActive = true;
        float lockingForce = 60.0f * inputMultiplier;
        auto wheelsToLock = vehData.mWheelsDriven;//getDrivenWheels();

        for (int i = 0; i < vehData.mWheelCount; i++) {
            if (i >= wheelsToLock.size() || wheelsToLock[i]) {
                ext.SetWheelPower(playerVehicle, i, -lockingForce * sgn(vehData.mVelocity.y));
                ext.SetWheelSkidSmokeEffect(playerVehicle, i, lockingForce);
            }
            else {
                float inpBrakeForce = *reinterpret_cast<float *>(vehData.mHandlingPtr + hOffsets.fBrakeForce) * carControls.BrakeVal;
                //ext.SetWheelBrakePressure(vehicle, i, inpBrakeForce);
            }
        }
        fakeRev(true, 1.0f);
        float oldEngineHealth = VEHICLE::GET_VEHICLE_ENGINE_HEALTH(playerVehicle);
        float damageToApply = settings.MTParams.MisshiftDamage * inputMultiplier;
        if (settings.MTOptions.EngDamage) {
            if (oldEngineHealth >= damageToApply) {
                VEHICLE::SET_VEHICLE_ENGINE_HEALTH(
                    playerVehicle,
                    oldEngineHealth - damageToApply);
            }
            else {
                VEHICLE::SET_VEHICLE_ENGINE_ON(playerVehicle, false, true, true);
            }
        }
        if (settings.Debug.DisplayInfo) {
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
    float activeBrakeThreshold = settings.MTParams.EngBrakeThreshold;

    if (vehData.mRPM >= activeBrakeThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > 5.0f && !gearStates.FakeNeutral) {
        float handlingBrakeForce = *reinterpret_cast<float *>(vehData.mHandlingPtr + hOffsets.fBrakeForce);
        float inpBrakeForce = handlingBrakeForce * carControls.BrakeVal;

        float throttleMultiplier = 1.0f - carControls.ThrottleVal;
        float clutchMultiplier = 1.0f - carControls.ClutchVal;
        float inputMultiplier = throttleMultiplier * clutchMultiplier;
        if (inputMultiplier > 0.05f) {
            wheelPatchStates.EngBrakeActive = true;
            float rpmMultiplier = (vehData.mRPM - activeBrakeThreshold) / (1.0f - activeBrakeThreshold);
            float engBrakeForce = settings.MTParams.EngBrakePower * inputMultiplier * rpmMultiplier;
            auto wheelsToBrake = vehData.mWheelsDriven;
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (wheelsToBrake[i]) {
                    ext.SetWheelBrakePressure(playerVehicle, i, inpBrakeForce + engBrakeForce);
                }
                else {
                    ext.SetWheelBrakePressure(playerVehicle, i, inpBrakeForce);
                }
            }
            if (settings.Debug.DisplayInfo) {
                showText(0.85, 0.500, 0.4, fmt::format("EngBrake:\t\t{:.3f}", inputMultiplier), 4);
                showText(0.85, 0.525, 0.4, fmt::format("Pressure:\t\t{:.3f}", engBrakeForce), 4);
                showText(0.85, 0.550, 0.4, fmt::format("BrkInput:\t\t{:.3f}", inpBrakeForce), 4);
            }
        }
        else {
            wheelPatchStates.EngBrakeActive = false;
        }
    }
    else {
        wheelPatchStates.EngBrakeActive = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Gearbox control
///////////////////////////////////////////////////////////////////////////////

void handleBrakePatch() {
    bool useABS = false;
    bool useTCS = false;

    bool ebrk = vehData.mHandbrake;
    bool brn = VEHICLE::IS_VEHICLE_IN_BURNOUT(playerVehicle);
    auto susps = vehData.mSuspensionTravel;
    auto lockUps = vehData.mWheelsLockedUp;
    auto speeds = vehData.mWheelTyreSpeeds;

    {
        bool lockedUp = false;
        auto brakePressures = ext.GetWheelBrakePressure(playerVehicle);
        for (int i = 0; i < vehData.mWheelCount; i++) {
            if (lockUps[i] && susps[i] > 0.0f && brakePressures[i] > 0.0f)
                lockedUp = true;
        }
        if (ebrk || brn)
            lockedUp = false;
        bool absNativePresent = vehData.mHasABS && settings.DriveAssists.ABSFilter;

        if (settings.DriveAssists.CustomABS && lockedUp && !absNativePresent)
            useABS = true;
    }
    
    {
        bool tractionLoss = false;
        if (settings.DriveAssists.TCMode != 0) {
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (speeds[i] > vehData.mVelocity.y + 2.5f && susps[i] > 0.0f && vehData.mWheelsDriven[i])
                    tractionLoss = true;
            }
            if (ebrk || brn)
                tractionLoss = false;
        }

        if (settings.DriveAssists.TCMode != 0 && tractionLoss)
            useTCS = true;
    }


    if (useTCS && settings.DriveAssists.TCMode == 2) {
        CONTROLS::DISABLE_CONTROL_ACTION(2, eControl::ControlVehicleAccelerate, true);
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~(TCS/T)");
    }

    if (wheelPatchStates.InduceBurnout) {
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        if (!MemoryPatcher::ThrottlePatcher.Patched()) {
            MemoryPatcher::PatchThrottle();
        }
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~Burnout");
    }
    else if (wheelPatchStates.EngLockActive) {
        if (!MemoryPatcher::ThrottlePatcher.Patched()) {
            MemoryPatcher::PatchThrottle();
        }
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~EngLock");
    }
    else if (useABS) {
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        for (uint8_t i = 0; i < lockUps.size(); i++) {
            if (lockUps[i])
                ext.SetWheelBrakePressure(playerVehicle, i, 0.0f);
            vehData.mWheelsAbs[i] = true;
        }
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~(ABS)");
    }
    else if (useTCS && settings.DriveAssists.TCMode == 1) {
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        auto pows = ext.GetWheelPower(playerVehicle);
        for (int i = 0; i < vehData.mWheelCount; i++) {
            if (speeds[i] > vehData.mVelocity.y + 2.5f && susps[i] > 0.0f && vehData.mWheelsDriven[i] && pows[i] > 0.1f) {
                ext.SetWheelBrakePressure(playerVehicle, i, map(speeds[i], vehData.mVelocity.y, vehData.mVelocity.y + 2.5f, 0.0f, 0.5f));
                vehData.mWheelsTcs[i] = true;
            }
            else {
                ext.SetWheelBrakePressure(playerVehicle, i, 0.0f);
                vehData.mWheelsTcs[i] = false;
            }
        }
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~(TCS/B)");
    }
    else if (wheelPatchStates.EngBrakeActive) {
        if (!MemoryPatcher::BrakePatcher.Patched()) {
            MemoryPatcher::PatchBrake();
        }
        if (settings.Debug.DisplayInfo)
            showText(0.45, 0.75, 1.0, "~r~EngBrake");
    }
    else {
        if (MemoryPatcher::BrakePatcher.Patched()) {
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (carControls.BrakeVal == 0.0f) {
                    ext.SetWheelBrakePressure(playerVehicle, i, 0.0f);
                }
            }
            MemoryPatcher::RestoreBrake();
        }
        if (MemoryPatcher::ThrottlePatcher.Patched()) {
            MemoryPatcher::RestoreThrottle();
        }
        for (int i = 0; i < vehData.mWheelCount; i++) {
            vehData.mWheelsTcs[i] = false;
            vehData.mWheelsAbs[i] = false;
        }
    }
}

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
    ext.SetCurrentRPM(playerVehicle, rpmVal);
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
                settings.ShiftOptions.UpshiftCut) {
                CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
            }
            else if (settings.ShiftOptions.DownshiftBlip) {
                float expectedRPM = vehData.mWheelAverageDrivenTyreSpeed / (vehData.mDriveMaxFlatVel / vehData.mGearRatios[vehData.mGearCurr - 1]);
                if (vehData.mRPM < expectedRPM * 0.75f)
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, 0.66f);
            }
        }

    }

    // Ignores clutch 
    if (!gearStates.Shifting) {
        if (settings.MTOptions.ShiftMode == EShiftMode::Automatic ||
            vehData.mClass == VehicleClass::Bike && settings.GameAssists.SimpleBike) {
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
        (gearStates.HitRPMSpeedLimiter && ENTITY::GET_ENTITY_SPEED(playerVehicle) > 2.0f)) {
        ext.SetThrottle(playerVehicle, 1.0f);
        fakeRev(false, 1.0f);
        CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleAccelerate, true);
        float counterForce = 0.25f*-(static_cast<float>(vehData.mGearTop) - static_cast<float>(vehData.mGearCurr))/static_cast<float>(vehData.mGearTop);
        ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(playerVehicle, 1, 0.0f, counterForce, 0.0f, true, true, true, true);
        //showText(0.4, 0.1, 1.0, "REV LIM SPD");
        //showText(0.4, 0.15, 1.0, "CF: " + std::to_string(counterForce));
    }
    if (gearStates.HitRPMLimiter) {
        ext.SetCurrentRPM(playerVehicle, 1.0f);
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
        bool rollingback = ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < 0.0 && 
                           carControls.BrakeVal > 0.1f && 
                           carControls.ThrottleVal > 0.05f;

        // When pressing clutch and throttle, handle clutch and RPM
        if (clutch > 0.4f &&
            carControls.ThrottleVal > 0.05f &&
            !gearStates.FakeNeutral && 
            !rollingback &&
            (!gearStates.Shifting || carControls.ClutchVal > 0.4f)) {
            fakeRev(false, 0);
            ext.SetThrottle(playerVehicle, carControls.ThrottleVal);
        }
        // Don't care about clutch slippage, just handle RPM now
        if (gearStates.FakeNeutral) {
            fakeRev(false, 0);
            ext.SetThrottle(playerVehicle, carControls.ThrottleVal);
        }
    }

    if (gearStates.FakeNeutral || clutch >= 1.0f) {
        if (ENTITY::GET_ENTITY_SPEED(playerVehicle) < 1.0f) {
            finalClutch = -5.0f;
        }
        else {
            finalClutch = -0.5f;
        }
    }

    ext.SetClutch(playerVehicle, finalClutch);
}

/*
 * Custom limiter thing
 */
void functionLimiter() {
    gearStates.HitRPMLimiter = vehData.mRPM > 1.0f;

    if (!settings.MTOptions.HardLimiter && 
        (vehData.mGearCurr == vehData.mGearTop || vehData.mGearCurr == 0)) {
        gearStates.HitRPMSpeedLimiter = false;
        return;
    }

    auto ratios = vehData.mGearRatios;
    float DriveMaxFlatVel = vehData.mDriveMaxFlatVel;
    float maxSpeed = DriveMaxFlatVel / ratios[vehData.mGearCurr];

    if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > maxSpeed && vehData.mRPM >= 1.0f) {
        gearStates.HitRPMSpeedLimiter = true;
    }
    else {
        gearStates.HitRPMSpeedLimiter = false;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Control override handling
///////////////////////////////////////////////////////////////////////////////

void functionRealReverse() {
    // Forward gear
    // Desired: Only brake
    if (vehData.mGearCurr > 0) {
        // LT behavior when stopped: Just brake
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal < carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < 0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y >= -0.5f) { // < 0.5 so reverse never triggers
                                                                    //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stop");
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetThrottleP(playerVehicle, 0.1f);
            ext.SetBrakeP(playerVehicle, 1.0f);
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
        }
        // LT behavior when rolling back: Brake
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal < carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < -0.5f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollback");
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleAccelerate, carControls.BrakeVal);
            ext.SetThrottle(playerVehicle, 0.0f);
            ext.SetThrottleP(playerVehicle, 0.1f);
            ext.SetBrakeP(playerVehicle, 1.0f);
        }
        // RT behavior when rolling back: Burnout
        if (!gearStates.FakeNeutral && carControls.ThrottleVal > 0.5f && carControls.ClutchVal < settings.MTParams.ClutchThreshold && 
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < -1.0f ) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Throttle @ Rollback");
            //CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.ThrottleVal);
            if (carControls.BrakeVal < 0.1f) {
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, false);
            }
            for (int i = 0; i < vehData.mWheelCount; i++) {
                if (ext.IsWheelPowered(playerVehicle, i)) {
                    ext.SetWheelBrakePressure(playerVehicle, i, 0.0f);
                    ext.SetWheelPower(playerVehicle, i, 2.0f * ext.GetDriveForce(playerVehicle));
                }
                else {
                    float handlingBrakeForce = *reinterpret_cast<float*>(vehData.mHandlingPtr + hOffsets.fBrakeForce);
                    float inpBrakeForce = handlingBrakeForce * carControls.BrakeVal;
                    ext.SetWheelPower(playerVehicle, i, 0.0f);
                    ext.SetWheelBrakePressure(playerVehicle, i, inpBrakeForce);
                }
            }
            fakeRev();
            ext.SetThrottle(playerVehicle, carControls.ThrottleVal);
            ext.SetThrottleP(playerVehicle, carControls.ThrottleVal);
            wheelPatchStates.InduceBurnout = true;
        }
        else {
            wheelPatchStates.InduceBurnout = false;
        }
    }
    // Reverse gear
    // Desired: RT reverses, LT brakes
    if (vehData.mGearCurr == 0) {
        // Enables reverse lights
        ext.SetThrottleP(playerVehicle, -0.1f);
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
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y <= -0.5f) {
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
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > 0.1f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Rollforwrd");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
            CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleBrake, carControls.BrakeVal);

            //CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetBrakeP(playerVehicle, 1.0f);
        }

        // LT behavior when still
        if (carControls.BrakeVal > 0.01f && carControls.ThrottleVal <= carControls.BrakeVal &&
            ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > -0.5f && ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y <= 0.1f) {
            //showText(0.3, 0.3, 0.5, "functionRealReverse: Brake @ Stopped");

            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
            CONTROLS::DISABLE_CONTROL_ACTION(0, ControlVehicleBrake, true);
            ext.SetBrakeP(playerVehicle, 1.0f);
        }
    }
}

void functionAutoReverse() {
    // Go forward
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > -1.0f &&
        vehData.mGearCurr == 0) {
        shiftTo(1, false);
    }

    // Reverse
    if (CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleBrake) && 
        !CONTROLS::IS_CONTROL_PRESSED(0, ControlVehicleAccelerate) &&
        ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < 1.0f &&
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
    if (!settings.MTOptions.Enable || !settings.Controller.BlockCarControls || vehData.mDomain != VehicleDomain::Road ||
        carControls.PrevInput != CarControls::Controller || !Util::VehicleAvailable(playerVehicle, playerPed)) {
        return;
    }
    if (settings.MTOptions.ShiftMode == EShiftMode::Automatic && vehData.mGearCurr > 1) {
        return;
    }

    if (settings.Controller.Native.Enable) {
        for (int i = 0; i < static_cast<int>(CarControls::LegacyControlType::SIZEOF_LegacyControlType); i++) {
            if (carControls.ControlNativeBlocks[i] == -1) continue;
            if (i != static_cast<int>(CarControls::LegacyControlType::ShiftUp) && 
                i != static_cast<int>(CarControls::LegacyControlType::ShiftDown)) continue;

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
        if (carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] != -1) {
            CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)], true);
        }
    }
    else {
        for (int i = 0; i < static_cast<int>(CarControls::ControllerControlType::SIZEOF_ControllerControlType); i++) {
            if (carControls.ControlXboxBlocks[i] == -1) continue;
            if (i != static_cast<int>(CarControls::ControllerControlType::ShiftUp) && 
                i != static_cast<int>(CarControls::ControllerControlType::ShiftDown)) continue;

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
        if (carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] != -1) {
            CONTROLS::DISABLE_CONTROL_ACTION(0, carControls.ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)], true);
        }
    }
}

// TODO: Proper held values from settings or something
void startStopEngine() {
    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle) &&
        (carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeldOver(CarControls::ControllerControlType::Engine, 200) ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeldOver(CarControls::LegacyControlType::Engine, 200) ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::Engine) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::Engine) ||
        settings.GameAssists.ThrottleStart && carControls.ThrottleVal > 0.75f && (carControls.ClutchVal > settings.MTParams.ClutchThreshold || gearStates.FakeNeutral))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(playerVehicle, true, false, true);
    }
    if (VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle) &&
        (carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeldOver(CarControls::ControllerControlType::Engine, 200) && settings.Controller.ToggleEngine ||
        carControls.PrevInput == CarControls::Controller	&& carControls.ButtonHeldOver(CarControls::LegacyControlType::Engine, 200) && settings.Controller.ToggleEngine ||
        carControls.PrevInput == CarControls::Keyboard		&& carControls.ButtonJustPressed(CarControls::KeyboardControlType::Engine) ||
        carControls.PrevInput == CarControls::Wheel			&& carControls.ButtonJustPressed(CarControls::WheelControlType::Engine))) {
        VEHICLE::SET_VEHICLE_ENGINE_ON(playerVehicle, false, true, true);
    }
}



///////////////////////////////////////////////////////////////////////////////
//                             Misc features
///////////////////////////////////////////////////////////////////////////////

void functionAutoLookback() {
    if (vehData.mGearTop > 0 && vehData.mGearCurr == 0) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }
}

void functionAutoGear1() {
    if (vehData.mThrottle < 0.1f && ENTITY::GET_ENTITY_SPEED(playerVehicle) < 0.1f && vehData.mGearCurr > 1) {
        shiftTo(1, false);
    }
}

void functionHillGravity() {
    // TODO: Needs improvement/proper fix
    if (carControls.BrakeVal == 0.0f
        && ENTITY::GET_ENTITY_SPEED(playerVehicle) < 2.0f &&
        VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(playerVehicle)) {
        float pitch = ENTITY::GET_ENTITY_PITCH(playerVehicle);;

        float clutchNeutral = gearStates.FakeNeutral ? 1.0f : carControls.ClutchVal;
        if (pitch < 0 || clutchNeutral) {
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
                playerVehicle, 1, 0.0f, -1 * (pitch / 150.0f) * 1.1f * clutchNeutral, 0.0f, true, true, true, true);
        }
        if (pitch > 10.0f || clutchNeutral)
            ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(
            playerVehicle, 1, 0.0f, -1 * (pitch / 90.0f) * 0.35f * clutchNeutral, 0.0f, true, true, true, true);
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

    if (settings.Debug.DisablePlayerHide) {
        shouldRun = false;
    }

    if (!shouldRun)
        return;

    bool visible = ENTITY::IS_ENTITY_VISIBLE(playerPed);
    bool shouldHide = false;

    if (settings.GameAssists.HidePlayerInFPV && CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() == 4 && Util::VehicleAvailable(playerVehicle, playerPed)) {
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

void readSettings() {
    settings.Read(&carControls);
    if (settings.Debug.LogLevel > 4) settings.Debug.LogLevel = 1;
    logger.SetMinLevel(static_cast<LogLevel>(settings.Debug.LogLevel));

    gearStates.FakeNeutral = settings.GameAssists.DefaultNeutral;
    menu.ReadSettings();
    logger.Write(INFO, "Settings read");
}

void threadCheckUpdate() {
    std::thread([]() {
        std::lock_guard releaseInfoLock(g_releaseInfoMutex);
        std::lock_guard checkUpdateLock(g_checkUpdateDoneMutex);
        std::lock_guard notifyUpdateLock(g_notifyUpdateMutex);

        bool newAvailable = CheckUpdate(g_releaseInfo);

        if (newAvailable && settings.Update.IgnoredVersion != g_releaseInfo.Version) {
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
                UI::Notify(fmt::format("Manual Transmission: Update available, new version: {}.",
                    g_releaseInfo.Version));
            }
            else if (g_releaseInfo.Version.empty()) {
                UI::Notify("Manual Transmission: Failed to check for update.");
            }
            else {
                UI::Notify(fmt::format("Manual Transmission: No update available, latest version: {}.",
                    g_releaseInfo.Version));
            }
        }
    }
}

void main() {
    logger.Write(INFO, "Script started");
    std::string absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + Constants::ModDir;
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

    if (settings.Update.EnableUpdate) {
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

    logger.Write(DEBUG, "START: Starting with MT:  %s", settings.MTOptions.Enable ? "ON" : "OFF");
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
