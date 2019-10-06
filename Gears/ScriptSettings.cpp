#include "ScriptSettings.hpp"

#include <string>
#include <fmt/format.h>
#include <simpleini/SimpleIni.h>

#include "Util/Logger.hpp"
#include "Input/keyboard.h"
#include "Input/CarControls.hpp"

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        logger.Write(ERROR, "[Settings] %s Failed to %s, SI_Error [%d]", \
        __FUNCTION__, operation, result); \
    }

#pragma warning(push)
#pragma warning(disable: 4244)
ScriptSettings::ScriptSettings()
    : Assists { 0 }
    , CustomSteering { 0, 1.0f, 90.0f, 1.0f, 1.0f }
    , FFB { 10000, 0 }
{ }

void ScriptSettings::SetFiles(const std::string &general, const std::string &wheel) {
    settingsGeneralFile = general;
    settingsWheelFile = wheel;
}

void ScriptSettings::Read(CarControls* scriptControl) {
    parseSettingsGeneral(scriptControl);
    parseSettingsWheel(scriptControl);
}

void ScriptSettings::SaveGeneral() const {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    settingsGeneral.SetBoolValue("OPTIONS", "Enable", EnableManual);
    settingsGeneral.SetLongValue("OPTIONS", "ShiftMode", ShiftMode);

    settingsGeneral.SetLongValue("OPTIONS", "ShiftMode", ShiftMode);
    settingsGeneral.SetBoolValue("OPTIONS", "SimpleBike", SimpleBike);
    settingsGeneral.SetBoolValue("OPTIONS", "EngineDamage", EngDamage);
    settingsGeneral.SetBoolValue("OPTIONS", "EngineStalling", EngStall);
    settingsGeneral.SetBoolValue("OPTIONS", "EngineStallingS", EngStallS);
    settingsGeneral.SetBoolValue("OPTIONS", "EngineBraking", EngBrake);
    settingsGeneral.SetBoolValue("OPTIONS", "EngineLocking", EngLock);
    settingsGeneral.SetBoolValue("OPTIONS", "ClutchCatching", ClutchCatching);
    settingsGeneral.SetBoolValue("OPTIONS", "ClutchShiftingH", ClutchShiftingH);
    settingsGeneral.SetBoolValue("OPTIONS", "ClutchShiftingS", ClutchShiftingS);
    settingsGeneral.SetBoolValue("OPTIONS", "DefaultNeutral", DefaultNeutral);

    settingsGeneral.SetDoubleValue("OPTIONS", "ClutchCatchpoint", ClutchThreshold * 100.0);
    settingsGeneral.SetDoubleValue("OPTIONS", "StallingThreshold", StallingThreshold * 100.0);
    settingsGeneral.SetDoubleValue("OPTIONS", "StallingRPM", StallingRPM);
    settingsGeneral.SetDoubleValue("OPTIONS", "RPMDamage", RPMDamage * 100.0);
    settingsGeneral.SetDoubleValue("OPTIONS", "MisshiftDamage", MisshiftDamage);
    settingsGeneral.SetDoubleValue("OPTIONS", "EngBrakePower", EngBrakePower);
    settingsGeneral.SetDoubleValue("OPTIONS", "EngBrakeThreshold", EngBrakeThreshold);

    settingsGeneral.SetBoolValue("OPTIONS", "HillBrakeWorkaround", HillBrakeWorkaround);
    settingsGeneral.SetBoolValue("OPTIONS", "AutoGear1", AutoGear1);
    settingsGeneral.SetBoolValue("OPTIONS", "AutoLookBack", AutoLookBack);
    settingsGeneral.SetBoolValue("OPTIONS", "ThrottleStart", ThrottleStart);
    settingsGeneral.SetBoolValue("OPTIONS", "HidePlayerInFPV", HidePlayerInFPV);
    settingsGeneral.SetBoolValue("OPTIONS", "HardLimiter", HardLimiter);
    settingsGeneral.SetBoolValue("OPTIONS", "CustomABS", CustomABS);
    settingsGeneral.SetBoolValue("OPTIONS", "ABSFilter", ABSFilter);

    // [ASSISTS]
    settingsGeneral.SetLongValue("ASSISTS", "TractionControl", Assists.TractionControl);
    
    //[CUSTOM_STEERING]
    settingsGeneral.SetLongValue("CUSTOM_STEERING", "Mode", CustomSteering.Mode);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "CountersteerMult", CustomSteering.CountersteerMult);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", CustomSteering.CountersteerLimit);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "SteeringMult", CustomSteering.SteeringMult);
    settingsGeneral.SetDoubleValue("CUSTOM_STEERING", "SteeringReduction", CustomSteering.SteeringReduction);

    // [SHIFT_OPTIONS]
    settingsGeneral.SetBoolValue("SHIFT_OPTIONS", "UpshiftCut", UpshiftCut);
    settingsGeneral.SetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", DownshiftBlip);
    settingsGeneral.SetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", ClutchRateMult);


    // [AUTO_BOX]
    settingsGeneral.SetDoubleValue("AUTO_BOX", "UpshiftLoad", UpshiftLoad);
    settingsGeneral.SetDoubleValue("AUTO_BOX", "DownshiftLoad", DownshiftLoad);
    settingsGeneral.SetDoubleValue("AUTO_BOX", "NextGearMinRPM", NextGearMinRPM);
    settingsGeneral.SetDoubleValue("AUTO_BOX", "CurrGearMinRPM", CurrGearMinRPM);
    settingsGeneral.SetDoubleValue("AUTO_BOX", "EcoRate", EcoRate);
    settingsGeneral.SetDoubleValue("AUTO_BOX", "DownshiftTimeoutMult", DownshiftTimeoutMult);

    // [HUD]
    settingsGeneral.SetBoolValue("HUD", "EnableHUD", HUD);
    settingsGeneral.SetBoolValue("HUD", "AlwaysHUD", AlwaysHUD);
    settingsGeneral.SetLongValue("HUD", "HUDFont", HUDFont);
    settingsGeneral.SetBoolValue("HUD", "GearIndicator", GearIndicator);
    settingsGeneral.SetDoubleValue("HUD", "GearXpos", GearXpos);
    settingsGeneral.SetDoubleValue("HUD", "GearYpos", GearYpos);
    settingsGeneral.SetDoubleValue("HUD", "GearSize", GearSize);
    settingsGeneral.SetLongValue("HUD", "GearTopColorR", GearTopColorR);
    settingsGeneral.SetLongValue("HUD", "GearTopColorG", GearTopColorG);
    settingsGeneral.SetLongValue("HUD", "GearTopColorB", GearTopColorB);

    settingsGeneral.SetBoolValue("HUD", "ShiftModeIndicator", ShiftModeIndicator);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeXpos", ShiftModeXpos);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeYpos", ShiftModeYpos);
    settingsGeneral.SetDoubleValue("HUD", "ShiftModeSize", ShiftModeSize);

    settingsGeneral.SetValue("HUD", "Speedo", Speedo.c_str());
    settingsGeneral.SetBoolValue("HUD", "SpeedoShowUnit", SpeedoShowUnit);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoXpos", SpeedoXpos);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoYpos", SpeedoYpos);
    settingsGeneral.SetDoubleValue("HUD", "SpeedoSize", SpeedoSize);

    settingsGeneral.SetBoolValue("HUD", "EnableRPMIndicator", RPMIndicator);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorXpos", RPMIndicatorXpos);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorYpos", RPMIndicatorYpos);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorWidth", RPMIndicatorWidth);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorHeight", RPMIndicatorHeight);
    settingsGeneral.SetDoubleValue("HUD", "RPMIndicatorRedline", RPMIndicatorRedline);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundR", RPMIndicatorBackgroundR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundG", RPMIndicatorBackgroundG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundB", RPMIndicatorBackgroundB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorBackgroundA", RPMIndicatorBackgroundA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundR", RPMIndicatorForegroundR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundG", RPMIndicatorForegroundG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundB", RPMIndicatorForegroundB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorForegroundA", RPMIndicatorForegroundA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineR", RPMIndicatorRedlineR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineG", RPMIndicatorRedlineG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineB", RPMIndicatorRedlineB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRedlineA", RPMIndicatorRedlineA);

    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitR", RPMIndicatorRevlimitR);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitG", RPMIndicatorRevlimitG);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitB", RPMIndicatorRevlimitB);
    settingsGeneral.SetLongValue("HUD", "RPMIndicatorRevlimitA", RPMIndicatorRevlimitA);

    settingsGeneral.SetBoolValue("HUD", "SteeringWheelInfo", SteeringWheelInfo);
    settingsGeneral.SetBoolValue("HUD", "AlwaysSteeringWheelInfo", AlwaysSteeringWheelInfo);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureX", SteeringWheelTextureX);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureY", SteeringWheelTextureY);
    settingsGeneral.SetDoubleValue("HUD", "SteeringWheelTextureSz", SteeringWheelTextureSz);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoX", PedalInfoX);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoY", PedalInfoY);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoH"	   , PedalInfoH);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoW"	   , PedalInfoW);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoPadX"  , PedalInfoPadX);
    settingsGeneral.SetDoubleValue("HUD", "PedalInfoPadY"  , PedalInfoPadY);

    settingsGeneral.SetLongValue("HUD", "PedalBackgroundA", PedalBackgroundA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleR", PedalInfoThrottleR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleG", PedalInfoThrottleG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleB", PedalInfoThrottleB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoThrottleA", PedalInfoThrottleA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeR", PedalInfoBrakeR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeG", PedalInfoBrakeG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeB", PedalInfoBrakeB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoBrakeA", PedalInfoBrakeA);

    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchR", PedalInfoClutchR);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchG", PedalInfoClutchG);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchB", PedalInfoClutchB);
    settingsGeneral.SetLongValue("HUD", "PedalInfoClutchA", PedalInfoClutchA);

    // [UPDATE]
    settingsGeneral.SetBoolValue("UPDATE", "EnableUpdate", EnableUpdate);
    if (!IgnoredVersion.empty())
        settingsGeneral.SetValue("UPDATE", "IgnoredVersion", IgnoredVersion.c_str());
    else
        settingsGeneral.SetValue("UPDATE", "IgnoredVersion", "v0.0.0");

    // [DEBUG]
    settingsGeneral.SetBoolValue("DEBUG", "DisplayInfo", DisplayInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayWheelInfo", DisplayWheelInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayFFBInfo", DisplayFFBInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayGearingInfo", DisplayGearingInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisplayNPCInfo", ShowNPCInfo);
    settingsGeneral.SetBoolValue("DEBUG", "DisableInputDetect", DisableInputDetect);

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

