#include "ScriptSettings.hpp"

#include <string>
#include <fmt/format.h>
#include <simpleini/SimpleIni.h>

#include "Util/Logger.hpp"
#include "Util/Util.hpp"
#include "Input/keyboard.h"
#include "Input/CarControls.hpp"
#include "VehicleConfig.h"

// TODO: Settings shouldn't *do* anything, other stuff just needs to take stuff from this.

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[Settings] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
    }

namespace {
    ScriptSettings localSettings;
    VehicleConfig* activeConfig = nullptr;
}
EShiftMode Next(EShiftMode mode) {
    return static_cast<EShiftMode>((static_cast<int>(mode) + 1) % 3);
}

ScriptSettings::ScriptSettings() = default;

void ScriptSettings::SetVehicleConfig(VehicleConfig* cfg) {
    activeConfig = cfg;
    localSettings = *this;
    if (!cfg)
        return;

    localSettings.MTOptions.ShiftMode = activeConfig->MTOptions.ShiftMode;
    localSettings.MTOptions.ClutchCreep = activeConfig->MTOptions.ClutchCreep;

    localSettings.MTParams.EngBrakePower      = activeConfig->MTParams.EngBrakePower    ;
    localSettings.MTParams.EngBrakeThreshold  = activeConfig->MTParams.EngBrakeThreshold;
    localSettings.MTParams.ClutchThreshold    = activeConfig->MTParams.ClutchThreshold  ;
    localSettings.MTParams.StallingThreshold  = activeConfig->MTParams.StallingThreshold;
    localSettings.MTParams.StallingRPM        = activeConfig->MTParams.StallingRPM      ;
    localSettings.MTParams.RPMDamage          = activeConfig->MTParams.RPMDamage        ;
    localSettings.MTParams.MisshiftDamage     = activeConfig->MTParams.MisshiftDamage   ;

    localSettings.DriveAssists.CustomABS = activeConfig->DriveAssists.CustomABS;
    localSettings.DriveAssists.ABSFilter = activeConfig->DriveAssists.ABSFilter;
    localSettings.DriveAssists.TCMode    = activeConfig->DriveAssists.TCMode   ;

    localSettings.ShiftOptions.UpshiftCut     = activeConfig->ShiftOptions.UpshiftCut    ;
    localSettings.ShiftOptions.DownshiftBlip  = activeConfig->ShiftOptions.DownshiftBlip ;
    localSettings.ShiftOptions.ClutchRateMult = activeConfig->ShiftOptions.ClutchRateMult;
    localSettings.ShiftOptions.RPMTolerance   = activeConfig->ShiftOptions.RPMTolerance  ;

    localSettings.AutoParams.UpshiftLoad                  = activeConfig->AutoParams.UpshiftLoad         ;
    localSettings.AutoParams.DownshiftLoad                = activeConfig->AutoParams.DownshiftLoad       ;
    localSettings.AutoParams.NextGearMinRPM               = activeConfig->AutoParams.NextGearMinRPM      ;
    localSettings.AutoParams.CurrGearMinRPM               = activeConfig->AutoParams.CurrGearMinRPM      ;
    localSettings.AutoParams.EcoRate                      = activeConfig->AutoParams.EcoRate             ;
    localSettings.AutoParams.DownshiftTimeoutMult         = activeConfig->AutoParams.DownshiftTimeoutMult;


    localSettings.Wheel.FFB.Enable         = activeConfig->Wheel.FFB.Enable        ;
    localSettings.Wheel.FFB.Scale          = activeConfig->Wheel.FFB.Scale         ;
    localSettings.Wheel.FFB.AntiDeadForce  = activeConfig->Wheel.FFB.AntiDeadForce ;
    localSettings.Wheel.FFB.SATAmpMult     = activeConfig->Wheel.FFB.SATAmpMult    ;
    localSettings.Wheel.FFB.SATMax         = activeConfig->Wheel.FFB.SATMax        ;
    localSettings.Wheel.FFB.SATFactor      = activeConfig->Wheel.FFB.SATFactor     ;
    localSettings.Wheel.FFB.DamperMax      = activeConfig->Wheel.FFB.DamperMax     ;
    localSettings.Wheel.FFB.DamperMin      = activeConfig->Wheel.FFB.DamperMin     ;
    localSettings.Wheel.FFB.DamperMinSpeed = activeConfig->Wheel.FFB.DamperMinSpeed;
    localSettings.Wheel.FFB.DetailMult     = activeConfig->Wheel.FFB.DetailMult    ;
    localSettings.Wheel.FFB.DetailLim      = activeConfig->Wheel.FFB.DetailLim     ;
    localSettings.Wheel.FFB.DetailMAW      = activeConfig->Wheel.FFB.DetailMAW     ;
    localSettings.Wheel.FFB.CollisionMult  = activeConfig->Wheel.FFB.CollisionMult ;

    localSettings.Wheel.Steering.AngleCar  = activeConfig->Wheel.Steering.Angle;
    localSettings.Wheel.Steering.AngleBike = activeConfig->Wheel.Steering.Angle;
    localSettings.Wheel.Steering.AngleBoat = activeConfig->Wheel.Steering.Angle;
}

