#include "ScriptSettings.hpp"

#include <string>

#include "simpleini/SimpleIni.h"
#include "Util/Logger.hpp"
#include "Input/keyboard.h"
#include "Input/ScriptControls.hpp"

#include "Util/Versions.h"

ScriptSettings::ScriptSettings() { }

void ScriptSettings::SetFiles(const std::string &general, const std::string &wheel, const std::string &stick) {
	settingsGeneralFile = general;
	settingsWheelFile = wheel;
	settingsStickFile = stick;
}

void ScriptSettings::Read(ScriptControls* scriptControl) {
	parseSettingsGeneral(scriptControl);
	parseSettingsWheel(scriptControl);
	parseSettingsStick(scriptControl);
}

void ScriptSettings::SaveGeneral() const {
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());
	
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

	settingsGeneral.SetDoubleValue("OPTIONS", "ClutchCatchpoint", ClutchCatchpoint * 100);
	settingsGeneral.SetDoubleValue("OPTIONS", "StallingThreshold", StallingThreshold * 100);
	settingsGeneral.SetDoubleValue("OPTIONS", "RPMDamage", RPMDamage * 100);
	settingsGeneral.SetDoubleValue("OPTIONS", "MisshiftDamage", MisshiftDamage);
	settingsGeneral.SetDoubleValue("OPTIONS", "EngBrakePower", EngBrakePower);
	settingsGeneral.SetDoubleValue("OPTIONS", "EngBrakeThreshold", EngBrakeThreshold);

	settingsGeneral.SetBoolValue("OPTIONS", "HillBrakeWorkaround", HillBrakeWorkaround);
	settingsGeneral.SetBoolValue("OPTIONS", "AutoGear1", AutoGear1);
	settingsGeneral.SetBoolValue("OPTIONS", "AutoLookBack", AutoLookBack);
	settingsGeneral.SetBoolValue("OPTIONS", "ThrottleStart", ThrottleStart);

	settingsGeneral.SetBoolValue("OPTIONS", "CrossScript", CrossScript);

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


	// [DEBUG]
	settingsGeneral.SetBoolValue("DEBUG", "DisplayInfo", DisplayInfo);
	settingsGeneral.SetBoolValue("DEBUG", "DisplayWheelInfo", DisplayWheelInfo);
	settingsGeneral.SetBoolValue("DEBUG", "DisplayGearingInfo", DisplayGearingInfo);

	settingsGeneral.SaveFile(settingsGeneralFile.c_str());
}

void ScriptSettings::SaveController(ScriptControls *scriptControl) const {
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());
	
	// [CONTROLLER]
	settingsGeneral.SetBoolValue("CONTROLLER", "ToggleEngine", ToggleEngine);
	settingsGeneral.SetLongValue("CONTROLLER", "ToggleTime", scriptControl->CToggleTime); 
	settingsGeneral.SetDoubleValue("CONTROLLER", "TriggerValue", scriptControl->GetXboxTrigger());
	settingsGeneral.SetBoolValue("CONTROLLER", "BlockCarControls", BlockCarControls);
	settingsGeneral.SetLongValue("CONTROLLER", "MaxTapTime", scriptControl->MaxTapTime);
	
	settingsGeneral.SetLongValue("CONTROLLER", "ShiftUpBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)]);
	settingsGeneral.SetLongValue("CONTROLLER", "ShiftDownBlocks", scriptControl->ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)]);

	// [CONTROLLER_LEGACY]
	settingsGeneral.SetBoolValue("CONTROLLER_LEGACY", "Enable", scriptControl->UseLegacyController);
	settingsGeneral.SetLongValue("CONTROLLER_LEGACY", "ShiftUpBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftUp)]);
	settingsGeneral.SetLongValue("CONTROLLER_LEGACY", "ShiftDownBlocks", scriptControl->ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftDown)]);

	settingsGeneral.SaveFile(settingsGeneralFile.c_str());
}

