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
    localSettings.MTParams.StallingRPM        = activeConfig->MTParams.StallingRPM      ;
    localSettings.MTParams.StallingRate       = activeConfig->MTParams.StallingRate     ;
    localSettings.MTParams.StallingSlip       = activeConfig->MTParams.StallingSlip     ;
    localSettings.MTParams.RPMDamage          = activeConfig->MTParams.RPMDamage        ;
    localSettings.MTParams.MisshiftDamage     = activeConfig->MTParams.MisshiftDamage   ;
    localSettings.MTParams.CreepIdleThrottle  = activeConfig->MTParams.CreepIdleThrottle;
    localSettings.MTParams.CreepIdleRPM       = activeConfig->MTParams.CreepIdleRPM     ;

    localSettings.DriveAssists.ABS.Enable       = activeConfig->DriveAssists.ABS.Enable      ;
    localSettings.DriveAssists.ABS.Filter       = activeConfig->DriveAssists.ABS.Filter      ;
    localSettings.DriveAssists.TCS.Enable       = activeConfig->DriveAssists.TCS.Enable      ;
    localSettings.DriveAssists.TCS.Mode         = activeConfig->DriveAssists.TCS.Mode        ;
    localSettings.DriveAssists.TCS.SlipMax      = activeConfig->DriveAssists.TCS.SlipMax     ;
    localSettings.DriveAssists.ESP.Enable       = activeConfig->DriveAssists.ESP.Enable      ;
    localSettings.DriveAssists.ESP.OverMin      = activeConfig->DriveAssists.ESP.OverMin     ;
    localSettings.DriveAssists.ESP.OverMax      = activeConfig->DriveAssists.ESP.OverMax     ;
    localSettings.DriveAssists.ESP.OverMinComp  = activeConfig->DriveAssists.ESP.OverMinComp ;
    localSettings.DriveAssists.ESP.OverMaxComp  = activeConfig->DriveAssists.ESP.OverMaxComp ;
    localSettings.DriveAssists.ESP.UnderMin     = activeConfig->DriveAssists.ESP.UnderMin    ;
    localSettings.DriveAssists.ESP.UnderMax     = activeConfig->DriveAssists.ESP.UnderMax    ;
    localSettings.DriveAssists.ESP.UnderMinComp = activeConfig->DriveAssists.ESP.UnderMinComp;
    localSettings.DriveAssists.ESP.UnderMaxComp = activeConfig->DriveAssists.ESP.UnderMaxComp;

    localSettings.DriveAssists.LSD.Enable = activeConfig->DriveAssists.LSD.Enable;
    localSettings.DriveAssists.LSD.Viscosity = activeConfig->DriveAssists.LSD.Viscosity;

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
    localSettings.AutoParams.UsingATCU = activeConfig->AutoParams.UsingATCU;


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

    // oh dear lord
    localSettings.HUD.Enable =               activeConfig->HUD.Enable;               
    localSettings.HUD.Font =                 activeConfig->HUD.Font;                 
    localSettings.HUD.Outline =              activeConfig->HUD.Outline;              

    localSettings.HUD.Gear.Enable =          activeConfig->HUD.Gear.Enable;          
    localSettings.HUD.Gear.XPos =            activeConfig->HUD.Gear.XPos;            
    localSettings.HUD.Gear.YPos =            activeConfig->HUD.Gear.YPos;            
    localSettings.HUD.Gear.Size =            activeConfig->HUD.Gear.Size;            
    localSettings.HUD.Gear.TopColorR =       activeConfig->HUD.Gear.TopColorR;       
    localSettings.HUD.Gear.TopColorG =       activeConfig->HUD.Gear.TopColorG;       
    localSettings.HUD.Gear.TopColorB =       activeConfig->HUD.Gear.TopColorB;       
    localSettings.HUD.Gear.ColorR =          activeConfig->HUD.Gear.ColorR;          
    localSettings.HUD.Gear.ColorG =          activeConfig->HUD.Gear.ColorG;          
    localSettings.HUD.Gear.ColorB =          activeConfig->HUD.Gear.ColorB;          

    localSettings.HUD.ShiftMode.Enable =     activeConfig->HUD.ShiftMode.Enable;     
    localSettings.HUD.ShiftMode.XPos =       activeConfig->HUD.ShiftMode.XPos;       
    localSettings.HUD.ShiftMode.YPos =       activeConfig->HUD.ShiftMode.YPos;       
    localSettings.HUD.ShiftMode.Size =       activeConfig->HUD.ShiftMode.Size;       
    localSettings.HUD.ShiftMode.ColorR =     activeConfig->HUD.ShiftMode.ColorR;     
    localSettings.HUD.ShiftMode.ColorG =     activeConfig->HUD.ShiftMode.ColorG;     
    localSettings.HUD.ShiftMode.ColorB =     activeConfig->HUD.ShiftMode.ColorB;     

    localSettings.HUD.Speedo.Speedo =        activeConfig->HUD.Speedo.Speedo;        
    localSettings.HUD.Speedo.ShowUnit =      activeConfig->HUD.Speedo.ShowUnit;      
    localSettings.HUD.Speedo.XPos =          activeConfig->HUD.Speedo.XPos;          
    localSettings.HUD.Speedo.YPos =          activeConfig->HUD.Speedo.YPos;          
    localSettings.HUD.Speedo.Size =          activeConfig->HUD.Speedo.Size;          
    localSettings.HUD.Speedo.ColorR =        activeConfig->HUD.Speedo.ColorR;        
    localSettings.HUD.Speedo.ColorG =        activeConfig->HUD.Speedo.ColorG;        
    localSettings.HUD.Speedo.ColorB =        activeConfig->HUD.Speedo.ColorB;        

    localSettings.HUD.RPMBar.Enable =        activeConfig->HUD.RPMBar.Enable;        
    localSettings.HUD.RPMBar.XPos =          activeConfig->HUD.RPMBar.XPos;          
    localSettings.HUD.RPMBar.YPos =          activeConfig->HUD.RPMBar.YPos;          
    localSettings.HUD.RPMBar.XSz =           activeConfig->HUD.RPMBar.XSz;           
    localSettings.HUD.RPMBar.YSz =           activeConfig->HUD.RPMBar.YSz;           
    localSettings.HUD.RPMBar.Redline =       activeConfig->HUD.RPMBar.Redline;       

    localSettings.HUD.RPMBar.BgR =           activeConfig->HUD.RPMBar.BgR;           
    localSettings.HUD.RPMBar.BgG =           activeConfig->HUD.RPMBar.BgG;           
    localSettings.HUD.RPMBar.BgB =           activeConfig->HUD.RPMBar.BgB;           
    localSettings.HUD.RPMBar.BgA =           activeConfig->HUD.RPMBar.BgA;           

    localSettings.HUD.RPMBar.FgR =           activeConfig->HUD.RPMBar.FgR;           
    localSettings.HUD.RPMBar.FgG =           activeConfig->HUD.RPMBar.FgG;           
    localSettings.HUD.RPMBar.FgB =           activeConfig->HUD.RPMBar.FgB;           
    localSettings.HUD.RPMBar.FgA =           activeConfig->HUD.RPMBar.FgA;           

    localSettings.HUD.RPMBar.RedlineR =      activeConfig->HUD.RPMBar.RedlineR;      
    localSettings.HUD.RPMBar.RedlineG =      activeConfig->HUD.RPMBar.RedlineG;      
    localSettings.HUD.RPMBar.RedlineB =      activeConfig->HUD.RPMBar.RedlineB;      
    localSettings.HUD.RPMBar.RedlineA =      activeConfig->HUD.RPMBar.RedlineA;      

    localSettings.HUD.RPMBar.RevLimitR =     activeConfig->HUD.RPMBar.RevLimitR;     
    localSettings.HUD.RPMBar.RevLimitG =     activeConfig->HUD.RPMBar.RevLimitG;     
    localSettings.HUD.RPMBar.RevLimitB =     activeConfig->HUD.RPMBar.RevLimitB;     
    localSettings.HUD.RPMBar.RevLimitA =     activeConfig->HUD.RPMBar.RevLimitA;     

    localSettings.HUD.DashIndicators.Enable =activeConfig->HUD.DashIndicators.Enable;
    localSettings.HUD.DashIndicators.XPos =  activeConfig->HUD.DashIndicators.XPos;  
    localSettings.HUD.DashIndicators.YPos =  activeConfig->HUD.DashIndicators.YPos;  
    localSettings.HUD.DashIndicators.Size =  activeConfig->HUD.DashIndicators.Size;  
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
    ini.SetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    ini.SetDoubleValue("MT_PARAMS", "StallingRate", MTParams.StallingRate);
    ini.SetDoubleValue("MT_PARAMS", "StallingSlip", MTParams.StallingSlip);
    ini.SetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    ini.SetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    ini.SetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    ini.SetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);
    ini.SetDoubleValue("MT_PARAMS", "CreepIdleThrottle", MTParams.CreepIdleThrottle);
    ini.SetDoubleValue("MT_PARAMS", "CreepIdleRPM", MTParams.CreepIdleRPM);

    // [GAMEPLAY_ASSISTS]
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "DisableAutostart", GameAssists.DisableAutostart);
    ini.SetBoolValue("GAMEPLAY_ASSISTS", "LeaveEngineRunning", GameAssists.LeaveEngineRunning);

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

    ini.SetBoolValue("DRIVING_ASSISTS", "LSD", DriveAssists.LSD.Enable);
    ini.SetDoubleValue("DRIVING_ASSISTS", "LSDViscosity", DriveAssists.LSD.Viscosity);

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

    ini.SetBoolValue("CUSTOM_STEERING", "MouseSteering", CustomSteering.MouseSteering);
    ini.SetDoubleValue("CUSTOM_STEERING", "MouseSensitivity", CustomSteering.MouseSensitivity);

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
    ini.SetBoolValue("AUTO_PARAMS", "UsingATCU", AutoParams.UsingATCU);

    // [HUD]
    ini.SetBoolValue("HUD", "EnableHUD", HUD.Enable);
    ini.SetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    ini.SetLongValue("HUD", "HUDFont", HUD.Font);
    ini.SetBoolValue("HUD", "Outline", HUD.Outline);
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

    ini.SetBoolValue("HUD", "MouseEnable", HUD.MouseSteering.Enable);
    ini.SetDoubleValue("HUD", "MouseXPos", HUD.MouseSteering.XPos);
    ini.SetDoubleValue("HUD", "MouseYPos", HUD.MouseSteering.YPos);
    ini.SetDoubleValue("HUD", "MouseXSz", HUD.MouseSteering.XSz);
    ini.SetDoubleValue("HUD", "MouseYSz", HUD.MouseSteering.YSz);
    ini.SetDoubleValue("HUD", "MouseMarkerXSz", HUD.MouseSteering.MarkerXSz);

    ini.SetLongValue("HUD", "MouseBgR", HUD.MouseSteering.BgR);
    ini.SetLongValue("HUD", "MouseBgG", HUD.MouseSteering.BgG);
    ini.SetLongValue("HUD", "MouseBgB", HUD.MouseSteering.BgB);
    ini.SetLongValue("HUD", "MouseBgA", HUD.MouseSteering.BgA);

    ini.SetLongValue("HUD", "MouseFgR", HUD.MouseSteering.FgR);
    ini.SetLongValue("HUD", "MouseFgG", HUD.MouseSteering.FgG);
    ini.SetLongValue("HUD", "MouseFgB", HUD.MouseSteering.FgB);
    ini.SetLongValue("HUD", "MouseFgA", HUD.MouseSteering.FgA);

    // [MISC]
    ini.SetBoolValue("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    ini.SetBoolValue("MISC", "DashExtensions", Misc.DashExtensions);
    ini.SetBoolValue("MISC", "SyncAnimations", Misc.SyncAnimations);
    ini.SetBoolValue("MISC", "HidePlayerInFPV", Misc.HidePlayerInFPV);

    // [CAM]
    ini.SetBoolValue("CAM", "Enable", Misc.Camera.Enable);
    ini.SetLongValue("CAM", "AttachId", Misc.Camera.AttachId);
    ini.SetBoolValue("CAM", "FollowMovement", Misc.Camera.FollowMovement);
    ini.SetDoubleValue("CAM", "MovementMultVel", Misc.Camera.MovementMultVel);
    ini.SetDoubleValue("CAM", "MovementMultRot", Misc.Camera.MovementMultRot);
    ini.SetDoubleValue("CAM", "MovementCap", Misc.Camera.MovementCap);
    ini.SetBoolValue("CAM", "RemoveHead", Misc.Camera.RemoveHead);
    ini.SetDoubleValue("CAM", "FOV", Misc.Camera.FOV);
    ini.SetDoubleValue("CAM", "OffsetHeight", Misc.Camera.OffsetHeight);
    ini.SetDoubleValue("CAM", "OffsetForward", Misc.Camera.OffsetForward);
    ini.SetDoubleValue("CAM", "OffsetSide", Misc.Camera.OffsetSide);
    ini.SetDoubleValue("CAM", "Pitch", Misc.Camera.Pitch);
    ini.SetDoubleValue("CAM", "LookTime", Misc.Camera.LookTime);
    ini.SetDoubleValue("CAM", "MouseLookTime", Misc.Camera.MouseLookTime);
    ini.SetLongValue("CAM", "MouseCenterTimeout", Misc.Camera.MouseCenterTimeout);
    ini.SetDoubleValue("CAM", "MouseSensitivity", Misc.Camera.MouseSensitivity);

    ini.SetBoolValue("CAM", "BikeDisable", Misc.Camera.Bike.Disable);
    ini.SetLongValue("CAM", "BikeAttachId", Misc.Camera.Bike.AttachId);
    ini.SetDoubleValue("CAM", "BikeFOV", Misc.Camera.Bike.FOV);
    ini.SetDoubleValue("CAM", "BikeOffsetHeight", Misc.Camera.Bike.OffsetHeight);
    ini.SetDoubleValue("CAM", "BikeOffsetForward", Misc.Camera.Bike.OffsetForward);
    ini.SetDoubleValue("CAM", "BikeOffsetSide", Misc.Camera.Bike.OffsetSide);
    ini.SetDoubleValue("CAM", "BikePitch", Misc.Camera.Bike.Pitch);

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
    MTParams.StallingRPM =          ini.GetDoubleValue("MT_PARAMS", "StallingRPM", MTParams.StallingRPM);
    MTParams.StallingRate =         ini.GetDoubleValue("MT_PARAMS", "StallingRate", MTParams.StallingRate);
    MTParams.StallingSlip =         ini.GetDoubleValue("MT_PARAMS", "StallingSlip", MTParams.StallingSlip);
    MTParams.RPMDamage =            ini.GetDoubleValue("MT_PARAMS", "RPMDamage", MTParams.RPMDamage);
    MTParams.MisshiftDamage =       ini.GetDoubleValue("MT_PARAMS", "MisshiftDamage", MTParams.MisshiftDamage);
    MTParams.EngBrakePower =        ini.GetDoubleValue("MT_PARAMS", "EngBrakePower", MTParams.EngBrakePower);
    MTParams.EngBrakeThreshold =    ini.GetDoubleValue("MT_PARAMS", "EngBrakeThreshold", MTParams.EngBrakeThreshold);
    MTParams.CreepIdleThrottle =    ini.GetDoubleValue("MT_PARAMS", "CreepIdleThrottle", MTParams.CreepIdleThrottle);
    MTParams.CreepIdleRPM      =    ini.GetDoubleValue("MT_PARAMS", "CreepIdleRPM"     , MTParams.CreepIdleRPM     );

    GameAssists.DefaultNeutral =    ini.GetBoolValue("GAMEPLAY_ASSISTS", "DefaultNeutral", GameAssists.DefaultNeutral);
    GameAssists.SimpleBike =        ini.GetBoolValue("GAMEPLAY_ASSISTS", "SimpleBike", GameAssists.SimpleBike);
    GameAssists.HillGravity =       ini.GetBoolValue("GAMEPLAY_ASSISTS", "HillBrakeWorkaround", GameAssists.HillGravity);
    GameAssists.AutoGear1 =         ini.GetBoolValue("GAMEPLAY_ASSISTS", "AutoGear1", GameAssists.AutoGear1);
    GameAssists.AutoLookBack =      ini.GetBoolValue("GAMEPLAY_ASSISTS", "AutoLookBack", GameAssists.AutoLookBack);
    GameAssists.ThrottleStart =     ini.GetBoolValue("GAMEPLAY_ASSISTS", "ThrottleStart", GameAssists.ThrottleStart);
    GameAssists.DisableAutostart =  ini.GetBoolValue("GAMEPLAY_ASSISTS", "DisableAutostart", GameAssists.DisableAutostart);
    GameAssists.LeaveEngineRunning = ini.GetBoolValue("GAMEPLAY_ASSISTS", "LeaveEngineRunning", GameAssists.LeaveEngineRunning);

    // [DRIVING_ASSISTS]
    DriveAssists.ABS.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "ABS", DriveAssists.ABS.Enable);
    DriveAssists.ABS.Filter = ini.GetBoolValue("DRIVING_ASSISTS", "ABSFilter", DriveAssists.ABS.Filter);
    DriveAssists.TCS.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "TCS", DriveAssists.TCS.Enable);
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

    DriveAssists.LSD.Enable = ini.GetBoolValue("DRIVING_ASSISTS", "LSD", DriveAssists.LSD.Enable);
    DriveAssists.LSD.Viscosity = ini.GetDoubleValue("DRIVING_ASSISTS", "LSDViscosity", DriveAssists.LSD.Viscosity);

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
    AutoParams.UsingATCU = ini.GetBoolValue("AUTO_PARAMS", "UsingATCU", AutoParams.UsingATCU);

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

    CustomSteering.MouseSteering = ini.GetBoolValue("CUSTOM_STEERING", "MouseSteering", CustomSteering.MouseSteering);
    CustomSteering.MouseSensitivity = ini.GetDoubleValue("CUSTOM_STEERING", "MouseSensitivity", CustomSteering.MouseSensitivity);

    // [HUD]
    HUD.Enable = ini.GetBoolValue("HUD", "EnableHUD", HUD.Enable);
    HUD.Always = ini.GetBoolValue("HUD", "AlwaysHUD", HUD.Always);
    HUD.Font = ini.GetLongValue("HUD", "HUDFont", HUD.Font);
    HUD.Outline = ini.GetBoolValue("HUD", "Outline", HUD.Outline);
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

    HUD.MouseSteering.Enable = ini.GetBoolValue("HUD", "MouseEnable", HUD.MouseSteering.Enable);
    HUD.MouseSteering.XPos = ini.GetDoubleValue("HUD", "MouseXPos", HUD.MouseSteering.XPos);
    HUD.MouseSteering.YPos = ini.GetDoubleValue("HUD", "MouseYPos", HUD.MouseSteering.YPos);
    HUD.MouseSteering.XSz = ini.GetDoubleValue("HUD", "MouseXSz", HUD.MouseSteering.XSz);
    HUD.MouseSteering.YSz = ini.GetDoubleValue("HUD", "MouseYSz", HUD.MouseSteering.YSz);
    HUD.MouseSteering.MarkerXSz = ini.GetDoubleValue("HUD", "MouseMarkerXSz", HUD.MouseSteering.MarkerXSz);

    HUD.MouseSteering.BgR = ini.GetLongValue("HUD", "MouseBgR", HUD.MouseSteering.BgR);
    HUD.MouseSteering.BgG = ini.GetLongValue("HUD", "MouseBgG", HUD.MouseSteering.BgG);
    HUD.MouseSteering.BgB = ini.GetLongValue("HUD", "MouseBgB", HUD.MouseSteering.BgB);
    HUD.MouseSteering.BgA = ini.GetLongValue("HUD", "MouseBgA", HUD.MouseSteering.BgA);

    HUD.MouseSteering.FgR = ini.GetLongValue("HUD", "MouseFgR", HUD.MouseSteering.FgR);
    HUD.MouseSteering.FgG = ini.GetLongValue("HUD", "MouseFgG", HUD.MouseSteering.FgG);
    HUD.MouseSteering.FgB = ini.GetLongValue("HUD", "MouseFgB", HUD.MouseSteering.FgB);
    HUD.MouseSteering.FgA = ini.GetLongValue("HUD", "MouseFgA", HUD.MouseSteering.FgA);

    // [MISC]
    Misc.UDPTelemetry = ini.GetBoolValue("MISC", "UDPTelemetry", Misc.UDPTelemetry);
    Misc.DashExtensions = ini.GetBoolValue("MISC", "DashExtensions", Misc.DashExtensions);
    Misc.SyncAnimations = ini.GetBoolValue("MISC", "SyncAnimations", Misc.SyncAnimations);
    Misc.HidePlayerInFPV = ini.GetBoolValue("MISC", "HidePlayerInFPV", Misc.HidePlayerInFPV);

    // [CAM]
    Misc.Camera.Enable = ini.GetBoolValue("CAM", "Enable", Misc.Camera.Enable);
    Misc.Camera.AttachId = ini.GetLongValue("CAM", "AttachId", Misc.Camera.AttachId);
    Misc.Camera.FollowMovement = ini.GetBoolValue("CAM", "FollowMovement", Misc.Camera.FollowMovement);
    Misc.Camera.MovementMultVel = ini.GetDoubleValue("CAM", "MovementMultVel", Misc.Camera.MovementMultVel);
    Misc.Camera.MovementMultRot = ini.GetDoubleValue("CAM", "MovementMultRot", Misc.Camera.MovementMultRot);
    Misc.Camera.MovementCap = ini.GetDoubleValue("CAM", "MovementCap", Misc.Camera.MovementCap);
    Misc.Camera.RemoveHead = ini.GetBoolValue("CAM", "RemoveHead", Misc.Camera.RemoveHead);
    Misc.Camera.FOV = ini.GetDoubleValue("CAM", "FOV", Misc.Camera.FOV);
    Misc.Camera.OffsetHeight = ini.GetDoubleValue("CAM", "OffsetHeight", Misc.Camera.OffsetHeight);
    Misc.Camera.OffsetForward = ini.GetDoubleValue("CAM", "OffsetForward", Misc.Camera.OffsetForward);
    Misc.Camera.OffsetSide = ini.GetDoubleValue("CAM", "OffsetSide", Misc.Camera.OffsetSide);
    Misc.Camera.Pitch = ini.GetDoubleValue("CAM", "Pitch", Misc.Camera.Pitch);
    Misc.Camera.LookTime = ini.GetDoubleValue("CAM", "LookTime", Misc.Camera.LookTime);
    Misc.Camera.MouseLookTime = ini.GetDoubleValue("CAM", "MouseLookTime", Misc.Camera.MouseLookTime);
    Misc.Camera.MouseCenterTimeout = ini.GetLongValue("CAM", "MouseCenterTimeout", Misc.Camera.MouseCenterTimeout);
    Misc.Camera.MouseSensitivity = ini.GetDoubleValue("CAM", "MouseSensitivity", Misc.Camera.MouseSensitivity);

    Misc.Camera.Bike.Disable = ini.GetBoolValue("CAM", "BikeDisable", Misc.Camera.Bike.Disable);
    Misc.Camera.Bike.AttachId = ini.GetLongValue("CAM", "BikeAttachId", Misc.Camera.Bike.AttachId);
    Misc.Camera.Bike.FOV = ini.GetDoubleValue("CAM", "BikeFOV", Misc.Camera.Bike.FOV);
    Misc.Camera.Bike.OffsetHeight = ini.GetDoubleValue("CAM", "BikeOffsetHeight", Misc.Camera.Bike.OffsetHeight);
    Misc.Camera.Bike.OffsetForward = ini.GetDoubleValue("CAM", "BikeOffsetForward", Misc.Camera.Bike.OffsetForward);
    Misc.Camera.Bike.OffsetSide = ini.GetDoubleValue("CAM", "BikeOffsetSide", Misc.Camera.Bike.OffsetSide);
    Misc.Camera.Bike.Pitch = ini.GetDoubleValue("CAM", "BikePitch", Misc.Camera.Bike.Pitch);

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
    Controller.BlockCarControls = ini.GetBoolValue("CONTROLLER", "BlockCarControls", Controller.BlockCarControls);
    Controller.IgnoreShiftsUI = ini.GetBoolValue("CONTROLLER", "IgnoreShiftsUI", Controller.IgnoreShiftsUI);
    Controller.BlockHShift = ini.GetBoolValue("CONTROLLER", "BlockHShift", Controller.BlockHShift);

    Controller.HoldTimeMs = ini.GetLongValue("CONTROLLER", "HoldTimeMs", Controller.HoldTimeMs);
    Controller.MaxTapTimeMs = ini.GetLongValue("CONTROLLER", "MaxTapTimeMs", Controller.MaxTapTimeMs);
    Controller.TriggerValue = ini.GetDoubleValue("CONTROLLER", "TriggerValue", Controller.TriggerValue);

    Controller.ToggleEngine = ini.GetBoolValue("CONTROLLER", "ToggleEngine", Controller.ToggleEngine);

    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Toggle)] = parseControllerItem<std::string>(ini, "Toggle", "UNKNOWN", "Toggle MT", "Usage: hold");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ToggleH)] = parseControllerItem<std::string>(ini, "ToggleShift", "B", "Change shift mode", "Usage: hold");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::CycleAssists)] = parseControllerItem<std::string>(ini, "CycleAssists", "UNKNOWN", "Cycle assists", "Usage: hold");

    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] =   parseControllerItem<std::string>(ini, "ShiftUp", "A", "Shift up", "Usage: tap");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = parseControllerItem<std::string>(ini, "ShiftDown", "X", "Shift down", "Usage: tap");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Engine)] =    parseControllerItem<std::string>(ini, "Engine", "DpadDown", "Engine", "Usage: hold");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Throttle)] =  parseControllerItem<std::string>(ini, "Throttle", "RightTrigger", "Throttle", "Usage: analog");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Brake)] =     parseControllerItem<std::string>(ini, "Brake", "LeftTrigger", "Brake", "Usage: analog");
    scriptControl->ControlXbox[static_cast<int>(CarControls::ControllerControlType::Clutch)] =    parseControllerItem<std::string>(ini, "Clutch", "LeftThumbUp", "Clutch", "Usage: hold/analog");

    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftUp)] =   ini.GetLongValue("CONTROLLER", "ShiftUpBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER", "ShiftDownBlocks", -1);
    scriptControl->ControlXboxBlocks[static_cast<int>(CarControls::ControllerControlType::Clutch)] =    ini.GetLongValue("CONTROLLER", "ClutchBlocks", -1);

    // [CONTROLLER_NATIVE]
    Controller.Native.Enable = ini.GetBoolValue("CONTROLLER_NATIVE", "Enable", Controller.Native.Enable);

    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Toggle)] =       parseControllerItem<eControl>(ini, "Toggle", static_cast<eControl>(-1), "Toggle MT", "Usage: hold");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ToggleH)] =      parseControllerItem<eControl>(ini, "ToggleShift", ControlFrontendCancel, "Change shift mode", "Usage: hold");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::CycleAssists)] = parseControllerItem<eControl>(ini, "CycleAssists", static_cast<eControl>(-1), "Cycle assists", "Usage: hold");

    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] =   parseControllerItem<eControl>(ini, "ShiftUp", ControlFrontendAccept, "Shift up", "Usage: tap");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = parseControllerItem<eControl>(ini, "ShiftDown", ControlFrontendX   , "Shift down", "Usage: tap");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Engine)] =    parseControllerItem<eControl>(ini, "Engine", ControlFrontendDown   , "Engine", "Usage: hold");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Throttle)] =  parseControllerItem<eControl>(ini, "Throttle", ControlFrontendRt   , "Throttle", "Usage: analog");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Brake)] =     parseControllerItem<eControl>(ini, "Brake", ControlFrontendLt      , "Brake", "Usage: analog");
    scriptControl->LegacyControls[static_cast<int>(CarControls::LegacyControlType::Clutch)] =    parseControllerItem<eControl>(ini, "Clutch", ControlFrontendAxisY  , "Clutch", "Usage: hold/analog");

    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftUp)] =   ini.GetLongValue("CONTROLLER_NATIVE", "ShiftUpBlocks", -1)  ;
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::ShiftDown)] = ini.GetLongValue("CONTROLLER_NATIVE", "ShiftDownBlocks", -1);
    scriptControl->ControlNativeBlocks[static_cast<int>(CarControls::LegacyControlType::Clutch)] =    ini.GetLongValue("CONTROLLER_NATIVE", "ClutchBlocks", -1)   ;

    // [KEYBOARD]
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Toggle)] = parseKeyboardItem(ini, "Toggle", "VK_OEM_5", "Toggle MT");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ToggleH)] = parseKeyboardItem(ini, "ToggleH", "VK_OEM_6", "Switch shift mode");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::CycleAssists)] = parseKeyboardItem(ini, "CycleAssists", "UNKNOWN", "Cycle assists");

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftUp)] = parseKeyboardItem(ini, "ShiftUp", "LSHIFT", "Shift up");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::ShiftDown)] = parseKeyboardItem(ini, "ShiftDown", "LCTRL", "Shift down");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Clutch)] = parseKeyboardItem(ini, "Clutch", "Z");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Engine)] = parseKeyboardItem(ini, "Engine", "X");

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Throttle)] = parseKeyboardItem(ini, "Throttle", "W");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::Brake)] = parseKeyboardItem(ini, "Brake", "S");

    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HR)] =  parseKeyboardItem(ini, "HR", "UNKNOWN", "H-pattern reverse");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H1)] =  parseKeyboardItem(ini, "H1", "UNKNOWN", "H-pattern 1");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H2)] =  parseKeyboardItem(ini, "H2", "UNKNOWN", "H-pattern 2");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H3)] =  parseKeyboardItem(ini, "H3", "UNKNOWN", "H-pattern 3");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H4)] =  parseKeyboardItem(ini, "H4", "UNKNOWN", "H-pattern 4");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H5)] =  parseKeyboardItem(ini, "H5", "UNKNOWN", "H-pattern 5");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H6)] =  parseKeyboardItem(ini, "H6", "UNKNOWN", "H-pattern 6");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H7)] =  parseKeyboardItem(ini, "H7", "UNKNOWN", "H-pattern 7");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H8)] =  parseKeyboardItem(ini, "H8", "UNKNOWN", "H-pattern 8");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H9)] =  parseKeyboardItem(ini, "H9", "UNKNOWN", "H-pattern 9");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::H10)] = parseKeyboardItem(ini, "H10", "UNKNOWN", "H-pattern 10");
    scriptControl->KBControl[static_cast<int>(CarControls::KeyboardControlType::HN)] =  parseKeyboardItem(ini, "HN", "UNKNOWN", "H-pattern neutral");
}

