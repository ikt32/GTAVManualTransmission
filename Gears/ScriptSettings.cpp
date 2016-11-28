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
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(SETTINGSFILE);

	settingsGeneral.GetBoolValue("OPTIONS", "Enable", true);
	// [OPTIONS]
	EnableManual        = settingsGeneral.GetBoolValue("OPTIONS", "Enable", true);
	ShiftMode           = settingsGeneral.GetLongValue("OPTIONS", "ShiftMode", 0);
	SimpleBike          = settingsGeneral.GetBoolValue("OPTIONS", "SimpleBike", false);
	EngDamage           = settingsGeneral.GetBoolValue("OPTIONS", "EngineDamage", false);
	EngStall            = settingsGeneral.GetBoolValue("OPTIONS", "EngineStalling", false);
	EngBrake            = settingsGeneral.GetBoolValue("OPTIONS", "EngineBraking", false);
	ClutchCatching      = settingsGeneral.GetBoolValue("OPTIONS", "ClutchCatching", false);
	ClutchShiftingH     = settingsGeneral.GetBoolValue("OPTIONS", "ClutchShiftingH", false);
	ClutchShiftingS     = settingsGeneral.GetBoolValue("OPTIONS", "ClutchShiftingS", false);
	DefaultNeutral      = settingsGeneral.GetBoolValue("OPTIONS", "DefaultNeutral", true);
	
	ClutchCatchpoint    = settingsGeneral.GetDoubleValue("OPTIONS", "ClutchCatchpoint", 15.0) / 100.0f;
	StallingThreshold   = settingsGeneral.GetDoubleValue("OPTIONS", "StallingThreshold", 75.0) / 100.0f;
	RPMDamage           = settingsGeneral.GetDoubleValue("OPTIONS", "RPMDamage", 15.0) / 100.0f;
	MisshiftDamage      = settingsGeneral.GetDoubleValue("OPTIONS", "MisshiftDamage", 10.0);

	HillBrakeWorkaround = settingsGeneral.GetBoolValue("OPTIONS", "HillBrakeWorkaround", false);
	AutoGear1           = settingsGeneral.GetBoolValue("OPTIONS", "AutoGear1", false);
	AutoLookBack        = settingsGeneral.GetBoolValue("OPTIONS", "AutoLookBack", false);
	ThrottleStart       = settingsGeneral.GetBoolValue("OPTIONS", "ThrottleStart", false);

	UITips              = settingsGeneral.GetBoolValue("OPTIONS", "UITips", true);
	UITips_OnlyNeutral  = settingsGeneral.GetBoolValue("OPTIONS", "UITips_OnlyNeutral", false);
	UITips_X            = settingsGeneral.GetDoubleValue("OPTIONS", "UITips_X", 95.0) / 100.0f;
	UITips_Y            = settingsGeneral.GetDoubleValue("OPTIONS", "UITips_Y", 95.0) / 100.0f;
	UITips_Size         = settingsGeneral.GetDoubleValue("OPTIONS", "UITips_Size", 15.0) / 100.0f;
	UITips_TopGearC_R   = settingsGeneral.GetLongValue("OPTIONS", "UITips_TopGearC_R", 255);
	UITips_TopGearC_G   = settingsGeneral.GetLongValue("OPTIONS", "UITips_TopGearC_G", 255);
	UITips_TopGearC_B   = settingsGeneral.GetLongValue("OPTIONS", "UITips_TopGearC_B", 255);

	CrossScript         = settingsGeneral.GetBoolValue("OPTIONS", "CrossScript", false);

	// [CONTROLLER]
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)]  = settingsGeneral.GetValue("CONTROLLER", "Toggle", "DpadRight");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ToggleH)] = settingsGeneral.GetValue("CONTROLLER", "ToggleShift", "B");
	scriptControl->CToggleTime = settingsGeneral.GetLongValue("CONTROLLER", "ToggleTime", 500);

	int tval = settingsGeneral.GetLongValue("CONTROLLER", "TriggerValue", 75);
	if (tval > 100 || tval < 0) {
		tval = 75;
	}
	scriptControl->SetXboxTrigger(tval);

	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)]   = settingsGeneral.GetValue("CONTROLLER", "ShiftUp", "A");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = settingsGeneral.GetValue("CONTROLLER", "ShiftDown", "X");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)]    = settingsGeneral.GetValue("CONTROLLER", "Clutch", "LeftThumbDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)]    = settingsGeneral.GetValue("CONTROLLER", "Engine", "DpadDown");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)]  = settingsGeneral.GetValue("CONTROLLER", "Throttle", "RightTrigger");
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)]     = settingsGeneral.GetValue("CONTROLLER", "Brake", "LeftTrigger");

	// [KEYBOARD]
	
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)]        = str2key(settingsGeneral.GetValue("KEYBOARD", "Toggle", "VK_OEM_5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)]       = str2key(settingsGeneral.GetValue("KEYBOARD", "ToggleH", "VK_OEM_6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)]       = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftUp", "SHIFT"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)]     = str2key(settingsGeneral.GetValue("KEYBOARD", "ShiftDown", "CTRL"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)]        = str2key(settingsGeneral.GetValue("KEYBOARD", "Clutch", "X"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)]        = str2key(settingsGeneral.GetValue("KEYBOARD", "Engine", "C"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)]      = str2key(settingsGeneral.GetValue("KEYBOARD", "Throttle", "W"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)]         = str2key(settingsGeneral.GetValue("KEYBOARD", "Brake", "S"));

	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "HR", "NUM0"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H1", "NUM1"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H2", "NUM2"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H3", "NUM3"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H4", "NUM4"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H5", "NUM5"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H6", "NUM6"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "H7", "NUM7"));
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)]            = str2key(settingsGeneral.GetValue("KEYBOARD", "HN", "NUM9"));


	parseSettingsWheel(scriptControl);

	// [DEBUG]
	Debug = settingsGeneral.GetBoolValue("DEBUG", "Info", false);
	
	// .ini version check
	INIver = settingsGeneral.GetLongValue("DEBUG", "INIver", 0);

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

