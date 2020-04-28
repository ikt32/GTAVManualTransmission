#include "VehicleConfig.h"
#include "Util/Util.hpp"
#include <filesystem>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[Settings] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
    }

VehicleConfig::VehicleConfig(const ScriptSettings& gSettings, const std::string& file) {
    loadSettings(gSettings, file);
}

#pragma warning(push)
#pragma warning(disable: 4244)
void VehicleConfig::loadSettings(const ScriptSettings& gSettings, const std::string& file) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(file.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    Name = std::filesystem::path(file).stem().string();

    // [ID]
    std::string allNames = ini.GetValue("ID", "ModelName", "");
    std::string allPlates = ini.GetValue("ID", "Plate", "");
    ModelNames = StrUtil::split(allNames, ' ');
    Plates = StrUtil::split(allPlates, ' ');
    Description = ini.GetValue("ID", "Description", "No description.");
    
    // [MT_OPTIONS]
    MTOptions.ShiftMode =
        static_cast<EShiftMode>(ini.GetLongValue("MT_OPTIONS", "ShiftMode", EToInt(gSettings.MTOptions.ShiftMode)));
    MTOptions.ClutchCreep = ini.GetBoolValue("MT_OPTIONS", "ClutchCatching", gSettings.MTOptions.ClutchCreep);
    MTOptions.ClutchShiftH = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingH", gSettings.MTOptions.ClutchShiftH);
    MTOptions.ClutchShiftS = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingS", gSettings.MTOptions.ClutchShiftS);

    // [MT_PARAMS]
    MTParams.ClutchThreshold = ini.GetDoubleValue("MT_PARAMS", "ClutchCatchpoint", gSettings.MTParams.ClutchThreshold);
    MTParams.StallingThreshold = ini.GetDoubleValue("MT_PARAMS", "StallingThreshold", gSettings.MTParams.StallingThreshold);
    MTParams.StallingRPM = ini.GetDoubleValue("MT_PARAMS", "StallingRPM", gSettings.MTParams.StallingRPM);
    MTParams.RPMDamage = ini.GetDoubleValue("MT_PARAMS", "RPMDamage", gSettings.MTParams.RPMDamage);
    MTParams.MisshiftDamage = ini.GetDoubleValue("MT_PARAMS", "MisshiftDamage", gSettings.MTParams.MisshiftDamage);
    MTParams.EngBrakePower = ini.GetDoubleValue("MT_PARAMS", "EngBrakePower", gSettings.MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold = ini.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", gSettings.MTParams.EngBrakeThreshold);

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
    ShiftOptions.UpshiftCut = ini.GetBoolValue("SHIFT_OPTIONS", "UpshiftCut", gSettings.ShiftOptions.UpshiftCut);
    ShiftOptions.DownshiftBlip = ini.GetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", gSettings.ShiftOptions.DownshiftBlip);
    ShiftOptions.ClutchRateMult = ini.GetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", gSettings.ShiftOptions.ClutchRateMult);
    ShiftOptions.RPMTolerance = ini.GetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", gSettings.ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    AutoParams.UpshiftLoad = ini.GetDoubleValue("AUTO_PARAMS", "UpshiftLoad", gSettings.AutoParams.UpshiftLoad);
    AutoParams.DownshiftLoad = ini.GetDoubleValue("AUTO_PARAMS", "DownshiftLoad", gSettings.AutoParams.DownshiftLoad);
    AutoParams.NextGearMinRPM = ini.GetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", gSettings.AutoParams.NextGearMinRPM);
    AutoParams.CurrGearMinRPM = ini.GetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", gSettings.AutoParams.CurrGearMinRPM);
    AutoParams.EcoRate = ini.GetDoubleValue("AUTO_PARAMS", "EcoRate", gSettings.AutoParams.EcoRate);
    AutoParams.DownshiftTimeoutMult = ini.GetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", gSettings.AutoParams.DownshiftTimeoutMult);

    // [FORCE_FEEDBACK]
    Wheel.FFB.Enable = ini.GetBoolValue("FORCE_FEEDBACK", "Enable", gSettings.Wheel.FFB.Enable);
    Wheel.FFB.Scale = ini.GetBoolValue("FORCE_FEEDBACK", "Scale", gSettings.Wheel.FFB.Scale);
    Wheel.FFB.AntiDeadForce = ini.GetLongValue("FORCE_FEEDBACK", "AntiDeadForce", gSettings.Wheel.FFB.AntiDeadForce);
    Wheel.FFB.SATAmpMult = ini.GetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", gSettings.Wheel.FFB.SATAmpMult);
    Wheel.FFB.SATMax = ini.GetLongValue("FORCE_FEEDBACK", "SATMax", gSettings.Wheel.FFB.SATMax);
    Wheel.FFB.SATFactor = ini.GetDoubleValue("FORCE_FEEDBACK", "SATFactor", gSettings.Wheel.FFB.SATFactor);
    Wheel.FFB.DamperMax = ini.GetLongValue("FORCE_FEEDBACK", "DamperMax", gSettings.Wheel.FFB.DamperMax);
    Wheel.FFB.DamperMin = ini.GetLongValue("FORCE_FEEDBACK", "DamperMin", gSettings.Wheel.FFB.DamperMin); ;
    Wheel.FFB.DamperMinSpeed = ini.GetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", gSettings.Wheel.FFB.DamperMinSpeed);
    Wheel.FFB.DetailMult = ini.GetDoubleValue("FORCE_FEEDBACK", "DetailMult", gSettings.Wheel.FFB.DetailMult);
    Wheel.FFB.DetailLim = ini.GetLongValue("FORCE_FEEDBACK", "DetailLim", gSettings.Wheel.FFB.DetailLim);
    Wheel.FFB.DetailMAW = ini.GetLongValue("FORCE_FEEDBACK", "DetailMaw", gSettings.Wheel.FFB.DetailMAW);
    Wheel.FFB.CollisionMult = ini.GetDoubleValue("FORCE_FEEDBACK", "CollisionMult", gSettings.Wheel.FFB.CollisionMult);
    Wheel.FFB.Gamma = ini.GetDoubleValue("FORCE_FEEDBACK", "Gamma", gSettings.Wheel.FFB.Gamma);
    Wheel.FFB.MaxSpeed = ini.GetDoubleValue("FORCE_FEEDBACK", "MaxSpeed", gSettings.Wheel.FFB.MaxSpeed);

    // [STEER]
    Wheel.Steering.Angle = ini.GetDoubleValue("STEER", "Angle", gSettings.Wheel.Steering.AngleCar);
    Wheel.Steering.Gamma = ini.GetDoubleValue("STEER", "Gamma", gSettings.Wheel.Steering.Gamma);

    // [HUD]
    HUD.Enable = ini.GetBoolValue("HUD", "EnableHUD", HUD.Enable);
    HUD.Font = ini.GetLongValue("HUD", "HUDFont", HUD.Font);
    HUD.Outline = ini.GetBoolValue("HUD", "Outline", HUD.Outline);

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

    HUD.DashIndicators.Enable = ini.GetBoolValue("HUD", "DashIndicators", HUD.DashIndicators.Enable);
    HUD.DashIndicators.XPos = ini.GetDoubleValue("HUD", "DashIndicatorsXpos", HUD.DashIndicators.XPos);
    HUD.DashIndicators.YPos = ini.GetDoubleValue("HUD", "DashIndicatorsYpos", HUD.DashIndicators.YPos);
    HUD.DashIndicators.Size = ini.GetDoubleValue("HUD", "DashIndicatorsSize", HUD.DashIndicators.Size);
}
#pragma warning(pop)
