#include "ScriptSettings.hpp"
#include <Windows.h>

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
	settingsGeneral.LoadFile(SETTINGSGENERAL);

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
	
	// [FILEVERSION]
	settings_general_version = settingsGeneral.GetLongValue("FILEVERSION", "VERSION", 0);

#pragma warning(pop)
}

void ScriptSettings::Save() const {
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(SETTINGSGENERAL);
	ini.SetBoolValue("OPTIONS", "Enable", EnableManual);
	ini.SetLongValue("OPTIONS", "ShiftMode", ShiftMode);
	ini.SaveFile(SETTINGSGENERAL);
}

void ScriptSettings::IsCorrectVersion() const {
	if (settings_general_version != CORRECTVGENERAL)
		throw std::runtime_error("Wrong settings_general.ini version");
	if (settings_wheel_version != CORRECTVWHEEL)
		throw std::runtime_error("Wrong settings_wheel.ini version");
}

void ScriptSettings::mapDevice2GUID(ScriptControls *scriptControl) {
	for (int i=0; i<static_cast<int>(ScriptControls::WheelAxisType::SIZEOF_WheelAxisType);i++)
	{
		for ()
	}

	scriptControl->WheelAxesGUIDs;


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


	// The DEVICE in the following sections should only be read by the directinput control parts
	// I'll need to write a program that fills these or at least configures this part of the file
	// idk somebody buy me a fancy fanatec set of stuff so i can pretend to care
	// i need a handbrake
	// santa pls

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

	// [THROTTLE]
	scriptControl->WheelAxesDevices[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		settingsWheel.GetValue("THROTTLE", "DEVICE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		settingsWheel.GetValue("THROTTLE", "AXLE", "");
	scriptControl->ThrottleUp = settingsWheel.GetLongValue("THROTTLE", "MIN", -1);
	scriptControl->ThrottleDown = settingsWheel.GetLongValue("THROTTLE", "MAX", -1);

	// [BRAKES]
	scriptControl->WheelAxesDevices[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		settingsWheel.GetValue("BRAKES", "DEVICE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		settingsWheel.GetValue("BRAKES", "AXLE", "");
	scriptControl->BrakeUp = settingsWheel.GetLongValue("BRAKES", "MIN", -1);
	scriptControl->BrakeDown = settingsWheel.GetLongValue("BRAKES", "MAX", -1);

	// [CLUTCH]
	scriptControl->WheelAxesDevices[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		settingsWheel.GetValue("CLUTCH", "DEVICE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		settingsWheel.GetValue("CLUTCH", "AXLE", "");
	scriptControl->ClutchUp = settingsWheel.GetLongValue("CLUTCH", "MIN", -1);
	scriptControl->ClutchDown = settingsWheel.GetLongValue("CLUTCH", "MAX", -1);

	// [HANDBRAKE_ANALOG]
	scriptControl->WheelAxesDevices[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		settingsWheel.GetValue("HANDBRAKE_ANALOG", "DEVICE", "");
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		settingsWheel.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
	scriptControl->HandbrakeDown = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
	scriptControl->HandbrakeUp = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

	// [SHIFTER]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H1)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H2)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H3)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H4)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H5)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H6)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::H7)] =
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::HR)] =
		settingsWheel.GetValue("SHIFTER", "DEVICE", ""); // lmao kill me
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

	// [SHIFT_UP]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		settingsWheel.GetValue("SHIFT_UP", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		settingsWheel.GetLongValue("SHIFT_UP", "BUTTON", -1);

	// [SHIFT_DOWN]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		settingsWheel.GetValue("SHIFT_DOWN", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		settingsWheel.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

	// [ENGINE]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		settingsWheel.GetValue("ENGINE", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		settingsWheel.GetLongValue("ENGINE", "BUTTON", -1);

	// [LIGHTS]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		settingsWheel.GetValue("LIGHTS", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		settingsWheel.GetLongValue("LIGHTS", "BUTTON", -1);

	// [HORN]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		settingsWheel.GetValue("HORN", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		settingsWheel.GetLongValue("HORN", "BUTTON", -1);

	// [LOOK_BACK]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		settingsWheel.GetValue("LOOK_BACK", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		settingsWheel.GetLongValue("LOOK_BACK", "BUTTON", -1);

	// [CHANGE_CAMERA]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		settingsWheel.GetValue("CHANGE_CAMERA", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		settingsWheel.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

	// [RADIO_NEXT]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		settingsWheel.GetValue("RADIO_NEXT", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		settingsWheel.GetLongValue("RADIO_NEXT", "BUTTON", -1);

	// [RADIO_PREVIOUS]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		settingsWheel.GetValue("RADIO_PREVIOUS", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		settingsWheel.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

	// [INDICATOR_LEFT]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		settingsWheel.GetValue("INDICATOR_LEFT", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		settingsWheel.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

	// [INDICATOR_RIGHT]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		settingsWheel.GetValue("INDICATOR_RIGHT", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		settingsWheel.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

	// [INDICATOR_HAZARD]
	scriptControl->WheelButtonDevices[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		settingsWheel.GetValue("INDICATOR_HAZARD", "DEVICE", "");
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		settingsWheel.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);
	
	// [TO_KEYBOARD]
	scriptControl->WheelToKeyDevice = settingsWheel.GetValue("TO_KEYBOARD", "DEVICE", "");
	for (int i = 0; i < MAX_RGBBUTTONS; i++) { // Ouch
		std::string entryString = settingsWheel.GetValue("TO_KEYBOARD", std::to_string(i).c_str(), "UNKNOWN");
		if (std::string(entryString).compare("UNKNOWN") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(entryString);
		}
	}

	// [FILEVERSION]
	settings_general_version = settingsWheel.GetLongValue("FILEVERSION", "VERSION", 0);

	mapDevice2GUID(scriptControl);
}