ScriptSettings ScriptSettings::operator()() {
    if (MTOptions.Override && activeConfig) {
        return localSettings;
    }
    return *this;
}

#pragma warning(push)
#pragma warning(disable: 4244)

void ScriptSettings::SetFiles(const std::string &general, const std::string &wheel) {
    settingsGeneralFile = general;
    settingsWheelFile = wheel;
}

void ScriptSettings::Read(CarControls* scriptControl) {
    parseSettingsGeneral();
    parseSettingsControls(scriptControl);
    parseSettingsWheel(scriptControl);
}

void ScriptSettings::SaveGeneral() const {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    settingsGeneral.SetBoolValue("MT_OPTIONS", "Enable", MTOptions.Enable);
    settingsGeneral.SetLongValue("MT_OPTIONS", "ShiftMode", EToInt(MTOptions.ShiftMode));
    settingsGeneral.SetBoolValue("MT_OPTIONS", "Override", MTOptions.Override);

    settingsGeneral.SetBoolValue("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    settingsGeneral.SetBoolValue("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    // [MT_PARAMS]
    settingsGeneral.SetDoubleValue("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "StallingThreshold", MTParams.StallingThreshold);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    settingsGeneral.SetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);

    // [GAMEPLAY_ASSISTS]
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "HidePlayerInFPV", GameAssists.HidePlayerInFPV);
    settingsGeneral.SetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);

    // [DRIVING_ASSISTS]
    settingsGeneral.SetBoolValue("DRIVING_ASSISTS", "CustomABS", DriveAssists.CustomABS);
    settingsGeneral.SetBoolValue("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABSFilter);
    settingsGeneral.SetLongValue("DRIVING_ASSISTS", "TractionControl", DriveAssists.TCMode);

    //[CUSTOM_STEERING]
    settingsGeneral.SetLongValue("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "SteeringMult", CustomSteering.SteeringMult);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);

    // [SHIFT_OPTIONS]
    settingsGeneral.SetBoolValue("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    settingsGeneral.SetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    settingsGeneral.SetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    settingsGeneral.SetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    settingsGeneral.SetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);

    // [HUD]
    settingsGeneral.SetBoolValue("HUD", "EnableHUD", HUD.Enable);
    settingsGeneral.SetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    settingsGeneral.SetLongValue("HUD", "HUDFont", HUD.Font);
    settingsGeneral.SetLongValue("HUD", "NotifyLevel", HUD.NotifyLevel);

    settingsGeneral.SetBoolValue("HUD", "GearIndicator", HUD.Gear.Enable);
    settingsGeneral.SetDoubleValue("HUD", "GearXpos", HUD.Gear.XPos);
    settingsGeneral.SetDoubleValue("HUD", "GearYpos", HUD.Gear.YPos);
    settingsGeneral.SetDoubleValue("HUD", "GearSize", HUD.Gear.Size);
    settingsGeneral.SetLongValue("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    settingsGeneral.SetLongValue("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    settingsGeneral.SetLongValue("HUD", "GearTopColorB", HUD.Gear.TopColorB);

    settingsGeneral.SetBoolValue("HUD", "ShiftModeIndicator", HUD.ShiftMode.Enable);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeSize", HUD.ShiftMode.Size);

    settingsGeneral.SetValue("HUD", "Speedo", HUD.Speedo.Speedo.c_str());
    settingsGeneral.SetBoolValue("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoSize", HUD.Speedo.Size);

    settingsGeneral.SetBoolValue("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    settingsGeneral.SetBoolValue("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    settingsGeneral.SetBoolValue("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoH"	   , HUD.Wheel.PedalYSz);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoW"	   , HUD.Wheel.PedalXSz);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoPadX"  , HUD.Wheel.PedalXPad);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoPadY"  , HUD.Wheel.PedalYPad);

    settingsGeneral.SetLongValue("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    // [UPDATE]
    settingsGeneral.SetBoolValue("UPDATE", "EnableUpdate", Update.EnableUpdate);
    if (!Update.IgnoredVersion.empty())
        settingsGeneral.SetValue("UPDATE", "IgnoredVersion", Update.IgnoredVersion.c_str());
    else
        settingsGeneral.SetValue("UPDATE", "IgnoredVersion", "v0.0.0");

    // [DEBUG]
    settingsGeneral.SetBoolValue("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayFFBInfo", Debug.DisplayFFBInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    settingsGeneral.SetBoolValue("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SaveController() const {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    settingsGeneral.SetLongValue("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    settingsGeneral.SetLongValue("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
    settingsGeneral.SetDoubleValue("CONTROLLER", "TriggerValue", Controller.TriggerValue);

    settingsGeneral.SetBoolValue("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);
    settingsGeneral.SetBoolValue("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    settingsGeneral.SetBoolValue("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    settingsGeneral.SetBoolValue("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    
    settingsGeneral.SetLongValue("CONTROLLER", "ShiftUpBlocks", Controller.ShiftUpBlocks);
    settingsGeneral.SetLongValue("CONTROLLER", "ShiftDownBlocks", Controller.ShiftDownBlocks);
    settingsGeneral.SetLongValue("CONTROLLER", "ClutchBlocks", Controller.ClutchBlocks);

    // [CONTROLLER_NATIVE]
    settingsGeneral.SetBoolValue("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);
    settingsGeneral.SetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", Controller.Native.ShiftUpBlocks);
    settingsGeneral.SetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", Controller.Native.ShiftDownBlocks);
    settingsGeneral.SetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", Controller.Native.ClutchBlocks);

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

// Axis information is saved by its own calibration methods
void ScriptSettings::SaveWheel() const {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    settingsWheel.SetBoolValue("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);

    settingsWheel.SetBoolValue("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    settingsWheel.SetBoolValue("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);

    settingsWheel.SetBoolValue("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    settingsWheel.SetBoolValue("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    settingsWheel.SetBoolValue("FORCE_FEEDBACK", "Scale", Wheel.FFB.Scale);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);

    // [INPUT_DEVICES]
    settingsWheel.SetValue("INPUT_DEVICES", nullptr, nullptr);

    // [STEER]
    settingsWheel.SetDoubleValue("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    settingsWheel.SetDoubleValue("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    settingsWheel.SetDoubleValue("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleBike",Wheel.Steering.AngleBike);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);
    settingsWheel.SetDoubleValue("STEER", "GAMMA", Wheel.Steering.Gamma);
    settingsWheel.SetDoubleValue("STEER", "GameSteerMultWheel", Wheel.Steering.SteerMult);

    // [THROTTLE]
    settingsWheel.SetDoubleValue("THROTTLE", "GAMMA", Wheel.Throttle.Gamma);
    settingsWheel.SetDoubleValue("THROTTLE", "ANTIDEADZONE", Wheel.Throttle.AntiDeadZone);

    // [BRAKE]
    settingsWheel.SetDoubleValue("BRAKE", "GAMMA", Wheel.Brake.Gamma);
    settingsWheel.SetDoubleValue("BRAKE", "ANTIDEADZONE", Wheel.Brake.AntiDeadZone);

    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::parseSettingsGeneral() {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    MTOptions.Enable = settingsGeneral.GetBoolValue("MT_OPTIONS", "Enable", MTOptions.Enable);
    MTOptions.ShiftMode = 
        static_cast<EShiftMode>(settingsGeneral.GetLongValue("MT_OPTIONS", "ShiftMode", EToInt(MTOptions.ShiftMode)));
    MTOptions.Override = settingsGeneral.GetBoolValue("MT_OPTIONS", "Override", MTOptions.Override);

    MTOptions.EngDamage = settingsGeneral.GetBoolValue("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    MTOptions.EngStallH = settingsGeneral.GetBoolValue("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    MTOptions.EngStallS = settingsGeneral.GetBoolValue("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    MTOptions.EngBrake = settingsGeneral.GetBoolValue("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    MTOptions.EngLock  = settingsGeneral.GetBoolValue("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    MTOptions.ClutchCreep = settingsGeneral.GetBoolValue("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    MTOptions.ClutchShiftH = settingsGeneral.GetBoolValue("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    MTOptions.ClutchShiftS = settingsGeneral.GetBoolValue("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    MTOptions.HardLimiter = settingsGeneral.GetBoolValue("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    MTParams.ClutchThreshold =      settingsGeneral.GetDoubleValue("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    MTParams.StallingThreshold =    settingsGeneral.GetDoubleValue("MT_PARAMS", "StallingThreshold", MTParams.StallingThreshold);
    MTParams.StallingRPM =          settingsGeneral.GetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    MTParams.RPMDamage =            settingsGeneral.GetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    MTParams.MisshiftDamage =       settingsGeneral.GetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    MTParams.EngBrakePower =        settingsGeneral.GetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold =    settingsGeneral.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);

    GameAssists.DefaultNeutral =    settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    GameAssists.SimpleBike =        settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    GameAssists.HillGravity =       settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    GameAssists.AutoGear1 =         settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    GameAssists.AutoLookBack =      settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    GameAssists.ThrottleStart =     settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    GameAssists.HidePlayerInFPV =   settingsGeneral.GetBoolValue("GAMEPLAY_ASSISTS", "HidePlayerInFPV", GameAssists.HidePlayerInFPV);

    // [DRIVING_ASSISTS]
    DriveAssists.CustomABS = settingsGeneral.GetBoolValue("DRIVING_ASSISTS", "CustomABS", DriveAssists.CustomABS);
    DriveAssists.ABSFilter = settingsGeneral.GetBoolValue("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABSFilter);
    DriveAssists.TCMode = settingsGeneral.GetLongValue("DRIVING_ASSISTS", "TractionControl", DriveAssists.TCMode);

    // [SHIFT_OPTIONS]
    ShiftOptions.UpshiftCut = settingsGeneral.GetBoolValue("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    ShiftOptions.DownshiftBlip = settingsGeneral.GetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    ShiftOptions.ClutchRateMult = settingsGeneral.GetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    ShiftOptions.RPMTolerance = settingsGeneral.GetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    AutoParams.UpshiftLoad = settingsGeneral.GetDoubleValue("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    AutoParams.DownshiftLoad =  settingsGeneral.GetDoubleValue("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    AutoParams.NextGearMinRPM = settingsGeneral.GetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    AutoParams.CurrGearMinRPM = settingsGeneral.GetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    AutoParams.EcoRate = settingsGeneral.GetDoubleValue("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    AutoParams.DownshiftTimeoutMult = settingsGeneral.GetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);

    // [CUSTOM_STEERING]
    CustomSteering.Mode = settingsGeneral.GetLongValue("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    CustomSteering.CountersteerMult = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    CustomSteering.CountersteerLimit = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    CustomSteering.SteeringMult = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "SteeringMult", CustomSteering.SteeringMult);
    CustomSteering.SteeringReduction = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    CustomSteering.Gamma = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);

    // [HUD]
    HUD.Enable = settingsGeneral.GetBoolValue("HUD", "EnableHUD", HUD.Enable);
    HUD.Always = settingsGeneral.GetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    HUD.Font = settingsGeneral.GetLongValue("HUD", "HUDFont", HUD.Font);
    HUD.NotifyLevel = settingsGeneral.GetLongValue("HUD", "NotifyLevel", HUD.NotifyLevel);

    HUD.Gear.Enable = settingsGeneral.GetBoolValue("HUD", "GearIndicator", HUD.Gear.Enable);
    HUD.Gear.XPos = settingsGeneral.GetDoubleValue("HUD", "GearXpos", HUD.Gear.XPos);
    HUD.Gear.YPos = settingsGeneral.GetDoubleValue("HUD", "GearYpos", HUD.Gear.YPos);
    HUD.Gear.Size = settingsGeneral.GetDoubleValue("HUD", "GearSize", HUD.Gear.Size);
    HUD.Gear.TopColorR = settingsGeneral.GetLongValue("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    HUD.Gear.TopColorG = settingsGeneral.GetLongValue("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    HUD.Gear.TopColorB = settingsGeneral.GetLongValue("HUD", "GearTopColorB", HUD.Gear.TopColorB);

    HUD.ShiftMode.Enable = settingsGeneral.GetBoolValue("HUD", "ShiftModeIndicator", true);
    HUD.ShiftMode.XPos = settingsGeneral.GetDoubleValue("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    HUD.ShiftMode.YPos = settingsGeneral.GetDoubleValue("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    HUD.ShiftMode.Size = settingsGeneral.GetDoubleValue("HUD", "ShiftModeSize", HUD.ShiftMode.Size);

    HUD.Speedo.Speedo = settingsGeneral.GetValue("HUD", "Speedo", HUD.Speedo.Speedo.c_str());
    HUD.Speedo.ShowUnit = settingsGeneral.GetBoolValue("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    HUD.Speedo.XPos = settingsGeneral.GetDoubleValue("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    HUD.Speedo.YPos = settingsGeneral.GetDoubleValue("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    HUD.Speedo.Size = settingsGeneral.GetDoubleValue("HUD", "SpeedoSize", HUD.Speedo.Size);

    HUD.RPMBar.Enable = settingsGeneral.GetBoolValue("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    HUD.RPMBar.XPos = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    HUD.RPMBar.YPos = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    HUD.RPMBar.XSz = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    HUD.RPMBar.YSz = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    HUD.RPMBar.Redline = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    HUD.RPMBar.BgR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    HUD.RPMBar.BgG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    HUD.RPMBar.BgB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    HUD.RPMBar.BgA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);
                                    
    HUD.RPMBar.FgR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    HUD.RPMBar.FgG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    HUD.RPMBar.FgB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    HUD.RPMBar.FgA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);
                                    
    HUD.RPMBar.RedlineR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    HUD.RPMBar.RedlineG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    HUD.RPMBar.RedlineB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    HUD.RPMBar.RedlineA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    HUD.RPMBar.RevLimitR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    HUD.RPMBar.RevLimitG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    HUD.RPMBar.RevLimitB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    HUD.RPMBar.RevLimitA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    HUD.Wheel.Enable = settingsGeneral.GetBoolValue("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    HUD.Wheel.Always = settingsGeneral.GetBoolValue("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    HUD.Wheel.ImgXPos = settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    HUD.Wheel.ImgYPos = settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    HUD.Wheel.ImgSize = settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);

    HUD.Wheel.PedalXPos = settingsGeneral.GetDoubleValue("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    HUD.Wheel.PedalYPos = settingsGeneral.GetDoubleValue("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    HUD.Wheel.PedalYSz = settingsGeneral.GetDoubleValue("HUD", "PedalInfoH", HUD.Wheel.PedalYSz);
    HUD.Wheel.PedalXSz = settingsGeneral.GetDoubleValue("HUD", "PedalInfoW", HUD.Wheel.PedalXSz);
    HUD.Wheel.PedalXPad = settingsGeneral.GetDoubleValue("HUD", "PedalInfoPadX", HUD.Wheel.PedalXPad);
    HUD.Wheel.PedalYPad = settingsGeneral.GetDoubleValue("HUD", "PedalInfoPadY", HUD.Wheel.PedalYPad);
    HUD.Wheel.PedalBgA = settingsGeneral.GetLongValue("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    HUD.Wheel.PedalThrottleR = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    HUD.Wheel.PedalThrottleG = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    HUD.Wheel.PedalThrottleB = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    HUD.Wheel.PedalThrottleA = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    HUD.Wheel.PedalBrakeR = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    HUD.Wheel.PedalBrakeG = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    HUD.Wheel.PedalBrakeB = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    HUD.Wheel.PedalBrakeA = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    HUD.Wheel.PedalClutchR = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    HUD.Wheel.PedalClutchG = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    HUD.Wheel.PedalClutchB = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    HUD.Wheel.PedalClutchA = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    // [UPDATE]
    Update.EnableUpdate = settingsGeneral.GetBoolValue("UPDATE", "EnableUpdate", Update.EnableUpdate);
    Update.IgnoredVersion = settingsGeneral.GetValue("UPDATE", "IgnoredVersion", Update.IgnoredVersion.c_str());

    // [DEBUG]
    Debug.LogLevel = settingsGeneral.GetLongValue("DEBUG", "LogLevel", Debug.LogLevel);
    Debug.DisplayInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    Debug.DisplayWheelInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    Debug.DisplayGearingInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    Debug.DisplayFFBInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayFFBInfo", Debug.DisplayFFBInfo);
    Debug.DisplayNPCInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);
    Debug.DisableInputDetect = settingsGeneral.GetBoolValue("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    Debug.DisablePlayerHide = settingsGeneral.GetBoolValue("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);
}

void ScriptSettings::parseSettingsControls(CarControls* scriptControl) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    // TODO: Fix this somehow
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Toggle)] =
        ini.GetValue("CONTROLLER", "Toggle", "UNKNOWN");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ToggleH)] =
        ini.GetValue("CONTROLLER", "ToggleShift", "B");

    Controller.BlockCarControls = ini.GetBoolValue("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    Controller.IgnoreShiftsUI = ini.GetBoolValue("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    Controller.BlockHShift = ini.GetBoolValue("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    Controller.HoldTimeMs = ini.GetLongValue("CONTROLLER", "ToggleTime", Controller.HoldTimeMs);
    Controller.MaxTapTimeMs = ini.GetLongValue("CONTROLLER", "MaxTapTime", Controller.MaxTapTimeMs);
    Controller.TriggerValue = ini.GetDoubleValue("CONTROLLER", "TriggerValue", Controller.TriggerValue);

    Controller.ToggleEngine = ini.GetBoolValue("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);

    // TODO: Also this
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = ini.GetValue("CONTROLLER", "ShiftUp", "A");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = ini.GetValue("CONTROLLER", "ShiftDown", "X");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Clutch)] = ini.GetValue("CONTROLLER", "Clutch", "LeftThumbUp");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Engine)] = ini.GetValue("CONTROLLER", "Engine", "DpadDown");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Throttle)] = ini.GetValue("CONTROLLER", "Throttle", "RightTrigger");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Brake)] = ini.GetValue("CONTROLLER", "Brake", "LeftTrigger");

    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = ini.GetLongValue("CONTROLLER", "ShiftUpBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER", "ShiftDownBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] = ini.GetLongValue("CONTROLLER", "ClutchBlocks", -1);

    // [CONTROLLER_NATIVE]
    Controller.Native.Enable = ini.GetBoolValue("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);

    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Toggle)] = ini.GetLongValue("CONTROLLER_NATIVE", "Toggle", -1);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ToggleH)] = ini.GetLongValue("CONTROLLER_NATIVE", "ToggleShift", ControlFrontendCancel);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUp", ControlFrontendAccept);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDown", ControlFrontendX);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Clutch)] = ini.GetLongValue("CONTROLLER_NATIVE", "Clutch", ControlFrontendAxisY);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Engine)] = ini.GetLongValue("CONTROLLER_NATIVE", "Engine", ControlFrontendDown);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Throttle)] = ini.GetLongValue("CONTROLLER_NATIVE", "Throttle", ControlFrontendLt);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Brake)] = ini.GetLongValue("CONTROLLER_NATIVE", "Brake", ControlFrontendRt);

    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] = ini.GetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", -1);

    // [KEYBOARD]
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Toggle)] = str2key(ini.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ToggleH)] = str2key(ini.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftUp)] = str2key(ini.GetValue("KEYBOARD", "ShiftUp", "LSHIFT"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftDown)] = str2key(ini.GetValue("KEYBOARD", "ShiftDown", "LCTRL"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Clutch)] = str2key(ini.GetValue("KEYBOARD", "Clutch", "Z"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Engine)] = str2key(ini.GetValue("KEYBOARD", "Engine", "X"));

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Throttle)] = str2key(ini.GetValue("KEYBOARD", "Throttle", "W"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Brake)] = str2key(ini.GetValue("KEYBOARD", "Brake", "S"));

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HR)] = str2key(ini.GetValue("KEYBOARD", "HR", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H1)] = str2key(ini.GetValue("KEYBOARD", "H1", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H2)] = str2key(ini.GetValue("KEYBOARD", "H2", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H3)] = str2key(ini.GetValue("KEYBOARD", "H3", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H4)] = str2key(ini.GetValue("KEYBOARD", "H4", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H5)] = str2key(ini.GetValue("KEYBOARD", "H5", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H6)] = str2key(ini.GetValue("KEYBOARD", "H6", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H7)] = str2key(ini.GetValue("KEYBOARD", "H7", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H8)] = str2key(ini.GetValue("KEYBOARD", "H8", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H9)] = str2key(ini.GetValue("KEYBOARD", "H9", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H10)] = str2key(ini.GetValue("KEYBOARD", "H10", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HN)] = str2key(ini.GetValue("KEYBOARD", "HN", "UNKNOWN"));

}