void ScriptSettings::parseSettingsWheel(CarControls *scriptControl) {
    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiKey();
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
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Toggle)] =
        parseWheelItem<int>(ini, "TOGGLE_MOD", -1, "Toggle MT");

    // [CHANGE_SHIFTMODE]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleH)] =
        parseWheelItem<int>(ini, "CHANGE_SHIFTMODE", -1, "Change shift mode");

    // [STEER]
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Steer)] =
        parseWheelItem<std::string>(ini, "STEER", "", "Steering");

    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::ForceFeedback)] =
        CarControls::SInput<std::string>("FFB",
            DeviceIndexToGUID(ini.GetLongValue("STEER", "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetValue("STEER", "FFB", ""),
            "Force feedback", "Force feedback");

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
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Throttle)] =
        parseWheelItem<std::string>(ini, "THROTTLE", "");
    Wheel.Throttle.Min = ini.GetLongValue("THROTTLE", "MIN", -1);
    Wheel.Throttle.Max = ini.GetLongValue("THROTTLE", "MAX", -1);
    Wheel.Throttle.AntiDeadZone = ini.GetDoubleValue("THROTTLE", "ANTIDEADZONE", 0.25);
    Wheel.Throttle.Gamma = ini.GetDoubleValue("THROTTLE", "GAMMA", 1.0);

    // [BRAKE]
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Brake)] =
        parseWheelItem<std::string>(ini, "BRAKE", "");
    Wheel.Brake.Min = ini.GetLongValue("BRAKE", "MIN", -1);
    Wheel.Brake.Max = ini.GetLongValue("BRAKE", "MAX", -1);
    Wheel.Brake.AntiDeadZone = ini.GetDoubleValue("BRAKE", "ANTIDEADZONE", 0.25);
    Wheel.Brake.Gamma = ini.GetDoubleValue("BRAKE", "GAMMA", 1.0);

    // [CLUTCH]
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Clutch)] =
        parseWheelItem<std::string>(ini, "CLUTCH", "");
    Wheel.Clutch.Min = ini.GetLongValue("CLUTCH", "MIN", -1);
    Wheel.Clutch.Max = ini.GetLongValue("CLUTCH", "MAX", -1);

    // [HANDBRAKE_ANALOG]
    scriptControl->WheelAxes[static_cast<int>(CarControls::WheelAxisType::Handbrake)] =
        parseWheelItem<std::string>(ini, "HANDBRAKE_ANALOG", "", "Handbrake (analog)");
    Wheel.HandbrakeA.Min = ini.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
    Wheel.HandbrakeA.Max = ini.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

    // enums HR through H10 are explicitly defined as 0 through 10
    // [HPATTERN_0]
    scriptControl->WheelButton[0] =
        parseWheelItem<int>(ini, "HPATTERN_0", -1, "H-pattern reverse");

    // [HPATTERN_<gear>]
    for (uint8_t i = 1; i < 11; ++i) {
        scriptControl->WheelButton[i] =
            parseWheelItem<int>(ini, fmt::format("HPATTERN_{}", i).c_str(), -1, fmt::format("H-pattern {}", i).c_str());
    }

    // [THROTTLE_BUTTON]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Throttle)] =
        parseWheelItem<int>(ini, "THROTTLE_BUTTON", -1, "Throttle (Button)");

    // [BRAKE_BUTTON]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Brake)] =
        parseWheelItem<int>(ini, "BRAKE_BUTTON", -1, "Brake (Button)");

    // [CLUTCH_BUTTON]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Clutch)] =
        parseWheelItem<int>(ini, "CLUTCH_BUTTON", -1, "Clutch (Button)");

    // [SHIFT_UP]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftUp)] =
        parseWheelItem<int>(ini, "SHIFT_UP", -1);

    // [SHIFT_DOWN]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ShiftDown)] =
        parseWheelItem<int>(ini, "SHIFT_DOWN", -1);

    // [HANDBRAKE]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Handbrake)] =
        parseWheelItem<int>(ini, "HANDBRAKE", -1);

    // [ENGINE]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Engine)] =
        parseWheelItem<int>(ini, "ENGINE", -1);

    // [LIGHTS]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Lights)] =
        parseWheelItem<int>(ini, "LIGHTS", -1);

    // [HORN]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Horn)] =
        parseWheelItem<int>(ini, "HORN", -1);

    // [LOOK_BACK]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookBack)] =
        parseWheelItem<int>(ini, "LOOK_BACK", -1);

    // [LOOK_LEFT]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookLeft)] =
        parseWheelItem<int>(ini, "LOOK_LEFT", -1);

    // [LOOK_RIGHT]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::LookRight)] =
        parseWheelItem<int>(ini, "LOOK_RIGHT", -1);

    // [CHANGE_CAMERA]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::Camera)] =
        parseWheelItem<int>(ini, "CHANGE_CAMERA", -1);

    // [RADIO_NEXT]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioNext)] =
        parseWheelItem<int>(ini, "RADIO_NEXT", -1);

    // [RADIO_PREVIOUS]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::RadioPrev)] =
        parseWheelItem<int>(ini, "RADIO_PREVIOUS", -1);

    // [INDICATOR_LEFT]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorLeft)] =
        parseWheelItem<int>(ini, "INDICATOR_LEFT", -1);

    // [INDICATOR_RIGHT]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorRight)] =
        parseWheelItem<int>(ini, "INDICATOR_RIGHT", -1);

    // [INDICATOR_HAZARD]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::IndicatorHazard)] =
        parseWheelItem<int>(ini, "INDICATOR_HAZARD", -1);

    // [AUTO_P]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::APark)] =
        parseWheelItem<int>(ini, "AUTO_P", -1, "Automatic park");

    // [AUTO_R]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::AReverse)] =
        parseWheelItem<int>(ini, "AUTO_R", -1, "Automatic reverse");

    // [AUTO_N]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ANeutral)] =
        parseWheelItem<int>(ini, "AUTO_N", -1, "Automatic neutral");

    // [AUTO_D]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ADrive)] =
        parseWheelItem<int>(ini, "AUTO_D", -1, "Automatic drive");

    // [CYCLE_ASSISTS]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::CycleAssists)] =
        parseWheelItem<int>(ini, "CYCLE_ASSISTS", -1);

    // [TOGGLE_ABS]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleABS)] =
        parseWheelItem<int>(ini, "TOGGLE_ABS", -1, "Toggle ABS");

    // [TOGGLE_ESC]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleESC)] =
        parseWheelItem<int>(ini, "TOGGLE_ESC", -1, "Toggle ESC");

    // [TOGGLE_TCS]
    scriptControl->WheelButton[static_cast<int>(CarControls::WheelControlType::ToggleTCS)] =
        parseWheelItem<int>(ini, "TOGGLE_TCS", -1, "Toggle TCS");

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
CarControls::SInput<T> ScriptSettings::parseWheelItem(CSimpleIniA& ini, const char* section, T default, const char* name) {
    std::string nameFmt = formatInputName(section, name);
    if constexpr (std::is_same<T, int>::value) {
        return CarControls::SInput<T>(section,
            DeviceIndexToGUID(ini.GetLongValue(section, "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetLongValue(section, "BUTTON", default), nameFmt.c_str(), "");
    }
    else if constexpr (std::is_same<T, std::string>::value) {
        return CarControls::SInput<T>(section,
            DeviceIndexToGUID(ini.GetLongValue(section, "DEVICE", -1), Wheel.InputDevices.RegisteredGUIDs),
            ini.GetValue(section, "AXLE", default.c_str()), nameFmt.c_str(), "");
    }
    else {
        static_assert(false, "Type must be string or int.");
    }
}

CarControls::SInput<int> ScriptSettings::parseKeyboardItem(CSimpleIniA& ini, const char* key, const char* default, const char* name) {
    std::string nameFmt = formatInputName(key, name);
    return CarControls::SInput<int>(key, {}, str2key(ini.GetValue("KEYBOARD", key, default)), nameFmt, "");
}

template <typename T>
CarControls::SInput<T> ScriptSettings::parseControllerItem(CSimpleIniA& ini, const char* key, T default, const char* name, const char* description) {
    if constexpr (std::is_same<T, eControl>::value) {
        return CarControls::SInput<T>(key, {},
            static_cast<T>(ini.GetLongValue("CONTROLLER_NATIVE", key, static_cast<int>(default))),
            name, description);
    }
    else if constexpr (std::is_same<T, std::string>::value) {
        return CarControls::SInput<T>(key, {},
            ini.GetValue("CONTROLLER", key, default.c_str()),
            name, description);
    }
    else {
        static_assert(false, "Type must be string or eControl.");
    }
}

#pragma warning(pop)
