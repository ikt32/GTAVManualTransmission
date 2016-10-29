#include "ScriptSettings.hpp"
#include <Windows.h>

#include "Util/Logger.hpp"
#include <string>
#include "Input/keyboard.h"
#include "Util/inih/cpp/INIReader.h"

ScriptSettings::ScriptSettings() {
}


void ScriptSettings::Read(ScriptControls* scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	INIReader reader(SETTINGSFILE);

	reader.GetBoolean("OPTIONS", "Enable", true);
	// [OPTIONS]
	EnableManual = reader.GetBoolean("OPTIONS", "Enable", true);
	ShiftMode = reader.GetInteger("OPTIONS", "ShiftMode", 0);
	SimpleBike = reader.GetBoolean("OPTIONS", "SimpleBike", false);
	EngDamage = reader.GetBoolean("OPTIONS", "EngineDamage", false);
	EngStall = reader.GetBoolean("OPTIONS", "EngineStalling", false);
	EngBrake = reader.GetBoolean("OPTIONS", "EngineBraking", false);
	ClutchCatching = reader.GetBoolean("OPTIONS", "ClutchCathing", false);
	ClutchShifting = reader.GetBoolean("OPTIONS", "ClutchShifting", false);
	ClutchShiftingS = reader.GetBoolean("OPTIONS", "ClutchShiftingS", false);
	DefaultNeutral = reader.GetBoolean("OPTIONS", "DefaultNeutral", true);
	
	ClutchCatchpoint = reader.GetReal("OPTIONS", "ClutchCatchpoint", 15.0) / 100.0f;
	StallingThreshold = reader.GetReal("OPTIONS", "StallingThreshold", 75.0) / 100.0f;
	RPMDamage = reader.GetReal("OPTIONS", "RPMDamage", 15.0) / 100.0f;
	MisshiftDamage = reader.GetReal("OPTIONS", "MisshiftDamage", 10.0);

	AutoLookBack = reader.GetBoolean("OPTIONS", "AutoLookBack", false);
	AutoGear1 = reader.GetBoolean("OPTIONS", "AutoGear1", false);
	HillBrakeWorkaround = reader.GetBoolean("OPTIONS", "HillBrakeWorkaround", false);
	
	UITips = reader.GetBoolean("OPTIONS", "UITips", true);
	UITips_OnlyNeutral = reader.GetBoolean("OPTIONS", "UITips_OnlyNeutral", false);
	UITips_X = reader.GetReal("OPTIONS", "UITips_X", 95.0) / 100.0f;
	UITips_Y = reader.GetReal("OPTIONS", "UITips_Y", 95.0) / 100.0f;
	UITips_Size = reader.GetReal("OPTIONS", "UITips_Size", 15.0) / 100.0f;
	UITips_TopGearC_R = reader.GetInteger("OPTIONS", "UITips_TopGearC_R", 255);
	UITips_TopGearC_G = reader.GetInteger("OPTIONS", "UITips_TopGearC_G", 255);
	UITips_TopGearC_B = reader.GetInteger("OPTIONS", "UITips_TopGearC_B", 255);

	CrossScript = reader.GetBoolean("OPTIONS", "CrossScript", false);

	// [CONTROLLER]
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)] = reader.Get("CONTROLLER", "Toggle", "DpadRight");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ToggleH)] = reader.Get("CONTROLLER", "ToggleShift", "B");
	scriptControl->CToggleTime = reader.GetInteger("CONTROLLER", "ToggleTime", 500);

	int tval = reader.GetInteger("CONTROLLER", "TriggerValue", 75);
	if (tval > 100 || tval < 0) {
		tval = 75;
	}
	scriptControl->SetXboxTrigger(tval);

	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)] = reader.Get("CONTROLLER", "ShiftUp", "A");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = reader.Get("CONTROLLER", "ShiftDown", "X");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)] = reader.Get("CONTROLLER", "Clutch", "LeftThumbDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)] = reader.Get("CONTROLLER", "Engine", "DpadDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)] = reader.Get("CONTROLLER", "Throttle", "RightTrigger");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)] = reader.Get("CONTROLLER", "Brake", "LeftTrigger");

	// [KEYBOARD]
	
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)] = str2key(reader.Get("KEYBOARD", "Toggle", "VK_OEM_5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)] = str2key(reader.Get("KEYBOARD", "ToggleH", "VK_OEM_6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)] = str2key(reader.Get("KEYBOARD", "ShiftUp", "SHIFT"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)] = str2key(reader.Get("KEYBOARD", "ShiftDown", "CTRL"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)] = str2key(reader.Get("KEYBOARD", "Clutch", "X"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)] = str2key(reader.Get("KEYBOARD", "Engine", "C"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)] = str2key(reader.Get("KEYBOARD", "Throttle", "W"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)] = str2key(reader.Get("KEYBOARD", "Brake", "S"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)]        = str2key(reader.Get("KEYBOARD", "HR", "NUM0"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)]        = str2key(reader.Get("KEYBOARD", "H1", "NUM1"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)]        = str2key(reader.Get("KEYBOARD", "H2", "NUM2"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)]        = str2key(reader.Get("KEYBOARD", "H3", "NUM3"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)]        = str2key(reader.Get("KEYBOARD", "H4", "NUM4"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)]        = str2key(reader.Get("KEYBOARD", "H5", "NUM5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)]        = str2key(reader.Get("KEYBOARD", "H6", "NUM6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)]        = str2key(reader.Get("KEYBOARD", "H7", "NUM7"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H8)]        = str2key(reader.Get("KEYBOARD", "H8", "NUM8"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)]        = str2key(reader.Get("KEYBOARD", "HN", "NUM9"));

	// [WHEELOPTIONS]
	WheelEnabled = reader.GetBoolean("WHEELOPTIONS", "Enable", false);
	WheelWithoutManual = reader.GetBoolean("WHEELOPTIONS", "WheelWithoutManual", true);

	FFEnable = reader.GetBoolean("WHEELOPTIONS", "FFEnable", true);
	FFGlobalMult =	reader.GetReal("WHEELOPTIONS", "FFGlobalMult", 100.0) / 100.0f;
	DamperMax =		reader.GetInteger("WHEELOPTIONS", "DamperMax", 50);
	DamperMin = reader.GetInteger("WHEELOPTIONS", "DamperMin", 20);
	TargetSpeed = reader.GetInteger("WHEELOPTIONS", "DamperTargetSpeed", 10);
	FFPhysics = reader.GetReal("WHEELOPTIONS", "PhysicsStrength", 100.0) / 100.0f;
	CenterStrength = reader.GetReal("WHEELOPTIONS", "CenterStrength", 100.0) / 100.0f;
	DetailStrength = reader.GetReal("WHEELOPTIONS", "DetailStrength", 100.0) / 1.0f;

	// [WHEELCONTROLS]
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =		reader.GetInteger("WHEELCONTROLS", "Toggle", 17);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =		reader.GetInteger("WHEELCONTROLS", "ToggleH", 6);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =		reader.GetInteger("WHEELCONTROLS", "ShiftUp", 4);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =	reader.GetInteger("WHEELCONTROLS", "ShiftDown", 5);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::HR)] =			reader.GetInteger("WHEELCONTROLS", "HR", 14);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H1)] =			reader.GetInteger("WHEELCONTROLS", "H1", 8 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H2)] =			reader.GetInteger("WHEELCONTROLS", "H2", 9 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H3)] =			reader.GetInteger("WHEELCONTROLS", "H3", 10);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H4)] =			reader.GetInteger("WHEELCONTROLS", "H4", 11);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H5)] =			reader.GetInteger("WHEELCONTROLS", "H5", 12);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H6)] =			reader.GetInteger("WHEELCONTROLS", "H6", 13);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] =	reader.GetInteger("WHEELCONTROLS", "Handbrake", 19);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Engine)] =		reader.GetInteger("WHEELCONTROLS", "Engine", 21   );

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Horn)] =			reader.GetInteger("WHEELCONTROLS", "Horn", 20 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Lights)] =		reader.GetInteger("WHEELCONTROLS", "Lights", 7);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =		reader.GetInteger("WHEELCONTROLS", "LookBack", 22);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Camera)] =		reader.GetInteger("WHEELCONTROLS", "Camera", 0);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =	reader.GetInteger("WHEELCONTROLS", "RadioPrev", 1);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =	reader.GetInteger("WHEELCONTROLS", "RadioNext", 2);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =	reader.GetInteger("WHEELCONTROLS", "IndicatorLeft", 19  );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =	reader.GetInteger("WHEELCONTROLS", "IndicatorRight", 21 );
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =	reader.GetInteger("WHEELCONTROLS", "IndicatorHazard", 15);

	// [WHEELAXIS]
	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] = reader.Get("WHEELAXIS", "Throttle", "lY");
	scriptControl->ThrottleMin = reader.GetInteger("WHEELAXIS", "ThrottleMin", 0);
	scriptControl->ThrottleMax = reader.GetInteger("WHEELAXIS", "ThrottleMax", 65535);
	
	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] = reader.Get("WHEELAXIS", "Brake", "lRz");
	scriptControl->BrakeMin = reader.GetInteger("WHEELAXIS", "BrakeMin", 0);
	scriptControl->BrakeMax = reader.GetInteger("WHEELAXIS", "BrakeMax", 65535);

	
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] = reader.Get("WHEELAXIS", "Clutch", "rglSlider1");
	scriptControl->ClutchMin = reader.GetInteger("WHEELAXIS", "ClutchMin", 0);
	scriptControl->ClutchMax = reader.GetInteger("WHEELAXIS", "ClutchMax", 65535);

	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] = reader.Get("WHEELAXIS", "Steer", "lX");

	scriptControl->SteerLeft =  reader.GetInteger("WHEELAXIS", "SteerLeft", 0);
	scriptControl->SteerRight = reader.GetInteger("WHEELAXIS", "SteerRight", 65535);
	
	scriptControl->FFAxis = reader.Get("WHEELAXIS", "FFAxis", "X");

	scriptControl->ClutchDisable = reader.GetBoolean("WHEELAXIS", "ClutchDisable", false);

	SteerAngleMax = reader.GetReal("WHEELAXIS", "SteerAngleMax", 900.0);
	SteerAngleCar = reader.GetReal("WHEELAXIS", "SteerAngleCar", 720.0);
	SteerAngleBike = reader.GetReal("WHEELAXIS", "SteerAngleBike", 180.0);

	// [WHEELKEYBOARD]
	for (int i = 0; i < MAX_RGBBUTTONS; i++) { // Ouch
		std::string entryString = reader.Get("WHEELKEYBOARD", std::to_string(i).c_str(), "NOPE");
		if (std::string(entryString).compare("NOPE") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(entryString);
		}
	}

	// [DEBUG]
	Debug = reader.GetBoolean("DEBUG", "Info", false);
	AltControls = reader.GetBoolean("DEBUG", "AltControls", false);
	SteerAngleAlt = reader.GetReal("DEBUG", "AltAngle", 180.0);
	
	// .ini version check
	INIver = reader.Get("DEBUG", "INIver", "0.0");

	CheckSettings();
	if (scriptControl->ClutchDisable) {
		ClutchShifting = false;
		WritePrivateProfileStringA("OPTIONS", "ClutchShifting", "0", SETTINGSFILE);
	}
#pragma warning(pop)
}

void ScriptSettings::Save() const {
	WritePrivateProfileStringA("OPTIONS", "Enable", (EnableManual ? " 1" : " 0"), SETTINGSFILE);
	WritePrivateProfileStringA("OPTIONS", "ShiftMode", std::to_string(ShiftMode).c_str(), SETTINGSFILE);
}

// Checks for conflicting settings and adjusts them
void ScriptSettings::CheckSettings() {
	Logger logger(LOGFILE);
	if (UITips_X > 100) {
		UITips_X = 100;
		logger.Write("UITips_X higher than 100, reverting");
		WritePrivateProfileStringA("OPTIONS", "UITips_X", "100", SETTINGSFILE);
	}
	if (UITips_Y > 100) {
		UITips_Y = 100;
		logger.Write("UITips_Y higher than 100, reverting");
		WritePrivateProfileStringA("OPTIONS", "UITips_Y", "100", SETTINGSFILE);
	}
}

bool ScriptSettings::IsCorrectVersion() const {
	return CORRECTVERSION == INIver;
}
