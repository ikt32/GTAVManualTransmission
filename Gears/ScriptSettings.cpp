#include "ScriptSettings.hpp"
#include <Windows.h>

#include "Util/Logger.hpp"
#include <string>
#include "Input/keyboard.h"

ScriptSettings::ScriptSettings() {
}


void ScriptSettings::Read(ScriptControls* scriptControl) {
	EnableManual = (GetPrivateProfileIntA("OPTIONS", "Enable", 1, SETTINGSFILE) == 1);
	ShiftMode = GetPrivateProfileIntA("OPTIONS", "ShiftMode", 0, SETTINGSFILE);
	SimpleBike = (GetPrivateProfileIntA("OPTIONS", "SimpleBike", 0, SETTINGSFILE) == 1);
	CrossScript = (GetPrivateProfileIntA("OPTIONS", "CrossScript", 0, SETTINGSFILE) == 1);
	
	AutoLookBack = (GetPrivateProfileIntA("OPTIONS", "AutoLookBack", 0, SETTINGSFILE) == 1);
	HillBrakeWorkaround = (GetPrivateProfileIntA("OPTIONS", "HillBrakeWorkaround", 0, SETTINGSFILE) == 1);

	EngDamage = (GetPrivateProfileIntA("OPTIONS", "EngineDamage", 0, SETTINGSFILE) == 1);
	EngStall = (GetPrivateProfileIntA("OPTIONS", "EngineStalling", 0, SETTINGSFILE) == 1);
	EngBrake = (GetPrivateProfileIntA("OPTIONS", "EngineBraking", 0, SETTINGSFILE) == 1);
	RPMDamage = GetPrivateProfileIntA("OPTIONS", "RPMDamage", 15, SETTINGSFILE) / 100.0f;
	MisshiftDamage = GetPrivateProfileIntA("OPTIONS", "MisshiftDamage", 10, SETTINGSFILE);

	ClutchCatching = (GetPrivateProfileIntA("OPTIONS", "ClutchCatching", 0, SETTINGSFILE) == 1);
	ClutchShifting = (GetPrivateProfileIntA("OPTIONS", "ClutchShifting", 0, SETTINGSFILE) == 1);
	ClutchCatchpoint = GetPrivateProfileIntA("OPTIONS", "ClutchCatchpoint", 15, SETTINGSFILE) / 100.0f;
	StallingThreshold = GetPrivateProfileIntA("OPTIONS", "StallingThreshold", 75, SETTINGSFILE) / 100.0f;
	DefaultNeutral = (GetPrivateProfileIntA("OPTIONS", "DefaultNeutral", 0, SETTINGSFILE) == 1);
	
	UITips = (GetPrivateProfileIntA("OPTIONS", "UITips", 1, SETTINGSFILE) == 1);
	UITips_OnlyNeutral = (GetPrivateProfileIntA("OPTIONS", "UITips_OnlyNeutral", 0, SETTINGSFILE) == 1);
	UITips_X = GetPrivateProfileIntA("OPTIONS", "UITips_X", 95, SETTINGSFILE) / 100.0f;
	UITips_Y = GetPrivateProfileIntA("OPTIONS", "UITips_Y", 95, SETTINGSFILE) / 100.0f;
	UITips_Size = GetPrivateProfileIntA("OPTIONS", "UITips_Size", 15, SETTINGSFILE) / 100.0f;
	UITips_TopGearC_R = GetPrivateProfileIntA("OPTIONS", "UITips_TopGearC_R", 255, SETTINGSFILE);
	UITips_TopGearC_G = GetPrivateProfileIntA("OPTIONS", "UITips_TopGearC_G", 255, SETTINGSFILE);
	UITips_TopGearC_B = GetPrivateProfileIntA("OPTIONS", "UITips_TopGearC_B", 255, SETTINGSFILE);

	Debug = (GetPrivateProfileIntA("DEBUG", "Info", 0, SETTINGSFILE) == 1);
	AltControls = (GetPrivateProfileIntA("DEBUG", "AltControls", 0, SETTINGSFILE) == 1);
	SteerAngleAlt = GetPrivateProfileIntA("DEBUG", "AltAngle", 180, SETTINGSFILE) / 1.0f;

	// Start Controller section
	char buffer[24] = {0};
	GetPrivateProfileStringA("CONTROLLER", "Toggle", "DpadRight", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Toggle)] = buffer;
	scriptControl->CToggleTime = GetPrivateProfileIntA("CONTROLLER", "ToggleTime", 500, SETTINGSFILE);
	GetPrivateProfileStringA("CONTROLLER", "ShiftUp", "A", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftUp)] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "ShiftDown", "X", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::ShiftDown)] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Clutch", "LeftThumbDown", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Clutch)] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Engine", "DpadDown", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Engine)] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Throttle", "RightTrigger", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Throttle)] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Brake", "LeftTrigger", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->ControlXbox[static_cast<int>(ScriptControls::ControllerControlType::Brake)] = buffer;

	int tval = GetPrivateProfileIntA("CONTROLLER", "TriggerValue", 75, SETTINGSFILE);
	if (tval > 100 || tval < 0) {
		tval = 75;
	}
	scriptControl->SetXboxTrigger(tval);


	// Start Keyboard section
	// How it feels to chew 5 gum
	char kbKeyBuffer[24];
	
	GetPrivateProfileStringA("KEYBOARD", "Toggle", "VK_OEM_5", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)]   = str2key(kbKeyBuffer);

	GetPrivateProfileStringA("KEYBOARD", "ToggleH", "VK_OEM_6", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)]   = str2key(kbKeyBuffer);

	GetPrivateProfileStringA("KEYBOARD", "ShiftUp", "SHIFT", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)]   = str2key(kbKeyBuffer);

	GetPrivateProfileStringA("KEYBOARD", "ShiftDown", "CTRL", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)] = str2key(kbKeyBuffer);

	GetPrivateProfileStringA("KEYBOARD", "Clutch", "X", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)]    = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "Engine", "C", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)]    = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "HR", "NUM0", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H1", "NUM1", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H2", "NUM2", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H3", "NUM3", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H4", "NUM4", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H5", "NUM5", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H6", "NUM6", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H7", "NUM7", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "H8", "NUM8", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H8)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "HN", "NUM9", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)]        = str2key(kbKeyBuffer);
	
	GetPrivateProfileStringA("KEYBOARD", "Throttle", "W", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)]  = str2key(kbKeyBuffer);

	GetPrivateProfileStringA("KEYBOARD", "Brake", "S", kbKeyBuffer, 24, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)]     = str2key(kbKeyBuffer);

	// Start wheel section
	WheelEnabled = (GetPrivateProfileIntA("WHEEL", "Enable", 0, SETTINGSFILE) == 1);
	WheelWithoutManual = (GetPrivateProfileIntA("WHEEL", "WheelWithoutManual", 1, SETTINGSFILE) == 1);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Toggle)] = GetPrivateProfileIntA("WHEEL", "Toggle", 17, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] = GetPrivateProfileIntA("WHEEL", "ToggleH", 6, SETTINGSFILE);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] = GetPrivateProfileIntA("WHEEL", "ShiftUp", 4, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] = GetPrivateProfileIntA("WHEEL", "ShiftDown", 5, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::HR)] = GetPrivateProfileIntA("WHEEL", "HR", 14, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H1)] = GetPrivateProfileIntA("WHEEL", "H1", 8, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H2)] = GetPrivateProfileIntA("WHEEL", "H2", 9, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H3)] = GetPrivateProfileIntA("WHEEL", "H3", 10, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H4)] = GetPrivateProfileIntA("WHEEL", "H4", 11, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H5)] = GetPrivateProfileIntA("WHEEL", "H5", 12, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::H6)] = GetPrivateProfileIntA("WHEEL", "H6", 13, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] = GetPrivateProfileIntA("WHEEL", "Handbrake", 19, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Engine)] = GetPrivateProfileIntA("WHEEL", "Engine", 21, SETTINGSFILE);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Horn)] = GetPrivateProfileIntA("WHEEL", "Horn", 20, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Lights)] = GetPrivateProfileIntA("WHEEL", "Lights", 7, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::LookBack)] = GetPrivateProfileIntA("WHEEL", "LookBack", 22, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::Camera)] = GetPrivateProfileIntA("WHEEL", "Camera", 0, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] = GetPrivateProfileIntA("WHEEL", "RadioPrev", 1, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] = GetPrivateProfileIntA("WHEEL", "RadioNext", 2, SETTINGSFILE);

	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] = GetPrivateProfileIntA("WHEEL", "IndicatorLeft", 19, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] = GetPrivateProfileIntA("WHEEL", "IndicatorRight", 21, SETTINGSFILE);
	scriptControl->WheelControl[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] = GetPrivateProfileIntA("WHEEL", "IndicatorHazard", 15, SETTINGSFILE);

	GetPrivateProfileStringA("WHEELAXIS", "Throttle", "lY", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] = buffer; 
	scriptControl->ThrottleMin = GetPrivateProfileIntA("WHEELAXIS", "ThrottleMin", 0, SETTINGSFILE);
	scriptControl->ThrottleMax = GetPrivateProfileIntA("WHEELAXIS", "ThrottleMax", 65535, SETTINGSFILE);
	
	GetPrivateProfileStringA("WHEELAXIS", "Brake", "lRz", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] = buffer;
	scriptControl->BrakeMin = GetPrivateProfileIntA("WHEELAXIS", "BrakeMin", 0, SETTINGSFILE);
	scriptControl->BrakeMax = GetPrivateProfileIntA("WHEELAXIS", "BrakeMax", 65535, SETTINGSFILE);

	GetPrivateProfileStringA("WHEELAXIS", "Clutch", "rglSlider1", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] = buffer;
	scriptControl->ClutchMin = GetPrivateProfileIntA("WHEELAXIS", "ClutchMin", 0, SETTINGSFILE);
	scriptControl->ClutchMax = GetPrivateProfileIntA("WHEELAXIS", "ClutchMax", 65535, SETTINGSFILE);

	GetPrivateProfileStringA("WHEELAXIS", "Steer", "lX", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] = buffer;
	scriptControl->SteerLeft = GetPrivateProfileIntA("WHEELAXIS", "SteerLeft", 0, SETTINGSFILE);
	scriptControl->SteerRight = GetPrivateProfileIntA("WHEELAXIS", "SteerRight", 65535, SETTINGSFILE);
	
	GetPrivateProfileStringA("WHEELAXIS", "FFAxis", "X", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->FFAxis = buffer;

	scriptControl->ClutchDisable = (GetPrivateProfileIntA("WHEELAXIS", "ClutchDisable", 0, SETTINGSFILE) == 1);

	SteerAngleMax = GetPrivateProfileIntA("WHEELAXIS", "SteerAngleMax", 900, SETTINGSFILE) / 1.0f;
	SteerAngleCar = GetPrivateProfileIntA("WHEELAXIS", "SteerAngleCar", 720, SETTINGSFILE) / 1.0f;
	SteerAngleBike = GetPrivateProfileIntA("WHEELAXIS", "SteerAngleBike", 180, SETTINGSFILE) / 1.0f;

	//WheelRange = GetPrivateProfileIntA("WHEEL", "WheelRange", 180, SETTINGSFILE);
	FFEnable = GetPrivateProfileIntA("WHEEL", "FFEnable", 1, SETTINGSFILE) == 1;
	DamperMax = GetPrivateProfileIntA("WHEEL", "DamperMax", 50, SETTINGSFILE);
	DamperMin = GetPrivateProfileIntA("WHEEL", "DamperMin", 20, SETTINGSFILE);
	TargetSpeed = GetPrivateProfileIntA("WHEEL", "DamperTargetSpeed", 10, SETTINGSFILE);
	FFPhysics = GetPrivateProfileIntA("WHEEL", "PhysicsStrength", 170, SETTINGSFILE) / 100.0f;
	CenterStrength = GetPrivateProfileIntA("WHEEL", "CenterStrength", 100, SETTINGSFILE) / 100.0f;
	DetailStrength = GetPrivateProfileIntA("WHEEL", "DetailStrength", 100, SETTINGSFILE) / 1.0f;

	char w2kBuffer[24];
	for (int i = 0; i < MAX_RGBBUTTONS; i++) { // Ouch
		GetPrivateProfileStringA("WHEELKEYBOARD", std::to_string(i).c_str(), "NOPE", w2kBuffer, 24, SETTINGSFILE);
		if (std::string(w2kBuffer).compare("NOPE") == 0) {
			scriptControl->WheelToKey[i] = -1;
		}
		else {
			scriptControl->WheelToKey[i] = str2key(w2kBuffer);
		}
	}

	CheckSettings();
	if (scriptControl->ClutchDisable) {
		ClutchShifting = false;
		WritePrivateProfileStringA("OPTIONS", "ClutchShifting", "0", SETTINGSFILE);
	}
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