// Axis information is saved by its own calibration methods
void ScriptSettings::SaveWheel(ScriptControls *scriptControl) const {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());

	// [OPTIONS]
	settingsWheel.SetBoolValue("OPTIONS", "EnableWheel", EnableWheel);
	settingsWheel.SetBoolValue("OPTIONS", "WheelWithoutManual", WheelWithoutManual);
//	settingsWheel.SetBoolValue("OPTIONS", "WheelBoatPlanes", WheelForBoat);
	settingsWheel.SetBoolValue("OPTIONS", "PatchSteering", PatchSteering);
	settingsWheel.SetBoolValue("OPTIONS", "PatchSteeringAlways", PatchSteeringAlways);
	settingsWheel.SetBoolValue("OPTIONS", "PatchSteeringControl", PatchSteeringControl);
	settingsWheel.SetBoolValue("OPTIONS", "LogitechLEDs", LogiLEDs);
	settingsWheel.SetBoolValue("OPTIONS", "HPatternKeyboard", HPatternKeyboard);

	settingsWheel.SetBoolValue("OPTIONS", "InvertSteer", scriptControl->InvertSteer);
	settingsWheel.SetBoolValue("OPTIONS", "InvertThrottle", scriptControl->InvertThrottle);
	settingsWheel.SetBoolValue("OPTIONS", "InvertBrake", scriptControl->InvertBrake);
	settingsWheel.SetBoolValue("OPTIONS", "InvertClutch", scriptControl->InvertClutch);

	// [FORCE_FEEDBACK]
	settingsWheel.SetBoolValue("FORCE_FEEDBACK", "Enable", EnableFFB);
	settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "GlobalMult", FFGlobalMult);
	settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMax", DamperMax);
	settingsWheel.SetLongValue("FORCE_FEEDBACK", "DamperMin", DamperMin);
	settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DamperTargetSpeed", TargetSpeed);
	settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "PhysicsStrength", PhysicsStrength);
	settingsWheel.SetDoubleValue("FORCE_FEEDBACK", "DetailStrength", DetailStrength);

	// [STEER]
	settingsWheel.SetDoubleValue("STEER", "ANTIDEADZONE", scriptControl->ADZSteer);
	settingsWheel.SetDoubleValue("STEER", "SteerAngleMax", SteerAngleMax );
	settingsWheel.SetDoubleValue("STEER", "SteerAngleCar", SteerAngleCar );
	settingsWheel.SetDoubleValue("STEER", "SteerAngleBike",SteerAngleBike);
	settingsWheel.SetDoubleValue("STEER", "SteerAngleBoat", SteerAngleBoat);
	settingsWheel.SetDoubleValue("STEER", "GameSteerMult", GameSteerMult );

	// [THROTTLE]
	settingsWheel.SetDoubleValue("THROTTLE", "ANTIDEADZONE", scriptControl->ADZThrottle);

	// [BRAKES]
	settingsWheel.SetDoubleValue("BRAKES", "ANTIDEADZONE", scriptControl->ADZBrake);

	settingsWheel.SaveFile(settingsWheelFile.c_str());
}

void ScriptSettings::SaveStick(ScriptControls *scriptControl) const {
	CSimpleIniA settingsStick;
	settingsStick.SetUnicode();
	settingsStick.LoadFile(settingsStickFile.c_str());

	settingsStick.SaveFile(settingsStickFile.c_str());
}

std::vector<GUID> ScriptSettings::GetGuids() {
	return reggdGuids;
}