void ScriptSettings::parseSettingsWheel(CarControls *scriptControl) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    Wheel.Options.Enable = settingsWheel.GetBoolValue("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);
    Wheel.Options.LogiLEDs = settingsWheel.GetBoolValue("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    Wheel.Options.HPatternKeyboard = settingsWheel.GetBoolValue("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);
    Wheel.Options.UseShifterForAuto = settingsWheel.GetBoolValue("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    Wheel.FFB.Enable = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    Wheel.FFB.Scale = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "Scale", Wheel.FFB.Scale);
    Wheel.FFB.AntiDeadForce = settingsWheel.GetLongValue("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);
    Wheel.FFB.SATAmpMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    Wheel.FFB.SATMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    Wheel.FFB.SATFactor = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    Wheel.FFB.DamperMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    Wheel.FFB.DamperMin = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin); ;
    Wheel.FFB.DamperMinSpeed = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    Wheel.FFB.DetailMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    Wheel.FFB.DetailLim = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    Wheel.FFB.DetailMAW = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    Wheel.FFB.CollisionMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);

    // [INPUT_DEVICES]
    int it = 0;
    Wheel.InputDevices.RegisteredGUIDs.clear();
    while (true) {
        std::string currDevIndex = std::string("DEV") + std::to_string(it);
        std::string currGuidIndex = std::string("GUID") + std::to_string(it);

        std::string currDevice = settingsWheel.GetValue("INPUT_DEVICES", currDevIndex.c_str(), "");
        if (currDevice == "")
            break;
        std::string currGuid = settingsWheel.GetValue("INPUT_DEVICES", currGuidIndex.c_str(), "");
        if (currGuid == "")
            break;

        GUID guid = String2GUID(currGuid);
        if (guid != GUID()) {
            Wheel.InputDevices.RegisteredGUIDs.push_back(guid);
        }
        else {
            logger.Write(ERROR, "Failed to parse GUID. GUID [%s] @ [%s]", currGuid.c_str(), currDevice.c_str());
        }
        it++;
    }
    nDevices = it;

    // [TOGGLE_MOD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("TOGGLE_MOD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        settingsWheel.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

    // [CHANGE_SHIFTMODE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);


    // [STEER]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("STEER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        settingsWheel.GetValue("STEER", "AXLE", "");
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::ForceFeedback)] =
        settingsWheel.GetValue("STEER", "FFB", "");
    Wheel.Steering.Min = settingsWheel.GetLongValue("STEER", "MIN", -1);
    Wheel.Steering.Max = settingsWheel.GetLongValue("STEER", "MAX", -1);

    Wheel.Steering.AntiDeadZone = settingsWheel.GetDoubleValue("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    Wheel.Steering.DeadZone = settingsWheel.GetDoubleValue("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    Wheel.Steering.DeadZoneOffset = settingsWheel.GetDoubleValue("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    Wheel.Steering.Gamma = settingsWheel.GetDoubleValue("STEER", "GAMMA", Wheel.Steering.Gamma);

    Wheel.Steering.AngleMax = settingsWheel.GetDoubleValue("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    Wheel.Steering.AngleCar = settingsWheel.GetDoubleValue("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    Wheel.Steering.AngleBike = settingsWheel.GetDoubleValue("STEER", "SteerAngleBike", Wheel.Steering.AngleBike);
    Wheel.Steering.AngleBoat = settingsWheel.GetDoubleValue("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);
    Wheel.Steering.SteerMult = settingsWheel.GetDoubleValue("STEER", "GameSteerMultWheel", Wheel.Steering.SteerMult);

    // [THROTTLE]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        settingsWheel.GetValue("THROTTLE", "AXLE", "");
    Wheel.Throttle.Min = settingsWheel.GetLongValue("THROTTLE", "MIN", -1);
    Wheel.Throttle.Max = settingsWheel.GetLongValue("THROTTLE", "MAX", -1);
    Wheel.Throttle.AntiDeadZone = settingsWheel.GetDoubleValue("THROTTLE", "ANTIDEADZONE", 0.25);
    Wheel.Throttle.Gamma = settingsWheel.GetDoubleValue("THROTTLE", "GAMMA", 1.0);

    // [BRAKE]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        settingsWheel.GetValue("BRAKE", "AXLE", "");
    Wheel.Brake.Min = settingsWheel.GetLongValue("BRAKE", "MIN", -1);
    Wheel.Brake.Max = settingsWheel.GetLongValue("BRAKE", "MAX", -1);
    Wheel.Brake.AntiDeadZone = settingsWheel.GetDoubleValue("BRAKE", "ANTIDEADZONE", 0.25);
    Wheel.Brake.Gamma = settingsWheel.GetDoubleValue("BRAKE", "GAMMA", 1.0);

    // [CLUTCH]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        settingsWheel.GetValue("CLUTCH", "AXLE", "");
    Wheel.Clutch.Min = settingsWheel.GetLongValue("CLUTCH", "MIN", -1);
    Wheel.Clutch.Max = settingsWheel.GetLongValue("CLUTCH", "MAX", -1);

    // [HANDBRAKE_ANALOG]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        settingsWheel.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
    Wheel.HandbrakeA.Min = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
    Wheel.HandbrakeA.Max = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

    // [SHIFTER]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H1)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H2)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H3)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H4)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H5)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H6)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H7)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H8)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H9)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::H10)] =
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::HR)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFTER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);

    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H1)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_1", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H2)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_2", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H3)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_3", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H4)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_4", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H5)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_5", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H6)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_6", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H7)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_7", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H8)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_8", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H9)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_9", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H10)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_10", -1); 
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::HR)] =
        settingsWheel.GetLongValue("SHIFTER", "GEAR_R", -1);

    // [THROTTLE_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        settingsWheel.GetLongValue("THROTTLE_BUTTON", "BUTTON", -1);

    // [BRAKE_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Brake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKE_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Brake)] =
        settingsWheel.GetLongValue("BRAKE_BUTTON", "BUTTON", -1);

    // [CLUTCH_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        settingsWheel.GetLongValue("CLUTCH_BUTTON", "BUTTON", -1);

    // [SHIFT_UP]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_UP", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        settingsWheel.GetLongValue("SHIFT_UP", "BUTTON", -1);

    // [SHIFT_DOWN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_DOWN", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        settingsWheel.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

    // [HANDBRAKE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        settingsWheel.GetLongValue("HANDBRAKE", "BUTTON", -1);

    // [ENGINE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Engine)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("ENGINE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Engine)] =
        settingsWheel.GetLongValue("ENGINE", "BUTTON", -1);

    // [LIGHTS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Lights)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LIGHTS", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Lights)] =
        settingsWheel.GetLongValue("LIGHTS", "BUTTON", -1);

    // [HORN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Horn)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HORN", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Horn)] =
        settingsWheel.GetLongValue("HORN", "BUTTON", -1);

    // [LOOK_BACK]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_BACK", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        settingsWheel.GetLongValue("LOOK_BACK", "BUTTON", -1);
    
    // [LOOK_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_LEFT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        settingsWheel.GetLongValue("LOOK_LEFT", "BUTTON", -1);
    
    // [LOOK_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_RIGHT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        settingsWheel.GetLongValue("LOOK_RIGHT", "BUTTON", -1);

    // [CHANGE_CAMERA]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Camera)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_CAMERA", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Camera)] =
        settingsWheel.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

    // [RADIO_NEXT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_NEXT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        settingsWheel.GetLongValue("RADIO_NEXT", "BUTTON", -1);

    // [RADIO_PREVIOUS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_PREVIOUS", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        settingsWheel.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

    // [INDICATOR_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_LEFT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        settingsWheel.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

    // [INDICATOR_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_RIGHT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        settingsWheel.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

    // [INDICATOR_HAZARD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_HAZARD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        settingsWheel.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);

    // [AUTO_P]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::APark)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_P", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::APark)] =
        settingsWheel.GetLongValue("AUTO_P", "BUTTON", -1);

    // [AUTO_R]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_R", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        settingsWheel.GetLongValue("AUTO_R", "BUTTON", -1);

    // [AUTO_N]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_N", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        settingsWheel.GetLongValue("AUTO_N", "BUTTON", -1);

    // [AUTO_D]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_D", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        settingsWheel.GetLongValue("AUTO_D", "BUTTON", -1);

    // [TO_KEYBOARD]
    scriptControl->WheelToKeyGUID = 
        DeviceIndexToGUID(settingsWheel.GetLongValue("TO_KEYBOARD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        std::string entryString = settingsWheel.GetValue("TO_KEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
        if (entryString == "UNKNOWN") {
            scriptControl->WheelToKey[i] = -1;
        }
        else {
            scriptControl->WheelToKey[i] = str2key(entryString);
        }
    }

}

