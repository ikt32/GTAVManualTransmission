#include "VehicleConfig.h"
#include "Util/Strings.hpp"
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
    DriveAssists.TCS.Enable = ini.GetLongValue("DRIVING_ASSISTS", "TCS", gSettings.DriveAssists.TCS.Enable);
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
    DriveAssists.AWD.UseCustomBaseBias = ini.GetBoolValue("DRIVING_ASSISTS",   "AWDCustomBaseBias", gSettings.DriveAssists.AWD.UseCustomBaseBias);
    DriveAssists.AWD.CustomBaseBias    = ini.GetDoubleValue("DRIVING_ASSISTS", "AWDCustomBaseBias", gSettings.DriveAssists.AWD.CustomBaseBias);
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

    // [CUSTOM_STEERING]
    CustomSteering.CustomRotationDegrees = ini.GetDoubleValue("CUSTOM_STEERING", "CustomRotationDegrees", gSettings.CustomSteering.CustomRotationDegrees);

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
#pragma warning(pop)