void ScriptSettings::parseSettingsGeneral(ScriptControls *scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());

	settingsGeneral.GetBoolValue("OPTIONS", "Enable", true);
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

	ClutchCatchpoint = settingsGeneral.GetDoubleValue("OPTIONS", "ClutchCatchpoint", 10.0) / 100.0f;
	StallingThreshold = settingsGeneral.GetDoubleValue("OPTIONS", "StallingThreshold", 85.0) / 100.0f;
	RPMDamage = settingsGeneral.GetDoubleValue("OPTIONS", "RPMDamage", 15.0) / 100.0f;
	MisshiftDamage = settingsGeneral.GetDoubleValue("OPTIONS", "MisshiftDamage", 20.0);
	EngBrakePower = settingsGeneral.GetDoubleValue("OPTIONS", "EngBrakePower", 1.0);
	EngBrakeThreshold = settingsGeneral.GetDoubleValue("OPTIONS", "EngBrakeThreshold", 0.75);

	HillBrakeWorkaround = settingsGeneral.GetBoolValue("OPTIONS", "HillBrakeWorkaround", false);
	AutoGear1 = settingsGeneral.GetBoolValue("OPTIONS", "AutoGear1", false);
	AutoLookBack = settingsGeneral.GetBoolValue("OPTIONS", "AutoLookBack", false);
	ThrottleStart = settingsGeneral.GetBoolValue("OPTIONS", "ThrottleStart", true);

	CrossScript = settingsGeneral.GetBoolValue("OPTIONS", "CrossScript", true);

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

	// [CONTROLLER]
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)] = settingsGeneral.GetValue("CONTROLLER", "Toggle", "UNKNOWN");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ToggleH)] = settingsGeneral.GetValue("CONTROLLER", "ToggleShift", "B");
	BlockCarControls = settingsGeneral.GetBoolValue("CONTROLLER", "BlockCarControls", false);

	scriptControl->CToggleTime = settingsGeneral.GetLongValue("CONTROLLER", "ToggleTime", 300);
	scriptControl->MaxTapTime = settingsGeneral.GetLongValue("CONTROLLER", "MaxTapTime", 200);

	double tval = settingsGeneral.GetDoubleValue("CONTROLLER", "TriggerValue", 0.85);
	if (tval > 1.0 || tval < 0.1) {
		tval = 0.85;
	}
	scriptControl->SetXboxTrigger(tval);

	ToggleEngine = settingsGeneral.GetBoolValue("CONTROLLER", "ToggleEngine", false);

	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)] = settingsGeneral.GetValue("CONTROLLER", "ShiftUp", "A");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = settingsGeneral.GetValue("CONTROLLER", "ShiftDown", "X");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)] = settingsGeneral.GetValue("CONTROLLER", "Clutch", "LeftThumbUp");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)] = settingsGeneral.GetValue("CONTROLLER", "Engine", "DpadDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)] = settingsGeneral.GetValue("CONTROLLER", "Throttle", "RightTrigger");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)] = settingsGeneral.GetValue("CONTROLLER", "Brake", "LeftTrigger");

	scriptControl->ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER", "ShiftUpBlocks", -1);
	scriptControl->ControlXboxBlocks[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER", "ShiftDownBlocks", -1);

	// [CONTROLLER_LEGACY]
	scriptControl->UseLegacyController = settingsGeneral.GetBoolValue("CONTROLLER_LEGACY", "Enable", false);

	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::Toggle)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Toggle", -1);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::ToggleH)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ToggleH", ControlFrontendCancel);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftUp", ControlFrontendAccept);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftDown", ControlFrontendX);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::Clutch)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Clutch", ControlFrontendAxisY);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::Engine)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Engine", ControlFrontendDown);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::Throttle)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Throttle", ControlFrontendLt);
	scriptControl->LegacyControls[static_cast<int>(ScriptControls::LegacyControlType::Brake)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "Brake", ControlFrontendRt);

	scriptControl->ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftUp)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftUpBlocks", -1);
	scriptControl->ControlNativeBlocks[static_cast<int>(ScriptControls::LegacyControlType::ShiftDown)] = settingsGeneral.GetLongValue("CONTROLLER_LEGACY", "ShiftDownBlocks", -1);

	// [KEYBOARD]
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftUp", "LSHIFT"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)] = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftDown", "LCTRL"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Clutch", "Z"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Engine", "X"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Throttle", "W"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)] = str2key(settingsGeneral.GetValue("KEYBOARD", "Brake", "S"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)] = str2key(settingsGeneral.GetValue("KEYBOARD", "HR", "NUM0"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H1", "NUM1"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H2", "NUM2"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H3", "NUM3"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H4", "NUM4"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H5", "NUM5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H6", "NUM6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)] = str2key(settingsGeneral.GetValue("KEYBOARD", "H7", "NUM7"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)] = str2key(settingsGeneral.GetValue("KEYBOARD", "HN", "NUM9"));



	// [DEBUG]
	DisplayInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayInfo", false);
	DisplayWheelInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayWheelInfo", false);
	DisplayGearingInfo = settingsGeneral.GetBoolValue("DEBUG", "DisplayGearingInfo", false);
