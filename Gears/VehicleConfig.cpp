#include "VehicleConfig.h"

#include "SettingsCommon.h"
#include "Util/Logger.hpp"
#include "Util/Strings.hpp"
#include <fmt/format.h>
#include <simpleini/SimpleIni.h>
#include <filesystem>
#include <cctype>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[VehicleConfig] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
    }

#define SAVE_VAL(section, key, option) \
    if (mBaseConfig == this || option != mBaseConfig->option || option.Changed()) { \
        SetValue(ini, section, key, option); \
        option.Reset(); \
    }

#define LOAD_VAL(section, key, option) \
    option.Set(GetValue(ini, section, key, baseConfig.option))

#define SAVE_VAL_MOVEMENT(prefix, source) { \
    SAVE_VAL("CAM", prefix "FollowMovement",    ##source.Follow); \
    SAVE_VAL("CAM", prefix "MovementMultVel",   ##source.RotationDirectionMult); \
    SAVE_VAL("CAM", prefix "MovementMultRot",   ##source.RotationRotationMult); \
    SAVE_VAL("CAM", prefix "MovementCap",       ##source.RotationMaxAngle); \
    SAVE_VAL("CAM", prefix "LongForwardMult",   ##source.LongForwardMult); \
    SAVE_VAL("CAM", prefix "LongBackwardMult",  ##source.LongBackwardMult); \
    SAVE_VAL("CAM", prefix "LongDeadzone",      ##source.LongDeadzone); \
    SAVE_VAL("CAM", prefix "LongGamma",         ##source.LongGamma); \
    SAVE_VAL("CAM", prefix "LongForwardLimit",  ##source.LongForwardLimit); \
    SAVE_VAL("CAM", prefix "LongBackwardLimit", ##source.LongBackwardLimit); \
    SAVE_VAL("CAM", prefix "PitchUpMult",       ##source.PitchUpMult); \
    SAVE_VAL("CAM", prefix "PitchDownMult",     ##source.PitchDownMult); \
    SAVE_VAL("CAM", prefix "PitchDeadzone",     ##source.PitchDeadzone); \
    SAVE_VAL("CAM", prefix "PitchUpMaxAngle",   ##source.PitchUpMaxAngle); \
    SAVE_VAL("CAM", prefix "PitchDownMaxAngle", ##source.PitchDownMaxAngle); \
}

#define LOAD_VAL_MOVEMENT(prefix, source) { \
    LOAD_VAL("CAM", prefix "FollowMovement",    ##source.Follow); \
    LOAD_VAL("CAM", prefix "MovementMultVel",   ##source.RotationDirectionMult); \
    LOAD_VAL("CAM", prefix "MovementMultRot",   ##source.RotationRotationMult); \
    LOAD_VAL("CAM", prefix "MovementCap",       ##source.RotationMaxAngle); \
    LOAD_VAL("CAM", prefix "LongForwardMult",   ##source.LongForwardMult); \
    LOAD_VAL("CAM", prefix "LongBackwardMult",  ##source.LongBackwardMult); \
    LOAD_VAL("CAM", prefix "LongDeadzone",      ##source.LongDeadzone); \
    LOAD_VAL("CAM", prefix "LongGamma",         ##source.LongGamma); \
    LOAD_VAL("CAM", prefix "LongForwardLimit",  ##source.LongForwardLimit); \
    LOAD_VAL("CAM", prefix "LongBackwardLimit", ##source.LongBackwardLimit); \
    LOAD_VAL("CAM", prefix "PitchUpMult",       ##source.PitchUpMult); \
    LOAD_VAL("CAM", prefix "PitchDownMult",     ##source.PitchDownMult); \
    LOAD_VAL("CAM", prefix "PitchDeadzone",     ##source.PitchDeadzone); \
    LOAD_VAL("CAM", prefix "PitchUpMaxAngle",   ##source.PitchUpMaxAngle); \
    LOAD_VAL("CAM", prefix "PitchDownMaxAngle", ##source.PitchDownMaxAngle); \
}

EShiftMode Next(EShiftMode mode) {
    return static_cast<EShiftMode>((static_cast<int>(mode) + 1) % 3);
}

VehicleConfig::VehicleConfig() : mBaseConfig(nullptr) { }

void VehicleConfig::SetFiles(VehicleConfig* baseConfig, const std::string& file) {
    mBaseConfig = baseConfig;
    mFile = file;
}

#pragma warning(push)
#pragma warning(disable: 4244)
void VehicleConfig::LoadSettings() {
    VehicleConfig* pConfig = mBaseConfig;

    // Current instance is the base config.
    if (!mBaseConfig) {
        pConfig = this;
    }

    auto& baseConfig = *pConfig;

    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mFile.c_str());
    CHECK_LOG_SI_ERROR(result, fmt::format("load {}", mFile).c_str());

    Name = std::filesystem::path(mFile).stem().string();

    // [ID]
    std::string allNames = ini.GetValue("ID", "ModelName", "");
    std::string allPlates = ini.GetValue("ID", "Plate", "");
    ModelNames = StrUtil::split(allNames, ' ');

    Plates = StrUtil::split(allPlates, ',');
    for (auto& plate : Plates) {
        auto first = plate.find_first_of('[');
        auto last = plate.find_last_of(']');

        if (first == std::string::npos ||
            last == std::string::npos) {
            plate = "DELETE_THIS_PLATE";
            continue;
        }

        first += 1;
        plate = plate.substr(first, last - first);
        plate.erase(std::find_if(plate.rbegin(), plate.rend(), [](int ch) {
            return !std::isspace(ch);
            }).base(), plate.end());
    }

    Plates.erase(std::remove_if(
        Plates.begin(), Plates.end(),
        [](const auto& elem) -> bool { return elem == "DELETE_THIS_PLATE"; }
    ), Plates.end());

    Description = ini.GetValue("ID", "Description", "No description.");

    // [MT_OPTIONS]
    MTOptions.ShiftMode.Set(static_cast<EShiftMode>(ini.GetLongValue("MT_OPTIONS", "ShiftMode", baseConfig.MTOptions.ShiftMode)));
    LOAD_VAL("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    LOAD_VAL("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    LOAD_VAL("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    LOAD_VAL("MT_OPTIONS", "SpeedLimiter", MTOptions.SpeedLimiter.Enable);
    LOAD_VAL("MT_OPTIONS", "SpeedLimiterSpeed", MTOptions.SpeedLimiter.Speed);

    // [MT_PARAMS]
    LOAD_VAL("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    LOAD_VAL("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    LOAD_VAL("MT_PARAMS", "StallingRate", MTParams.StallingRate);
    LOAD_VAL("MT_PARAMS", "StallingSlip", MTParams.StallingSlip);
    LOAD_VAL("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    LOAD_VAL("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    LOAD_VAL("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    LOAD_VAL("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);
    LOAD_VAL("MT_PARAMS", "CreepIdleThrottle", MTParams.CreepIdleThrottle);
    LOAD_VAL("MT_PARAMS", "CreepIdleRPM", MTParams.CreepIdleRPM);

    // [DRIVING_ASSISTS]
    LOAD_VAL("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    LOAD_VAL("DRIVING_ASSISTS", "ABSFlash", DriveAssists.ABS.Flash);

    LOAD_VAL("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "TCSMode", DriveAssists.TCS.Mode);
    LOAD_VAL("DRIVING_ASSISTS", "TCSSlipMin", DriveAssists.TCS.SlipMin);
    LOAD_VAL("DRIVING_ASSISTS", "TCSSlipMax", DriveAssists.TCS.SlipMax);

    LOAD_VAL("DRIVING_ASSISTS", "ESP", DriveAssists.ESP.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "ESPOverMin", DriveAssists.ESP.OverMin);
    LOAD_VAL("DRIVING_ASSISTS", "ESPOverMax", DriveAssists.ESP.OverMax);
    LOAD_VAL("DRIVING_ASSISTS", "ESPOverMinComp", DriveAssists.ESP.OverMinComp);
    LOAD_VAL("DRIVING_ASSISTS", "ESPOverMaxComp", DriveAssists.ESP.OverMaxComp);
    LOAD_VAL("DRIVING_ASSISTS", "ESPUnderMin", DriveAssists.ESP.UnderMin);
    LOAD_VAL("DRIVING_ASSISTS", "ESPUnderMax", DriveAssists.ESP.UnderMax);
    LOAD_VAL("DRIVING_ASSISTS", "ESPUnderMinComp", DriveAssists.ESP.UnderMinComp);
    LOAD_VAL("DRIVING_ASSISTS", "ESPUnderMaxComp", DriveAssists.ESP.UnderMaxComp);

    LOAD_VAL("DRIVING_ASSISTS", "LSD", DriveAssists.LSD.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "LSDViscosity", DriveAssists.LSD.Viscosity);

    LOAD_VAL("DRIVING_ASSISTS", "AWD", DriveAssists.AWD.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "AWDBiasAtMaxTransfer", DriveAssists.AWD.BiasAtMaxTransfer);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUseCustomBaseBias", DriveAssists.AWD.UseCustomBaseBias);
    LOAD_VAL("DRIVING_ASSISTS", "AWDCustomBaseBias", DriveAssists.AWD.CustomBaseBias);
    LOAD_VAL("DRIVING_ASSISTS", "AWDCustomMin", DriveAssists.AWD.CustomMin);
    LOAD_VAL("DRIVING_ASSISTS", "AWDCustomMax", DriveAssists.AWD.CustomMax);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUseTraction", DriveAssists.AWD.UseTraction);
    LOAD_VAL("DRIVING_ASSISTS", "AWDTractionLossMin", DriveAssists.AWD.TractionLossMin);
    LOAD_VAL("DRIVING_ASSISTS", "AWDTractionLossMax", DriveAssists.AWD.TractionLossMax);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUseOversteer", DriveAssists.AWD.UseOversteer);
    LOAD_VAL("DRIVING_ASSISTS", "AWDOversteerMin", DriveAssists.AWD.OversteerMin);
    LOAD_VAL("DRIVING_ASSISTS", "AWDOversteerMax", DriveAssists.AWD.OversteerMax);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUseUndersteer", DriveAssists.AWD.UseUndersteer);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUndersteerMin", DriveAssists.AWD.UndersteerMin);
    LOAD_VAL("DRIVING_ASSISTS", "AWDUndersteerMax", DriveAssists.AWD.UndersteerMax);
    DriveAssists.AWD.SpecialFlags.Set(ini.GetLongValue("DRIVING_ASSISTS", "AWDSpecialFlags", baseConfig.DriveAssists.AWD.SpecialFlags));

    LOAD_VAL("DRIVING_ASSISTS", "LaunchControl", DriveAssists.LaunchControl.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "LaunchControlRPM", DriveAssists.LaunchControl.RPM);
    LOAD_VAL("DRIVING_ASSISTS", "LaunchControlSlipMin", DriveAssists.LaunchControl.SlipMin);
    LOAD_VAL("DRIVING_ASSISTS", "LaunchControlSlipMax", DriveAssists.LaunchControl.SlipMax);

    LOAD_VAL("DRIVING_ASSISTS", "CruiseControl", DriveAssists.CruiseControl.Enable);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlSpeed", DriveAssists.CruiseControl.Speed);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMaxAcceleration", DriveAssists.CruiseControl.MaxAcceleration);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlAdaptive", DriveAssists.CruiseControl.Adaptive);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMinFollowDistance", DriveAssists.CruiseControl.MinFollowDistance);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMaxFollowDistance", DriveAssists.CruiseControl.MaxFollowDistance);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMinDistanceSpeedMult", DriveAssists.CruiseControl.MinDistanceSpeedMult);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMaxDistanceSpeedMult", DriveAssists.CruiseControl.MaxDistanceSpeedMult);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMinDeltaBrakeMult", DriveAssists.CruiseControl.MinDeltaBrakeMult);
    LOAD_VAL("DRIVING_ASSISTS", "CruiseControlMaxDeltaBrakeMult", DriveAssists.CruiseControl.MaxDeltaBrakeMult);

    // [SHIFT_OPTIONS]
    LOAD_VAL("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    LOAD_VAL("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    LOAD_VAL("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    LOAD_VAL("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    LOAD_VAL("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    LOAD_VAL("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    LOAD_VAL("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    LOAD_VAL("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    LOAD_VAL("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    LOAD_VAL("AUTO_PARAMS", "UpshiftTimeoutMult", AutoParams.UpshiftTimeoutMult);
    LOAD_VAL("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);
    LOAD_VAL("AUTO_PARAMS", "UsingATCU", AutoParams.UsingATCU);

    //[STEERING_OVERRIDE]
    LOAD_VAL("STEERING", "CSUseCustomLock", Steering.CustomSteering.UseCustomLock);
    LOAD_VAL("STEERING", "CSSoftLock", Steering.CustomSteering.SoftLock);
    LOAD_VAL("STEERING", "CSSteeringMult", Steering.CustomSteering.SteeringMult);

    LOAD_VAL("STEERING", "WSoftLock", Steering.Wheel.SoftLock);
    LOAD_VAL("STEERING", "WSteeringMult", Steering.Wheel.SteeringMult);

    // [CAM]
    LOAD_VAL("CAM", "Enable", Misc.Camera.Enable);
    LOAD_VAL("CAM", "AttachId", Misc.Camera.AttachId);

    LOAD_VAL("CAM", "RemoveHead", Misc.Camera.RemoveHead);
    LOAD_VAL("CAM", "LookTime", Misc.Camera.LookTime);
    LOAD_VAL("CAM", "MouseLookTime", Misc.Camera.MouseLookTime);
    LOAD_VAL("CAM", "MouseCenterTimeout", Misc.Camera.MouseCenterTimeout);
    LOAD_VAL("CAM", "MouseSensitivity", Misc.Camera.MouseSensitivity);

    LOAD_VAL("CAM", "PedFOV", Misc.Camera.Ped.FOV);
    LOAD_VAL("CAM", "PedOffsetHeight", Misc.Camera.Ped.OffsetHeight);
    LOAD_VAL("CAM", "PedOffsetForward", Misc.Camera.Ped.OffsetForward);
    LOAD_VAL("CAM", "PedOffsetSide", Misc.Camera.Ped.OffsetSide);
    LOAD_VAL("CAM", "PedPitch", Misc.Camera.Ped.Pitch);

    LOAD_VAL_MOVEMENT("Ped", Misc.Camera.Ped.Movement);

    LOAD_VAL("CAM", "Vehicle1FOV", Misc.Camera.Vehicle1.FOV);
    LOAD_VAL("CAM", "Vehicle1OffsetHeight", Misc.Camera.Vehicle1.OffsetHeight);
    LOAD_VAL("CAM", "Vehicle1OffsetForward", Misc.Camera.Vehicle1.OffsetForward);
    LOAD_VAL("CAM", "Vehicle1OffsetSide", Misc.Camera.Vehicle1.OffsetSide);
    LOAD_VAL("CAM", "Vehicle1Pitch", Misc.Camera.Vehicle1.Pitch);

    LOAD_VAL_MOVEMENT("Vehicle1", Misc.Camera.Vehicle1.Movement);

    LOAD_VAL("CAM", "Vehicle2FOV", Misc.Camera.Vehicle2.FOV);
    LOAD_VAL("CAM", "Vehicle2OffsetHeight", Misc.Camera.Vehicle2.OffsetHeight);
    LOAD_VAL("CAM", "Vehicle2OffsetForward", Misc.Camera.Vehicle2.OffsetForward);
    LOAD_VAL("CAM", "Vehicle2OffsetSide", Misc.Camera.Vehicle2.OffsetSide);
    LOAD_VAL("CAM", "Vehicle2Pitch", Misc.Camera.Vehicle2.Pitch);

    LOAD_VAL_MOVEMENT("Vehicle2", Misc.Camera.Vehicle2.Movement);

    LOAD_VAL("CAM", "BikeDisable", Misc.Camera.Bike.Disable);
    LOAD_VAL("CAM", "BikeAttachId", Misc.Camera.Bike.AttachId);
    LOAD_VAL("CAM", "BikeFOV", Misc.Camera.Bike.FOV);
    LOAD_VAL("CAM", "BikeOffsetHeight", Misc.Camera.Bike.OffsetHeight);
    LOAD_VAL("CAM", "BikeOffsetForward", Misc.Camera.Bike.OffsetForward);
    LOAD_VAL("CAM", "BikeOffsetSide", Misc.Camera.Bike.OffsetSide);
    LOAD_VAL("CAM", "BikePitch", Misc.Camera.Bike.Pitch);

    LOAD_VAL_MOVEMENT("Bike", Misc.Camera.Bike.Movement);
}

void VehicleConfig::SaveSettings() {
    saveGeneral();
}

void VehicleConfig::SaveSettings(VehicleConfig* baseConfig, const std::string& customPath) {
    std::string oldPath = mFile;
    mFile = customPath;

    VehicleConfig* oldConfig = mBaseConfig;
    mBaseConfig = baseConfig;

    saveGeneral();

    mBaseConfig = oldConfig;
    mFile = oldPath;
}

void VehicleConfig::saveGeneral() {
    if (!mBaseConfig)
        logger.Write(FATAL, "VehicleConfig::mBaseConfig not set. Skipping save!");

    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mFile.c_str());
    CHECK_LOG_SI_ERROR(result, fmt::format("load {}", mFile).c_str());

    // [ID]
    std::string modelNames = fmt::format("{}", fmt::join(ModelNames, " "));
    ini.SetValue("ID", "ModelName", modelNames.c_str());

    std::vector<std::string> fmtPlates;
    for(const auto& plate : Plates) {
        fmtPlates.push_back(fmt::format("[{}]", plate));
    }

    std::string plates = fmt::format("{}", fmt::join(fmtPlates, ", "));
    ini.SetValue("ID", "Plate", plates.c_str());

    ini.SetValue("ID", "Description", Description.c_str());

    // [MT_OPTIONS]
    SAVE_VAL("MT_OPTIONS", "ShiftMode", MTOptions.ShiftMode);
    SAVE_VAL("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    SAVE_VAL("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    SAVE_VAL("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);
    SAVE_VAL("MT_OPTIONS", "SpeedLimiter", MTOptions.SpeedLimiter.Enable);
    SAVE_VAL("MT_OPTIONS", "SpeedLimiterSpeed", MTOptions.SpeedLimiter.Speed);

    // [MT_PARAMS]
    SAVE_VAL("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    SAVE_VAL("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    SAVE_VAL("MT_PARAMS", "StallingRate", MTParams.StallingRate);
    SAVE_VAL("MT_PARAMS", "StallingSlip", MTParams.StallingSlip);
    SAVE_VAL("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    SAVE_VAL("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    SAVE_VAL("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    SAVE_VAL("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);
    SAVE_VAL("MT_PARAMS", "CreepIdleThrottle", MTParams.CreepIdleThrottle);
    SAVE_VAL("MT_PARAMS", "CreepIdleRPM", MTParams.CreepIdleRPM);

    // [DRIVING_ASSISTS]
    SAVE_VAL("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    SAVE_VAL("DRIVING_ASSISTS", "ABSFlash", DriveAssists.ABS.Flash);

    SAVE_VAL("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "TCSMode", DriveAssists.TCS.Mode);
    SAVE_VAL("DRIVING_ASSISTS", "TCSSlipMin", DriveAssists.TCS.SlipMin);
    SAVE_VAL("DRIVING_ASSISTS", "TCSSlipMax", DriveAssists.TCS.SlipMax);
    
    SAVE_VAL("DRIVING_ASSISTS", "ESP", DriveAssists.ESP.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "ESPOverMin", DriveAssists.ESP.OverMin);
    SAVE_VAL("DRIVING_ASSISTS", "ESPOverMax", DriveAssists.ESP.OverMax);
    SAVE_VAL("DRIVING_ASSISTS", "ESPOverMinComp", DriveAssists.ESP.OverMinComp);
    SAVE_VAL("DRIVING_ASSISTS", "ESPOverMaxComp", DriveAssists.ESP.OverMaxComp);
    SAVE_VAL("DRIVING_ASSISTS", "ESPUnderMin", DriveAssists.ESP.UnderMin);
    SAVE_VAL("DRIVING_ASSISTS", "ESPUnderMax", DriveAssists.ESP.UnderMax);
    SAVE_VAL("DRIVING_ASSISTS", "ESPUnderMinComp", DriveAssists.ESP.UnderMinComp);
    SAVE_VAL("DRIVING_ASSISTS", "ESPUnderMaxComp", DriveAssists.ESP.UnderMaxComp);

    SAVE_VAL("DRIVING_ASSISTS", "LSD", DriveAssists.LSD.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "LSDViscosity", DriveAssists.LSD.Viscosity);

    SAVE_VAL("DRIVING_ASSISTS", "AWD", DriveAssists.AWD.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "AWDBiasAtMaxTransfer", DriveAssists.AWD.BiasAtMaxTransfer);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUseCustomBaseBias", DriveAssists.AWD.UseCustomBaseBias);
    SAVE_VAL("DRIVING_ASSISTS", "AWDCustomBaseBias", DriveAssists.AWD.CustomBaseBias);
    SAVE_VAL("DRIVING_ASSISTS", "AWDCustomMin", DriveAssists.AWD.CustomMin);
    SAVE_VAL("DRIVING_ASSISTS", "AWDCustomMax", DriveAssists.AWD.CustomMax);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUseTraction", DriveAssists.AWD.UseTraction);
    SAVE_VAL("DRIVING_ASSISTS", "AWDTractionLossMin", DriveAssists.AWD.TractionLossMin);
    SAVE_VAL("DRIVING_ASSISTS", "AWDTractionLossMax", DriveAssists.AWD.TractionLossMax);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUseOversteer", DriveAssists.AWD.UseOversteer);
    SAVE_VAL("DRIVING_ASSISTS", "AWDOversteerMin", DriveAssists.AWD.OversteerMin);
    SAVE_VAL("DRIVING_ASSISTS", "AWDOversteerMax", DriveAssists.AWD.OversteerMax);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUseUndersteer", DriveAssists.AWD.UseUndersteer);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUndersteerMin", DriveAssists.AWD.UndersteerMin);
    SAVE_VAL("DRIVING_ASSISTS", "AWDUndersteerMax", DriveAssists.AWD.UndersteerMax);

    if (mBaseConfig == this || DriveAssists.AWD.SpecialFlags != mBaseConfig->DriveAssists.AWD.SpecialFlags || DriveAssists.AWD.SpecialFlags.Changed()) {
        ini.SetLongValue("DRIVING_ASSISTS", "AWDSpecialFlags", DriveAssists.AWD.SpecialFlags, nullptr, true);
        DriveAssists.AWD.SpecialFlags.Reset();
    }

    SAVE_VAL("DRIVING_ASSISTS", "LaunchControl", DriveAssists.LaunchControl.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "LaunchControlRPM", DriveAssists.LaunchControl.RPM);
    SAVE_VAL("DRIVING_ASSISTS", "LaunchControlSlipMin", DriveAssists.LaunchControl.SlipMin);
    SAVE_VAL("DRIVING_ASSISTS", "LaunchControlSlipMax", DriveAssists.LaunchControl.SlipMax);

    SAVE_VAL("DRIVING_ASSISTS", "CruiseControl", DriveAssists.CruiseControl.Enable);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlSpeed", DriveAssists.CruiseControl.Speed);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMaxAcceleration", DriveAssists.CruiseControl.MaxAcceleration);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlAdaptive", DriveAssists.CruiseControl.Adaptive);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMinFollowDistance", DriveAssists.CruiseControl.MinFollowDistance);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMaxFollowDistance", DriveAssists.CruiseControl.MaxFollowDistance);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMinDistanceSpeedMult", DriveAssists.CruiseControl.MinDistanceSpeedMult);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMaxDistanceSpeedMult", DriveAssists.CruiseControl.MaxDistanceSpeedMult);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMinDeltaBrakeMult", DriveAssists.CruiseControl.MinDeltaBrakeMult);
    SAVE_VAL("DRIVING_ASSISTS", "CruiseControlMaxDeltaBrakeMult", DriveAssists.CruiseControl.MaxDeltaBrakeMult);

    //[STEERING_OVERRIDE]
    SAVE_VAL("STEERING", "CSUseCustomLock", Steering.CustomSteering.UseCustomLock);
    SAVE_VAL("STEERING", "CSSoftLock", Steering.CustomSteering.SoftLock);
    SAVE_VAL("STEERING", "CSSteeringMult", Steering.CustomSteering.SteeringMult);

    SAVE_VAL("STEERING", "WSoftLock", Steering.Wheel.SoftLock);
    SAVE_VAL("STEERING", "WSteeringMult", Steering.Wheel.SteeringMult);

    // [SHIFT_OPTIONS]
    SAVE_VAL("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    SAVE_VAL("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    SAVE_VAL("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    SAVE_VAL("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    SAVE_VAL("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    SAVE_VAL("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    SAVE_VAL("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    SAVE_VAL("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    SAVE_VAL("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    SAVE_VAL("AUTO_PARAMS", "UpshiftTimeoutMult", AutoParams.UpshiftTimeoutMult);
    SAVE_VAL("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);
    SAVE_VAL("AUTO_PARAMS", "UsingATCU", AutoParams.UsingATCU);

    // [CAM]
    SAVE_VAL("CAM", "Enable", Misc.Camera.Enable);
    SAVE_VAL("CAM", "AttachId", Misc.Camera.AttachId);

    SAVE_VAL("CAM", "RemoveHead", Misc.Camera.RemoveHead);
    SAVE_VAL("CAM", "LookTime", Misc.Camera.LookTime);
    SAVE_VAL("CAM", "MouseLookTime", Misc.Camera.MouseLookTime);
    SAVE_VAL("CAM", "MouseCenterTimeout", Misc.Camera.MouseCenterTimeout);
    SAVE_VAL("CAM", "MouseSensitivity", Misc.Camera.MouseSensitivity);

    SAVE_VAL("CAM", "PedFOV", Misc.Camera.Ped.FOV);
    SAVE_VAL("CAM", "PedOffsetHeight", Misc.Camera.Ped.OffsetHeight);
    SAVE_VAL("CAM", "PedOffsetForward", Misc.Camera.Ped.OffsetForward);
    SAVE_VAL("CAM", "PedOffsetSide", Misc.Camera.Ped.OffsetSide);
    SAVE_VAL("CAM", "PedPitch", Misc.Camera.Ped.Pitch);

    SAVE_VAL_MOVEMENT("Ped", Misc.Camera.Ped.Movement);

    SAVE_VAL("CAM", "Vehicle1FOV", Misc.Camera.Vehicle1.FOV);
    SAVE_VAL("CAM", "Vehicle1OffsetHeight", Misc.Camera.Vehicle1.OffsetHeight);
    SAVE_VAL("CAM", "Vehicle1OffsetForward", Misc.Camera.Vehicle1.OffsetForward);
    SAVE_VAL("CAM", "Vehicle1OffsetSide", Misc.Camera.Vehicle1.OffsetSide);
    SAVE_VAL("CAM", "Vehicle1Pitch", Misc.Camera.Vehicle1.Pitch);

    SAVE_VAL_MOVEMENT("Vehicle1", Misc.Camera.Vehicle1.Movement);

    SAVE_VAL("CAM", "Vehicle2FOV", Misc.Camera.Vehicle2.FOV);
    SAVE_VAL("CAM", "Vehicle2OffsetHeight", Misc.Camera.Vehicle2.OffsetHeight);
    SAVE_VAL("CAM", "Vehicle2OffsetForward", Misc.Camera.Vehicle2.OffsetForward);
    SAVE_VAL("CAM", "Vehicle2OffsetSide", Misc.Camera.Vehicle2.OffsetSide);
    SAVE_VAL("CAM", "Vehicle2Pitch", Misc.Camera.Vehicle2.Pitch);

    SAVE_VAL_MOVEMENT("Vehicle2", Misc.Camera.Vehicle2.Movement);

    SAVE_VAL("CAM", "BikeDisable", Misc.Camera.Bike.Disable);
    SAVE_VAL("CAM", "BikeAttachId", Misc.Camera.Bike.AttachId);
    SAVE_VAL("CAM", "BikeFOV", Misc.Camera.Bike.FOV);
    SAVE_VAL("CAM", "BikeOffsetHeight", Misc.Camera.Bike.OffsetHeight);
    SAVE_VAL("CAM", "BikeOffsetForward", Misc.Camera.Bike.OffsetForward);
    SAVE_VAL("CAM", "BikeOffsetSide", Misc.Camera.Bike.OffsetSide);
    SAVE_VAL("CAM", "BikePitch", Misc.Camera.Bike.Pitch);

    SAVE_VAL_MOVEMENT("Bike", Misc.Camera.Bike.Movement);

    result = ini.SaveFile(mFile.c_str());
    CHECK_LOG_SI_ERROR(result, fmt::format("save {}", mFile).c_str());
}

#pragma warning(pop)