ptrdiff_t ScriptSettings::SteeringAppendDevice(const GUID &dev_guid, const std::string& dev_name) {
    auto found = find(Wheel.InputDevices.RegisteredGUIDs.begin(), Wheel.InputDevices.RegisteredGUIDs.end(), dev_guid);
    if (found != Wheel.InputDevices.RegisteredGUIDs.end()) {
        // present! Return index
        // Dependent on implementation of reading this but it should work(TM). Lotsa assumptions.
        return distance(Wheel.InputDevices.RegisteredGUIDs.begin(), found);
    }
    // missing! Add & return index afterwards
    auto newIndex = distance(Wheel.InputDevices.RegisteredGUIDs.begin(), Wheel.InputDevices.RegisteredGUIDs.end());
    std::string newDEV = "DEV" + std::to_string(newIndex);
    std::string newGUID = "GUID" + std::to_string(newIndex);


    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue("INPUT_DEVICES", newDEV.c_str(), dev_name.c_str());
    settingsWheel.SetValue("INPUT_DEVICES", newGUID.c_str(), GUID2String(dev_guid).c_str());
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
    return newIndex;
}
void ScriptSettings::SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string & axis, int minVal, int maxVal) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    settingsWheel.SetValue(confTag.c_str(), "AXLE", axis.c_str());
    settingsWheel.SetValue(confTag.c_str(), "MIN", std::to_string(minVal).c_str());
    settingsWheel.SetValue(confTag.c_str(), "MAX", std::to_string(maxVal).c_str());
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveFFBAxis(const std::string & confTag, ptrdiff_t index, const std::string & axis) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    settingsWheel.SetValue(confTag.c_str(), "FFB", axis.c_str());
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveButton(const std::string & confTag, ptrdiff_t index, int button) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    settingsWheel.SetLongValue(confTag.c_str(), "BUTTON", button);
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveHShifter(const std::string & confTag, ptrdiff_t index, const std::vector<int>& button) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());

    settingsWheel.SetLongValue(confTag.c_str(), "GEAR_R", button[0]);
    for (uint8_t i = 1; i < button.size(); ++i) {
        settingsWheel.SetLongValue(confTag.c_str(), fmt::format("GEAR_{}", i).c_str(), button[i]);
    }

    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringAddWheelToKey(const std::string &confTag, ptrdiff_t index, int button, const std::string &keyName) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    settingsWheel.SetValue(confTag.c_str(), std::to_string(button).c_str(), keyName.c_str());
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