#pragma warning(pop)

}

void ScriptSettings::parseSettingsWheel(ScriptControls *scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());

	// [OPTIONS]
	EnableWheel = settingsWheel.GetBoolValue("OPTIONS", "EnableWheel", true);
	WheelWithoutManual = settingsWheel.GetBoolValue("OPTIONS", "WheelWithoutManual", true);
//	WheelForBoat = settingsWheel.GetBoolValue("OPTIONS", "WheelForBoat", false);
	PatchSteering = settingsWheel.GetBoolValue("OPTIONS", "PatchSteering", true);
	PatchSteeringAlways = settingsWheel.GetBoolValue("OPTIONS", "PatchSteeringAlways", false);
	PatchSteeringControl = settingsWheel.GetBoolValue("OPTIONS", "PatchSteeringControl", true);
	LogiLEDs = settingsWheel.GetBoolValue("OPTIONS", "LogitechLEDs", false);
	HPatternKeyboard = settingsWheel.GetBoolValue("OPTIONS", "HPatternKeyboard", false);

	scriptControl->InvertSteer =	settingsWheel.GetBoolValue("OPTIONS", "InvertSteer", false);
	scriptControl->InvertThrottle = settingsWheel.GetBoolValue("OPTIONS", "InvertThrottle", false);
	scriptControl->InvertBrake =	settingsWheel.GetBoolValue("OPTIONS", "InvertBrake", false);
	scriptControl->InvertClutch =	settingsWheel.GetBoolValue("OPTIONS", "InvertClutch", false);

	// [FORCE_FEEDBACK]
	EnableFFB = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "Enable", true);
	FFGlobalMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "GlobalMult", 1.0);
	DamperMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMax", 67);
	DamperMin = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMin", 12);
	TargetSpeed = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DamperTargetSpeed", 1.2);
	PhysicsStrength = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "PhysicsStrength", 1.7);
	DetailStrength = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DetailStrength", 1.6);

	// [INPUT_DEVICES]
	int it = 0;
	reggdGuids.clear();
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
			logger.Write("CLSIDFromString error: " + errStr);
		}
		reggdGuids.push_back(guid);		
		it++;
	}
	nDevices = it;

	// [TOGGLE_MOD]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("TOGGLE_MOD", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		settingsWheel.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

	// [CHANGE_SHIFTMODE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);


	// [STEER]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("STEER", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		settingsWheel.GetValue("STEER", "AXLE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::ForceFeedback)] =
		settingsWheel.GetValue("STEER", "FFB", "");
	scriptControl->SteerLeft = settingsWheel.GetLongValue("STEER", "MIN", -1);
	scriptControl->SteerRight = settingsWheel.GetLongValue("STEER", "MAX", -1);

	scriptControl->ADZSteer = settingsWheel.GetDoubleValue("STEER", "ANTIDEADZONE", 0.25);

	SteerAngleMax = settingsWheel.GetDoubleValue("STEER", "SteerAngleMax", 900.0);
	SteerAngleCar = settingsWheel.GetDoubleValue("STEER", "SteerAngleCar", 720.0);
	SteerAngleBike = settingsWheel.GetDoubleValue("STEER", "SteerAngleBike", 180.0);
	SteerAngleBoat = settingsWheel.GetDoubleValue("STEER", "SteerAngleBoat", 360.0);

	GameSteerMult = settingsWheel.GetDoubleValue("STEER", "GameSteerMult", 1.0);

	// [THROTTLE]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		settingsWheel.GetValue("THROTTLE", "AXLE", "");
	scriptControl->ThrottleUp = settingsWheel.GetLongValue("THROTTLE", "MIN", -1);
	scriptControl->ThrottleDown = settingsWheel.GetLongValue("THROTTLE", "MAX", -1);
	scriptControl->ADZThrottle = settingsWheel.GetDoubleValue("THROTTLE", "ANTIDEADZONE", 0.25);

	// [BRAKES]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKES", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		settingsWheel.GetValue("BRAKES", "AXLE", "");
	scriptControl->BrakeUp = settingsWheel.GetLongValue("BRAKES", "MIN", -1);
	scriptControl->BrakeDown = settingsWheel.GetLongValue("BRAKES", "MAX", -1);
	scriptControl->ADZBrake = settingsWheel.GetDoubleValue("BRAKES", "ANTIDEADZONE", 0.25);

	// [CLUTCH]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		settingsWheel.GetValue("CLUTCH", "AXLE", "");
	scriptControl->ClutchUp = settingsWheel.GetLongValue("CLUTCH", "MIN", -1);
	scriptControl->ClutchDown = settingsWheel.GetLongValue("CLUTCH", "MAX", -1);

	// [HANDBRAKE_ANALOG]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		settingsWheel.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
	scriptControl->HandbrakeDown = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
	scriptControl->HandbrakeUp = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

	// [SHIFTER]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H1)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H2)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H3)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H4)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H5)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H6)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H7)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::HR)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFTER", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H1)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_1", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H2)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_2", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H3)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_3", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H4)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_4", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H5)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_5", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H6)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_6", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H7)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_7", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::HR)] =
		settingsWheel.GetLongValue("SHIFTER", "GEAR_R", -1);

	// [CLUTCH_BUTTON]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Clutch)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH_BUTTON", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Clutch)] =
		settingsWheel.GetLongValue("CLUTCH_BUTTON", "BUTTON", -1);

	// [SHIFT_UP]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_UP", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		settingsWheel.GetLongValue("SHIFT_UP", "BUTTON", -1);

	// [SHIFT_DOWN]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_DOWN", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		settingsWheel.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

	// [HANDBRAKE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] =
		settingsWheel.GetLongValue("HANDBRAKE", "BUTTON", -1);

	// [ENGINE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("ENGINE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		settingsWheel.GetLongValue("ENGINE", "BUTTON", -1);

	// [LIGHTS]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LIGHTS", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		settingsWheel.GetLongValue("LIGHTS", "BUTTON", -1);

	// [HORN]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HORN", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		settingsWheel.GetLongValue("HORN", "BUTTON", -1);

	// [LOOK_BACK]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_BACK", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		settingsWheel.GetLongValue("LOOK_BACK", "BUTTON", -1);
	
	// [LOOK_LEFT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::LookLeft)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_LEFT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookLeft)] =
		settingsWheel.GetLongValue("LOOK_LEFT", "BUTTON", -1);
	
	// [LOOK_RIGHT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::LookRight)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_RIGHT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookRight)] =
		settingsWheel.GetLongValue("LOOK_RIGHT", "BUTTON", -1);

	// [CHANGE_CAMERA]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_CAMERA", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		settingsWheel.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

	// [RADIO_NEXT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_NEXT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		settingsWheel.GetLongValue("RADIO_NEXT", "BUTTON", -1);

	// [RADIO_PREVIOUS]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_PREVIOUS", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		settingsWheel.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

	// [INDICATOR_LEFT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_LEFT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		settingsWheel.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

	// [INDICATOR_RIGHT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_RIGHT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		settingsWheel.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

	// [INDICATOR_HAZARD]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_HAZARD", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		settingsWheel.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);
	
	// [TO_KEYBOARD]
	scriptControl->WheelToKeyGUID = 
		DeviceIndexToGUID(settingsWheel.GetLongValue("TO_KEYBOARD", "DEVICE", -1), reggdGuids);
	for (int i = 0; i < WheelDirectInput::MAX_RGBBUTTONS; i++) { // Ouch
		std::string entryString = settingsWheel.GetValue("TO_KEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
		if (std::string(entryString).compare("UNKNOWN") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(entryString);
		}
	}

	// [FILEVERSION]
	//settings_wheel_version = settingsWheel.GetValue("FILEVERSION", "VERSION", "000");
#pragma warning(pop)

}

