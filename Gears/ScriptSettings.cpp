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
    localSettings.MTOptions.ClutchShiftH = activeConfig->MTOptions.ClutchShiftH;
    localSettings.MTOptions.ClutchShiftS = activeConfig->MTOptions.ClutchShiftS;

    localSettings.MTParams.EngBrakePower      = activeConfig->MTParams.EngBrakePower    ;
    localSettings.MTParams.EngBrakeThreshold  = activeConfig->MTParams.EngBrakeThreshold;
    localSettings.MTParams.ClutchThreshold    = activeConfig->MTParams.ClutchThreshold  ;
    localSettings.MTParams.StallingThreshold  = activeConfig->MTParams.StallingThreshold;
    localSettings.MTParams.StallingRPM        = activeConfig->MTParams.StallingRPM      ;
    localSettings.MTParams.RPMDamage          = activeConfig->MTParams.RPMDamage        ;
    localSettings.MTParams.MisshiftDamage     = activeConfig->MTParams.MisshiftDamage   ;

    localSettings.DriveAssists.ABS.Enable       = activeConfig->DriveAssists.ABS.Enable      ;
    localSettings.DriveAssists.ABS.Filter       = activeConfig->DriveAssists.ABS.Filter      ;
    localSettings.DriveAssists.TCS.Mode         = activeConfig->DriveAssists.TCS.Mode        ;
    localSettings.DriveAssists.TCS.SlipMax      = activeConfig->DriveAssists.TCS.SlipMax     ;
    localSettings.DriveAssists.ESP.OverMin      = activeConfig->DriveAssists.ESP.OverMin     ;
    localSettings.DriveAssists.ESP.OverMax      = activeConfig->DriveAssists.ESP.OverMax     ;
    localSettings.DriveAssists.ESP.OverMinComp  = activeConfig->DriveAssists.ESP.OverMinComp ;
    localSettings.DriveAssists.ESP.OverMaxComp  = activeConfig->DriveAssists.ESP.OverMaxComp ;
    localSettings.DriveAssists.ESP.UnderMin     = activeConfig->DriveAssists.ESP.UnderMin    ;
    localSettings.DriveAssists.ESP.UnderMax     = activeConfig->DriveAssists.ESP.UnderMax    ;
    localSettings.DriveAssists.ESP.UnderMinComp = activeConfig->DriveAssists.ESP.UnderMinComp;
    localSettings.DriveAssists.ESP.UnderMaxComp = activeConfig->DriveAssists.ESP.UnderMaxComp;

    localSettings.ShiftOptions.UpshiftCut     = activeConfig->ShiftOptions.UpshiftCut    ;
    localSettings.ShiftOptions.DownshiftBlip  = activeConfig->ShiftOptions.DownshiftBlip ;
    localSettings.ShiftOptions.ClutchRateMult = activeConfig->ShiftOptions.ClutchRateMult;
    localSettings.ShiftOptions.RPMTolerance   = activeConfig->ShiftOptions.RPMTolerance  ;

    localSettings.AutoParams.UpshiftLoad          = activeConfig->AutoParams.UpshiftLoad         ;
    localSettings.AutoParams.DownshiftLoad        = activeConfig->AutoParams.DownshiftLoad       ;
    localSettings.AutoParams.NextGearMinRPM       = activeConfig->AutoParams.NextGearMinRPM      ;
    localSettings.AutoParams.CurrGearMinRPM       = activeConfig->AutoParams.CurrGearMinRPM      ;
    localSettings.AutoParams.EcoRate              = activeConfig->AutoParams.EcoRate             ;
    localSettings.AutoParams.DownshiftTimeoutMult = activeConfig->AutoParams.DownshiftTimeoutMult;


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
    localSettings.Wheel.FFB.Gamma          = activeConfig->Wheel.FFB.Gamma         ;
    localSettings.Wheel.FFB.MaxSpeed       = activeConfig->Wheel.FFB.MaxSpeed      ;


    localSettings.Wheel.Steering.AngleCar  = activeConfig->Wheel.Steering.Angle;
    localSettings.Wheel.Steering.AngleBike = activeConfig->Wheel.Steering.Angle;
    localSettings.Wheel.Steering.AngleBoat = activeConfig->Wheel.Steering.Angle;
    localSettings.Wheel.Steering.Gamma     = activeConfig->Wheel.Steering.Gamma;
}

ScriptSettings ScriptSettings::operator()() {
    if (MTOptions.Override && activeConfig) {
        return localSettings;
    }
    return *this;
}

#pragma warning(push)
#pragma warning(disable: 4244)

void ScriptSettings::SetFiles(const std::string &general, const std::string& controls, const std::string &wheel) {
    settingsGeneralFile = general;
    settingsControlsFile = controls;
    settingsWheelFile = wheel;
}

void ScriptSettings::Read(CarControls* scriptControl) {
    parseSettingsGeneral();
    parseSettingsControls(scriptControl);
    parseSettingsWheel(scriptControl);
}