void ScriptSettings::SaveController(CarControls *scriptControl) const {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [CONTROLLER]
    settingsGeneral.SetBoolValue("CONTROLLER", "ToggleEngine", ToggleEngine);
    settingsGeneral.SetLongValue("CONTROLLER", "ToggleTime", scriptControl->CToggleTime); 
    settingsGeneral.SetDoubleValue("CONTROLLER", "TriggerValue", scriptControl->GetControllerTrigger());
    settingsGeneral.SetBoolValue("CONTROLLER", "BlockCarControls", BlockCarControls);
    settingsGeneral.SetBoolValue("CONTROLLER", "IgnoreShiftsUI", IgnoreShiftsUI);
    settingsGeneral.SetBoolValue("CONTROLLER", "BlockHShift", BlockHShift);

    settingsGeneral.SetLongValue("CONTROLLER", "MaxTapTime", scriptControl->MaxTapTime);
    
    settingsGeneral.SetLongValue("CONTROLLER", "ShiftUpBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)]);
    settingsGeneral.SetLongValue("CONTROLLER", "ShiftDownBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)]);
    settingsGeneral.SetLongValue("CONTROLLER", "ClutchBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)]);

    // [CONTROLLER_LEGACY]
    settingsGeneral.SetBoolValue("CONTROLLER_LEGACY", "Enable", scriptControl->UseLegacyController);
    settingsGeneral.SetLongValue("CONTROLLER_LEGACY", "ShiftUpBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)]);
    settingsGeneral.SetLongValue("CONTROLLER_LEGACY", "ShiftDownBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)]);
    settingsGeneral.SetLongValue("CONTROLLER_LEGACY", "ClutchBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)]);

    result = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

// Axis information is saved by its own calibration methods
void ScriptSettings::SaveWheel(CarControls *scriptControl) const {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    settingsWheel.SetBoolValue("OPTIONS", "EnableWheel", EnableWheel);
    settingsWheel.SetBoolValue("OPTIONS", "WheelWithoutManual", WheelWithoutManual);

    settingsWheel.SetBoolValue("OPTIONS", "LogitechLEDs", LogiLEDs);
    settingsWheel.SetBoolValue("OPTIONS", "HPatternKeyboard", HPatternKeyboard);

    settingsWheel.SetDoubleValue("OPTIONS", "GameSteerMultWheel", GameSteerMultWheel);
    settingsWheel.SetBoolValue("OPTIONS", "UseShifterForAuto", UseShifterForAuto);

    // [FORCE_FEEDBACK]
    settingsWheel.SetBoolValue("FORCE_FEEDBACK", "Enable", EnableFFB);
    settingsWheel.SetBoolValue("FORCE_FEEDBACK", "Scale", ScaleFFB);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", SATAmpMult);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "SATMax", FFB.SATMax);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "AntiDeadForce", FFB.AntiDeadForce);

    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DetailMult", DetailMult);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "CollisionMult", CollisionMult);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMax", DamperMax);
    settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMin", DamperMin);
    settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", DamperMinSpeed);

    // [STEER]
    settingsWheel.SetDoubleValue("STEER", "ANTIDEADZONE", scriptControl->ADZSteer);
    settingsWheel.SetDoubleValue("STEER", "DEADZONE", scriptControl->DZSteer);
    settingsWheel.SetDoubleValue("STEER", "DEADZONEOFFSET", scriptControl->DZSteerOffset);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleMax", SteerAngleMax );
    settingsWheel.SetDoubleValue("STEER", "SteerAngleCar", SteerAngleCar );
    settingsWheel.SetDoubleValue("STEER", "SteerAngleBike",SteerAngleBike);
    settingsWheel.SetDoubleValue("STEER", "SteerAngleBoat", SteerAngleBoat);
    settingsWheel.SetDoubleValue("STEER", "GAMMA", SteerGamma);

    // [THROTTLE]
    settingsWheel.SetDoubleValue("THROTTLE", "ANTIDEADZONE", scriptControl->ADZThrottle);
    settingsWheel.SetDoubleValue("THROTTLE", "GAMMA", ThrottleGamma);

    // [BRAKES]
    settingsWheel.SetDoubleValue("BRAKES", "ANTIDEADZONE", scriptControl->ADZBrake);
    settingsWheel.SetDoubleValue("BRAKES", "GAMMA", BrakeGamma);

    result = settingsWheel.SaveFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

std::vector<GUID> ScriptSettings::GetGuids() {
    return RegisteredGUIDs;
}

void ScriptSettings::parseSettingsGeneral(CarControls *scriptControl) {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    SI_Error result = settingsGeneral.LoadFile(settingsGeneralFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    EnableManual = settingsGeneral.GetBoolValue("OPTIONS", "Enable", true);
    ShiftMode = (ShiftModes)settingsGeneral.GetLongValue("OPTIONS", "ShiftMode", 0);
    SimpleBike = settingsGeneral.GetBoolValue("OPTIONS", "SimpleBike", false);
    EngDamage = settingsGeneral.GetBoolValue("OPTIONS", "EngineDamage", false);
    EngStall = settingsGeneral.GetBoolValue("OPTIONS", "EngineStalling", true);
    EngStallS = settingsGeneral.GetBoolValue("OPTIONS", "EngineStallingS", false);
    EngBrake = settingsGeneral.GetBoolValue("OPTIONS", "EngineBraking", true);
    EngLock  = settingsGeneral.GetBoolValue("OPTIONS", "EngineLocking", false);
    ClutchCatching = settingsGeneral.GetBoolValue("OPTIONS", "ClutchCatching", true);
    ClutchShiftingH = settingsGeneral.GetBoolValue("OPTIONS", "ClutchShiftingH", true);
    ClutchShiftingS = settingsGeneral.GetBoolValue("OPTIONS", "ClutchShiftingS", false);
    DefaultNeutral = settingsGeneral.GetBoolValue("OPTIONS", "DefaultNeutral", true);

    ClutchThreshold = settingsGeneral.GetDoubleValue("OPTIONS", "ClutchCatchpoint", 10.0) / 100.0f;
    StallingThreshold = settingsGeneral.GetDoubleValue("OPTIONS", "StallingThreshold", 85.0) / 100.0f;
    StallingRPM = settingsGeneral.GetDoubleValue("OPTIONS", "StallingRPM", 0.1);
    RPMDamage = settingsGeneral.GetDoubleValue("OPTIONS", "RPMDamage", 15.0) / 100.0f;
    MisshiftDamage = settingsGeneral.GetDoubleValue("OPTIONS", "MisshiftDamage", 20.0);
    EngBrakePower = settingsGeneral.GetDoubleValue("OPTIONS", "EngBrakePower", 1.0);
    EngBrakeThreshold = settingsGeneral.GetDoubleValue("OPTIONS", "EngBrakeThreshold", 0.75);

    HillBrakeWorkaround = settingsGeneral.GetBoolValue("OPTIONS", "HillBrakeWorkaround", false);
    AutoGear1 = settingsGeneral.GetBoolValue("OPTIONS", "AutoGear1", false);
    AutoLookBack = settingsGeneral.GetBoolValue("OPTIONS", "AutoLookBack", false);
    ThrottleStart = settingsGeneral.GetBoolValue("OPTIONS", "ThrottleStart", true);
    HidePlayerInFPV = settingsGeneral.GetBoolValue("OPTIONS", "HidePlayerInFPV", false);
    HardLimiter = settingsGeneral.GetBoolValue("OPTIONS", "HardLimiter", false);
    CustomABS = settingsGeneral.GetBoolValue("OPTIONS", "CustomABS", false);
    ABSFilter = settingsGeneral.GetBoolValue("OPTIONS", "ABSFilter", false);

    // [ASSISTS]
    Assists.TractionControl = settingsGeneral.GetLongValue("ASSISTS", "TractionControl", 0);

    // [SHIFT_OPTIONS]
    UpshiftCut = settingsGeneral.GetBoolValue("SHIFT_OPTIONS", "UpshiftCut", true);
    DownshiftBlip = settingsGeneral.GetBoolValue("SHIFT_OPTIONS", "DownshiftBlip", true);
    ClutchRateMult = settingsGeneral.GetDoubleValue("SHIFT_OPTIONS", "ClutchRateMult", 1.0f);

    // [AUTO_BOX]
    UpshiftLoad = settingsGeneral.GetDoubleValue("AUTO_BOX", "UpshiftLoad", 0.05f);
    DownshiftLoad =  settingsGeneral.GetDoubleValue("AUTO_BOX", "DownshiftLoad", 0.60f);
    NextGearMinRPM = settingsGeneral.GetDoubleValue("AUTO_BOX", "NextGearMinRPM", 0.33f);
    CurrGearMinRPM = settingsGeneral.GetDoubleValue("AUTO_BOX", "CurrGearMinRPM", 0.27f);
    EcoRate = settingsGeneral.GetDoubleValue("AUTO_BOX", "EcoRate", 0.05f);
    DownshiftTimeoutMult = settingsGeneral.GetDoubleValue("AUTO_BOX", "DownshiftTimeoutMult", 1.0f);

    // [CUSTOM_STEERING]
    CustomSteering.Mode = settingsGeneral.GetLongValue("CUSTOM_STEERING", "Mode", 0);
    CustomSteering.CountersteerMult = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "CountersteerMult", 1.0f);
    CustomSteering.CountersteerLimit = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "CountersteerLimit", 15.0f);
    CustomSteering.SteeringMult = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "SteeringMult", 1.0f);
    CustomSteering.SteeringReduction = settingsGeneral.GetDoubleValue("CUSTOM_STEERING", "SteeringReduction", 1.0f);

    // [HUD]
    HUD = settingsGeneral.GetBoolValue			("HUD", "EnableHUD", true);
    AlwaysHUD = settingsGeneral.GetBoolValue	("HUD", "AlwaysHUD", false);
    HUDFont = settingsGeneral.GetLongValue		("HUD", "HUDFont", 4);

    GearIndicator = settingsGeneral.GetBoolValue("HUD", "GearIndicator", true);
    GearXpos = settingsGeneral.GetDoubleValue	("HUD", "GearXpos", 0.952500);
    GearYpos = settingsGeneral.GetDoubleValue	("HUD", "GearYpos", 0.885000);
    GearSize = settingsGeneral.GetDoubleValue	("HUD", "GearSize", 0.700000);
    GearTopColorR = settingsGeneral.GetLongValue("HUD", "GearTopColorR", 255);
    GearTopColorG = settingsGeneral.GetLongValue("HUD", "GearTopColorG", 63);
    GearTopColorB = settingsGeneral.GetLongValue("HUD", "GearTopColorB", 63);

    ShiftModeIndicator = settingsGeneral.GetBoolValue("HUD", "ShiftModeIndicator", true);
    ShiftModeXpos = settingsGeneral.GetDoubleValue("HUD", "ShiftModeXpos", 0.935000);
    ShiftModeYpos = settingsGeneral.GetDoubleValue("HUD", "ShiftModeYpos", 0.885000);
    ShiftModeSize = settingsGeneral.GetDoubleValue("HUD", "ShiftModeSize", 0.700000);

    Speedo = settingsGeneral.GetValue("HUD", "Speedo", "kph");
    SpeedoShowUnit = settingsGeneral.GetBoolValue("HUD", "SpeedoShowUnit", true);
    SpeedoXpos = settingsGeneral.GetDoubleValue("HUD", "SpeedoXpos", 0.860000);
    SpeedoYpos = settingsGeneral.GetDoubleValue("HUD", "SpeedoYpos", 0.885000);
    SpeedoSize = settingsGeneral.GetDoubleValue("HUD", "SpeedoSize", 0.700000);

    RPMIndicator = settingsGeneral.GetBoolValue("HUD", "EnableRPMIndicator", true);
    RPMIndicatorXpos = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorXpos", 0.120001);
    RPMIndicatorYpos = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorYpos", 0.765000);
    RPMIndicatorWidth = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorWidth", 0.140000);
    RPMIndicatorHeight = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorHeight", 0.005000);
    RPMIndicatorRedline = settingsGeneral.GetDoubleValue("HUD", "RPMIndicatorRedline", 0.845000);

    RPMIndicatorBackgroundR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundR", 0);
    RPMIndicatorBackgroundG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundG", 0);
    RPMIndicatorBackgroundB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundB", 0);
    RPMIndicatorBackgroundA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorBackgroundA", 128);
                                    
    RPMIndicatorForegroundR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundR", 255);
    RPMIndicatorForegroundG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundG", 255);
    RPMIndicatorForegroundB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundB", 255);
    RPMIndicatorForegroundA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorForegroundA", 255);
                                    
    RPMIndicatorRedlineR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineR", 255);
    RPMIndicatorRedlineG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineG", 92);
    RPMIndicatorRedlineB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineB", 0);
    RPMIndicatorRedlineA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRedlineA", 255);

    RPMIndicatorRevlimitR = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitR", 255);
    RPMIndicatorRevlimitG = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitG", 0);
    RPMIndicatorRevlimitB = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitB", 0);
    RPMIndicatorRevlimitA = settingsGeneral.GetLongValue("HUD", "RPMIndicatorRevlimitA", 255);

    SteeringWheelInfo	  = settingsGeneral.GetBoolValue("HUD", "SteeringWheelInfo", true);
    AlwaysSteeringWheelInfo = settingsGeneral.GetBoolValue("HUD", "AlwaysSteeringWheelInfo", false);
    SteeringWheelTextureX = settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureX", 0.230000);
    SteeringWheelTextureY = settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureY", 0.890000);
    SteeringWheelTextureSz= settingsGeneral.GetDoubleValue("HUD", "SteeringWheelTextureSz", 0.060000);
    PedalInfoX			  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoX", 0.290000);
    PedalInfoY			  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoY", 0.890000);
    PedalInfoH			  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoH", 0.100000);
    PedalInfoW			  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoW", 0.040000);
    PedalInfoPadX		  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoPadX", 0.000000);
    PedalInfoPadY		  = settingsGeneral.GetDoubleValue("HUD", "PedalInfoPadY", 0.000000);
    
    PedalBackgroundA = settingsGeneral.GetLongValue("HUD", "PedalBackgroundA", 92);

    PedalInfoThrottleR = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleR", 0);
    PedalInfoThrottleG = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleG", 255);
    PedalInfoThrottleB = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleB", 0);
    PedalInfoThrottleA = settingsGeneral.GetLongValue("HUD", "PedalInfoThrottleA", 255);

    PedalInfoBrakeR = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeR", 255);
    PedalInfoBrakeG = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeG", 0);
    PedalInfoBrakeB = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeB", 0);
    PedalInfoBrakeA = settingsGeneral.GetLongValue("HUD", "PedalInfoBrakeA", 255);

    PedalInfoClutchR = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchR", 0);
    PedalInfoClutchG = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchG", 0);
    PedalInfoClutchB = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchB", 255);
    PedalInfoClutchA = settingsGeneral.GetLongValue("HUD", "PedalInfoClutchA", 255);

    // [CONTROLLER]
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Toggle)] = settingsGeneral.GetValue("CONTROLLER", "Toggle", "UNKNOWN");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ToggleH)] = settingsGeneral.GetValue("CONTROLLER", "ToggleShift", "B");
    BlockCarControls = settingsGeneral.GetBoolValue("CONTROLLER", "BlockCarControls", false);
    IgnoreShiftsUI = settingsGeneral.GetBoolValue("CONTROLLER", "IgnoreShiftsUI", false);
    BlockHShift = settingsGeneral.GetBoolValue("CONTROLLER", "BlockHShift", true);

    scriptControl->CToggleTime = settingsGeneral.GetLongValue("CONTROLLER", "ToggleTime", 300);
    scriptControl->MaxTapTime = settingsGeneral.GetLongValue("CONTROLLER", "MaxTapTime", 200);

    double tval = settingsGeneral.GetDoubleValue("CONTROLLER", "TriggerValue", 0.85);
    if (tval > 1.0 || tval < 0.1) {
        tval = 0.85;
    }
    scriptControl->SetControllerTriggerLevel(tval);

    ToggleEngine = settingsGeneral.GetBoolValue("CONTROLLER", "ToggleEngine", false);

    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = settingsGeneral.GetValue("CONTROLLER", "ShiftUp", "A");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = settingsGeneral.GetValue("CONTROLLER", "ShiftDown", "X");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Clutch)] = settingsGeneral.GetValue("CONTROLLER", "Clutch", "LeftThumbUp");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Engine)] = settingsGeneral.GetValue("CONTROLLER", "Engine", "DpadDown");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Throttle)] = settingsGeneral.GetValue("CONTROLLER", "Throttle", "RightTrigger");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Brake)] = settingsGeneral.GetValue("CONTROLLER", "Brake", "LeftTrigger");

    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER", "ShiftUpBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER", "ShiftDownBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] = settingsGeneral.GetLongValue("CONTROLLER", "ClutchBlocks", -1);

    // [CONTROLLER_LEGACY]
    scriptControl->UseLegacyController = settingsGeneral.GetBoolValue("CONTROLLER_LEGACY", "Enable", false);

    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Toggle)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Toggle", -1);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ToggleH)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ToggleShift", ControlFrontendCancel);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftUp", ControlFrontendAccept);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftDown", ControlFrontendX);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Clutch)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Clutch", ControlFrontendAxisY);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Engine)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Engine", ControlFrontendDown);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Throttle)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Throttle", ControlFrontendLt);
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Brake)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Brake", ControlFrontendRt);

    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftUpBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftDownBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ClutchBlocks", -1);

    // [KEYBOARD]
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Toggle)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ToggleH)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftUp)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftUp", "LSHIFT"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftDown)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftDown", "LCTRL"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Clutch)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Clutch", "Z"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Engine)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Engine", "X"));

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Throttle)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Throttle", "W"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Brake)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Brake", "S"));

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HR)] = str2key(settingsGeneral.GetValue("KEYBOARD", "HR", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H1)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H1", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H2)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H2", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H3)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H3", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H4)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H4", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H5)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H5", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H6)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H6", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H7)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H7", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H8)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H8", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H9)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H9", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H10)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H10", "UNKNOWN"));
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HN)] = str2key(settingsGeneral.GetValue("KEYBOARD", "HN", "UNKNOWN"));

    // [UPDATE]
    EnableUpdate = settingsGeneral.GetBoolValue("UPDATE", "EnableUpdate", true);
    IgnoredVersion = settingsGeneral.GetValue("UPDATE", "IgnoredVersion", "v0.0.0");

    // [DEBUG]
    DisplayInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayInfo", false);
    DisplayWheelInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayWheelInfo", false);
    DisplayGearingInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayGearingInfo", false);
    DisplayFFBInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayFFBInfo", false);
    ShowNPCInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayNPCInfo", false);
    LogLevel = settingsGeneral.GetLongValue("DEBUG", "LogLevel", INFO);
    DisableInputDetect = settingsGeneral.GetBoolValue("DEBUG", "DisableInputDetect", false);
}

