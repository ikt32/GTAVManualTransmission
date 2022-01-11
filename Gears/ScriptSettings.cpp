#include "ScriptSettings.hpp"

#include "VehicleConfig.h"

#include "SettingsCommon.h"
#include "Util/GUID.h"
#include "Util/Logger.hpp"
#include "Util/Strings.hpp"
#include "Input/keyboard.h"
#include "Input/CarControls.hpp"

#include <simpleini/SimpleIni.h>
#include <fmt/format.h>

#include <string>

// TODO: Settings shouldn't *do* anything, other stuff just needs to take stuff from this.

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[Settings] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
        mError = true; \
    }

#define SAVE_VAL(section, key, option) \
    SetValue(ini, section, key, option)

#define LOAD_VAL(section, key, option) \
    option = GetValue(ini, section, key, option)

namespace {
    VehicleConfig* activeConfig = nullptr;

    // Returns a tidied-up version of the configuration tag, if no name is provided.
    std::string formatInputName(const char* cfgTag, const char* name) {
        std::string nameFmt;
        if (name == nullptr) {
            nameFmt = cfgTag;
            std::transform(nameFmt.begin(), nameFmt.end(), nameFmt.begin(), [](char ch) {
                return ch == '_' ? ' ' : ch;
                });
            nameFmt[0] = std::toupper(nameFmt[0]);
            for (std::size_t i = 1; i < nameFmt.length(); ++i)
                nameFmt[i] = std::tolower(nameFmt[i]);
        }
        else {
            nameFmt = name;
        }
        return nameFmt;
    }
}

ScriptSettings::ScriptSettings() = default;

void ScriptSettings::SetVehicleConfig(VehicleConfig* cfg) {
    activeConfig = cfg;
}

VehicleConfig& ScriptSettings::operator()() {
    if (activeConfig) {
        return *activeConfig;
    }
    return baseConfig;
}

bool ScriptSettings::ConfigActive() {
    return activeConfig != nullptr;
}

VehicleConfig* ScriptSettings::BaseConfig() {
    return &baseConfig;
}

bool ScriptSettings::Error() const {
    return mError;
}

#pragma warning(push)
#pragma warning(disable: 4244)

void ScriptSettings::SetFiles(const std::string &general, const std::string& controls, const std::string &wheel) {
    settingsGeneralFile = general;
    settingsControlsFile = controls;
    settingsWheelFile = wheel;

    baseConfig.SetFiles(&baseConfig, general);
}

void ScriptSettings::Read(CarControls* scriptControl) {
    parseSettingsGeneral();
    parseSettingsControls(scriptControl);
    parseSettingsWheel(scriptControl);
    baseConfig.LoadSettings();
}

