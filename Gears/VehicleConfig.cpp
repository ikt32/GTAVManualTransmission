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
    CSimpleIniA settingsIni;
    settingsIni.SetUnicode();
    SI_Error result = settingsIni.LoadFile(file.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    Name = std::filesystem::path(file).stem().string();

    // [ID]
    std::string allNames = settingsIni.GetValue("ID", "ModelName", "");
    std::string allPlates = settingsIni.GetValue("ID", "Plate", "");
    ModelNames = StrUtil::split(allNames, ' ');
    Plates = StrUtil::split(allPlates, ' ');
    
    // [MT_OPTIONS]
    MTOptions.ShiftMode =
        static_cast<EShiftMode>(settingsIni.GetLongValue("MT_OPTIONS", "ShiftMode", EToInt(gSettings.MTOptions.ShiftMode)));
    MTOptions.ClutchCreep = settingsIni.GetBoolValue("MT_OPTIONS", "ClutchCatching", gSettings.MTOptions.ClutchCreep);

    // [MT_PARAMS]
    MTParams.ClutchThreshold = settingsIni.GetDoubleValue("MT_PARAMS", "ClutchCatchpoint", gSettings.MTParams.ClutchThreshold);
    MTParams.StallingThreshold = settingsIni.GetDoubleValue("MT_PARAMS", "StallingThreshold", gSettings.MTParams.StallingThreshold);
    MTParams.StallingRPM = settingsIni.GetDoubleValue("MT_PARAMS", "StallingRPM", gSettings.MTParams.StallingRPM);
    MTParams.RPMDamage = settingsIni.GetDoubleValue("MT_PARAMS", "RPMDamage", gSettings.MTParams.RPMDamage);
    MTParams.MisshiftDamage = settingsIni.GetDoubleValue("MT_PARAMS", "MisshiftDamage", gSettings.MTParams.MisshiftDamage);
    MTParams.EngBrakePower = settingsIni.GetDoubleValue("MT_PARAMS", "EngBrakePower", gSettings.MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold = settingsIni.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", gSettings.MTParams.EngBrakeThreshold);

    // [DRIVING_ASSISTS]
    DriveAssists.CustomABS = settingsIni.GetBoolValue("DRIVING_ASSISTS", "CustomABS", gSettings.DriveAssists.CustomABS);
    DriveAssists.ABSFilter = settingsIni.GetBoolValue("DRIVING_ASSISTS", "ABSFilter", gSettings.DriveAssists.ABSFilter);
    DriveAssists.TCMode = settingsIni.GetLongValue("DRIVING_ASSISTS", "TractionControl", gSettings.DriveAssists.TCMode);

    // [SHIFT_OPTIONS]
    ShiftOptions.UpshiftCut = settingsIni.GetBoolValue("SHIFT_OPTIONS", "UpshiftCut", gSettings.ShiftOptions.UpshiftCut);
    ShiftOptions.DownshiftBlip = settingsIni.GetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", gSettings.ShiftOptions.DownshiftBlip);
    ShiftOptions.ClutchRateMult = settingsIni.GetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", gSettings.ShiftOptions.ClutchRateMult);
    ShiftOptions.RPMTolerance = settingsIni.GetDoubleValue("SHIFT_OPTIONS", "RPMTolerance", gSettings.ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    AutoParams.UpshiftLoad = settingsIni.GetDoubleValue("AUTO_PARAMS", "UpshiftLoad", gSettings.AutoParams.UpshiftLoad);
    AutoParams.DownshiftLoad = settingsIni.GetDoubleValue("AUTO_PARAMS", "DownshiftLoad", gSettings.AutoParams.DownshiftLoad);
    AutoParams.NextGearMinRPM = settingsIni.GetDoubleValue("AUTO_PARAMS", "NextGearMinRPM", gSettings.AutoParams.NextGearMinRPM);
    AutoParams.CurrGearMinRPM = settingsIni.GetDoubleValue("AUTO_PARAMS", "CurrGearMinRPM", gSettings.AutoParams.CurrGearMinRPM);
    AutoParams.EcoRate = settingsIni.GetDoubleValue("AUTO_PARAMS", "EcoRate", gSettings.AutoParams.EcoRate);
    AutoParams.DownshiftTimeoutMult = settingsIni.GetDoubleValue("AUTO_PARAMS", "DownshiftTimeoutMult", gSettings.AutoParams.DownshiftTimeoutMult);

    // [FORCE_FEEDBACK]
    Wheel.FFB.Enable = settingsIni.GetBoolValue("FORCE_FEEDBACK", "Enable", gSettings.Wheel.FFB.Enable);
    Wheel.FFB.Scale = settingsIni.GetBoolValue("FORCE_FEEDBACK", "Scale", gSettings.Wheel.FFB.Scale);
    Wheel.FFB.AntiDeadForce = settingsIni.GetLongValue("FORCE_FEEDBACK", "AntiDeadForce", gSettings.Wheel.FFB.AntiDeadForce);
    Wheel.FFB.SATAmpMult = settingsIni.GetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", gSettings.Wheel.FFB.SATAmpMult);
    Wheel.FFB.SATMax = settingsIni.GetLongValue("FORCE_FEEDBACK", "SATMax", gSettings.Wheel.FFB.SATMax);
    Wheel.FFB.SATFactor = settingsIni.GetDoubleValue("FORCE_FEEDBACK", "SATFactor", gSettings.Wheel.FFB.SATFactor);
    Wheel.FFB.DamperMax = settingsIni.GetLongValue("FORCE_FEEDBACK", "DamperMax", gSettings.Wheel.FFB.DamperMax);
    Wheel.FFB.DamperMin = settingsIni.GetLongValue("FORCE_FEEDBACK", "DamperMin", gSettings.Wheel.FFB.DamperMin); ;
    Wheel.FFB.DamperMinSpeed = settingsIni.GetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", gSettings.Wheel.FFB.DamperMinSpeed);
    Wheel.FFB.DetailMult = settingsIni.GetDoubleValue("FORCE_FEEDBACK", "DetailMult", gSettings.Wheel.FFB.DetailMult);
    Wheel.FFB.DetailLim = settingsIni.GetLongValue("FORCE_FEEDBACK", "DetailLim", gSettings.Wheel.FFB.DetailLim);
    Wheel.FFB.DetailMAW = settingsIni.GetLongValue("FORCE_FEEDBACK", "DetailMaw", gSettings.Wheel.FFB.DetailMAW);
    Wheel.FFB.CollisionMult = settingsIni.GetDoubleValue("FORCE_FEEDBACK", "CollisionMult", gSettings.Wheel.FFB.CollisionMult);

    // [STEER]
    Wheel.Steering.Angle = settingsIni.GetDoubleValue("STEER", "Steer", gSettings.Wheel.Steering.AngleCar);
}
#pragma warning(pop)