// We dump the controls in another file, but still combine total information (GUIDs/indexes)
void ScriptSettings::parseSettingsStick(ScriptControls *scriptControl) {
	CSimpleIniA settingsStick;
	settingsStick.SetUnicode();
	settingsStick.LoadFile(settingsStickFile.c_str());

	// [THROTTLE]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::Throttle)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("THROTTLE", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::Throttle)] =
		settingsStick.GetValue("THROTTLE", "AXLE", "");
	scriptControl->PlaneThrottleMin = settingsStick.GetLongValue("THROTTLE", "MIN", -1);
	scriptControl->PlaneThrottleMax = settingsStick.GetLongValue("THROTTLE", "MAX", -1);

	// [BRAKE]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::Brake)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("BRAKE", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::Brake)] =
		settingsStick.GetValue("BRAKE", "AXLE", "");
	scriptControl->PlaneBrakeMin = settingsStick.GetLongValue("BRAKE", "MIN", -1);
	scriptControl->PlaneBrakeMin = settingsStick.GetLongValue("BRAKE", "MAX", -1);

	// [PITCH]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::Pitch)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("PITCH", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::Pitch)] =
		settingsStick.GetValue("PITCH", "AXLE", "");
	scriptControl->PlanePitchMin = settingsStick.GetLongValue("PITCH", "MIN", -1);
	scriptControl->PlanePitchMin = settingsStick.GetLongValue("PITCH", "MAX", -1);

	// [ROLL]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::Roll)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("ROLL", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::Roll)] =
		settingsStick.GetValue("ROLL", "AXLE", "");
	scriptControl->PlaneRollMin = settingsStick.GetLongValue("ROLL", "MIN", -1);
	scriptControl->PlaneRollMin = settingsStick.GetLongValue("ROLL", "MAX", -1);

	// [RUDDER_L]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::RudderL)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("RUDDER_L", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::RudderL)] =
		settingsStick.GetValue("RUDDER_L", "AXLE", "");
	scriptControl->PlaneRudderLMin = settingsStick.GetLongValue("RUDDER_L", "MIN", -1);
	scriptControl->PlaneRudderLMin = settingsStick.GetLongValue("RUDDER_L", "MAX", -1);

	// [RUDDER_R]
	scriptControl->StickAxesGUIDs[static_cast<int>(ScriptControls::StickAxisType::RudderR)] =
		DeviceIndexToGUID(settingsStick.GetLongValue("RUDDER_R", "DEVICE", -1), reggdGuids);
	scriptControl->StickAxes[static_cast<int>(ScriptControls::StickAxisType::RudderR)] =
		settingsStick.GetValue("RUDDER_R", "AXLE", "");
	scriptControl->PlaneRudderRMin = settingsStick.GetLongValue("RUDDER_R", "MIN", -1);
	scriptControl->PlaneRudderRMin = settingsStick.GetLongValue("RUDDER_R", "MAX", -1);
}