void ScriptSettings::SaveGeneral() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    SAVE_VAL("MT_OPTIONS", "Enable", MTOptions.Enable);

    SAVE_VAL("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    SAVE_VAL("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    SAVE_VAL("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    SAVE_VAL("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    SAVE_VAL("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    SAVE_VAL("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    // [GAMEPLAY_ASSISTS]
    SAVE_VAL("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    SAVE_VAL("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    SAVE_VAL("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    SAVE_VAL("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    SAVE_VAL("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    SAVE_VAL("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    SAVE_VAL("GAMEPLAY_ASSISTS", "DisableAutostart", GameAssists.DisableAutostart);
    SAVE_VAL("GAMEPLAY_ASSISTS", "LeaveEngineRunning", GameAssists.LeaveEngineRunning);

    //[CUSTOM_STEERING]
    SAVE_VAL("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    SAVE_VAL("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    SAVE_VAL("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    SAVE_VAL("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    SAVE_VAL("CUSTOM_STEERING", "NoReductionHandbrake", CustomSteering.NoReductionHandbrake);
    SAVE_VAL("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);
    SAVE_VAL("CUSTOM_STEERING", "SteerTime", CustomSteering.SteerTime);
    SAVE_VAL("CUSTOM_STEERING", "CenterTime", CustomSteering.CenterTime);

    SAVE_VAL("CUSTOM_STEERING", "MouseSteering", CustomSteering.Mouse.Enable);
    SAVE_VAL("CUSTOM_STEERING", "MouseSensitivity", CustomSteering.Mouse.Sensitivity);
    SAVE_VAL("CUSTOM_STEERING", "MouseDisableSteerAssist", CustomSteering.Mouse.DisableSteerAssist);
    SAVE_VAL("CUSTOM_STEERING", "MouseDisableReduction", CustomSteering.Mouse.DisableReduction);

    // [HUD]
    SAVE_VAL("HUD", "EnableHUD", HUD.Enable);
    SAVE_VAL("HUD", "AlwaysHUD", HUD.Always);
    SAVE_VAL("HUD", "HUDFont", HUD.Font);
    SAVE_VAL("HUD", "Outline", HUD.Outline);
    SAVE_VAL("HUD", "NotifyLevel", HUD.NotifyLevel);

    SAVE_VAL("HUD", "GearIndicator", HUD.Gear.Enable);
    SAVE_VAL("HUD", "GearXpos", HUD.Gear.XPos);
    SAVE_VAL("HUD", "GearYpos", HUD.Gear.YPos);
    SAVE_VAL("HUD", "GearSize", HUD.Gear.Size);
    SAVE_VAL("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    SAVE_VAL("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    SAVE_VAL("HUD", "GearTopColorB", HUD.Gear.TopColorB);
    SAVE_VAL("HUD", "GearColorR", HUD.Gear.ColorR);
    SAVE_VAL("HUD", "GearColorG", HUD.Gear.ColorG);
    SAVE_VAL("HUD", "GearColorB", HUD.Gear.ColorB);

    SAVE_VAL("HUD", "ShiftModeIndicator", HUD.ShiftMode.Enable);
    SAVE_VAL("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    SAVE_VAL("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    SAVE_VAL("HUD", "ShiftModeSize", HUD.ShiftMode.Size);
    SAVE_VAL("HUD", "ShiftModeColorR", HUD.ShiftMode.ColorR);
    SAVE_VAL("HUD", "ShiftModeColorG", HUD.ShiftMode.ColorG);
    SAVE_VAL("HUD", "ShiftModeColorB", HUD.ShiftMode.ColorB);

    SAVE_VAL("HUD", "Speedo", HUD.Speedo.Speedo);
    SAVE_VAL("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    SAVE_VAL("HUD", "SpeedoUseDrivetrain", HUD.Speedo.UseDrivetrain);
    SAVE_VAL("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    SAVE_VAL("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    SAVE_VAL("HUD", "SpeedoSize", HUD.Speedo.Size);
    SAVE_VAL("HUD", "SpeedoColorR", HUD.Speedo.ColorR);
    SAVE_VAL("HUD", "SpeedoColorG", HUD.Speedo.ColorG);
    SAVE_VAL("HUD", "SpeedoColorB", HUD.Speedo.ColorB);

    SAVE_VAL("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    SAVE_VAL("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    SAVE_VAL("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    SAVE_VAL("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    SAVE_VAL("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    SAVE_VAL("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    SAVE_VAL("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    SAVE_VAL("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    SAVE_VAL("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    SAVE_VAL("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);

    SAVE_VAL("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    SAVE_VAL("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    SAVE_VAL("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    SAVE_VAL("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);

    SAVE_VAL("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    SAVE_VAL("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    SAVE_VAL("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    SAVE_VAL("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    SAVE_VAL("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    SAVE_VAL("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    SAVE_VAL("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    SAVE_VAL("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    SAVE_VAL("HUD", "RPMIndicatorLaunchStagedR", HUD.RPMBar.LaunchControlStagedR);
    SAVE_VAL("HUD", "RPMIndicatorLaunchStagedG", HUD.RPMBar.LaunchControlStagedG);
    SAVE_VAL("HUD", "RPMIndicatorLaunchStagedB", HUD.RPMBar.LaunchControlStagedB);
    SAVE_VAL("HUD", "RPMIndicatorLaunchStagedA", HUD.RPMBar.LaunchControlStagedA);

    SAVE_VAL("HUD", "RPMIndicatorLaunchActiveR", HUD.RPMBar.LaunchControlActiveR);
    SAVE_VAL("HUD", "RPMIndicatorLaunchActiveG", HUD.RPMBar.LaunchControlActiveG);
    SAVE_VAL("HUD", "RPMIndicatorLaunchActiveB", HUD.RPMBar.LaunchControlActiveB);
    SAVE_VAL("HUD", "RPMIndicatorLaunchActiveA", HUD.RPMBar.LaunchControlActiveA);

    SAVE_VAL("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    SAVE_VAL("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    SAVE_VAL("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    SAVE_VAL("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    SAVE_VAL("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);
    SAVE_VAL("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    SAVE_VAL("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    SAVE_VAL("HUD", "PedalInfoH"	   , HUD.Wheel.PedalYSz);
    SAVE_VAL("HUD", "PedalInfoW"	   , HUD.Wheel.PedalXSz);
    SAVE_VAL("HUD", "PedalInfoPadX"  , HUD.Wheel.PedalXPad);
    SAVE_VAL("HUD", "PedalInfoPadY"  , HUD.Wheel.PedalYPad);

    SAVE_VAL("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    SAVE_VAL("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    SAVE_VAL("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    SAVE_VAL("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    SAVE_VAL("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    SAVE_VAL("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    SAVE_VAL("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    SAVE_VAL("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    SAVE_VAL("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    SAVE_VAL("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    SAVE_VAL("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    SAVE_VAL("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    SAVE_VAL("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    SAVE_VAL("HUD", "FFBEnable", HUD.Wheel.FFB.Enable);

    SAVE_VAL("HUD", "FFBXPos", HUD.Wheel.FFB.XPos  );
    SAVE_VAL("HUD", "FFBYPos", HUD.Wheel.FFB.YPos  );
    SAVE_VAL("HUD", "FFBXSz", HUD.Wheel.FFB.XSz   );
    SAVE_VAL("HUD", "FFBYSz", HUD.Wheel.FFB.YSz   );

    SAVE_VAL("HUD", "FFBBgR", HUD.Wheel.FFB.BgR   );
    SAVE_VAL("HUD", "FFBBgG", HUD.Wheel.FFB.BgG   );
    SAVE_VAL("HUD", "FFBBgB", HUD.Wheel.FFB.BgB   );
    SAVE_VAL("HUD", "FFBBgA", HUD.Wheel.FFB.BgA   );

    SAVE_VAL("HUD", "FFBFgR", HUD.Wheel.FFB.FgR   );
    SAVE_VAL("HUD", "FFBFgG", HUD.Wheel.FFB.FgG   );
    SAVE_VAL("HUD", "FFBFgB", HUD.Wheel.FFB.FgB   );
    SAVE_VAL("HUD", "FFBFgA", HUD.Wheel.FFB.FgA   );

    SAVE_VAL("HUD", "FFBLimitR", HUD.Wheel.FFB.LimitR);
    SAVE_VAL("HUD", "FFBLimitG", HUD.Wheel.FFB.LimitG);
    SAVE_VAL("HUD", "FFBLimitB", HUD.Wheel.FFB.LimitB);
    SAVE_VAL("HUD", "FFBLimitA", HUD.Wheel.FFB.LimitA);

    SAVE_VAL("HUD", "DashIndicators", HUD.DashIndicators.Enable);
    SAVE_VAL("HUD", "DashIndicatorsXpos", HUD.DashIndicators.XPos);
    SAVE_VAL("HUD", "DashIndicatorsYpos", HUD.DashIndicators.YPos);
    SAVE_VAL("HUD", "DashIndicatorsSize", HUD.DashIndicators.Size);

    SAVE_VAL("HUD", "DsProtEnable", HUD.DsProt.Enable);
    SAVE_VAL("HUD", "DsProtXpos", HUD.DsProt.XPos);
    SAVE_VAL("HUD", "DsProtYpos", HUD.DsProt.YPos);
    SAVE_VAL("HUD", "DsProtSize", HUD.DsProt.Size);

    SAVE_VAL("HUD", "MouseEnable", HUD.MouseSteering.Enable);
    SAVE_VAL("HUD", "MouseXPos", HUD.MouseSteering.XPos);
    SAVE_VAL("HUD", "MouseYPos", HUD.MouseSteering.YPos);
    SAVE_VAL("HUD", "MouseXSz", HUD.MouseSteering.XSz);
    SAVE_VAL("HUD", "MouseYSz", HUD.MouseSteering.YSz);
    SAVE_VAL("HUD", "MouseMarkerXSz", HUD.MouseSteering.MarkerXSz);

    SAVE_VAL("HUD", "MouseBgR", HUD.MouseSteering.BgR);
    SAVE_VAL("HUD", "MouseBgG", HUD.MouseSteering.BgG);
    SAVE_VAL("HUD", "MouseBgB", HUD.MouseSteering.BgB);
    SAVE_VAL("HUD", "MouseBgA", HUD.MouseSteering.BgA);

    SAVE_VAL("HUD", "MouseFgR", HUD.MouseSteering.FgR);
    SAVE_VAL("HUD", "MouseFgG", HUD.MouseSteering.FgG);
    SAVE_VAL("HUD", "MouseFgB", HUD.MouseSteering.FgB);
    SAVE_VAL("HUD", "MouseFgA", HUD.MouseSteering.FgA);

    // [MISC]
    SAVE_VAL("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    SAVE_VAL("MISC", "DashExtensions", Misc.DashExtensions);
    SAVE_VAL("MISC", "SyncAnimations", Misc.SyncAnimations);
    SAVE_VAL("MISC", "HidePlayerInFPV", Misc.HidePlayerInFPV);
    SAVE_VAL("MISC", "HideWheelInFPV", Misc.HideWheelInFPV);

    // [UPDATE]
    SAVE_VAL("UPDATE", "EnableUpdate", Update.EnableUpdate);
    if (!Update.IgnoredVersion.empty())
        SAVE_VAL("UPDATE", "IgnoredVersion", Update.IgnoredVersion.c_str());
    else
        SAVE_VAL("UPDATE", "IgnoredVersion", "v0.0.0");

    // [DEBUG]
    SAVE_VAL("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    SAVE_VAL("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    SAVE_VAL("DEBUG", "DisplayMaterialInfo", Debug.DisplayMaterialInfo);
    SAVE_VAL("DEBUG", "DisplayTractionInfo", Debug.DisplayTractionInfo);
    SAVE_VAL("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    SAVE_VAL("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);

    SAVE_VAL("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    SAVE_VAL("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);
    SAVE_VAL("DEBUG", "DisableNPCGearbox", Debug.DisableNPCGearbox);
    SAVE_VAL("DEBUG", "DisableNPCBrake", Debug.DisableNPCBrake);
    SAVE_VAL("DEBUG", "DisableFPVCam", Debug.DisableFPVCam);

    SAVE_VAL("DEBUG", "EnableTimers", Debug.Metrics.EnableTimers);

    SAVE_VAL("DEBUG", "EnableGForce", Debug.Metrics.GForce.Enable);
    SAVE_VAL("DEBUG", "GForcePosX", Debug.Metrics.GForce.PosX);
    SAVE_VAL("DEBUG", "GForcePosY", Debug.Metrics.GForce.PosY);
    SAVE_VAL("DEBUG", "GForceSize", Debug.Metrics.GForce.Size);

    result = ini.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");

    baseConfig.SaveSettings();
    if (activeConfig) {
        activeConfig->SaveSettings();
    }
}

void ScriptSettings::SaveController(CarControls* scriptControl) const {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    SAVE_VAL("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    SAVE_VAL("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
    SAVE_VAL("CONTROLLER", "TriggerValue", Controller.TriggerValue);
    SAVE_VAL("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);

    SAVE_VAL("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    SAVE_VAL("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    SAVE_VAL("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    SAVE_VAL("CONTROLLER", "CustomDeadzone", Controller.CustomDeadzone);
    SAVE_VAL("CONTROLLER", "DeadzoneLeftThumb", Controller.DeadzoneLeftThumb);
    SAVE_VAL("CONTROLLER", "DeadzoneRightThumb", Controller.DeadzoneRightThumb);

    SAVE_VAL("CONTROLLER", "ShiftUpBlocks",   scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    SAVE_VAL("CONTROLLER", "ShiftDownBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    SAVE_VAL("CONTROLLER", "ClutchBlocks",    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);

    // [CONTROLLER_NATIVE]
    SAVE_VAL("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);
    SAVE_VAL("CONTROLLER_NATIVE", "ShiftUpBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    SAVE_VAL("CONTROLLER_NATIVE", "ShiftDownBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    SAVE_VAL("CONTROLLER_NATIVE", "ClutchBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);

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
    SAVE_VAL("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);

    SAVE_VAL("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    SAVE_VAL("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);

    SAVE_VAL("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    SAVE_VAL("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    SAVE_VAL("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    SAVE_VAL("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    SAVE_VAL("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    SAVE_VAL("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin);
    SAVE_VAL("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    SAVE_VAL("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    SAVE_VAL("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    SAVE_VAL("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    SAVE_VAL("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);
    SAVE_VAL("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);

    SAVE_VAL("FORCE_FEEDBACK", "ResponseCurve", Wheel.FFB.ResponseCurve);
    SAVE_VAL("FORCE_FEEDBACK", "SlipOptMin", Wheel.FFB.SlipOptMin);
    SAVE_VAL("FORCE_FEEDBACK", "SlipOptMinMult", Wheel.FFB.SlipOptMinMult);
    SAVE_VAL("FORCE_FEEDBACK", "SlipOptMax", Wheel.FFB.SlipOptMax);
    SAVE_VAL("FORCE_FEEDBACK", "SlipOptMaxMult", Wheel.FFB.SlipOptMaxMult);

    SAVE_VAL("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    SAVE_VAL("FORCE_FEEDBACK", "Gamma", Wheel.FFB.Gamma);
    SAVE_VAL("FORCE_FEEDBACK", "MaxSpeed", Wheel.FFB.MaxSpeed);

    // [INPUT_DEVICES]
    ini.SetValue("INPUT_DEVICES", nullptr, nullptr);

    // [STEER]
    SAVE_VAL("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    SAVE_VAL("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    SAVE_VAL("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    SAVE_VAL("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    SAVE_VAL("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    SAVE_VAL("STEER", "SteerAngleBike",Wheel.Steering.AngleBike);
    SAVE_VAL("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);
    SAVE_VAL("STEER", "GAMMA", Wheel.Steering.Gamma);

    // [THROTTLE]
    SAVE_VAL("THROTTLE", "GAMMA", Wheel.Throttle.Gamma);
    SAVE_VAL("THROTTLE", "ANTIDEADZONE", Wheel.Throttle.AntiDeadZone);

    // [BRAKE]
    SAVE_VAL("BRAKE", "GAMMA", Wheel.Brake.Gamma);
    SAVE_VAL("BRAKE", "ANTIDEADZONE", Wheel.Brake.AntiDeadZone);

    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::parseSettingsGeneral() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [MT_OPTIONS]
    LOAD_VAL("MT_OPTIONS", "Enable", MTOptions.Enable);

    LOAD_VAL("MT_OPTIONS", "EngineDamage", MTOptions.EngDamage);
    LOAD_VAL("MT_OPTIONS", "EngineStalling", MTOptions.EngStallH);
    LOAD_VAL("MT_OPTIONS", "EngineStallingS", MTOptions.EngStallS);
    LOAD_VAL("MT_OPTIONS", "EngineBraking", MTOptions.EngBrake);
    LOAD_VAL("MT_OPTIONS", "EngineLocking", MTOptions.EngLock);
    LOAD_VAL("MT_OPTIONS", "HardLimiter", MTOptions.HardLimiter);

    LOAD_VAL("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    LOAD_VAL("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    LOAD_VAL("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    LOAD_VAL("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    LOAD_VAL("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    LOAD_VAL("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    LOAD_VAL("GAMEPLAY_ASSISTS", "DisableAutostart", GameAssists.DisableAutostart);
    LOAD_VAL("GAMEPLAY_ASSISTS", "LeaveEngineRunning", GameAssists.LeaveEngineRunning);

    // [CUSTOM_STEERING]
    LOAD_VAL("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    LOAD_VAL("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    LOAD_VAL("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    LOAD_VAL("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);
    LOAD_VAL("CUSTOM_STEERING", "NoReductionHandbrake", CustomSteering.NoReductionHandbrake);
    LOAD_VAL("CUSTOM_STEERING", "Gamma", CustomSteering.Gamma);
    LOAD_VAL("CUSTOM_STEERING", "SteerTime", CustomSteering.SteerTime);
    LOAD_VAL("CUSTOM_STEERING", "CenterTime", CustomSteering.CenterTime);

    LOAD_VAL("CUSTOM_STEERING", "MouseSteering", CustomSteering.Mouse.Enable);
    LOAD_VAL("CUSTOM_STEERING", "MouseSensitivity", CustomSteering.Mouse.Sensitivity);
    LOAD_VAL("CUSTOM_STEERING", "MouseDisableSteerAssist", CustomSteering.Mouse.DisableSteerAssist);
    LOAD_VAL("CUSTOM_STEERING", "MouseDisableReduction", CustomSteering.Mouse.DisableReduction);

    // [HUD]
    LOAD_VAL("HUD", "EnableHUD", HUD.Enable);
    LOAD_VAL("HUD", "AlwaysHUD", HUD.Always);
    LOAD_VAL("HUD", "HUDFont", HUD.Font);
    LOAD_VAL("HUD", "Outline", HUD.Outline);
    LOAD_VAL("HUD", "NotifyLevel", HUD.NotifyLevel);

    LOAD_VAL("HUD", "GearIndicator", HUD.Gear.Enable);
    LOAD_VAL("HUD", "GearXpos", HUD.Gear.XPos);
    LOAD_VAL("HUD", "GearYpos", HUD.Gear.YPos);
    LOAD_VAL("HUD", "GearSize", HUD.Gear.Size);
    LOAD_VAL("HUD", "GearTopColorR", HUD.Gear.TopColorR);
    LOAD_VAL("HUD", "GearTopColorG", HUD.Gear.TopColorG);
    LOAD_VAL("HUD", "GearTopColorB", HUD.Gear.TopColorB);
    LOAD_VAL("HUD", "GearColorR", HUD.Gear.ColorR);
    LOAD_VAL("HUD", "GearColorG", HUD.Gear.ColorG);
    LOAD_VAL("HUD", "GearColorB", HUD.Gear.ColorB);

    LOAD_VAL("HUD", "ShiftModeIndicator", HUD.ShiftMode.Enable);
    LOAD_VAL("HUD", "ShiftModeXpos", HUD.ShiftMode.XPos);
    LOAD_VAL("HUD", "ShiftModeYpos", HUD.ShiftMode.YPos);
    LOAD_VAL("HUD", "ShiftModeSize", HUD.ShiftMode.Size);
    LOAD_VAL("HUD", "ShiftModeColorR", HUD.ShiftMode.ColorR);
    LOAD_VAL("HUD", "ShiftModeColorG", HUD.ShiftMode.ColorG);
    LOAD_VAL("HUD", "ShiftModeColorB", HUD.ShiftMode.ColorB);

    LOAD_VAL("HUD", "Speedo", HUD.Speedo.Speedo);
    LOAD_VAL("HUD", "SpeedoShowUnit", HUD.Speedo.ShowUnit);
    LOAD_VAL("HUD", "SpeedoUseDrivetrain", HUD.Speedo.UseDrivetrain);
    LOAD_VAL("HUD", "SpeedoXpos", HUD.Speedo.XPos);
    LOAD_VAL("HUD", "SpeedoYpos", HUD.Speedo.YPos);
    LOAD_VAL("HUD", "SpeedoSize", HUD.Speedo.Size);
    LOAD_VAL("HUD", "SpeedoColorR", HUD.Speedo.ColorR);
    LOAD_VAL("HUD", "SpeedoColorG", HUD.Speedo.ColorG);
    LOAD_VAL("HUD", "SpeedoColorB", HUD.Speedo.ColorB);

    LOAD_VAL("HUD", "EnableRPMIndicator", HUD.RPMBar.Enable);
    LOAD_VAL("HUD", "RPMIndicatorXpos", HUD.RPMBar.XPos);
    LOAD_VAL("HUD", "RPMIndicatorYpos", HUD.RPMBar.YPos);
    LOAD_VAL("HUD", "RPMIndicatorWidth", HUD.RPMBar.XSz);
    LOAD_VAL("HUD", "RPMIndicatorHeight", HUD.RPMBar.YSz);
    LOAD_VAL("HUD", "RPMIndicatorRedline", HUD.RPMBar.Redline);

    LOAD_VAL("HUD", "RPMIndicatorBackgroundR", HUD.RPMBar.BgR);
    LOAD_VAL("HUD", "RPMIndicatorBackgroundG", HUD.RPMBar.BgG);
    LOAD_VAL("HUD", "RPMIndicatorBackgroundB", HUD.RPMBar.BgB);
    LOAD_VAL("HUD", "RPMIndicatorBackgroundA", HUD.RPMBar.BgA);
                                    
    LOAD_VAL("HUD", "RPMIndicatorForegroundR", HUD.RPMBar.FgR);
    LOAD_VAL("HUD", "RPMIndicatorForegroundG", HUD.RPMBar.FgG);
    LOAD_VAL("HUD", "RPMIndicatorForegroundB", HUD.RPMBar.FgB);
    LOAD_VAL("HUD", "RPMIndicatorForegroundA", HUD.RPMBar.FgA);
                                    
    LOAD_VAL("HUD", "RPMIndicatorRedlineR", HUD.RPMBar.RedlineR);
    LOAD_VAL("HUD", "RPMIndicatorRedlineG", HUD.RPMBar.RedlineG);
    LOAD_VAL("HUD", "RPMIndicatorRedlineB", HUD.RPMBar.RedlineB);
    LOAD_VAL("HUD", "RPMIndicatorRedlineA", HUD.RPMBar.RedlineA);

    LOAD_VAL("HUD", "RPMIndicatorRevlimitR", HUD.RPMBar.RevLimitR);
    LOAD_VAL("HUD", "RPMIndicatorRevlimitG", HUD.RPMBar.RevLimitG);
    LOAD_VAL("HUD", "RPMIndicatorRevlimitB", HUD.RPMBar.RevLimitB);
    LOAD_VAL("HUD", "RPMIndicatorRevlimitA", HUD.RPMBar.RevLimitA);

    LOAD_VAL("HUD", "RPMIndicatorLaunchStagedR", HUD.RPMBar.LaunchControlStagedR);
    LOAD_VAL("HUD", "RPMIndicatorLaunchStagedG", HUD.RPMBar.LaunchControlStagedG);
    LOAD_VAL("HUD", "RPMIndicatorLaunchStagedB", HUD.RPMBar.LaunchControlStagedB);
    LOAD_VAL("HUD", "RPMIndicatorLaunchStagedA", HUD.RPMBar.LaunchControlStagedA);

    LOAD_VAL("HUD", "RPMIndicatorLaunchActiveR", HUD.RPMBar.LaunchControlActiveR);
    LOAD_VAL("HUD", "RPMIndicatorLaunchActiveG", HUD.RPMBar.LaunchControlActiveG);
    LOAD_VAL("HUD", "RPMIndicatorLaunchActiveB", HUD.RPMBar.LaunchControlActiveB);
    LOAD_VAL("HUD", "RPMIndicatorLaunchActiveA", HUD.RPMBar.LaunchControlActiveA);

    LOAD_VAL("HUD", "SteeringWheelInfo", HUD.Wheel.Enable);
    LOAD_VAL("HUD", "AlwaysSteeringWheelInfo", HUD.Wheel.Always);
    LOAD_VAL("HUD", "SteeringWheelTextureX", HUD.Wheel.ImgXPos);
    LOAD_VAL("HUD", "SteeringWheelTextureY", HUD.Wheel.ImgYPos);
    LOAD_VAL("HUD", "SteeringWheelTextureSz", HUD.Wheel.ImgSize);

    LOAD_VAL("HUD", "PedalInfoX", HUD.Wheel.PedalXPos);
    LOAD_VAL("HUD", "PedalInfoY", HUD.Wheel.PedalYPos);
    LOAD_VAL("HUD", "PedalInfoH", HUD.Wheel.PedalYSz);
    LOAD_VAL("HUD", "PedalInfoW", HUD.Wheel.PedalXSz);
    LOAD_VAL("HUD", "PedalInfoPadX", HUD.Wheel.PedalXPad);
    LOAD_VAL("HUD", "PedalInfoPadY", HUD.Wheel.PedalYPad);
    LOAD_VAL("HUD", "PedalBackgroundA", HUD.Wheel.PedalBgA);

    LOAD_VAL("HUD", "PedalInfoThrottleR", HUD.Wheel.PedalThrottleR);
    LOAD_VAL("HUD", "PedalInfoThrottleG", HUD.Wheel.PedalThrottleG);
    LOAD_VAL("HUD", "PedalInfoThrottleB", HUD.Wheel.PedalThrottleB);
    LOAD_VAL("HUD", "PedalInfoThrottleA", HUD.Wheel.PedalThrottleA);

    LOAD_VAL("HUD", "PedalInfoBrakeR", HUD.Wheel.PedalBrakeR);
    LOAD_VAL("HUD", "PedalInfoBrakeG", HUD.Wheel.PedalBrakeG);
    LOAD_VAL("HUD", "PedalInfoBrakeB", HUD.Wheel.PedalBrakeB);
    LOAD_VAL("HUD", "PedalInfoBrakeA", HUD.Wheel.PedalBrakeA);

    LOAD_VAL("HUD", "PedalInfoClutchR", HUD.Wheel.PedalClutchR);
    LOAD_VAL("HUD", "PedalInfoClutchG", HUD.Wheel.PedalClutchG);
    LOAD_VAL("HUD", "PedalInfoClutchB", HUD.Wheel.PedalClutchB);
    LOAD_VAL("HUD", "PedalInfoClutchA", HUD.Wheel.PedalClutchA);

    LOAD_VAL("HUD", "FFBEnable", HUD.Wheel.FFB.Enable);

    LOAD_VAL("HUD", "FFBXPos", HUD.Wheel.FFB.XPos);
    LOAD_VAL("HUD", "FFBYPos", HUD.Wheel.FFB.YPos);
    LOAD_VAL("HUD", "FFBXSz", HUD.Wheel.FFB.XSz);
    LOAD_VAL("HUD", "FFBYSz", HUD.Wheel.FFB.YSz);

    LOAD_VAL("HUD", "FFBBgR", HUD.Wheel.FFB.BgR);
    LOAD_VAL("HUD", "FFBBgG", HUD.Wheel.FFB.BgG);
    LOAD_VAL("HUD", "FFBBgB", HUD.Wheel.FFB.BgB);
    LOAD_VAL("HUD", "FFBBgA", HUD.Wheel.FFB.BgA);

    LOAD_VAL("HUD", "FFBFgR", HUD.Wheel.FFB.FgR);
    LOAD_VAL("HUD", "FFBFgG", HUD.Wheel.FFB.FgG);
    LOAD_VAL("HUD", "FFBFgB", HUD.Wheel.FFB.FgB);
    LOAD_VAL("HUD", "FFBFgA", HUD.Wheel.FFB.FgA);

    LOAD_VAL("HUD", "FFBLimitR", HUD.Wheel.FFB.LimitR);
    LOAD_VAL("HUD", "FFBLimitG", HUD.Wheel.FFB.LimitG);
    LOAD_VAL("HUD", "FFBLimitB", HUD.Wheel.FFB.LimitB);
    LOAD_VAL("HUD", "FFBLimitA", HUD.Wheel.FFB.LimitA);

    LOAD_VAL("HUD", "DashIndicators", HUD.DashIndicators.Enable);
    LOAD_VAL("HUD", "DashIndicatorsXpos", HUD.DashIndicators.XPos);
    LOAD_VAL("HUD", "DashIndicatorsYpos", HUD.DashIndicators.YPos);
    LOAD_VAL("HUD", "DashIndicatorsSize", HUD.DashIndicators.Size);

    LOAD_VAL("HUD", "DsProtEnable", HUD.DsProt.Enable);
    LOAD_VAL("HUD", "DsProtXpos", HUD.DsProt.XPos);
    LOAD_VAL("HUD", "DsProtYpos", HUD.DsProt.YPos);
    LOAD_VAL("HUD", "DsProtSize", HUD.DsProt.Size);

    LOAD_VAL("HUD", "MouseEnable", HUD.MouseSteering.Enable);
    LOAD_VAL("HUD", "MouseXPos", HUD.MouseSteering.XPos);
    LOAD_VAL("HUD", "MouseYPos", HUD.MouseSteering.YPos);
    LOAD_VAL("HUD", "MouseXSz", HUD.MouseSteering.XSz);
    LOAD_VAL("HUD", "MouseYSz", HUD.MouseSteering.YSz);
    LOAD_VAL("HUD", "MouseMarkerXSz", HUD.MouseSteering.MarkerXSz);

    LOAD_VAL("HUD", "MouseBgR", HUD.MouseSteering.BgR);
    LOAD_VAL("HUD", "MouseBgG", HUD.MouseSteering.BgG);
    LOAD_VAL("HUD", "MouseBgB", HUD.MouseSteering.BgB);
    LOAD_VAL("HUD", "MouseBgA", HUD.MouseSteering.BgA);

    LOAD_VAL("HUD", "MouseFgR", HUD.MouseSteering.FgR);
    LOAD_VAL("HUD", "MouseFgG", HUD.MouseSteering.FgG);
    LOAD_VAL("HUD", "MouseFgB", HUD.MouseSteering.FgB);
    LOAD_VAL("HUD", "MouseFgA", HUD.MouseSteering.FgA);

    // [MISC]
    LOAD_VAL("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    LOAD_VAL("MISC", "DashExtensions", Misc.DashExtensions);
    LOAD_VAL("MISC", "SyncAnimations", Misc.SyncAnimations);
    LOAD_VAL("MISC", "HidePlayerInFPV", Misc.HidePlayerInFPV);
    LOAD_VAL("MISC", "HideWheelInFPV", Misc.HideWheelInFPV);

    // [UPDATE]
    LOAD_VAL("UPDATE", "EnableUpdate", Update.EnableUpdate);
    LOAD_VAL("UPDATE", "IgnoredVersion", Update.IgnoredVersion);

    // [DEBUG]
    LOAD_VAL("DEBUG", "LogLevel", Debug.LogLevel);
    LOAD_VAL("DEBUG", "CancelAnimOnUnload", Debug.CancelAnimOnUnload);

    LOAD_VAL("DEBUG", "DisplayInfo", Debug.DisplayInfo);
    LOAD_VAL("DEBUG", "DisplayWheelInfo", Debug.DisplayWheelInfo);
    LOAD_VAL("DEBUG", "DisplayMaterialInfo", Debug.DisplayMaterialInfo);
    LOAD_VAL("DEBUG", "DisplayTractionInfo", Debug.DisplayTractionInfo);
    LOAD_VAL("DEBUG", "DisplayGearingInfo", Debug.DisplayGearingInfo);
    LOAD_VAL("DEBUG", "DisplayNPCInfo", Debug.DisplayNPCInfo);

    LOAD_VAL("DEBUG", "DisableInputDetect", Debug.DisableInputDetect);
    LOAD_VAL("DEBUG", "DisablePlayerHide", Debug.DisablePlayerHide);
    LOAD_VAL("DEBUG", "DisableNPCGearbox", Debug.DisableNPCGearbox);
    LOAD_VAL("DEBUG", "DisableNPCBrake", Debug.DisableNPCBrake);
    LOAD_VAL("DEBUG", "DisableFPVCam", Debug.DisableFPVCam);

    LOAD_VAL("DEBUG", "EnableTimers", Debug.Metrics.EnableTimers);

    int it = 0;
    Debug.Metrics.Timers.clear();
    while (true) {
        std::string unitKey = fmt::format("Timer{}Unit", it);
        std::string limAKey = fmt::format("Timer{}LimA", it);
        std::string limBKey = fmt::format("Timer{}LimB", it);
        std::string toleranceKey = fmt::format("Timer{}Tolerance", it);

        std::string unit = ini.GetValue("DEBUG", unitKey.c_str(), "");
        if (unit.empty()) {
            if (it == 0) {
                logger.Write(INFO, "[Settings] Timers: No timers registered");
            }
            else if (it == 1) {
                logger.Write(INFO, "[Settings] Timers: Registered 1 timer");
            }
            else {
                logger.Write(INFO, "[Settings] Timers: Registered %d timers", it);
            }
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

    LOAD_VAL("DEBUG", "EnableGForce", Debug.Metrics.GForce.Enable);
    LOAD_VAL("DEBUG", "GForcePosX", Debug.Metrics.GForce.PosX);
    LOAD_VAL("DEBUG", "GForcePosY", Debug.Metrics.GForce.PosY);
    LOAD_VAL("DEBUG", "GForceSize", Debug.Metrics.GForce.Size);
}


#define GET_CT(control) \
    static_cast<int>(CarControls::ControllerControlType::control) \

#define GET_LT(control) \
    static_cast<int>(CarControls::ControllerControlType::control) \

#define GET_KT(control) \
    static_cast<int>(CarControls::KeyboardControlType::control) \

void ScriptSettings::parseSettingsControls(CarControls* scriptControl) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(settingsControlsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    LOAD_VAL("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    LOAD_VAL("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    LOAD_VAL("CONTROLLER", "BlockHShift", Controller.BlockHShift);
    LOAD_VAL("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);

    LOAD_VAL("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    LOAD_VAL("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
    LOAD_VAL("CONTROLLER", "TriggerValue", Controller.TriggerValue);

    LOAD_VAL("CONTROLLER", "CustomDeadzone", Controller.CustomDeadzone);
    LOAD_VAL("CONTROLLER", "DeadzoneLeftThumb", Controller.DeadzoneLeftThumb);
    LOAD_VAL("CONTROLLER", "DeadzoneRightThumb", Controller.DeadzoneRightThumb);

    scriptControl->ControlXbox[GET_CT(Toggle)] = parseControllerItem<std::string>(ini, "Toggle", "UNKNOWN", "Toggle MT", "Usage: hold");
    scriptControl->ControlXbox[GET_CT(ToggleH)] = parseControllerItem<std::string>(ini, "ToggleShift", "B", "Change shift mode", "Usage: hold");
    scriptControl->ControlXbox[GET_CT(CycleAssists)] = parseControllerItem<std::string>(ini, "CycleAssists", "UNKNOWN", "Cycle assists", "Usage: hold");
    scriptControl->ControlXbox[GET_CT(ToggleLC)] = parseControllerItem<std::string>(ini, "ToggleLC", "UNKNOWN", "Toggle launch control", "Usage: hold");
    scriptControl->ControlXbox[GET_CT(ToggleCC)] = parseControllerItem<std::string>(ini, "ToggleCC", "UNKNOWN", "Toggle cruise control", "Usage: hold");

    scriptControl->ControlXbox[GET_CT(ShiftUp)]   = parseControllerItem<std::string>(ini, "ShiftUp", "A", "Shift up", "Usage: tap");
    scriptControl->ControlXbox[GET_CT(ShiftDown)] = parseControllerItem<std::string>(ini, "ShiftDown", "X", "Shift down", "Usage: tap");
    scriptControl->ControlXbox[GET_CT(Engine)]    = parseControllerItem<std::string>(ini, "Engine", "DpadDown", "Engine", "Usage: hold");
    scriptControl->ControlXbox[GET_CT(Throttle)]  = parseControllerItem<std::string>(ini, "Throttle", "RightTrigger", "Throttle", "Usage: analog");
    scriptControl->ControlXbox[GET_CT(Brake)]     = parseControllerItem<std::string>(ini, "Brake", "LeftTrigger", "Brake", "Usage: analog");
    scriptControl->ControlXbox[GET_CT(Clutch)]    = parseControllerItem<std::string>(ini, "Clutch", "LeftThumbUp", "Clutch", "Usage: hold/analog");

    scriptControl->ControlXbox[GET_CT(SteerLeft)]  = parseControllerItem<std::string>(ini, "SteerLeft", "LeftThumbLeft", "Steer left", "Usage: analog");
    scriptControl->ControlXbox[GET_CT(SteerRight)] = parseControllerItem<std::string>(ini, "SteerRight", "LeftThumbRight", "Steer right", "Usage: analog");

    scriptControl->ControlXboxBlocks[GET_CT(ShiftUp)]   = ini.GetLongValue("CONTROLLER", "ShiftUpBlocks", ControlVehicleDuck);
    scriptControl->ControlXboxBlocks[GET_CT(ShiftDown)] = ini.GetLongValue("CONTROLLER", "ShiftDownBlocks", ControlVehicleSelectNextWeapon);
    scriptControl->ControlXboxBlocks[GET_CT(Clutch)]    = ini.GetLongValue("CONTROLLER", "ClutchBlocks", -1);

    // [CONTROLLER_NATIVE]
    LOAD_VAL("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);

    scriptControl->LegacyControls[GET_LT(Toggle)]       = parseControllerItem<eControl>(ini, "Toggle", static_cast<eControl>(-1), "Toggle MT", "Usage: hold");
    scriptControl->LegacyControls[GET_LT(ToggleH)]      = parseControllerItem<eControl>(ini, "ToggleShift", ControlFrontendCancel, "Change shift mode", "Usage: hold");
    scriptControl->LegacyControls[GET_LT(CycleAssists)] = parseControllerItem<eControl>(ini, "CycleAssists", static_cast<eControl>(-1), "Cycle assists", "Usage: hold");
    scriptControl->LegacyControls[GET_LT(ToggleLC)]     = parseControllerItem<eControl>(ini, "ToggleLC", static_cast<eControl>(-1), "Toggle launch control", "Usage: hold");
    scriptControl->LegacyControls[GET_LT(ToggleCC)]     = parseControllerItem<eControl>(ini, "ToggleCC", static_cast<eControl>(-1), "Toggle cruise control", "Usage: hold");

    scriptControl->LegacyControls[GET_LT(ShiftUp)]      = parseControllerItem<eControl>(ini, "ShiftUp", ControlFrontendAccept, "Shift up", "Usage: tap");
    scriptControl->LegacyControls[GET_LT(ShiftDown)]    = parseControllerItem<eControl>(ini, "ShiftDown", ControlFrontendX   , "Shift down", "Usage: tap");
    scriptControl->LegacyControls[GET_LT(Engine)]       = parseControllerItem<eControl>(ini, "Engine", ControlFrontendDown   , "Engine", "Usage: hold");
    scriptControl->LegacyControls[GET_LT(Throttle)]     = parseControllerItem<eControl>(ini, "Throttle", ControlFrontendRt   , "Throttle", "Usage: analog");
    scriptControl->LegacyControls[GET_LT(Brake)]        = parseControllerItem<eControl>(ini, "Brake", ControlFrontendLt      , "Brake", "Usage: analog");
    scriptControl->LegacyControls[GET_LT(Clutch)]       = parseControllerItem<eControl>(ini, "Clutch", ControlFrontendAxisY  , "Clutch", "Usage: hold/analog");

    scriptControl->ControlNativeBlocks[GET_LT(ShiftUp)]   = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", ControlVehicleDuck);
    scriptControl->ControlNativeBlocks[GET_LT(ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", ControlVehicleSelectNextWeapon);
    scriptControl->ControlNativeBlocks[GET_LT(Clutch)]    = ini.GetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", -1);

    // [KEYBOARD]
    scriptControl->KBControl[GET_KT(Toggle)] = parseKeyboardItem(ini, "Toggle", "VK_OEM_5", "Toggle MT");
    scriptControl->KBControl[GET_KT(ToggleH)] = parseKeyboardItem(ini, "ToggleH", "VK_OEM_6", "Switch shift mode");

    scriptControl->KBControl[GET_KT(IndicatorLeft)] = parseKeyboardItem(ini, "IndicatorLeft", "UNKNOWN", "Indicator left");
    scriptControl->KBControl[GET_KT(IndicatorRight)] = parseKeyboardItem(ini, "IndicatorRight", "UNKNOWN", "Indicator right");
    scriptControl->KBControl[GET_KT(IndicatorHazard)] = parseKeyboardItem(ini, "IndicatorHazard", "UNKNOWN", "Hazard lights");

    scriptControl->KBControl[GET_KT(CycleAssists)] = parseKeyboardItem(ini, "CycleAssists", "UNKNOWN", "Cycle assists");
    scriptControl->KBControl[GET_KT(ToggleABS)] = parseKeyboardItem(ini, "ToggleABS", "UNKNOWN", "Toggle ABS");
    scriptControl->KBControl[GET_KT(ToggleESC)] = parseKeyboardItem(ini, "ToggleESC", "UNKNOWN", "Toggle ESC");
    scriptControl->KBControl[GET_KT(ToggleTCS)] = parseKeyboardItem(ini, "ToggleTCS", "UNKNOWN", "Toggle TCS");
    scriptControl->KBControl[GET_KT(DriveBiasFInc)] = parseKeyboardItem(ini, "DriveBiasFInc", "UNKNOWN", "Front drive bias +5%");
    scriptControl->KBControl[GET_KT(DriveBiasFDec)] = parseKeyboardItem(ini, "DriveBiasFDec", "UNKNOWN", "Front drive bias -5%");
    scriptControl->KBControl[GET_KT(ToggleLC)] = parseKeyboardItem(ini, "ToggleLC", "UNKNOWN", "Toggle launch control");
    scriptControl->KBControl[GET_KT(ToggleCC)] = parseKeyboardItem(ini, "ToggleCC", "UNKNOWN", "Toggle cruise control");
    scriptControl->KBControl[GET_KT(CCInc)] = parseKeyboardItem(ini, "CCInc", "UNKNOWN", "Cruise control +5");
    scriptControl->KBControl[GET_KT(CCDec)] = parseKeyboardItem(ini, "CCDec", "UNKNOWN", "Cruise control -5");

    scriptControl->KBControl[GET_KT(ShiftUp)] = parseKeyboardItem(ini, "ShiftUp", "LSHIFT", "Shift up");
    scriptControl->KBControl[GET_KT(ShiftDown)] = parseKeyboardItem(ini, "ShiftDown", "LCTRL", "Shift down");
    scriptControl->KBControl[GET_KT(Clutch)] = parseKeyboardItem(ini, "Clutch", "Z");
    scriptControl->KBControl[GET_KT(Engine)] = parseKeyboardItem(ini, "Engine", "X");

    scriptControl->KBControl[GET_KT(Throttle)] = parseKeyboardItem(ini, "Throttle", "W");
    scriptControl->KBControl[GET_KT(Brake)] = parseKeyboardItem(ini, "Brake", "S");

    scriptControl->KBControl[GET_KT(HR)] =  parseKeyboardItem(ini, "HR", "UNKNOWN", "H-pattern reverse");
    scriptControl->KBControl[GET_KT(H1)] =  parseKeyboardItem(ini, "H1", "UNKNOWN", "H-pattern 1");
    scriptControl->KBControl[GET_KT(H2)] =  parseKeyboardItem(ini, "H2", "UNKNOWN", "H-pattern 2");
    scriptControl->KBControl[GET_KT(H3)] =  parseKeyboardItem(ini, "H3", "UNKNOWN", "H-pattern 3");
    scriptControl->KBControl[GET_KT(H4)] =  parseKeyboardItem(ini, "H4", "UNKNOWN", "H-pattern 4");
    scriptControl->KBControl[GET_KT(H5)] =  parseKeyboardItem(ini, "H5", "UNKNOWN", "H-pattern 5");
    scriptControl->KBControl[GET_KT(H6)] =  parseKeyboardItem(ini, "H6", "UNKNOWN", "H-pattern 6");
    scriptControl->KBControl[GET_KT(H7)] =  parseKeyboardItem(ini, "H7", "UNKNOWN", "H-pattern 7");
    scriptControl->KBControl[GET_KT(H8)] =  parseKeyboardItem(ini, "H8", "UNKNOWN", "H-pattern 8");
    scriptControl->KBControl[GET_KT(H9)] =  parseKeyboardItem(ini, "H9", "UNKNOWN", "H-pattern 9");
    scriptControl->KBControl[GET_KT(H10)] = parseKeyboardItem(ini, "H10", "UNKNOWN", "H-pattern 10");
    scriptControl->KBControl[GET_KT(HN)] =  parseKeyboardItem(ini, "HN", "UNKNOWN", "H-pattern neutral");
}

#undef GET_KT
#undef GET_LT
#undef GET_CT

#define GET_WT(control) \
    static_cast<int>(CarControls::WheelControlType::control) \

#define GET_AT(control) \
    static_cast<int>(CarControls::WheelAxisType::control) \

void ScriptSettings::parseSettingsWheel(CarControls *scriptControl) {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiKey();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    LOAD_VAL("MT_OPTIONS", "EnableWheel", Wheel.Options.Enable);
    LOAD_VAL("MT_OPTIONS", "LogitechLEDs", Wheel.Options.LogiLEDs);
    LOAD_VAL("MT_OPTIONS", "HPatternKeyboard", Wheel.Options.HPatternKeyboard);
    LOAD_VAL("MT_OPTIONS", "UseShifterForAuto", Wheel.Options.UseShifterForAuto);

    // [FORCE_FEEDBACK]
    LOAD_VAL("FORCE_FEEDBACK", "Enable", Wheel.FFB.Enable);
    LOAD_VAL("FORCE_FEEDBACK", "SATAmpMult", Wheel.FFB.SATAmpMult);
    LOAD_VAL("FORCE_FEEDBACK", "SATMax", Wheel.FFB.SATMax);
    LOAD_VAL("FORCE_FEEDBACK", "DamperMax", Wheel.FFB.DamperMax);
    LOAD_VAL("FORCE_FEEDBACK", "DamperMin", Wheel.FFB.DamperMin);
    LOAD_VAL("FORCE_FEEDBACK", "DamperMinSpeed", Wheel.FFB.DamperMinSpeed);
    LOAD_VAL("FORCE_FEEDBACK", "DetailMult", Wheel.FFB.DetailMult);
    LOAD_VAL("FORCE_FEEDBACK", "DetailLim", Wheel.FFB.DetailLim);
    LOAD_VAL("FORCE_FEEDBACK", "DetailMaw", Wheel.FFB.DetailMAW);
    LOAD_VAL("FORCE_FEEDBACK", "CollisionMult", Wheel.FFB.CollisionMult);
    LOAD_VAL("FORCE_FEEDBACK", "AntiDeadForce", Wheel.FFB.AntiDeadForce);
    Wheel.FFB.LUTFile = ini.GetValue("FORCE_FEEDBACK", "LUTFile", "");

    LOAD_VAL("FORCE_FEEDBACK", "ResponseCurve", Wheel.FFB.ResponseCurve);
    LOAD_VAL("FORCE_FEEDBACK", "SlipOptMin", Wheel.FFB.SlipOptMin);
    LOAD_VAL("FORCE_FEEDBACK", "SlipOptMinMult", Wheel.FFB.SlipOptMinMult);
    LOAD_VAL("FORCE_FEEDBACK", "SlipOptMax", Wheel.FFB.SlipOptMax);
    LOAD_VAL("FORCE_FEEDBACK", "SlipOptMaxMult", Wheel.FFB.SlipOptMaxMult);

    LOAD_VAL("FORCE_FEEDBACK", "SATFactor", Wheel.FFB.SATFactor);
    LOAD_VAL("FORCE_FEEDBACK", "Gamma", Wheel.FFB.Gamma);
    LOAD_VAL("FORCE_FEEDBACK", "MaxSpeed", Wheel.FFB.MaxSpeed);

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
    scriptControl->WheelButton[GET_WT(Toggle)] =
        parseWheelItem<int>(ini, "TOGGLE_MOD", -1, "Toggle MT");

    // [CHANGE_SHIFTMODE]
    scriptControl->WheelButton[GET_WT(ToggleH)] =
        parseWheelItem<int>(ini, "CHANGE_SHIFTMODE", -1, "Change shift mode");

    // [STEER]
    scriptControl->WheelAxes[GET_AT(Steer)] =
        parseWheelItem<std::string>(ini, "STEER", "", "Steering");

    scriptControl->WheelAxes[GET_AT(ForceFeedback)] =
        CarControls::SInput<std::string>("FFB",
            DeviceIndexToGUID(ini.GetLongValue("STEER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetValue("STEER", "FFB", ""),
            "Force feedback", "Force feedback");

    LOAD_VAL("STEER", "MIN", Wheel.Steering.Min);
    LOAD_VAL("STEER", "MAX", Wheel.Steering.Max);

    LOAD_VAL("STEER", "ANTIDEADZONE", Wheel.Steering.AntiDeadZone);
    LOAD_VAL("STEER", "DEADZONE", Wheel.Steering.DeadZone);
    LOAD_VAL("STEER", "DEADZONEOFFSET", Wheel.Steering.DeadZoneOffset);
    LOAD_VAL("STEER", "GAMMA", Wheel.Steering.Gamma);

    LOAD_VAL("STEER", "SteerAngleMax", Wheel.Steering.AngleMax);
    LOAD_VAL("STEER", "SteerAngleCar", Wheel.Steering.AngleCar);
    LOAD_VAL("STEER", "SteerAngleBike", Wheel.Steering.AngleBike);
    LOAD_VAL("STEER", "SteerAngleBoat", Wheel.Steering.AngleBoat);

    // [THROTTLE]
    scriptControl->WheelAxes[GET_AT(Throttle)] =
        parseWheelItem<std::string>(ini, "THROTTLE", "");

    LOAD_VAL("THROTTLE", "MIN", Wheel.Throttle.Min);
    LOAD_VAL("THROTTLE", "MAX", Wheel.Throttle.Max);
    LOAD_VAL("THROTTLE", "ANTIDEADZONE", Wheel.Throttle.AntiDeadZone);
    LOAD_VAL("THROTTLE", "GAMMA", Wheel.Throttle.Gamma);

    // [BRAKE]
    scriptControl->WheelAxes[GET_AT(Brake)] =
        parseWheelItem<std::string>(ini, "BRAKE", "");

    LOAD_VAL("BRAKE", "MIN", Wheel.Brake.Min);
    LOAD_VAL("BRAKE", "MAX", Wheel.Brake.Max);
    LOAD_VAL("BRAKE", "ANTIDEADZONE", Wheel.Brake.AntiDeadZone);
    LOAD_VAL("BRAKE", "GAMMA", Wheel.Brake.Gamma);

    // [CLUTCH]
    scriptControl->WheelAxes[GET_AT(Clutch)] =
        parseWheelItem<std::string>(ini, "CLUTCH", "");

    LOAD_VAL("CLUTCH", "MIN", Wheel.Clutch.Min);
    LOAD_VAL("CLUTCH", "MAX", Wheel.Clutch.Max);

    // [HANDBRAKE_ANALOG]
    scriptControl->WheelAxes[GET_AT(Handbrake)] =
        parseWheelItem<std::string>(ini, "HANDBRAKE_ANALOG", "", "Handbrake (analog)");

    LOAD_VAL("HANDBRAKE_ANALOG", "MIN", Wheel.HandbrakeA.Min);
    LOAD_VAL("HANDBRAKE_ANALOG", "MAX", Wheel.HandbrakeA.Max);

    // enums HR through H10 are explicitly defined as 0 through 10
    // [HPATTERN_0]
    scriptControl->WheelButton[0] =
        parseWheelItem<int>(ini, "HPATTERN_0", -1, "H-pattern reverse");

    // [HPATTERN_<gear>]
    for (uint8_t i = 1; i < 11; ++i) {
        std::string iniStr = fmt::format("HPATTERN_{}", i);
        std::string menuStr = fmt::format("H-pattern {}", i);
        scriptControl->WheelButton[i] =
            parseWheelItem<int>(ini, iniStr.c_str(), -1, menuStr.c_str());
    }

    // [THROTTLE_BUTTON]
    scriptControl->WheelButton[GET_WT(Throttle)] =
        parseWheelItem<int>(ini, "THROTTLE_BUTTON", -1, "Throttle (Button)");

    // [BRAKE_BUTTON]
    scriptControl->WheelButton[GET_WT(Brake)] =
        parseWheelItem<int>(ini, "BRAKE_BUTTON", -1, "Brake (Button)");

    // [CLUTCH_BUTTON]
    scriptControl->WheelButton[GET_WT(Clutch)] =
        parseWheelItem<int>(ini, "CLUTCH_BUTTON", -1, "Clutch (Button)");

    // [SHIFT_UP]
    scriptControl->WheelButton[GET_WT(ShiftUp)] =
        parseWheelItem<int>(ini, "SHIFT_UP", -1);

    // [SHIFT_DOWN]
    scriptControl->WheelButton[GET_WT(ShiftDown)] =
        parseWheelItem<int>(ini, "SHIFT_DOWN", -1);

    // [HANDBRAKE]
    scriptControl->WheelButton[GET_WT(Handbrake)] =
        parseWheelItem<int>(ini, "HANDBRAKE", -1);

    // [ENGINE]
    scriptControl->WheelButton[GET_WT(Engine)] =
        parseWheelItem<int>(ini, "ENGINE", -1);

    // [LIGHTS]
    scriptControl->WheelButton[GET_WT(Lights)] =
        parseWheelItem<int>(ini, "LIGHTS", -1, "Lights (Cycle)");

    // [LIGHTS_LOW]
    scriptControl->WheelButton[GET_WT(LightsLow)] =
        parseWheelItem<int>(ini, "LIGHTS_LOW", -1, "Lights (Toggle low beams)");

    // [LIGHTS_HIGH]
    scriptControl->WheelButton[GET_WT(LightsHigh)] =
        parseWheelItem<int>(ini, "LIGHTS_HIGH", -1, "Lights (Toggle high beams)");

    // [LIGHTS_HIGH_FLASH]
    scriptControl->WheelButton[GET_WT(LightsHighFlash)] =
        parseWheelItem<int>(ini, "LIGHTS_HIGH_FLASH", -1, "Lights (Flash high beams)");

    // [HORN]
    scriptControl->WheelButton[GET_WT(Horn)] =
        parseWheelItem<int>(ini, "HORN", -1);

    // [LOOK_BACK]
    scriptControl->WheelButton[GET_WT(LookBack)] =
        parseWheelItem<int>(ini, "LOOK_BACK", -1);

    // [LOOK_LEFT]
    scriptControl->WheelButton[GET_WT(LookLeft)] =
        parseWheelItem<int>(ini, "LOOK_LEFT", -1);

    // [LOOK_RIGHT]
    scriptControl->WheelButton[GET_WT(LookRight)] =
        parseWheelItem<int>(ini, "LOOK_RIGHT", -1);

    // [CHANGE_CAMERA]
    scriptControl->WheelButton[GET_WT(Camera)] =
        parseWheelItem<int>(ini, "CHANGE_CAMERA", -1);

    // [RADIO_NEXT]
    scriptControl->WheelButton[GET_WT(RadioNext)] =
        parseWheelItem<int>(ini, "RADIO_NEXT", -1);

    // [RADIO_PREVIOUS]
    scriptControl->WheelButton[GET_WT(RadioPrev)] =
        parseWheelItem<int>(ini, "RADIO_PREVIOUS", -1);

    // [INDICATOR_LEFT]
    scriptControl->WheelButton[GET_WT(IndicatorLeft)] =
        parseWheelItem<int>(ini, "INDICATOR_LEFT", -1);

    // [INDICATOR_RIGHT]
    scriptControl->WheelButton[GET_WT(IndicatorRight)] =
        parseWheelItem<int>(ini, "INDICATOR_RIGHT", -1);

    // [INDICATOR_HAZARD]
    scriptControl->WheelButton[GET_WT(IndicatorHazard)] =
        parseWheelItem<int>(ini, "INDICATOR_HAZARD", -1);

    // [AUTO_P]
    scriptControl->WheelButton[GET_WT(APark)] =
        parseWheelItem<int>(ini, "AUTO_P", -1, "Automatic park");

    // [AUTO_R]
    scriptControl->WheelButton[GET_WT(AReverse)] =
        parseWheelItem<int>(ini, "AUTO_R", -1, "Automatic reverse");

    // [AUTO_N]
    scriptControl->WheelButton[GET_WT(ANeutral)] =
        parseWheelItem<int>(ini, "AUTO_N", -1, "Automatic neutral");

    // [AUTO_D]
    scriptControl->WheelButton[GET_WT(ADrive)] =
        parseWheelItem<int>(ini, "AUTO_D", -1, "Automatic drive");

    // [CYCLE_ASSISTS]
    scriptControl->WheelButton[GET_WT(CycleAssists)] =
        parseWheelItem<int>(ini, "CYCLE_ASSISTS", -1);

    // [TOGGLE_ABS]
    scriptControl->WheelButton[GET_WT(ToggleABS)] =
        parseWheelItem<int>(ini, "TOGGLE_ABS", -1, "Toggle ABS");

    // [TOGGLE_ESC]
    scriptControl->WheelButton[GET_WT(ToggleESC)] =
        parseWheelItem<int>(ini, "TOGGLE_ESC", -1, "Toggle ESC");

    // [TOGGLE_TCS]
    scriptControl->WheelButton[GET_WT(ToggleTCS)] =
        parseWheelItem<int>(ini, "TOGGLE_TCS", -1, "Toggle TCS");

    // [DRIVE_BIAS_F_INC]
    scriptControl->WheelButton[GET_WT(DriveBiasFInc)] =
        parseWheelItem<int>(ini, "DRIVE_BIAS_F_INC", -1, "Front drive bias +5%");

    // [DRIVE_BIAS_F_DEC]
    scriptControl->WheelButton[GET_WT(DriveBiasFDec)] =
        parseWheelItem<int>(ini, "DRIVE_BIAS_F_DEC", -1, "Front drive bias -5%");

    // [TOGGLE_LC]
    scriptControl->WheelButton[GET_WT(ToggleLC)] =
        parseWheelItem<int>(ini, "TOGGLE_LC", -1, "Toggle launch control");

    // [TOGGLE_CC]
    scriptControl->WheelButton[GET_WT(ToggleCC)] = 
        parseWheelItem<int>(ini, "TOGGLE_CC", -1, "Toggle cruise control");

    // [TOGGLE_CC]
    scriptControl->WheelButton[GET_WT(CCInc)] =
        parseWheelItem<int>(ini, "CC_INC", -1, "Cruise control +5");

    // [TOGGLE_CC]
    scriptControl->WheelButton[GET_WT(CCDec)] =
        parseWheelItem<int>(ini, "CC_DEC", -1, "Cruise control -5");

    // [TO_KEYBOARD]
    scriptControl->WheelToKeyGUID = 
        DeviceIndexToGUID(ini.GetLongValue("TO_KEYBOARD", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs);
    for (int i = 0; i < MAX_RGBBUTTONS; i++) {
        scriptControl->WheelToKey[i].clear();
        CSimpleIniA::TNamesDepend values;
        if (ini.GetAllValues("TO_KEYBOARD", std::to_string(i).c_str(), values)) {
            values.sort(CSimpleIniA::Entry::LoadOrder());
            for (const auto& entry : values) {
                scriptControl->WheelToKey[i].push_back(str2key(entry.pItem));
            }
        }
    }

    for (const auto& pov : WheelDirectInput::POVDirections) {
        scriptControl->WheelToKeyPov[pov].clear();
        CSimpleIniA::TNamesDepend values;
        if (ini.GetAllValues("TO_KEYBOARD", std::to_string(pov).c_str(), values)) {
            values.sort(CSimpleIniA::Entry::LoadOrder());
            for (const auto& entry : values) {
                scriptControl->WheelToKeyPov[pov].push_back(str2key(entry.pItem));
            }
        }
    }
}

#undef GET_AT
#undef GET_WT

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

    if (confTag == "STEER")
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

void ScriptSettings::SteeringAddWheelToKey(const std::string &confTag, ptrdiff_t index, int button, const std::string &keyName) {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiKey();
    SI_Error result = ini.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    std::string device = ini.GetValue(confTag.c_str(), "DEVICE", "");
    if (device.empty()) {
        ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    }
    else if (device != std::to_string(index)) {
        ini.Delete(confTag.c_str(), "DEVICE");
        ini.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
    }

    ini.SetValue(confTag.c_str(), std::to_string(button).c_str(), keyName.c_str());
    result = ini.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

bool ScriptSettings::SteeringClearWheelToKey(int button) {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiKey();
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

template <typename T>
CarControls::SInput<T> ScriptSettings::parseWheelItem(CSimpleIniA& ini, const char* section, T defaultValue, const char* name) {
    std::string nameFmt = formatInputName(section, name);
    if constexpr (std::is_same<T, int>::value) {
        return CarControls::SInput<T>(section,
            DeviceIndexToGUID(ini.GetLongValue(section, "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetLongValue(section, "BUTTON", defaultValue), nameFmt.c_str(), "");
    }
    else if constexpr (std::is_same<T, std::string>::value) {
        return CarControls::SInput<T>(section,
            DeviceIndexToGUID(ini.GetLongValue(section, "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetValue(section, "AXLE", defaultValue.c_str()), nameFmt.c_str(), "");
    }
    else {
        static_assert(false, "Type must be string or int.");
    }
}

CarControls::SInput<int> ScriptSettings::parseKeyboardItem(CSimpleIniA& ini, const char* key, const char* defaultValue, const char* name) {
    std::string nameFmt = formatInputName(key, name);
    return CarControls::SInput<int>(key, {}, str2key(ini.GetValue("KEYBOARD", key, defaultValue)), nameFmt, "");
}

template <typename T>
CarControls::SInput<T> ScriptSettings::parseControllerItem(CSimpleIniA& ini, const char* key, T defaultValue, const char* name, const char* description) {
    if constexpr (std::is_same<T, eControl>::value) {
        return CarControls::SInput<T>(key, {},
            static_cast<T>(ini.GetLongValue("CONTROLLER_NATIVE", key, static_cast<int>(defaultValue))),
            name, description);
    }
    else if constexpr (std::is_same<T, std::string>::value) {
        return CarControls::SInput<T>(key, {},
            ini.GetValue("CONTROLLER", key, defaultValue.c_str()),
            name, description);
    }
    else {
        static_assert(false, "Type must be string or eControl.");
    }
}

#pragma warning(pop)