void ScriptSettings::SaveGeneral() const {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    ini.SetBoolValue("MT_OPTIONS", "Enable", MTOptions.Enable);
    ini.SetLongValue("MT_OPTIONS", "ShiftMode", EToInt(MTOptions.ShiftMode));
    ini.SetBoolValue("MT_OPTIONS", "Override", MTOptions.Override);

    ini.SetBoolValue("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    ini.SetBoolValue("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    ini.SetBoolValue("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    ini.SetBoolValue("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    ini.SetBoolValue("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    ini.SetBoolValue("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    ini.SetBoolValue("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    ini.SetBoolValue("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    ini.SetBoolValue("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    // [MT_PARAMS]
    ini.SetDoubleValue("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    ini.SetDoubleValue("MT_PARAMS", "StallingThreshold", MTParams.StallingThreshold);
    ini.SetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    ini.SetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    ini.SetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    ini.SetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    ini.SetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);

    // [GAMEPLAY_ASSISTS]
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "HidePlayerInFPV", GameAssists.HidePlayerInFPV);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);

    // [DRIVING_ASSISTS]
    ini.SetBoolValue("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    ini.SetBoolValue("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    ini.SetBoolValue("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
    ini.SetLongValue("DRIVING_ASSISTS", "TCSMode", DriveAssists.TCS.Mode);
    ini.SetDoubleValue("DRIVING_ASSISTS", "TCSSlipMax", DriveAssists.TCS.SlipMax);
    ini.SetBoolValue("DRIVING_ASSISTS", "ESP", DriveAssists.ESP.Enable);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPOverMin", DriveAssists.ESP.OverMin);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPOverMax", DriveAssists.ESP.OverMax);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPOverMinComp", DriveAssists.ESP.OverMinComp);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPOverMaxComp", DriveAssists.ESP.OverMaxComp);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPUnderMin", DriveAssists.ESP.UnderMin);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPUnderMax", DriveAssists.ESP.UnderMax);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPUnderMinComp", DriveAssists.ESP.UnderMinComp);
    ini.SetDoubleValue("DRIVING_ASSISTS", "ESPUnderMaxComp", DriveAssists.ESP.UnderMaxComp);

    //[CUSTOM_STEERING]
    ini.SetLongValue("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    ini.SetDoubleValue("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    ini.SetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    ini.SetDoubleValue("CUSTOM_STEERING", "SteeringMult", CustomSteering.SteeringMult);
    ini.SetDoubleValue("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    ini.SetDoubleValue("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);
    ini.SetDoubleValue("CUSTOM_STEERING", "SteerTime", CustomSteering.SteerTime);
    ini.SetDoubleValue("CUSTOM_STEERING", "CenterTime", CustomSteering.CenterTime);

    ini.SetBoolValue("CUSTOM_STEERING", "CustomRotation", CustomSteering.CustomRotation);
    ini.SetDoubleValue("CUSTOM_STEERING", "CustomRotationDegrees", CustomSteering.CustomRotationDegrees);

    // [SHIFT_OPTIONS]
    ini.SetBoolValue("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    ini.SetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    ini.SetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    ini.SetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    ini.SetDoubleValue("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    ini.SetDoubleValue("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    ini.SetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    ini.SetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    ini.SetDoubleValue("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    ini.SetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);

    // [HUD]
    ini.SetBoolValue("HUD", "EnableHUD", HUD.Enable);
    ini.SetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    ini.SetLongValue("HUD", "HUDFont", HUD.Font);
    ini.SetLongValue("HUD", "NotifyLevel", HUD.NotifyLevel);

    ini.SetBoolValue("HUD", "GearIndicator", HUD.Gear.Enable);
    ini.SetDoubleValue("HUD", "GearXpos", HUD.Gear.XPos);
    ini.SetDoubleValue("HUD", "GearYpos", HUD.Gear.YPos);
    ini.SetDoubleValue("HUD", "GearSize", HUD.Gear.Size);
    ini.SetLongValue("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    ini.SetLongValue("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    ini.SetLongValue("HUD", "GearTopColorB", HUD.Gear.TopColorB);
    ini.SetLongValue("HUD", "GearColorR", HUD.Gear.ColorR);
    ini.SetLongValue("HUD", "GearColorG", HUD.Gear.ColorG);
    ini.SetLongValue("HUD", "GearColorB", HUD.Gear.ColorB);

    ini.SetBoolValue("HUD", "ShiftModeIndicator", HUD.ShiftMode.Enable);
    ini.SetDoubleValue("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    ini.SetDoubleValue("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    ini.SetDoubleValue("HUD", "ShiftModeSize", HUD.ShiftMode.Size);
    ini.SetLongValue("HUD", "ShiftModeColorR", HUD.ShiftMode.ColorR);
    ini.SetLongValue("HUD", "ShiftModeColorG", HUD.ShiftMode.ColorG);
    ini.SetLongValue("HUD", "ShiftModeColorB", HUD.ShiftMode.ColorB);

    ini.SetValue("HUD", "Speedo", HUD.Speedo.Speedo.c_str());
    ini.SetBoolValue("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    ini.SetDoubleValue("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    ini.SetDoubleValue("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    ini.SetDoubleValue("HUD", "SpeedoSize", HUD.Speedo.Size);
    ini.SetLongValue("HUD", "SpeedoColorR", HUD.Speedo.ColorR);
    ini.SetLongValue("HUD", "SpeedoColorG", HUD.Speedo.ColorG);
    ini.SetLongValue("HUD", "SpeedoColorB", HUD.Speedo.ColorB);

    ini.SetBoolValue("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    ini.SetDoubleValue("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    ini.SetDoubleValue("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    ini.SetDoubleValue("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    ini.SetDoubleValue("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    ini.SetDoubleValue("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    ini.SetLongValue("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    ini.SetLongValue("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    ini.SetLongValue("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    ini.SetLongValue("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);

    ini.SetLongValue("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    ini.SetLongValue("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    ini.SetLongValue("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    ini.SetLongValue("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);

    ini.SetLongValue("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    ini.SetLongValue("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    ini.SetLongValue("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    ini.SetLongValue("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    ini.SetLongValue("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    ini.SetLongValue("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    ini.SetLongValue("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    ini.SetLongValue("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    ini.SetBoolValue("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    ini.SetBoolValue("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    ini.SetDoubleValue("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    ini.SetDoubleValue("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    ini.SetDoubleValue("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);
    ini.SetDoubleValue("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    ini.SetDoubleValue("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    ini.SetDoubleValue("HUD", "PedalInfoH"	   , HUD.Wheel.PedalYSz);
    ini.SetDoubleValue("HUD", "PedalInfoW"	   , HUD.Wheel.PedalXSz);
    ini.SetDoubleValue("HUD", "PedalInfoPadX"  , HUD.Wheel.PedalXPad);
    ini.SetDoubleValue("HUD", "PedalInfoPadY"  , HUD.Wheel.PedalYPad);

    ini.SetLongValue("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    ini.SetLongValue("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    ini.SetLongValue("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    ini.SetLongValue("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    ini.SetLongValue("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    ini.SetLongValue("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    ini.SetLongValue("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    ini.SetLongValue("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    ini.SetLongValue("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    ini.SetLongValue("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    ini.SetLongValue("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    ini.SetLongValue("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    ini.SetLongValue("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    ini.SetBoolValue("HUD", "DashIndicators", HUD.DashIndicators.Enable);
    ini.SetDoubleValue("HUD", "DashIndicatorsXpos", HUD.DashIndicators.XPos);
    ini.SetDoubleValue("HUD", "DashIndicatorsYpos", HUD.DashIndicators.YPos);
    ini.SetDoubleValue("HUD", "DashIndicatorsSize", HUD.DashIndicators.Size);

    // [MISC]
    ini.SetBoolValue("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    ini.SetBoolValue("MISC", "DashExtensions", Misc.DashExtensions);

    // [UPDATE]
    ini.SetBoolValue("UPDATE", "EnableUpdate", Update.EnableUpdate);
    if (!Update.IgnoredVersion.empty())
        ini.SetValue("UPDATE", "IgnoredVersion", Update.IgnoredVersion.c_str());
    else
        ini.SetValue("UPDATE", "IgnoredVersion", "v0.0.0");

    // [DEBUG]
    ini.SetBoolValue("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    ini.SetBoolValue("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    ini.SetBoolValue("DEBUG", "DisplayFFBInfo", Debug.DisplayFFBInfo);
    ini.SetBoolValue("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    ini.SetBoolValue("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);
    ini.SetBoolValue("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    ini.SetBoolValue("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);

    ini.SetBoolValue("DEBUG", "EnableTimers", Debug.Metrics.EnableTimers);

    ini.SetBoolValue("DEBUG", "EnableGForce", Debug.Metrics.GForce.Enable);
    ini.SetDoubleValue("DEBUG", "GForcePosX", Debug.Metrics.GForce.PosX);
    ini.SetDoubleValue("DEBUG", "GForcePosY", Debug.Metrics.GForce.PosY);
    ini.SetDoubleValue("DEBUG", "GForceSize", Debug.Metrics.GForce.Size);

    result = ini.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SaveController(CarControls* scriptControl) const {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    ini.SetLongValue("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    ini.SetLongValue("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
    ini.SetDoubleValue("CONTROLLER", "TriggerValue", Controller.TriggerValue);

    ini.SetBoolValue("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);
    ini.SetBoolValue("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    ini.SetBoolValue("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    ini.SetBoolValue("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    
    ini.SetLongValue("CONTROLLER", "ShiftUpBlocks",   scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    ini.SetLongValue("CONTROLLER", "ShiftDownBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    ini.SetLongValue("CONTROLLER", "ClutchBlocks",    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);

    // [CONTROLLER_NATIVE]
    ini.SetBoolValue("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);
    ini.SetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    ini.SetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    ini.SetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);

    result = ini.SaveFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

// Axis information is saved by its own calibration methods
void ScriptSettings::SaveWheel() const {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    ini.SetBoolValue("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);
    ini.SetBoolValue("MT_OPTIONS", "SyncRotation", Wheel.Options.SyncRotation);

    ini.SetBoolValue("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    ini.SetBoolValue("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);

    ini.SetBoolValue("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    ini.SetBoolValue("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    ini.SetBoolValue("FORCE_FEEDBACK", "Scale", Wheel.FFB.Scale);
    ini.SetLongValue("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);
    ini.SetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    ini.SetLongValue("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    ini.SetDoubleValue("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    ini.SetDoubleValue("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    ini.SetLongValue("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    ini.SetLongValue("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    ini.SetLongValue("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    ini.SetLongValue("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin);
    ini.SetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    ini.SetDoubleValue("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);
    ini.SetDoubleValue("FORCE_FEEDBACK", "Gamma", Wheel.FFB.Gamma);
    ini.SetDoubleValue("FORCE_FEEDBACK", "MaxSpeed", Wheel.FFB.MaxSpeed);

    // [INPUT_DEVICES]
    ini.SetValue("INPUT_DEVICES", nullptr, nullptr);

    // [STEER]
    ini.SetDoubleValue("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    ini.SetDoubleValue("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    ini.SetDoubleValue("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    ini.SetDoubleValue("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    ini.SetDoubleValue("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    ini.SetDoubleValue("STEER", "SteerAngleBike",Wheel.Steering.AngleBike);
    ini.SetDoubleValue("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);
    ini.SetDoubleValue("STEER", "GAMMA", Wheel.Steering.Gamma);
    ini.SetDoubleValue("STEER", "GameSteerMultWheel", Wheel.Steering.SteerMult);

    // [THROTTLE]
    ini.SetDoubleValue("THROTTLE", "GAMMA", Wheel.Throttle.Gamma);
    ini.SetDoubleValue("THROTTLE", "ANTIDEADZONE", Wheel.Throttle.AntiDeadZone);

    // [BRAKE]
    ini.SetDoubleValue("BRAKE", "GAMMA", Wheel.Brake.Gamma);
    ini.SetDoubleValue("BRAKE", "ANTIDEADZONE", Wheel.Brake.AntiDeadZone);

    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::parseSettingsGeneral() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    MTOptions.Enable = ini.GetBoolValue("MT_OPTIONS", "Enable", MTOptions.Enable);
    MTOptions.ShiftMode = 
        static_cast<EShiftMode>(ini.GetLongValue("MT_OPTIONS", "ShiftMode", EToInt(MTOptions.ShiftMode)));
    MTOptions.Override = ini.GetBoolValue("MT_OPTIONS", "Override", MTOptions.Override);

    MTOptions.EngDamage = ini.GetBoolValue("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    MTOptions.EngStallH = ini.GetBoolValue("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    MTOptions.EngStallS = ini.GetBoolValue("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    MTOptions.EngBrake = ini.GetBoolValue("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    MTOptions.EngLock  = ini.GetBoolValue("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    MTOptions.ClutchCreep = ini.GetBoolValue("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    MTOptions.ClutchShiftH = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    MTOptions.ClutchShiftS = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    MTOptions.HardLimiter = ini.GetBoolValue("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    MTParams.ClutchThreshold =      ini.GetDoubleValue("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    MTParams.StallingThreshold =    ini.GetDoubleValue("MT_PARAMS", "StallingThreshold", MTParams.StallingThreshold);
    MTParams.StallingRPM =          ini.GetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    MTParams.RPMDamage =            ini.GetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    MTParams.MisshiftDamage =       ini.GetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    MTParams.EngBrakePower =        ini.GetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold =    ini.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);

    GameAssists.DefaultNeutral =    ini.GetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    GameAssists.SimpleBike =        ini.GetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    GameAssists.HillGravity =       ini.GetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    GameAssists.AutoGear1 =         ini.GetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    GameAssists.AutoLookBack =      ini.GetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    GameAssists.ThrottleStart =     ini.GetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    GameAssists.HidePlayerInFPV =   ini.GetBoolValue("GAMEPLAY_ASSISTS", "HidePlayerInFPV", GameAssists.HidePlayerInFPV);

    // [DRIVING_ASSISTS]
    DriveAssists.ABS.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    DriveAssists.ABS.Filter = ini.GetBoolValue("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    DriveAssists.TCS.Enable = ini.GetLongValue("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
    DriveAssists.TCS.Mode = ini.GetLongValue("DRIVING_ASSISTS", "TCSMode", DriveAssists.TCS.Mode);
    DriveAssists.TCS.SlipMax = ini.GetDoubleValue("DRIVING_ASSISTS", "TCSSlipMax", DriveAssists.TCS.SlipMax);
    DriveAssists.ESP.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "ESP", DriveAssists.ESP.Enable);
    DriveAssists.ESP.OverMin = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMin", DriveAssists.ESP.OverMin);
    DriveAssists.ESP.OverMax = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMax", DriveAssists.ESP.OverMax);
    DriveAssists.ESP.OverMinComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMinComp", DriveAssists.ESP.OverMinComp);
    DriveAssists.ESP.OverMaxComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMaxComp", DriveAssists.ESP.OverMaxComp);
    DriveAssists.ESP.UnderMin = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMin", DriveAssists.ESP.UnderMin);
    DriveAssists.ESP.UnderMax = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMax", DriveAssists.ESP.UnderMax);
    DriveAssists.ESP.UnderMinComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMinComp", DriveAssists.ESP.UnderMinComp);
    DriveAssists.ESP.UnderMaxComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMaxComp", DriveAssists.ESP.UnderMaxComp);

    // [SHIFT_OPTIONS]
    ShiftOptions.UpshiftCut = ini.GetBoolValue("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    ShiftOptions.DownshiftBlip = ini.GetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    ShiftOptions.ClutchRateMult = ini.GetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    ShiftOptions.RPMTolerance = ini.GetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    AutoParams.UpshiftLoad = ini.GetDoubleValue("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    AutoParams.DownshiftLoad =  ini.GetDoubleValue("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    AutoParams.NextGearMinRPM = ini.GetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    AutoParams.CurrGearMinRPM = ini.GetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    AutoParams.EcoRate = ini.GetDoubleValue("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    AutoParams.DownshiftTimeoutMult = ini.GetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);

    // [CUSTOM_STEERING]
    CustomSteering.Mode = ini.GetLongValue("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    CustomSteering.CountersteerMult = ini.GetDoubleValue("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    CustomSteering.CountersteerLimit = ini.GetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    CustomSteering.SteeringMult = ini.GetDoubleValue("CUSTOM_STEERING", "SteeringMult", CustomSteering.SteeringMult);
    CustomSteering.SteeringReduction = ini.GetDoubleValue("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    CustomSteering.Gamma = ini.GetDoubleValue("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);
    CustomSteering.SteerTime = ini.GetDoubleValue("CUSTOM_STEERING", "SteerTime", CustomSteering.SteerTime);
    CustomSteering.CenterTime = ini.GetDoubleValue("CUSTOM_STEERING", "CenterTime", CustomSteering.CenterTime);

    CustomSteering.CustomRotation = ini.GetBoolValue("CUSTOM_STEERING", "CustomRotation", CustomSteering.CustomRotation);
    CustomSteering.CustomRotationDegrees = ini.GetDoubleValue("CUSTOM_STEERING", "CustomRotationDegrees", CustomSteering.CustomRotationDegrees);

    // [HUD]
    HUD.Enable = ini.GetBoolValue("HUD", "EnableHUD", HUD.Enable);
    HUD.Always = ini.GetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    HUD.Font = ini.GetLongValue("HUD", "HUDFont", HUD.Font);
    HUD.NotifyLevel = ini.GetLongValue("HUD", "NotifyLevel", HUD.NotifyLevel);

    HUD.Gear.Enable = ini.GetBoolValue("HUD", "GearIndicator", HUD.Gear.Enable);
    HUD.Gear.XPos = ini.GetDoubleValue("HUD", "GearXpos", HUD.Gear.XPos);
    HUD.Gear.YPos = ini.GetDoubleValue("HUD", "GearYpos", HUD.Gear.YPos);
    HUD.Gear.Size = ini.GetDoubleValue("HUD", "GearSize", HUD.Gear.Size);
    HUD.Gear.TopColorR = ini.GetLongValue("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    HUD.Gear.TopColorG = ini.GetLongValue("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    HUD.Gear.TopColorB = ini.GetLongValue("HUD", "GearTopColorB", HUD.Gear.TopColorB);
    HUD.Gear.ColorR = ini.GetLongValue("HUD", "GearColorR", HUD.Gear.ColorR);
    HUD.Gear.ColorG = ini.GetLongValue("HUD", "GearColorG", HUD.Gear.ColorG);
    HUD.Gear.ColorB = ini.GetLongValue("HUD", "GearColorB", HUD.Gear.ColorB);

    HUD.ShiftMode.Enable = ini.GetBoolValue("HUD", "ShiftModeIndicator", true);
    HUD.ShiftMode.XPos = ini.GetDoubleValue("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    HUD.ShiftMode.YPos = ini.GetDoubleValue("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    HUD.ShiftMode.Size = ini.GetDoubleValue("HUD", "ShiftModeSize", HUD.ShiftMode.Size);
    HUD.ShiftMode.ColorR = ini.GetLongValue("HUD", "ShiftModeColorR", HUD.ShiftMode.ColorR);
    HUD.ShiftMode.ColorG = ini.GetLongValue("HUD", "ShiftModeColorG", HUD.ShiftMode.ColorG);
    HUD.ShiftMode.ColorB = ini.GetLongValue("HUD", "ShiftModeColorB", HUD.ShiftMode.ColorB);

    HUD.Speedo.Speedo = ini.GetValue("HUD", "Speedo", HUD.Speedo.Speedo.c_str());
    HUD.Speedo.ShowUnit = ini.GetBoolValue("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    HUD.Speedo.XPos = ini.GetDoubleValue("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    HUD.Speedo.YPos = ini.GetDoubleValue("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    HUD.Speedo.Size = ini.GetDoubleValue("HUD", "SpeedoSize", HUD.Speedo.Size);
    HUD.Speedo.ColorR = ini.GetLongValue("HUD", "SpeedoColorR", HUD.Speedo.ColorR);
    HUD.Speedo.ColorG = ini.GetLongValue("HUD", "SpeedoColorG", HUD.Speedo.ColorG);
    HUD.Speedo.ColorB = ini.GetLongValue("HUD", "SpeedoColorB", HUD.Speedo.ColorB);

    HUD.RPMBar.Enable = ini.GetBoolValue("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    HUD.RPMBar.XPos = ini.GetDoubleValue("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    HUD.RPMBar.YPos = ini.GetDoubleValue("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    HUD.RPMBar.XSz = ini.GetDoubleValue("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    HUD.RPMBar.YSz = ini.GetDoubleValue("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    HUD.RPMBar.Redline = ini.GetDoubleValue("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    HUD.RPMBar.BgR = ini.GetLongValue("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    HUD.RPMBar.BgG = ini.GetLongValue("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    HUD.RPMBar.BgB = ini.GetLongValue("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    HUD.RPMBar.BgA = ini.GetLongValue("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);
                                    
    HUD.RPMBar.FgR = ini.GetLongValue("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    HUD.RPMBar.FgG = ini.GetLongValue("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    HUD.RPMBar.FgB = ini.GetLongValue("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    HUD.RPMBar.FgA = ini.GetLongValue("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);
                                    
    HUD.RPMBar.RedlineR = ini.GetLongValue("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    HUD.RPMBar.RedlineG = ini.GetLongValue("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    HUD.RPMBar.RedlineB = ini.GetLongValue("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    HUD.RPMBar.RedlineA = ini.GetLongValue("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    HUD.RPMBar.RevLimitR = ini.GetLongValue("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    HUD.RPMBar.RevLimitG = ini.GetLongValue("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    HUD.RPMBar.RevLimitB = ini.GetLongValue("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    HUD.RPMBar.RevLimitA = ini.GetLongValue("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    HUD.Wheel.Enable = ini.GetBoolValue("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    HUD.Wheel.Always = ini.GetBoolValue("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    HUD.Wheel.ImgXPos = ini.GetDoubleValue("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    HUD.Wheel.ImgYPos = ini.GetDoubleValue("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    HUD.Wheel.ImgSize = ini.GetDoubleValue("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);

    HUD.Wheel.PedalXPos = ini.GetDoubleValue("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    HUD.Wheel.PedalYPos = ini.GetDoubleValue("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    HUD.Wheel.PedalYSz = ini.GetDoubleValue("HUD", "PedalInfoH", HUD.Wheel.PedalYSz);
    HUD.Wheel.PedalXSz = ini.GetDoubleValue("HUD", "PedalInfoW", HUD.Wheel.PedalXSz);
    HUD.Wheel.PedalXPad = ini.GetDoubleValue("HUD", "PedalInfoPadX", HUD.Wheel.PedalXPad);
    HUD.Wheel.PedalYPad = ini.GetDoubleValue("HUD", "PedalInfoPadY", HUD.Wheel.PedalYPad);
    HUD.Wheel.PedalBgA = ini.GetLongValue("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    HUD.Wheel.PedalThrottleR = ini.GetLongValue("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    HUD.Wheel.PedalThrottleG = ini.GetLongValue("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    HUD.Wheel.PedalThrottleB = ini.GetLongValue("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    HUD.Wheel.PedalThrottleA = ini.GetLongValue("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    HUD.Wheel.PedalBrakeR = ini.GetLongValue("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    HUD.Wheel.PedalBrakeG = ini.GetLongValue("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    HUD.Wheel.PedalBrakeB = ini.GetLongValue("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    HUD.Wheel.PedalBrakeA = ini.GetLongValue("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    HUD.Wheel.PedalClutchR = ini.GetLongValue("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    HUD.Wheel.PedalClutchG = ini.GetLongValue("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    HUD.Wheel.PedalClutchB = ini.GetLongValue("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    HUD.Wheel.PedalClutchA = ini.GetLongValue("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    HUD.DashIndicators.Enable = ini.GetBoolValue("HUD", "DashIndicators", HUD.DashIndicators.Enable);
    HUD.DashIndicators.XPos = ini.GetDoubleValue("HUD", "DashIndicatorsXpos", HUD.DashIndicators.XPos);
    HUD.DashIndicators.YPos = ini.GetDoubleValue("HUD", "DashIndicatorsYpos", HUD.DashIndicators.YPos);
    HUD.DashIndicators.Size = ini.GetDoubleValue("HUD", "DashIndicatorsSize", HUD.DashIndicators.Size);

    // [MISC]
    Misc.UDPTelemetry = ini.GetBoolValue("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    Misc.DashExtensions = ini.GetBoolValue("MISC", "DashExtensions", Misc.DashExtensions);

    // [UPDATE]
    Update.EnableUpdate = ini.GetBoolValue("UPDATE", "EnableUpdate", Update.EnableUpdate);
    Update.IgnoredVersion = ini.GetValue("UPDATE", "IgnoredVersion", Update.IgnoredVersion.c_str());

    // [DEBUG]
    Debug.LogLevel = ini.GetLongValue("DEBUG", "LogLevel", Debug.LogLevel);
    Debug.DisplayInfo = ini.GetBoolValue("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    Debug.DisplayWheelInfo = ini.GetBoolValue("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    Debug.DisplayGearingInfo = ini.GetBoolValue("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    Debug.DisplayFFBInfo = ini.GetBoolValue("DEBUG", "DisplayFFBInfo", Debug.DisplayFFBInfo);
    Debug.DisplayNPCInfo = ini.GetBoolValue("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);
    Debug.DisableInputDetect = ini.GetBoolValue("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    Debug.DisablePlayerHide = ini.GetBoolValue("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);

    Debug.Metrics.EnableTimers = ini.GetBoolValue("DEBUG", "EnableTimers", Debug.Metrics.EnableTimers);

    int it = 0;
    Debug.Metrics.Timers.clear();
    while (true) {
        std::string unitKey = fmt::format("Timer{}Unit", it);
        std::string limAKey = fmt::format("Timer{}LimA", it);
        std::string limBKey = fmt::format("Timer{}LimB", it);
        std::string toleranceKey = fmt::format("Timer{}Tolerance", it);

        std::string unit = ini.GetValue("DEBUG", unitKey.c_str(), "");
        if (unit == "") {
            logger.Write(INFO, "[Settings] Timers: Stopped after %d timers", it);
            break;
        }

        float limA = ini.GetDoubleValue("DEBUG", limAKey.c_str(), 0.0f);
        float limB = ini.GetDoubleValue("DEBUG", limBKey.c_str(), 0.0f);
        float tolerance = ini.GetDoubleValue("DEBUG", toleranceKey.c_str(), 0.1f);

        if (limA == 0.0f && limB == 0.0f) {
            logger.Write(WARN, "[Settings] Timer%d: Invalid limits, skipping", it);
            it++;
            continue;
        }

        switch (joaat(unit.c_str())) {
            case joaat("kph"): // fall-through
            case joaat("mph"): // fall-through
            case joaat("m/s"):
                logger.Write(INFO, "[Settings] Timer%d: Added [%f - %f] [%s] timer",
                    it, limA, limB, unit.c_str());
                break;
            default:
                logger.Write(WARN, "[Settings] Timer%d: Skipping. Invalid unit: %s",
                    it, unit.c_str());
                logger.Write(WARN, "[Settings] Timer%d: Valid units: kph, mph or m/s",
                    it);
                it++;
                continue;
        }

        Debug.Metrics.Timers.push_back(TimerParams{ unit, limA, limB, tolerance });
        it++;
    }

    Debug.Metrics.GForce.Enable = ini.GetBoolValue("DEBUG", "EnableGForce", Debug.Metrics.GForce.Enable);
    Debug.Metrics.GForce.PosX = ini.GetDoubleValue("DEBUG", "GForcePosX", Debug.Metrics.GForce.PosX);
    Debug.Metrics.GForce.PosY = ini.GetDoubleValue("DEBUG", "GForcePosY", Debug.Metrics.GForce.PosY);
    Debug.Metrics.GForce.Size = ini.GetDoubleValue("DEBUG", "GForceSize", Debug.Metrics.GForce.Size);
}

void ScriptSettings::parseSettingsControls(CarControls* scriptControl) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    // TODO: Fix this somehow
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Toggle)] =
        ini.GetValue("CONTROLLER", "Toggle", "UNKNOWN");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ToggleH)] =
        ini.GetValue("CONTROLLER", "ToggleShift", "B");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::SwitchAssist)] = 
        ini.GetValue("CONTROLLER", "SwitchAssist", "UNKNOWN");

    Controller.BlockCarControls = ini.GetBoolValue("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    Controller.IgnoreShiftsUI = ini.GetBoolValue("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    Controller.BlockHShift = ini.GetBoolValue("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    Controller.HoldTimeMs = ini.GetLongValue("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    Controller.MaxTapTimeMs = ini.GetLongValue("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
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
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::SwitchAssist)] = ini.GetLongValue("CONTROLLER_NATIVE", "SwitchAssist", -1);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUp", ControlFrontendAccept);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDown", ControlFrontendX);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Clutch)] = ini.GetLongValue("CONTROLLER_NATIVE", "Clutch", ControlFrontendAxisY);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Engine)] = ini.GetLongValue("CONTROLLER_NATIVE", "Engine", ControlFrontendDown);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Throttle)] = ini.GetLongValue("CONTROLLER_NATIVE", "Throttle", ControlFrontendRt);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Brake)] = ini.GetLongValue("CONTROLLER_NATIVE", "Brake", ControlFrontendLt);

    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] =   ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", -1)  ;
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] =    ini.GetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", -1)   ;

    // [KEYBOARD]
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Toggle)] = str2key(ini.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ToggleH)] = str2key(ini.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::SwitchAssist)] = str2key(ini.GetValue("KEYBOARD", "SwitchAssist", "UNKNOWN"));

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
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    Wheel.Options.Enable = ini.GetBoolValue("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);
    Wheel.Options.SyncRotation = ini.GetBoolValue("MT_OPTIONS", "SyncRotation", Wheel.Options.SyncRotation);
    Wheel.Options.LogiLEDs = ini.GetBoolValue("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    Wheel.Options.HPatternKeyboard = ini.GetBoolValue("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);
    Wheel.Options.UseShifterForAuto = ini.GetBoolValue("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    Wheel.FFB.Enable = ini.GetBoolValue("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    Wheel.FFB.Scale = ini.GetBoolValue("FORCE_FEEDBACK", "Scale", Wheel.FFB.Scale);
    Wheel.FFB.AntiDeadForce = ini.GetLongValue("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);
    Wheel.FFB.SATAmpMult = ini.GetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    Wheel.FFB.SATMax = ini.GetLongValue("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    Wheel.FFB.SATFactor = ini.GetDoubleValue("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    Wheel.FFB.DamperMax = ini.GetLongValue("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    Wheel.FFB.DamperMin = ini.GetLongValue("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin); ;
    Wheel.FFB.DamperMinSpeed = ini.GetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    Wheel.FFB.DetailMult = ini.GetDoubleValue("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    Wheel.FFB.DetailLim = ini.GetLongValue("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    Wheel.FFB.DetailMAW = ini.GetLongValue("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    Wheel.FFB.CollisionMult = ini.GetDoubleValue("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);
    Wheel.FFB.Gamma = ini.GetDoubleValue("FORCE_FEEDBACK", "Gamma", Wheel.FFB.Gamma);
    Wheel.FFB.MaxSpeed = ini.GetDoubleValue("FORCE_FEEDBACK", "MaxSpeed", Wheel.FFB.MaxSpeed);

    // [INPUT_DEVICES]
    int it = 0;
    Wheel.InputDevices.RegisteredGUIDs.clear();
    while (true) {
        std::string currDevIndex = std::string("DEV") + std::to_string(it);
        std::string currGuidIndex = std::string("GUID") + std::to_string(it);

        std::string currDevice = ini.GetValue("INPUT_DEVICES", currDevIndex.c_str(), "");
        if (currDevice == "")
            break;
        std::string currGuid = ini.GetValue("INPUT_DEVICES", currGuidIndex.c_str(), "");
        if (currGuid == "")
            break;

        GUID guid = String2GUID(currGuid);
        if (guid != GUID()) {
            Wheel.InputDevices.RegisteredGUIDs.push_back(guid);
        }
        else {
            logger.Write(ERROR, "[Settings] Failed to parse GUID. GUID [%s] @ [%s]", currGuid.c_str(), currDevice.c_str());
        }
        it++;
    }
    nDevices = it;

    // [TOGGLE_MOD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        DeviceIndexToGUID(ini.GetLongValue("TOGGLE_MOD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        ini.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

    // [CHANGE_SHIFTMODE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        DeviceIndexToGUID(ini.GetLongValue("CHANGE_SHIFTMODE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        ini.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);

    // [SWITCH_ASSIST]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::SwitchAssist)] =
        DeviceIndexToGUID(ini.GetLongValue("SWITCH_ASSIST", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::SwitchAssist)] =
        ini.GetLongValue("SWITCH_ASSIST", "BUTTON", -1);

    // [STEER]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        DeviceIndexToGUID(ini.GetLongValue("STEER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        ini.GetValue("STEER", "AXLE", "");
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::ForceFeedback)] =
        ini.GetValue("STEER", "FFB", "");
    Wheel.Steering.Min = ini.GetLongValue("STEER", "MIN", -1);
    Wheel.Steering.Max = ini.GetLongValue("STEER", "MAX", -1);

    Wheel.Steering.AntiDeadZone = ini.GetDoubleValue("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    Wheel.Steering.DeadZone = ini.GetDoubleValue("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    Wheel.Steering.DeadZoneOffset = ini.GetDoubleValue("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    Wheel.Steering.Gamma = ini.GetDoubleValue("STEER", "GAMMA", Wheel.Steering.Gamma);

    Wheel.Steering.AngleMax = ini.GetDoubleValue("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    Wheel.Steering.AngleCar = ini.GetDoubleValue("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    Wheel.Steering.AngleBike = ini.GetDoubleValue("STEER", "SteerAngleBike", Wheel.Steering.AngleBike);
    Wheel.Steering.AngleBoat = ini.GetDoubleValue("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);
    Wheel.Steering.SteerMult = ini.GetDoubleValue("STEER", "GameSteerMultWheel", Wheel.Steering.SteerMult);

    // [THROTTLE]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        DeviceIndexToGUID(ini.GetLongValue("THROTTLE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        ini.GetValue("THROTTLE", "AXLE", "");
    Wheel.Throttle.Min = ini.GetLongValue("THROTTLE", "MIN", -1);
    Wheel.Throttle.Max = ini.GetLongValue("THROTTLE", "MAX", -1);
    Wheel.Throttle.AntiDeadZone = ini.GetDoubleValue("THROTTLE", "ANTIDEADZONE", 0.25);
    Wheel.Throttle.Gamma = ini.GetDoubleValue("THROTTLE", "GAMMA", 1.0);

    // [BRAKE]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        DeviceIndexToGUID(ini.GetLongValue("BRAKE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        ini.GetValue("BRAKE", "AXLE", "");
    Wheel.Brake.Min = ini.GetLongValue("BRAKE", "MIN", -1);
    Wheel.Brake.Max = ini.GetLongValue("BRAKE", "MAX", -1);
    Wheel.Brake.AntiDeadZone = ini.GetDoubleValue("BRAKE", "ANTIDEADZONE", 0.25);
    Wheel.Brake.Gamma = ini.GetDoubleValue("BRAKE", "GAMMA", 1.0);

    // [CLUTCH]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        DeviceIndexToGUID(ini.GetLongValue("CLUTCH", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        ini.GetValue("CLUTCH", "AXLE", "");
    Wheel.Clutch.Min = ini.GetLongValue("CLUTCH", "MIN", -1);
    Wheel.Clutch.Max = ini.GetLongValue("CLUTCH", "MAX", -1);

    // [HANDBRAKE_ANALOG]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        DeviceIndexToGUID(ini.GetLongValue("HANDBRAKE_ANALOG", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        ini.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
    Wheel.HandbrakeA.Min = ini.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
    Wheel.HandbrakeA.Max = ini.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

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
        DeviceIndexToGUID(ini.GetLongValue("SHIFTER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);

    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H1)] =
        ini.GetLongValue("SHIFTER", "GEAR_1", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H2)] =
        ini.GetLongValue("SHIFTER", "GEAR_2", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H3)] =
        ini.GetLongValue("SHIFTER", "GEAR_3", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H4)] =
        ini.GetLongValue("SHIFTER", "GEAR_4", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H5)] =
        ini.GetLongValue("SHIFTER", "GEAR_5", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H6)] =
        ini.GetLongValue("SHIFTER", "GEAR_6", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H7)] =
        ini.GetLongValue("SHIFTER", "GEAR_7", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H8)] =
        ini.GetLongValue("SHIFTER", "GEAR_8", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H9)] =
        ini.GetLongValue("SHIFTER", "GEAR_9", -1);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::H10)] =
        ini.GetLongValue("SHIFTER", "GEAR_10", -1); 
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::HR)] =
        ini.GetLongValue("SHIFTER", "GEAR_R", -1);

    // [THROTTLE_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        DeviceIndexToGUID(ini.GetLongValue("THROTTLE_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        ini.GetLongValue("THROTTLE_BUTTON", "BUTTON", -1);

    // [BRAKE_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Brake)] =
        DeviceIndexToGUID(ini.GetLongValue("BRAKE_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Brake)] =
        ini.GetLongValue("BRAKE_BUTTON", "BUTTON", -1);

    // [CLUTCH_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        DeviceIndexToGUID(ini.GetLongValue("CLUTCH_BUTTON", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        ini.GetLongValue("CLUTCH_BUTTON", "BUTTON", -1);

    // [SHIFT_UP]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        DeviceIndexToGUID(ini.GetLongValue("SHIFT_UP", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        ini.GetLongValue("SHIFT_UP", "BUTTON", -1);

    // [SHIFT_DOWN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        DeviceIndexToGUID(ini.GetLongValue("SHIFT_DOWN", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        ini.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

    // [HANDBRAKE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        DeviceIndexToGUID(ini.GetLongValue("HANDBRAKE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        ini.GetLongValue("HANDBRAKE", "BUTTON", -1);

    // [ENGINE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Engine)] =
        DeviceIndexToGUID(ini.GetLongValue("ENGINE", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Engine)] =
        ini.GetLongValue("ENGINE", "BUTTON", -1);

    // [LIGHTS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Lights)] =
        DeviceIndexToGUID(ini.GetLongValue("LIGHTS", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Lights)] =
        ini.GetLongValue("LIGHTS", "BUTTON", -1);

    // [HORN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Horn)] =
        DeviceIndexToGUID(ini.GetLongValue("HORN", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Horn)] =
        ini.GetLongValue("HORN", "BUTTON", -1);

    // [LOOK_BACK]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        DeviceIndexToGUID(ini.GetLongValue("LOOK_BACK", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        ini.GetLongValue("LOOK_BACK", "BUTTON", -1);
    
    // [LOOK_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        DeviceIndexToGUID(ini.GetLongValue("LOOK_LEFT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        ini.GetLongValue("LOOK_LEFT", "BUTTON", -1);
    
    // [LOOK_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        DeviceIndexToGUID(ini.GetLongValue("LOOK_RIGHT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        ini.GetLongValue("LOOK_RIGHT", "BUTTON", -1);

    // [CHANGE_CAMERA]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Camera)] =
        DeviceIndexToGUID(ini.GetLongValue("CHANGE_CAMERA", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Camera)] =
        ini.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

    // [RADIO_NEXT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        DeviceIndexToGUID(ini.GetLongValue("RADIO_NEXT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        ini.GetLongValue("RADIO_NEXT", "BUTTON", -1);

    // [RADIO_PREVIOUS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        DeviceIndexToGUID(ini.GetLongValue("RADIO_PREVIOUS", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        ini.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

    // [INDICATOR_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        DeviceIndexToGUID(ini.GetLongValue("INDICATOR_LEFT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        ini.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

    // [INDICATOR_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        DeviceIndexToGUID(ini.GetLongValue("INDICATOR_RIGHT", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        ini.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

    // [INDICATOR_HAZARD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        DeviceIndexToGUID(ini.GetLongValue("INDICATOR_HAZARD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        ini.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);

    // [AUTO_P]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::APark)] =
        DeviceIndexToGUID(ini.GetLongValue("AUTO_P", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::APark)] =
        ini.GetLongValue("AUTO_P", "BUTTON", -1);

    // [AUTO_R]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        DeviceIndexToGUID(ini.GetLongValue("AUTO_R", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        ini.GetLongValue("AUTO_R", "BUTTON", -1);

    // [AUTO_N]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        DeviceIndexToGUID(ini.GetLongValue("AUTO_N", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        ini.GetLongValue("AUTO_N", "BUTTON", -1);

    // [AUTO_D]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        DeviceIndexToGUID(ini.GetLongValue("AUTO_D", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        ini.GetLongValue("AUTO_D", "BUTTON", -1);

    // [TO_KEYBOARD]
    scriptControl->WheelToKeyGUID = 
        DeviceIndexToGUID(ini.GetLongValue("TO_KEYBOARD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        std::string entryString = ini.GetValue("TO_KEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
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


    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue("INPUT_DEVICES", newDEV.c_str(), dev_name.c_str());
    ini.SetValue("INPUT_DEVICES", newGUID.c_str(), GUID2String(dev_guid).c_str());
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
    return newIndex;
}
void ScriptSettings::SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string & axis, int minVal, int maxVal) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    ini.SetValue(confTag.c_str(), "AXLE", axis.c_str());
    ini.SetValue(confTag.c_str(), "MIN", std::to_string(minVal).c_str());
    ini.SetValue(confTag.c_str(), "MAX", std::to_string(maxVal).c_str());
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveFFBAxis(const std::string & confTag, ptrdiff_t index, const std::string & axis) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    ini.SetValue(confTag.c_str(), "FFB", axis.c_str());
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveButton(const std::string & confTag, ptrdiff_t index, int button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    ini.SetLongValue(confTag.c_str(), "BUTTON", button);
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringSaveHShifter(const std::string & confTag, ptrdiff_t index, const std::vector<int>& button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());

    ini.SetLongValue(confTag.c_str(), "GEAR_R", button[0]);
    for (uint8_t i = 1; i < button.size(); ++i) {
        ini.SetLongValue(confTag.c_str(), fmt::format("GEAR_{}", i).c_str(), button[i]);
    }

    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SteeringAddWheelToKey(const std::string &confTag, ptrdiff_t index, int button, const std::string &keyName) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    ini.SetValue(confTag.c_str(), std::to_string(button).c_str(), keyName.c_str());
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

bool ScriptSettings::SteeringClearWheelToKey(int button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    bool deleted = ini.Delete("TO_KEYBOARD", std::to_string(button).c_str(), true);
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
    return deleted;
}

void ScriptSettings::KeyboardSaveKey(const std::string &confTag, const std::string &key) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue("KEYBOARD", confTag.c_str(), key.c_str());
    result = ini.SaveFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}
void ScriptSettings::ControllerSaveButton(const std::string &confTag, const std::string &button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetValue("CONTROLLER", confTag.c_str(), button.c_str());

    result = ini.SaveFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::LControllerSaveButton(const std::string &confTag, int button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    ini.SetLongValue("CONTROLLER_NATIVE", confTag.c_str(), button);

    result = ini.SaveFile(settingsControlsFile.c_str());
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
