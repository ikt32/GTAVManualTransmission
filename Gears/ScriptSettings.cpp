#include "ScriptSettings.hpp"
#include <Windows.h>

#include "Util/Logger.hpp"
#include <string>
#include "Input/keyboard.h"
#include "Util/simpleini/SimpleIni.h"

ScriptSettings::ScriptSettings() {
}


void ScriptSettings::Read(ScriptControls* scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(SETTINGSFILE);

	ini.GetBoolValue("OPTIONS", "Enable", true);
	// [OPTIONS]
	EnableManual        = ini.GetBoolValue("OPTIONS", "Enable", true);
	ShiftMode           = ini.GetLongValue("OPTIONS", "ShiftMode", 0);
	SimpleBike          = ini.GetBoolValue("OPTIONS", "SimpleBike", false);
	EngDamage           = ini.GetBoolValue("OPTIONS", "EngineDamage", false);
	EngStall            = ini.GetBoolValue("OPTIONS", "EngineStalling", false);
	EngBrake            = ini.GetBoolValue("OPTIONS", "EngineBraking", false);
	ClutchCatching      = ini.GetBoolValue("OPTIONS", "ClutchCatching", false);
	ClutchShiftingH      = ini.GetBoolValue("OPTIONS", "ClutchShiftingH", false);
	ClutchShiftingS     = ini.GetBoolValue("OPTIONS", "ClutchShiftingS", false);
	DefaultNeutral      = ini.GetBoolValue("OPTIONS", "DefaultNeutral", true);
	
	ClutchCatchpoint    = ini.GetDoubleValue("OPTIONS", "ClutchCatchpoint", 15.0) / 100.0f;
	StallingThreshold   = ini.GetDoubleValue("OPTIONS", "StallingThreshold", 75.0) / 100.0f;
	RPMDamage           = ini.GetDoubleValue("OPTIONS", "RPMDamage", 15.0) / 100.0f;
	MisshiftDamage      = ini.GetDoubleValue("OPTIONS", "MisshiftDamage", 10.0);

	AutoLookBack        = ini.GetBoolValue("OPTIONS", "AutoLookBack", false);
	AutoGear1           = ini.GetBoolValue("OPTIONS", "AutoGear1", false);
	HillBrakeWorkaround = ini.GetBoolValue("OPTIONS", "HillBrakeWorkaround", false);
	
	UITips              = ini.GetBoolValue("OPTIONS", "UITips", true);
	UITips_OnlyNeutral  = ini.GetBoolValue("OPTIONS", "UITips_OnlyNeutral", false);
	UITips_X            = ini.GetDoubleValue("OPTIONS", "UITips_X", 95.0) / 100.0f;
	UITips_Y            = ini.GetDoubleValue("OPTIONS", "UITips_Y", 95.0) / 100.0f;
	UITips_Size         = ini.GetDoubleValue("OPTIONS", "UITips_Size", 15.0) / 100.0f;
	UITips_TopGearC_R   = ini.GetLongValue("OPTIONS", "UITips_TopGearC_R", 255);
	UITips_TopGearC_G   = ini.GetLongValue("OPTIONS", "UITips_TopGearC_G", 255);
	UITips_TopGearC_B   = ini.GetLongValue("OPTIONS", "UITips_TopGearC_B", 255);

	CrossScript         = ini.GetBoolValue("OPTIONS", "CrossScript", false);

	// [CONTROLLER]
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)]  = ini.GetValue("CONTROLLER", "Toggle", "DpadRight");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ToggleH)] = ini.GetValue("CONTROLLER", "ToggleShift", "B");
	scriptControl->CToggleTime = ini.GetLongValue("CONTROLLER", "ToggleTime", 500);

	int tval = ini.GetLongValue("CONTROLLER", "TriggerValue", 75);
	if (tval > 100 || tval < 0) {
		tval = 75;
	}
	scriptControl->SetXboxTrigger(tval);

	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)]   = ini.GetValue("CONTROLLER", "ShiftUp", "A");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = ini.GetValue("CONTROLLER", "ShiftDown", "X");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)]    = ini.GetValue("CONTROLLER", "Clutch", "LeftThumbDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)]    = ini.GetValue("CONTROLLER", "Engine", "DpadDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]  = ini.GetValue("CONTROLLER", "Throttle", "RightTrigger");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)]     = ini.GetValue("CONTROLLER", "Brake", "LeftTrigger");

	// [KEYBOARD]
	
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)]        = str2key(ini.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)]       = str2key(ini.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)]       = str2key(ini.GetValue("KEYBOARD", "ShiftUp", "SHIFT"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)]     = str2key(ini.GetValue("KEYBOARD", "ShiftDown", "CTRL"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)]        = str2key(ini.GetValue("KEYBOARD", "Clutch", "X"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)]        = str2key(ini.GetValue("KEYBOARD", "Engine", "C"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)]      = str2key(ini.GetValue("KEYBOARD", "Throttle", "W"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)]         = str2key(ini.GetValue("KEYBOARD", "Brake", "S"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)]            = str2key(ini.GetValue("KEYBOARD", "HR", "NUM0"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)]            = str2key(ini.GetValue("KEYBOARD", "H1", "NUM1"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)]            = str2key(ini.GetValue("KEYBOARD", "H2", "NUM2"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)]            = str2key(ini.GetValue("KEYBOARD", "H3", "NUM3"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)]            = str2key(ini.GetValue("KEYBOARD", "H4", "NUM4"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)]            = str2key(ini.GetValue("KEYBOARD", "H5", "NUM5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)]            = str2key(ini.GetValue("KEYBOARD", "H6", "NUM6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)]            = str2key(ini.GetValue("KEYBOARD", "H7", "NUM7"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)]            = str2key(ini.GetValue("KEYBOARD", "HN", "NUM9"));

	// [WHEELOPTIONS]
	WheelEnabled = ini.GetBoolValue("WHEELOPTIONS", "Enable", false);
	WheelWithoutManual = ini.GetBoolValue("WHEELOPTIONS", "WheelWithoutManual", true);

	FFEnable = ini.GetBoolValue("WHEELOPTIONS", "FFEnable", true);
	FFGlobalMult =	ini.GetDoubleValue("WHEELOPTIONS", "FFGlobalMult", 100.0) / 100.0f;
	DamperMax =		ini.GetLongValue("WHEELOPTIONS", "DamperMax", 50);
	DamperMin = ini.GetLongValue("WHEELOPTIONS", "DamperMin", 20);
	TargetSpeed = ini.GetLongValue("WHEELOPTIONS", "DamperTargetSpeed", 10);
	FFPhysics = ini.GetDoubleValue("WHEELOPTIONS", "PhysicsStrength", 100.0) / 100.0f;
	DetailStrength = ini.GetDoubleValue("WHEELOPTIONS", "DetailStrength", 100.0) / 1.0f;

	// [WHEELCONTROLS]
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Toggle)]          = ini.GetLongValue("WHEELCONTROLS", "Toggle", 17);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ToggleH)]         = ini.GetLongValue("WHEELCONTROLS", "ToggleH", 6);
																									   
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)]         = ini.GetLongValue("WHEELCONTROLS", "ShiftUp", 4);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)]       = ini.GetLongValue("WHEELCONTROLS", "ShiftDown", 5);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::HR)]              = ini.GetLongValue("WHEELCONTROLS", "HR", 14);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H1)]              = ini.GetLongValue("WHEELCONTROLS", "H1", 8 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H2)]              = ini.GetLongValue("WHEELCONTROLS", "H2", 9 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H3)]              = ini.GetLongValue("WHEELCONTROLS", "H3", 10);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H4)]              = ini.GetLongValue("WHEELCONTROLS", "H4", 11);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H5)]              = ini.GetLongValue("WHEELCONTROLS", "H5", 12);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H6)]              = ini.GetLongValue("WHEELCONTROLS", "H6", 13);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H6)]              = ini.GetLongValue("WHEELCONTROLS", "H7", -1);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Handbrake)]       = ini.GetLongValue("WHEELCONTROLS", "Handbrake", 19);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Engine)]          = ini.GetLongValue("WHEELCONTROLS", "Engine", 21   );
																									   
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Horn)]            = ini.GetLongValue("WHEELCONTROLS", "Horn", 20 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Lights)]          = ini.GetLongValue("WHEELCONTROLS", "Lights", 7);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::LookBack)]        = ini.GetLongValue("WHEELCONTROLS", "LookBack", 22);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Camera)]          = ini.GetLongValue("WHEELCONTROLS", "Camera", 0);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)]       = ini.GetLongValue("WHEELCONTROLS", "RadioPrev", 1);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioNext)]       = ini.GetLongValue("WHEELCONTROLS", "RadioNext", 2);
																									   
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)]   = ini.GetLongValue("WHEELCONTROLS", "IndicatorLeft", 19  );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)]  = ini.GetLongValue("WHEELCONTROLS", "IndicatorRight", 21 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] = ini.GetLongValue("WHEELCONTROLS", "IndicatorHazard", 15);

	// [WHEELAXIS]
	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] = ini.GetValue("WHEELAXIS", "Throttle", "lY");
	scriptControl->ThrottleUp = ini.GetLongValue("WHEELAXIS", "ThrottleUp", 65535);
	scriptControl->ThrottleDown = ini.GetLongValue("WHEELAXIS", "ThrottleDown", 0);
	
	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] = ini.GetValue("WHEELAXIS", "Brake", "lRz");
	scriptControl->BrakeUp = ini.GetLongValue("WHEELAXIS", "BrakeUp", 65535);
	scriptControl->BrakeDown = ini.GetLongValue("WHEELAXIS", "BrakeDown", 0);

	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] = ini.GetValue("WHEELAXIS", "Clutch", "rglSlider1");
	scriptControl->ClutchUp = ini.GetLongValue("WHEELAXIS", "ClutchUp", 65535);
	scriptControl->ClutchDown = ini.GetLongValue("WHEELAXIS", "ClutchDown", 0);

	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] = ini.GetValue("WHEELAXIS", "Steer", "lX");
	scriptControl->SteerLeft =  ini.GetLongValue("WHEELAXIS", "SteerLeft", 0);
	scriptControl->SteerRight = ini.GetLongValue("WHEELAXIS", "SteerRight", 65535);

	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] = ini.GetValue("WHEELAXIS", "Handbrake", "UNKNOWN");
	scriptControl->HandbrakeDown = ini.GetLongValue("WHEELAXIS", "HandbrakeDown", 0);
	scriptControl->HandbrakeUp = ini.GetLongValue("WHEELAXIS", "HandbrakeUp", 65535);
	
	scriptControl->FFAxis = ini.GetValue("WHEELAXIS", "FFAxis", "X");

	scriptControl->ClutchDisable = ini.GetBoolValue("WHEELAXIS", "ClutchDisable", false);

	SteerAngleMax = ini.GetDoubleValue("WHEELAXIS", "SteerAngleMax", 900.0);
	SteerAngleCar = ini.GetDoubleValue("WHEELAXIS", "SteerAngleCar", 720.0);
	SteerAngleBike = ini.GetDoubleValue("WHEELAXIS", "SteerAngleBike", 180.0);

	// [WHEELKEYBOARD]
	for (int i = 0; i < MAX_RGBBUTTONS; i++) { // Ouch
		std::string entryString = ini.GetValue("WHEELKEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
		if (std::string(entryString).compare("UNKNOWN") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(entryString);
		}
	}

	// [DEBUG]
	Debug = ini.GetBoolValue("DEBUG", "Info", false);
	AltControls = ini.GetBoolValue("DEBUG", "AltControls", false);
	SteerAngleAlt = ini.GetDoubleValue("DEBUG", "AltAngle", 180.0);
	
	// .ini version check
	INIver = ini.GetValue("DEBUG", "INIver", "0.0");

#pragma warning(pop)
}

void ScriptSettings::Save() const {
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(SETTINGSFILE);
	ini.SetValue("OPTIONS", "Enable", EnableManual ? " 1" : " 0");
	ini.SetLongValue("OPTIONS", "ShiftMode", ShiftMode);
	ini.SaveFile(SETTINGSFILE);
}

bool ScriptSettings::IsCorrectVersion() const {
	return CORRECTVERSION == INIver;
}