void ScriptSettings::parseSettingsWheel(CarControls *scriptControl) {
    CSimpleIniA settingsWheel;
    settingsWheel.SetUnicode();
    SI_Error result = settingsWheel.LoadFile(settingsWheelFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    // [OPTIONS]
    EnableWheel = settingsWheel.GetBoolValue("OPTIONS", "EnableWheel", true);
    WheelWithoutManual = settingsWheel.GetBoolValue("OPTIONS", "WheelWithoutManual", true);
    LogiLEDs = settingsWheel.GetBoolValue("OPTIONS", "LogitechLEDs", false);
    HPatternKeyboard = settingsWheel.GetBoolValue("OPTIONS", "HPatternKeyboard", false);

    GameSteerMultWheel = settingsWheel.GetDoubleValue("OPTIONS", "GameSteerMultWheel", 1.0);

    UseShifterForAuto = settingsWheel.GetBoolValue("OPTIONS", "UseShifterForAuto", false);

    // [FORCE_FEEDBACK]
    EnableFFB = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "Enable", true);
    ScaleFFB = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "Scale", true);
    SATAmpMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "SATAmpMult", 1.0);
    FFB.SATMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "SATMax", 10000);
    FFB.AntiDeadForce = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "AntiDeadForce", 0);

    DetailMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DetailMult", 2.5);
    CollisionMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "CollisionMult", 1.0);

    DamperMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMax", 67);
    DamperMin = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMin", 12);
    DamperMinSpeed = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DamperMinSpeed", 1.2);


    // [INPUT_DEVICES]
    int it = 0;
    RegisteredGUIDs.clear();
    while (true) {
        std::string currDevIndex = std::string("DEV") + std::to_string(it);
        std::string currGuidIndex = std::string("GUID") + std::to_string(it);

        std::string currDevice = settingsWheel.GetValue("INPUT_DEVICES", currDevIndex.c_str(), "");
        if (currDevice == "")
            break;
        std::string currGuid = settingsWheel.GetValue("INPUT_DEVICES", currGuidIndex.c_str(), "");
        if (currGuid == "")
            break;

        std::wstring clsidStr;
        clsidStr.assign(currGuid.begin(), currGuid.end());
        GUID guid;
        HRESULT hr = CLSIDFromString(clsidStr.c_str(), &guid);
        if (hr != NOERROR) {
            std::string errStr;
            switch (hr) {
                case CO_E_CLASSSTRING:
                    errStr = "The class string was improperly formatted.";
                    break;
                case REGDB_E_CLASSNOTREG:
                    errStr = "The CLSID corresponding to the class string was not found in the registry.";
                    break;
                case REGDB_E_READREGDB:
                    errStr = "The registry could not be opened for reading.";
                    break;
                default:
                    errStr = "Something went terribly wrong.";
                    break;
            }
            logger.Write(DEBUG, "CLSIDFromString error: " + errStr);
        }
        RegisteredGUIDs.push_back(guid);		
        it++;
    }
    nDevices = it;

    // [TOGGLE_MOD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("TOGGLE_MOD", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        settingsWheel.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

    // [CHANGE_SHIFTMODE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);


    // [STEER]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("STEER", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        settingsWheel.GetValue("STEER", "AXLE", "");
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::ForceFeedback)] =
        settingsWheel.GetValue("STEER", "FFB", "");
    scriptControl->SteerMin = settingsWheel.GetLongValue("STEER", "MIN", -1);
    scriptControl->SteerMax = settingsWheel.GetLongValue("STEER", "MAX", -1);

    scriptControl->ADZSteer = settingsWheel.GetDoubleValue("STEER", "ANTIDEADZONE", 0.25);
    scriptControl->DZSteer = settingsWheel.GetDoubleValue("STEER", "DEADZONE", 0.0);
    scriptControl->DZSteerOffset = settingsWheel.GetDoubleValue("STEER", "DEADZONEOFFSET", 0.0);
    SteerGamma = settingsWheel.GetDoubleValue("STEER", "GAMMA", 1.0);

    SteerAngleMax = settingsWheel.GetDoubleValue("STEER", "SteerAngleMax", 900.0);
    SteerAngleCar = settingsWheel.GetDoubleValue("STEER", "SteerAngleCar", 720.0);
    SteerAngleBike = settingsWheel.GetDoubleValue("STEER", "SteerAngleBike", 180.0);
    SteerAngleBoat = settingsWheel.GetDoubleValue("STEER", "SteerAngleBoat", 360.0);

    // [THROTTLE]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        settingsWheel.GetValue("THROTTLE", "AXLE", "");
    scriptControl->ThrottleMin = settingsWheel.GetLongValue("THROTTLE", "MIN", -1);
    scriptControl->ThrottleMax = settingsWheel.GetLongValue("THROTTLE", "MAX", -1);
    scriptControl->ADZThrottle = settingsWheel.GetDoubleValue("THROTTLE", "ANTIDEADZONE", 0.25);
    ThrottleGamma = settingsWheel.GetDoubleValue("THROTTLE", "GAMMA", 1.0);

    // [BRAKES]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKES", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        settingsWheel.GetValue("BRAKES", "AXLE", "");
    scriptControl->BrakeMin = settingsWheel.GetLongValue("BRAKES", "MIN", -1);
    scriptControl->BrakeMax = settingsWheel.GetLongValue("BRAKES", "MAX", -1);
    scriptControl->ADZBrake = settingsWheel.GetDoubleValue("BRAKES", "ANTIDEADZONE", 0.25);
    BrakeGamma = settingsWheel.GetDoubleValue("BRAKES", "GAMMA", 1.0);

    // [CLUTCH]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        settingsWheel.GetValue("CLUTCH", "AXLE", "");
    scriptControl->ClutchMin = settingsWheel.GetLongValue("CLUTCH", "MIN", -1);
    scriptControl->ClutchMax = settingsWheel.GetLongValue("CLUTCH", "MAX", -1);

    // [HANDBRAKE_ANALOG]
    scriptControl->WheelAxesGUIDs[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        settingsWheel.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
    scriptControl->HandbrakeMin = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
    scriptControl->HandbrakeMax = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

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
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFTER", "DEVICE", -1), RegisteredGUIDs);

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
        DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE_BUTTON", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        settingsWheel.GetLongValue("THROTTLE_BUTTON", "BUTTON", -1);

    // [BRAKE_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Brake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKE_BUTTON", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Brake)] =
        settingsWheel.GetLongValue("BRAKE_BUTTON", "BUTTON", -1);

    // [CLUTCH_BUTTON]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH_BUTTON", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        settingsWheel.GetLongValue("CLUTCH_BUTTON", "BUTTON", -1);

    // [SHIFT_UP]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_UP", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        settingsWheel.GetLongValue("SHIFT_UP", "BUTTON", -1);

    // [SHIFT_DOWN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_DOWN", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        settingsWheel.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

    // [HANDBRAKE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        settingsWheel.GetLongValue("HANDBRAKE", "BUTTON", -1);

    // [ENGINE]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Engine)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("ENGINE", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Engine)] =
        settingsWheel.GetLongValue("ENGINE", "BUTTON", -1);

    // [LIGHTS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Lights)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LIGHTS", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Lights)] =
        settingsWheel.GetLongValue("LIGHTS", "BUTTON", -1);

    // [HORN]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Horn)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("HORN", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Horn)] =
        settingsWheel.GetLongValue("HORN", "BUTTON", -1);

    // [LOOK_BACK]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_BACK", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        settingsWheel.GetLongValue("LOOK_BACK", "BUTTON", -1);
    
    // [LOOK_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_LEFT", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        settingsWheel.GetLongValue("LOOK_LEFT", "BUTTON", -1);
    
    // [LOOK_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_RIGHT", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        settingsWheel.GetLongValue("LOOK_RIGHT", "BUTTON", -1);

    // [CHANGE_CAMERA]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::Camera)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_CAMERA", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Camera)] =
        settingsWheel.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

    // [RADIO_NEXT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_NEXT", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        settingsWheel.GetLongValue("RADIO_NEXT", "BUTTON", -1);

    // [RADIO_PREVIOUS]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_PREVIOUS", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        settingsWheel.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

    // [INDICATOR_LEFT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_LEFT", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        settingsWheel.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

    // [INDICATOR_RIGHT]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_RIGHT", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        settingsWheel.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

    // [INDICATOR_HAZARD]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_HAZARD", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        settingsWheel.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);

    // [AUTO_P]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::APark)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_P", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::APark)] =
        settingsWheel.GetLongValue("AUTO_P", "BUTTON", -1);

    // [AUTO_R]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_R", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        settingsWheel.GetLongValue("AUTO_R", "BUTTON", -1);

    // [AUTO_N]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_N", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        settingsWheel.GetLongValue("AUTO_N", "BUTTON", -1);

    // [AUTO_D]
    scriptControl->WheelButtonGUIDs[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        DeviceIndexToGUID(settingsWheel.GetLongValue("AUTO_D", "DEVICE", -1), RegisteredGUIDs);
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        settingsWheel.GetLongValue("AUTO_D", "BUTTON", -1);

    // [TO_KEYBOARD]
    scriptControl->WheelToKeyGUID = 
        DeviceIndexToGUID(settingsWheel.GetLongValue("TO_KEYBOARD", "DEVICE", -1), RegisteredGUIDs);
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
    auto found = find(RegisteredGUIDs.begin(), RegisteredGUIDs.end(), dev_guid);
    if (found != RegisteredGUIDs.end()) {
        // present! Return index
        // Dependent on implementation of reading this but it should work(TM). Lotsa assumptions.
        return distance(RegisteredGUIDs.begin(), found);
    }
    // missing! Add & return index afterwards
    auto newIndex = distance(RegisteredGUIDs.begin(), RegisteredGUIDs.end());
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

    settingsGeneral.SetLongValue("CONTROLLER_LEGACY", confTag.c_str(), button);

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
    for (auto guid : RegisteredGUIDs) {
        if (guid == guidToFind) {
            return i;
        }
        i++;
    }
    return -1;
}

#pragma warning(pop)