bool ScriptSettings::SteeringClearWheelToKey(int button) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    bool deleted = settingsWheel.Delete("TO_KEYBOARD", std::to_string(button).c_str(), true);
    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
    return deleted;
}

void ScriptSettings::KeyboardSaveKey(const std::string &confTag, const std::string &key) {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsGeneral.SetValue("KEYBOARD", confTag.c_str(), key.c_str());
    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}
void ScriptSettings::ControllerSaveButton(const std::string &confTag, const std::string &button) {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsGeneral.SetValue("CONTROLLER", confTag.c_str(), button.c_str());

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::LControllerSaveButton(const std::string &confTag, int button) {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    settingsGeneral.SetLongValue("CONTROLLER_NATIVE", confTag.c_str(), button);

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

GUID ScriptSettings::DeviceIndexToGUID(int device, std::vector<GUID> guids) {
    if (device < 0) {
        return{};
    }
    if (device > nDevices - 1) {
        return{};
    }
    return guids[device];
}

int ScriptSettings::GUIDToDeviceIndex(GUID guidToFind) {
    int i = 0;
    for (auto guid : Wheel.InputDevices.RegisteredGUIDs) {
        if (guid == guidToFind) {
            return i;
        }
        i++;
    }
    return -1;
}

#pragma warning(pop)
