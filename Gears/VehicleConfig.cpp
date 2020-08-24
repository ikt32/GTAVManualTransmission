#include "VehicleConfig.h"
#include "Util/Logger.hpp"
#include "Util/Strings.hpp"
#include <fmt/format.h>
#include <simpleini/SimpleIni.h>
#include <filesystem>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[VehicleConfig] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
    }

#define SET_VALUE(section, key, option) \
    if (mBaseConfig == this || option != mBaseConfig->option) { \
        SetValue(ini, section, key, option); \
    }

void SetValue(CSimpleIniA & ini, const char* section, const char* key, int val) {
    ini.SetLongValue(section, key, val);
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, const std::string & val) {
    ini.SetValue(section, key, val.c_str());
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, bool val) {
    ini.SetBoolValue(section, key, val);
}

void SetValue(CSimpleIniA & ini, const char* section, const char* key, float val) {
    ini.SetDoubleValue(section, key, static_cast<double>(val));
}

EShiftMode Next(EShiftMode mode) {
    return static_cast<EShiftMode>((static_cast<int>(mode) + 1) % 3);
}

VehicleConfig::VehicleConfig() : mBaseConfig(nullptr) { }

VehicleConfig::VehicleConfig(VehicleConfig* baseConfig, const std::string& file)
    : mBaseConfig(baseConfig) {
    mFile = file;
    LoadSettings(file);
}