ptrdiff_t ScriptSettings::SteeringAppendDevice(const GUID &dev_guid, std::string dev_name) {
	auto found = find(reggdGuids.begin(), reggdGuids.end(), dev_guid);
	if (found != reggdGuids.end()) {
		// present! Return index
		// Dependent on implementation of reading this but it should work(TM). Lotsa assumptions.
		return distance(reggdGuids.begin(), found);
	}
	// missing! Add & return index afterwards
	auto newIndex = distance(reggdGuids.begin(), reggdGuids.end());
	std::string newDEV = "DEV" + std::to_string(newIndex);
	std::string newGUID = "GUID" + std::to_string(newIndex);


	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue("INPUT_DEVICES", newDEV.c_str(), dev_name.c_str());
	settingsWheel.SetValue("INPUT_DEVICES", newGUID.c_str(), GUID2String(dev_guid).c_str());
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
	return newIndex;
}
void ScriptSettings::SteeringSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string & axis, int minVal, int maxVal) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsWheel.SetValue(confTag.c_str(), "AXLE", axis.c_str());
	settingsWheel.SetValue(confTag.c_str(), "MIN", std::to_string(minVal).c_str());
	settingsWheel.SetValue(confTag.c_str(), "MAX", std::to_string(maxVal).c_str());
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
}