void ScriptSettings::parseSettingsWheel(ScriptControls *scriptControl) {
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(SETTINGSWHEEL);

	// [OPTIONS]
	WheelEnabled = settingsWheel.GetBoolValue("OPTIONS", "EnableWheel", false);
	WheelWithoutManual = settingsWheel.GetBoolValue("OPTIONS", "WheelWithoutManual", true);
	AltControls = settingsWheel.GetBoolValue("OPTIONS", "WheelBoatPlanes", false);

	// [FORCE_FEEDBACK]
	FFEnable = settingsWheel.GetBoolValue("FORCE_FEEDBACK", "FFEnable", true);
	FFGlobalMult = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "FFGlobalMult", 100.0) / 100.0f;
	DamperMax = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMax", 50);
	DamperMin = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperMin", 20);
	TargetSpeed = settingsWheel.GetLongValue("FORCE_FEEDBACK", "DamperTargetSpeed", 10);
	FFPhysics = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "PhysicsStrength", 100.0) / 100.0f;
	DetailStrength = settingsWheel.GetDoubleValue("FORCE_FEEDBACK", "DetailStrength", 100.0) / 1.0f;


	// So the settings reader should not give a singlest FUCK about DirectInput mumbo jump
	// I'll need to write a program that fills these or at least configures this part of the file
	// idk somebody buy me a fancy fanatec set of stuff so i can pretend to care
	// i need a handbrake

	// [INPUT_DEVICES]
	bool searching = true;
	int it = 0;
	std::vector<std::pair<std::string, GUID>> guids;
	while (searching) {
		std::string currDevIndex = "DEV" + it;
		std::string currGuidIndex = "GUID" + it;

		std::string currDevice = settingsWheel.GetValue("INPUT_DEVICES", currDevIndex.c_str(), "");
		if (currDevice == "")
			break;
		std::string currGuid = settingsWheel.GetValue("INPUT_DEVICES", currGuidIndex.c_str(), "");
		if (currGuid == "")
			break;

		// the fuck, Microsoft?
		LPCOLESTR the_fuck = LPWSTR(currGuid.c_str());

		GUID bstrGuid;
		CLSIDFromString(the_fuck, &bstrGuid);
		guids.emplace_back(currDevice, bstrGuid);
	}

	// I fucking hate this
	// THERE HAS TO BE A BETTER WAY RIGHT send help
	// [TOGGLE_MOD]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		settingsWheel.GetValue("TOGGLE_MOD", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		settingsWheel.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

	// [CHANGE_SHIFTMODE]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		settingsWheel.GetValue("CHANGE_SHIFTMODE", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);


	// [STEER]
	scriptControl->WheelAxesDevices[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		settingsWheel.GetValue("STEER", "DEVICE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		settingsWheel.GetValue("STEER", "AXLE", "");
	scriptControl->SteerLeft = settingsWheel.GetLongValue("STEER", "MIN", -1);
	scriptControl->SteerRight = settingsWheel.GetLongValue("STEER", "MAX", -1);
	SteerAngleMax = settingsWheel.GetDoubleValue("STEER", "SteerAngleMax", 900.0);
	SteerAngleCar = settingsWheel.GetDoubleValue("STEER", "SteerAngleCar", 720.0);
	SteerAngleBike = settingsWheel.GetDoubleValue("STEER", "SteerAngleBike", 180.0);
	SteerAngleAlt = settingsWheel.GetDoubleValue("STEER", "SteerAngleAlt", 180.0);

	// Todo: process this below
	// Todo: Also think about disableClutch and stuff
	// Todo: Also error checking
	// Todo: Also a life
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] = settingsWheel.GetValue("WHEELAXIS", "Throttle", "lY");
	scriptControl->ThrottleUp = settingsWheel.GetLongValue("WHEELAXIS", "ThrottleUp", 65535);
	scriptControl->ThrottleDown = settingsWheel.GetLongValue("WHEELAXIS", "ThrottleDown", 0);


	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] = settingsWheel.GetValue("WHEELAXIS", "Brake", "lRz");
	scriptControl->BrakeUp = settingsWheel.GetLongValue("WHEELAXIS", "BrakeUp", 65535);
	scriptControl->BrakeDown = settingsWheel.GetLongValue("WHEELAXIS", "BrakeDown", 0);


	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] = settingsWheel.GetValue("WHEELAXIS", "Clutch", "rglSlider1");
	scriptControl->ClutchUp = settingsWheel.GetLongValue("WHEELAXIS", "ClutchUp", 65535);
	scriptControl->ClutchDown = settingsWheel.GetLongValue("WHEELAXIS", "ClutchDown", 0);

	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] = settingsWheel.GetValue("WHEELAXIS", "Handbrake", "UNKNOWN");
	scriptControl->HandbrakeDown = settingsWheel.GetLongValue("WHEELAXIS", "HandbrakeDown", 0);
	scriptControl->HandbrakeUp = settingsWheel.GetLongValue("WHEELAXIS", "HandbrakeUp", 65535);

	scriptControl->FFAxis = settingsWheel.GetValue("WHEELAXIS", "FFAxis", "X");

	scriptControl->ClutchDisable = settingsWheel.GetBoolValue("WHEELAXIS", "ClutchDisable", false);




	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] = settingsWheel.GetLongValue("WHEELCONTROLS", "ShiftUp", 4);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] = settingsWheel.GetLongValue("WHEELCONTROLS", "ShiftDown", 5);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::HR)] = settingsWheel.GetLongValue("WHEELCONTROLS", "HR", 14);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H1)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H1", 8);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H2)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H2", 9);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H3)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H3", 10);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H4)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H4", 11);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H5)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H5", 12);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H6)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H6", 13);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::H7)] = settingsWheel.GetLongValue("WHEELCONTROLS", "H7", -1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] = settingsWheel.GetLongValue("WHEELCONTROLS", "Handbrake", 19);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Engine)] = settingsWheel.GetLongValue("WHEELCONTROLS", "Engine", 21);

	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Horn)] = settingsWheel.GetLongValue("WHEELCONTROLS", "Horn", 20);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Lights)] = settingsWheel.GetLongValue("WHEELCONTROLS", "Lights", 7);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookBack)] = settingsWheel.GetLongValue("WHEELCONTROLS", "LookBack", 22);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Camera)] = settingsWheel.GetLongValue("WHEELCONTROLS", "Camera", 0);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] = settingsWheel.GetLongValue("WHEELCONTROLS", "RadioPrev", 1);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] = settingsWheel.GetLongValue("WHEELCONTROLS", "RadioNext", 2);

	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] = settingsWheel.GetLongValue("WHEELCONTROLS", "IndicatorLeft", 19);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] = settingsWheel.GetLongValue("WHEELCONTROLS", "IndicatorRight", 21);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] = settingsWheel.GetLongValue("WHEELCONTROLS", "IndicatorHazard", 15);

	
	// [WHEELKEYBOARD]
	for (int i = 0; i < MAX_RGBBUTTONS; i++) { // Ouch
		std::string entryString = settingsWheel.GetValue("WHEELKEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
		if (std::string(entryString).compare("UNKNOWN") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(entryString);
		}
	}
}