#pragma warning(push)
#pragma warning(disable: 4244)
void VehicleConfig::LoadSettings(const std::string& file) {
    VehicleConfig* pConfig = mBaseConfig;

    // Current instance is the base config.
    if (!mBaseConfig) {
        pConfig = this;
    }

    const auto& gSettings = *pConfig;

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
        static_cast<EShiftMode>(ini.GetLongValue("MT_OPTIONS", "ShiftMode", static_cast<int>(gSettings.MTOptions.ShiftMode)));
    MTOptions.ClutchCreep = ini.GetBoolValue("MT_OPTIONS", "ClutchCatching", gSettings.MTOptions.ClutchCreep);
    MTOptions.ClutchShiftH = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingH", gSettings.MTOptions.ClutchShiftH);
    MTOptions.ClutchShiftS = ini.GetBoolValue("MT_OPTIONS", "ClutchShiftingS", gSettings.MTOptions.ClutchShiftS);

    // [MT_PARAMS]
    MTParams.ClutchThreshold = ini.GetDoubleValue("MT_PARAMS", "ClutchCatchpoint", gSettings.MTParams.ClutchThreshold);
    MTParams.StallingRPM = ini.GetDoubleValue("MT_PARAMS", "StallingRPM", gSettings.MTParams.StallingRPM);
    MTParams.StallingRate = ini.GetDoubleValue("MT_PARAMS", "StallingRate", gSettings.MTParams.StallingRate);
    MTParams.StallingSlip = ini.GetDoubleValue("MT_PARAMS", "StallingSlip", gSettings.MTParams.StallingSlip);
    MTParams.RPMDamage = ini.GetDoubleValue("MT_PARAMS", "RPMDamage", gSettings.MTParams.RPMDamage);
    MTParams.MisshiftDamage = ini.GetDoubleValue("MT_PARAMS", "MisshiftDamage", gSettings.MTParams.MisshiftDamage);
    MTParams.EngBrakePower = ini.GetDoubleValue("MT_PARAMS", "EngBrakePower", gSettings.MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold = ini.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", gSettings.MTParams.EngBrakeThreshold);
    MTParams.CreepIdleThrottle = ini.GetDoubleValue("MT_PARAMS", "CreepIdleThrottle", gSettings.MTParams.CreepIdleThrottle);
    MTParams.CreepIdleRPM = ini.GetDoubleValue("MT_PARAMS", "CreepIdleRPM", gSettings.MTParams.CreepIdleRPM);

    // [DRIVING_ASSISTS]
    DriveAssists.ABS.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "ABS", gSettings.DriveAssists.ABS.Enable);
    DriveAssists.ABS.Filter = ini.GetBoolValue("DRIVING_ASSISTS", "ABSFilter", gSettings.DriveAssists.ABS.Filter);
    DriveAssists.TCS.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "TCS", gSettings.DriveAssists.TCS.Enable);
    DriveAssists.TCS.Mode = ini.GetLongValue("DRIVING_ASSISTS", "TCSMode", gSettings.DriveAssists.TCS.Mode);
    DriveAssists.TCS.SlipMax = ini.GetDoubleValue("DRIVING_ASSISTS", "TCSSlipMax", gSettings.DriveAssists.TCS.SlipMax);
    DriveAssists.ESP.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "ESP", gSettings.DriveAssists.ESP.Enable);
    DriveAssists.ESP.OverMin = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMin", gSettings.DriveAssists.ESP.OverMin);
    DriveAssists.ESP.OverMax = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMax", gSettings.DriveAssists.ESP.OverMax);
    DriveAssists.ESP.OverMinComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMinComp", gSettings.DriveAssists.ESP.OverMinComp);
    DriveAssists.ESP.OverMaxComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPOverMaxComp", gSettings.DriveAssists.ESP.OverMaxComp);
    DriveAssists.ESP.UnderMin = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMin", gSettings.DriveAssists.ESP.UnderMin);
    DriveAssists.ESP.UnderMax = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMax", gSettings.DriveAssists.ESP.UnderMax);
    DriveAssists.ESP.UnderMinComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMinComp", gSettings.DriveAssists.ESP.UnderMinComp);
    DriveAssists.ESP.UnderMaxComp = ini.GetDoubleValue("DRIVING_ASSISTS", "ESPUnderMaxComp", gSettings.DriveAssists.ESP.UnderMaxComp);

    DriveAssists.LSD.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "LSD", gSettings.DriveAssists.LSD.Enable);
    DriveAssists.LSD.Viscosity = ini.GetDoubleValue("DRIVING_ASSISTS", "LSDViscosity", gSettings.DriveAssists.LSD.Viscosity);

    DriveAssists.AWD.Enable            = ini.GetBoolValue("DRIVING_ASSISTS",   "AWD", gSettings.DriveAssists.AWD.Enable);
    DriveAssists.AWD.BiasAtMaxTransfer = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDBiasAtMaxTransfer", gSettings.DriveAssists.AWD.BiasAtMaxTransfer);
    DriveAssists.AWD.UseCustomBaseBias = ini.GetBoolValue("DRIVING_ASSISTS",   "AWDUseCustomBaseBias", gSettings.DriveAssists.AWD.UseCustomBaseBias);
    DriveAssists.AWD.CustomBaseBias    = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDCustomBaseBias", gSettings.DriveAssists.AWD.CustomBaseBias);
    DriveAssists.AWD.CustomMin         = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDCustomMin", gSettings.DriveAssists.AWD.CustomMin);
    DriveAssists.AWD.CustomMax         = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDCustomMax", gSettings.DriveAssists.AWD.CustomMax);
    DriveAssists.AWD.UseTraction       = ini.GetBoolValue("DRIVING_ASSISTS",   "AWDUseTraction", gSettings.DriveAssists.AWD.UseTraction);
    DriveAssists.AWD.TractionLossMin   = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDTractionLossMin", gSettings.DriveAssists.AWD.TractionLossMin);
    DriveAssists.AWD.TractionLossMax   = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDTractionLossMax", gSettings.DriveAssists.AWD.TractionLossMax);
    DriveAssists.AWD.UseOversteer      = ini.GetBoolValue("DRIVING_ASSISTS",   "AWDUseOversteer", gSettings.DriveAssists.AWD.UseOversteer);
    DriveAssists.AWD.OversteerMin      = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDOversteerMin", gSettings.DriveAssists.AWD.OversteerMin);
    DriveAssists.AWD.OversteerMax      = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDOversteerMax", gSettings.DriveAssists.AWD.OversteerMax);
    DriveAssists.AWD.UseUndersteer     = ini.GetBoolValue("DRIVING_ASSISTS",   "AWDUseUndersteer", gSettings.DriveAssists.AWD.UseUndersteer);
    DriveAssists.AWD.UndersteerMin     = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDUndersteerMin", gSettings.DriveAssists.AWD.UndersteerMin);
    DriveAssists.AWD.UndersteerMax     = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDUndersteerMax", gSettings.DriveAssists.AWD.UndersteerMax);
    DriveAssists.AWD.SpecialFlags      = ini.GetLongValue("DRIVING_ASSISTS",   "AWDSpecialFlags", gSettings.DriveAssists.AWD.SpecialFlags);

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
    AutoParams.UsingATCU = ini.GetBoolValue("AUTO_PARAMS", "UsingATCU", gSettings.AutoParams.UsingATCU);

    //[STEERING_OVERRIDE]
    SteeringOverride.UseForCustomSteering = ini.GetBoolValue("STEERING_OVERRIDE", "UseForCustomSteering", SteeringOverride.UseForCustomSteering);
    SteeringOverride.SoftLockCustomSteering = ini.GetDoubleValue("STEERING_OVERRIDE", "SoftLockCustomSteering", SteeringOverride.SoftLockCustomSteering);
    SteeringOverride.SoftLockWheelInput = ini.GetDoubleValue("STEERING_OVERRIDE", "SoftLockWheelInput", SteeringOverride.SoftLockWheelInput);

    // [CAM]
    Misc.Camera.Enable = ini.GetBoolValue("CAM", "Enable", gSettings.Misc.Camera.Enable);
    Misc.Camera.AttachId = ini.GetLongValue("CAM", "AttachId", gSettings.Misc.Camera.AttachId);

    Misc.Camera.Movement.Follow = ini.GetBoolValue("CAM", "FollowMovement", gSettings.Misc.Camera.Movement.Follow);
    Misc.Camera.Movement.RotationDirectionMult = ini.GetDoubleValue("CAM", "MovementMultVel", gSettings.Misc.Camera.Movement.RotationDirectionMult);
    Misc.Camera.Movement.RotationRotationMult = ini.GetDoubleValue("CAM", "MovementMultRot", gSettings.Misc.Camera.Movement.RotationRotationMult);
    Misc.Camera.Movement.RotationMaxAngle = ini.GetDoubleValue("CAM", "MovementCap", gSettings.Misc.Camera.Movement.RotationMaxAngle);

    Misc.Camera.Movement.LongForwardMult = ini.GetDoubleValue("CAM", "LongForwardMult", gSettings.Misc.Camera.Movement.LongForwardMult);
    Misc.Camera.Movement.LongBackwardMult = ini.GetDoubleValue("CAM", "LongBackwardMult", gSettings.Misc.Camera.Movement.LongBackwardMult);
    Misc.Camera.Movement.LongDeadzone = ini.GetDoubleValue("CAM", "LongDeadzone", gSettings.Misc.Camera.Movement.LongDeadzone);
    Misc.Camera.Movement.LongGamma = ini.GetDoubleValue("CAM", "LongGamma", gSettings.Misc.Camera.Movement.LongGamma);
    Misc.Camera.Movement.LongForwardLimit = ini.GetDoubleValue("CAM", "LongForwardLimit", gSettings.Misc.Camera.Movement.LongForwardLimit);
    Misc.Camera.Movement.LongBackwardLimit = ini.GetDoubleValue("CAM", "LongBackwardLimit", gSettings.Misc.Camera.Movement.LongBackwardLimit);

    Misc.Camera.RemoveHead = ini.GetBoolValue("CAM", "RemoveHead", gSettings.Misc.Camera.RemoveHead);
    Misc.Camera.FOV = ini.GetDoubleValue("CAM", "FOV", gSettings.Misc.Camera.FOV);
    Misc.Camera.OffsetHeight = ini.GetDoubleValue("CAM", "OffsetHeight", gSettings.Misc.Camera.OffsetHeight);
    Misc.Camera.OffsetForward = ini.GetDoubleValue("CAM", "OffsetForward", gSettings.Misc.Camera.OffsetForward);
    Misc.Camera.OffsetSide = ini.GetDoubleValue("CAM", "OffsetSide", gSettings.Misc.Camera.OffsetSide);
    Misc.Camera.Pitch = ini.GetDoubleValue("CAM", "Pitch", gSettings.Misc.Camera.Pitch);
    Misc.Camera.LookTime = ini.GetDoubleValue("CAM", "LookTime", gSettings.Misc.Camera.LookTime);
    Misc.Camera.MouseLookTime = ini.GetDoubleValue("CAM", "MouseLookTime", gSettings.Misc.Camera.MouseLookTime);
    Misc.Camera.MouseCenterTimeout = ini.GetLongValue("CAM", "MouseCenterTimeout", gSettings.Misc.Camera.MouseCenterTimeout);
    Misc.Camera.MouseSensitivity = ini.GetDoubleValue("CAM", "MouseSensitivity", gSettings.Misc.Camera.MouseSensitivity);

    Misc.Camera.Bike.Disable = ini.GetBoolValue("CAM", "BikeDisable", gSettings.Misc.Camera.Bike.Disable);
    Misc.Camera.Bike.AttachId = ini.GetLongValue("CAM", "BikeAttachId", gSettings.Misc.Camera.Bike.AttachId);
    Misc.Camera.Bike.FOV = ini.GetDoubleValue("CAM", "BikeFOV", gSettings.Misc.Camera.Bike.FOV);
    Misc.Camera.Bike.OffsetHeight = ini.GetDoubleValue("CAM", "BikeOffsetHeight", gSettings.Misc.Camera.Bike.OffsetHeight);
    Misc.Camera.Bike.OffsetForward = ini.GetDoubleValue("CAM", "BikeOffsetForward", gSettings.Misc.Camera.Bike.OffsetForward);
    Misc.Camera.Bike.OffsetSide = ini.GetDoubleValue("CAM", "BikeOffsetSide", gSettings.Misc.Camera.Bike.OffsetSide);
    Misc.Camera.Bike.Pitch = ini.GetDoubleValue("CAM", "BikePitch", gSettings.Misc.Camera.Bike.Pitch);
}