void ScriptSettings::SteeringSaveFFBAxis(const std::string & confTag, ptrdiff_t index, const std::string & axis) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsWheel.SetValue(confTag.c_str(), "FFB", axis.c_str());
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
}

void ScriptSettings::SteeringSaveButton(const std::string & confTag, ptrdiff_t index, int button) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsWheel.SetLongValue(confTag.c_str(), "BUTTON", button);
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
}

void ScriptSettings::SteeringSaveHShifter(const std::string & confTag, ptrdiff_t index, int button[numGears]) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_R", button[0]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_1", button[1]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_2", button[2]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_3", button[3]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_4", button[4]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_5", button[5]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_6", button[6]);
	settingsWheel.SetLongValue(confTag.c_str(), "GEAR_7", button[7]);
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
}

void ScriptSettings::SteeringAddWheelToKey(const std::string &confTag, ptrdiff_t index, int button, const std::string &keyName) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	settingsWheel.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsWheel.SetValue(confTag.c_str(), std::to_string(button).c_str(), keyName.c_str());
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
}

bool ScriptSettings::SteeringClearWheelToKey(int button) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());
	bool result = settingsWheel.Delete("TO_KEYBOARD", std::to_string(button).c_str(), true);
	int err = settingsWheel.SaveFile(settingsWheelFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsWheelFile);
	return result;
}

void ScriptSettings::StickSaveAxis(const std::string &confTag, ptrdiff_t index, const std::string &axis, int minVal, int maxVal) {
	CSimpleIniA settingsStick;
	settingsStick.SetUnicode();
	settingsStick.LoadFile(settingsStickFile.c_str());
	settingsStick.SetValue(confTag.c_str(), "DEVICE", std::to_string(index).c_str());
	settingsStick.SetValue(confTag.c_str(), "AXLE", axis.c_str());
	settingsStick.SetValue(confTag.c_str(), "MIN", std::to_string(minVal).c_str());
	settingsStick.SetValue(confTag.c_str(), "MAX", std::to_string(maxVal).c_str());
	int err = settingsStick.SaveFile(settingsStickFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsStickFile);
}

void ScriptSettings::KeyboardSaveKey(const std::string &confTag, const std::string &key) {
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());
	settingsGeneral.SetValue("KEYBOARD", confTag.c_str(), key.c_str());
	int err = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsGeneralFile);
}
void ScriptSettings::ControllerSaveButton(const std::string &confTag, const std::string &button) {
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());
	settingsGeneral.SetValue("CONTROLLER", confTag.c_str(), button.c_str());
	//settingsGeneral.SetLongValue("CONTROLLER", (confTag + "Blocks").c_str(), btnToBlock);

	int err = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsGeneralFile);
}

void ScriptSettings::LControllerSaveButton(const std::string &confTag, int button) {
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());
	settingsGeneral.SetLongValue("CONTROLLER_LEGACY", confTag.c_str(), button);
	//settingsGeneral.SetLongValue("CONTROLLER_LEGACY", (confTag + "Blocks").c_str(), btnToBlock);

	int err = settingsGeneral.SaveFile(settingsGeneralFile.c_str());
	if (err < 0)
		logger.Write("Unable to save to " + settingsGeneralFile);
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
	for (auto guid : reggdGuids) {
		if (guid == guidToFind) {
			return i;
		}
		i++;
	}
	return -1;
}