void VehicleConfig::SaveSettings() {
    saveGeneral();
}

void VehicleConfig::saveGeneral() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [ID]
    ini.SetValue("ID", "ModelName", fmt::format("{}", fmt::join(ModelNames, " ")).c_str());
    //std::string allPlates = ini.GetValue("ID", "Plate", "");
    //ModelNames = StrUtil::split(allNames, ' ');
    //Plates = StrUtil::split(allPlates, ' ');
    //Description = ini.GetValue("ID", "Description", "No description.");

    // [MT_OPTIONS]
    if (MTOptions.ShiftMode != mBaseConfig->MTOptions.ShiftMode) {
        SetValue(ini, "MT_OPTIONS", "ShiftMode", static_cast<int>(MTOptions.ShiftMode));
    }

    SET_VALUE("MT_OPTIONS", "ClutchCatching", MTOptions.ClutchCreep);
    SET_VALUE("MT_OPTIONS", "ClutchShiftingH", MTOptions.ClutchShiftH);
    SET_VALUE("MT_OPTIONS", "ClutchShiftingS", MTOptions.ClutchShiftS);

    // [MT_PARAMS]
    SET_VALUE("MT_PARAMS", "ClutchCatchpoint", MTParams.ClutchThreshold);
    SET_VALUE("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    SET_VALUE("MT_PARAMS", "StallingRate", MTParams.StallingRate);
    SET_VALUE("MT_PARAMS", "StallingSlip", MTParams.StallingSlip);
    SET_VALUE("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    SET_VALUE("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    SET_VALUE("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    SET_VALUE("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);
    SET_VALUE("MT_PARAMS", "CreepIdleThrottle", MTParams.CreepIdleThrottle);
    SET_VALUE("MT_PARAMS", "CreepIdleRPM", MTParams.CreepIdleRPM);

    // [DRIVING_ASSISTS]
    SET_VALUE("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    SET_VALUE("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    SET_VALUE("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
    SET_VALUE("DRIVING_ASSISTS", "TCSMode", DriveAssists.TCS.Mode);
    SET_VALUE("DRIVING_ASSISTS", "TCSSlipMax", DriveAssists.TCS.SlipMax);
    SET_VALUE("DRIVING_ASSISTS", "ESP", DriveAssists.ESP.Enable);
    SET_VALUE("DRIVING_ASSISTS", "ESPOverMin", DriveAssists.ESP.OverMin);
    SET_VALUE("DRIVING_ASSISTS", "ESPOverMax", DriveAssists.ESP.OverMax);
    SET_VALUE("DRIVING_ASSISTS", "ESPOverMinComp", DriveAssists.ESP.OverMinComp);
    SET_VALUE("DRIVING_ASSISTS", "ESPOverMaxComp", DriveAssists.ESP.OverMaxComp);
    SET_VALUE("DRIVING_ASSISTS", "ESPUnderMin", DriveAssists.ESP.UnderMin);
    SET_VALUE("DRIVING_ASSISTS", "ESPUnderMax", DriveAssists.ESP.UnderMax);
    SET_VALUE("DRIVING_ASSISTS", "ESPUnderMinComp", DriveAssists.ESP.UnderMinComp);
    SET_VALUE("DRIVING_ASSISTS", "ESPUnderMaxComp", DriveAssists.ESP.UnderMaxComp);

    SET_VALUE("DRIVING_ASSISTS", "LSD", DriveAssists.LSD.Enable);
    SET_VALUE("DRIVING_ASSISTS", "LSDViscosity", DriveAssists.LSD.Viscosity);

    SET_VALUE("DRIVING_ASSISTS", "AWD", DriveAssists.AWD.Enable);
    SET_VALUE("DRIVING_ASSISTS", "AWDBiasAtMaxTransfer", DriveAssists.AWD.BiasAtMaxTransfer);
    SET_VALUE("DRIVING_ASSISTS", "AWDUseCustomBaseBias", DriveAssists.AWD.UseCustomBaseBias);
    SET_VALUE("DRIVING_ASSISTS", "AWDCustomBaseBias", DriveAssists.AWD.CustomBaseBias);
    SET_VALUE("DRIVING_ASSISTS", "AWDCustomMin", DriveAssists.AWD.CustomMin);
    SET_VALUE("DRIVING_ASSISTS", "AWDCustomMax", DriveAssists.AWD.CustomMax);
    SET_VALUE("DRIVING_ASSISTS", "AWDUseTraction", DriveAssists.AWD.UseTraction);
    SET_VALUE("DRIVING_ASSISTS", "AWDTractionLossMin", DriveAssists.AWD.TractionLossMin);
    SET_VALUE("DRIVING_ASSISTS", "AWDTractionLossMax", DriveAssists.AWD.TractionLossMax);
    SET_VALUE("DRIVING_ASSISTS", "AWDUseOversteer", DriveAssists.AWD.UseOversteer);
    SET_VALUE("DRIVING_ASSISTS", "AWDOversteerMin", DriveAssists.AWD.OversteerMin);
    SET_VALUE("DRIVING_ASSISTS", "AWDOversteerMax", DriveAssists.AWD.OversteerMax);
    SET_VALUE("DRIVING_ASSISTS", "AWDUseUndersteer", DriveAssists.AWD.UseUndersteer);
    SET_VALUE("DRIVING_ASSISTS", "AWDUndersteerMin", DriveAssists.AWD.UndersteerMin);
    SET_VALUE("DRIVING_ASSISTS", "AWDUndersteerMax", DriveAssists.AWD.UndersteerMax);

    if (DriveAssists.AWD.SpecialFlags != mBaseConfig->DriveAssists.AWD.SpecialFlags)
        ini.SetLongValue("DRIVING_ASSISTS", "AWDSpecialFlags", DriveAssists.AWD.SpecialFlags, nullptr, true);

    //[STEERING_OVERRIDE]
    SET_VALUE("STEERING_OVERRIDE", "UseForCustomSteering", SteeringOverride.UseForCustomSteering);
    SET_VALUE("STEERING_OVERRIDE", "SoftLockCustomSteering", SteeringOverride.SoftLockCustomSteering);
    SET_VALUE("STEERING_OVERRIDE", "SoftLockWheelInput", SteeringOverride.SoftLockWheelInput);

    // [SHIFT_OPTIONS]
    SET_VALUE("SHIFT_OPTIONS", "UpshiftCut", ShiftOptions.UpshiftCut);
    SET_VALUE("SHIFT_OPTIONS", "DownshiftBlip", ShiftOptions.DownshiftBlip);
    SET_VALUE("SHIFT_OPTIONS", "ClutchRateMult", ShiftOptions.ClutchRateMult);
    SET_VALUE("SHIFT_OPTIONS", "RPMTolerance", ShiftOptions.RPMTolerance);

    // [AUTO_PARAMS]
    SET_VALUE("AUTO_PARAMS", "UpshiftLoad", AutoParams.UpshiftLoad);
    SET_VALUE("AUTO_PARAMS", "DownshiftLoad", AutoParams.DownshiftLoad);
    SET_VALUE("AUTO_PARAMS", "NextGearMinRPM", AutoParams.NextGearMinRPM);
    SET_VALUE("AUTO_PARAMS", "CurrGearMinRPM", AutoParams.CurrGearMinRPM);
    SET_VALUE("AUTO_PARAMS", "EcoRate", AutoParams.EcoRate);
    SET_VALUE("AUTO_PARAMS", "DownshiftTimeoutMult", AutoParams.DownshiftTimeoutMult);
    SET_VALUE("AUTO_PARAMS", "UsingATCU", AutoParams.UsingATCU);

    // [CAM]
    SET_VALUE("CAM", "Enable", Misc.Camera.Enable);
    SET_VALUE("CAM", "AttachId", Misc.Camera.AttachId);

    SET_VALUE("CAM", "FollowMovement", Misc.Camera.Movement.Follow);
    SET_VALUE("CAM", "MovementMultVel", Misc.Camera.Movement.RotationDirectionMult);
    SET_VALUE("CAM", "MovementMultRot", Misc.Camera.Movement.RotationRotationMult);
    SET_VALUE("CAM", "MovementCap", Misc.Camera.Movement.RotationMaxAngle);

    SET_VALUE("CAM", "LongForwardMult", Misc.Camera.Movement.LongForwardMult);
    SET_VALUE("CAM", "LongBackwardMult", Misc.Camera.Movement.LongBackwardMult);
    SET_VALUE("CAM", "LongDeadzone", Misc.Camera.Movement.LongDeadzone);
    SET_VALUE("CAM", "LongGamma", Misc.Camera.Movement.LongGamma);
    SET_VALUE("CAM", "LongForwardLimit", Misc.Camera.Movement.LongForwardLimit);
    SET_VALUE("CAM", "LongBackwardLimit", Misc.Camera.Movement.LongBackwardLimit);

    SET_VALUE("CAM", "RemoveHead", Misc.Camera.RemoveHead);
    SET_VALUE("CAM", "FOV", Misc.Camera.FOV);
    SET_VALUE("CAM", "OffsetHeight", Misc.Camera.OffsetHeight);
    SET_VALUE("CAM", "OffsetForward", Misc.Camera.OffsetForward);
    SET_VALUE("CAM", "OffsetSide", Misc.Camera.OffsetSide);
    SET_VALUE("CAM", "Pitch", Misc.Camera.Pitch);
    SET_VALUE("CAM", "LookTime", Misc.Camera.LookTime);
    SET_VALUE("CAM", "MouseLookTime", Misc.Camera.MouseLookTime);
    SET_VALUE("CAM", "MouseCenterTimeout", Misc.Camera.MouseCenterTimeout);
    SET_VALUE("CAM", "MouseSensitivity", Misc.Camera.MouseSensitivity);

    SET_VALUE("CAM", "BikeDisable", Misc.Camera.Bike.Disable);
    SET_VALUE("CAM", "BikeAttachId", Misc.Camera.Bike.AttachId);
    SET_VALUE("CAM", "BikeFOV", Misc.Camera.Bike.FOV);
    SET_VALUE("CAM", "BikeOffsetHeight", Misc.Camera.Bike.OffsetHeight);
    SET_VALUE("CAM", "BikeOffsetForward", Misc.Camera.Bike.OffsetForward);
    SET_VALUE("CAM", "BikeOffsetSide", Misc.Camera.Bike.OffsetSide);
    SET_VALUE("CAM", "BikePitch", Misc.Camera.Bike.Pitch);

    result = ini.SaveFile(mFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

#pragma warning(pop)
