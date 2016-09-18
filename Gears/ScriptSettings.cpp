#include "ScriptSettings.hpp"
#include <Windows.h>

#include "Logger.hpp"
#include <string>

ScriptSettings::ScriptSettings() {
	EnableManual = true;
	RealReverse = true;
	SimpleBike = true;

	EngDamage = false;
	EngStall = false;
	EngBrake = false;
	ClutchCatching = false;
	ClutchShifting = false;
	DefaultNeutral = false;
	UITips = false;
	ShiftMode = 0;
	WheelEnabled = false;
	WheelRange = 180;
	FFEnable = true;
	DamperMax = 60;
	DamperMin = 30;
	FFPhysics = 1.0f;
	CenterStrength = 1.0f;
	DisableDpad = false;

	Debug = false;

	UITips_X = 0.0f;
	UITips_Y = 0.0f;
	UITips_Size = 0.0f;

	ClutchCatchpoint = 0.0f;
	StallingThreshold = 0.0f;
	RPMDamage = 0.0f;
	MisshiftDamage = 0;
	WheelWithoutManual = true;
	CrossScript = false;
	TargetSpeed = 10;
	HillBrakeWorkaround = false;
}


void ScriptSettings::Read(ScriptControls* scriptControl) {
	EnableManual = (GetPrivateProfileIntA("OPTIONS", "Enable", 1, SETTINGSFILE) == 1);
	ShiftMode = GetPrivateProfileIntA("OPTIONS", "ShiftMode", 0, SETTINGSFILE);
	RealReverse = (GetPrivateProfileIntA("OPTIONS", "RealReverse", 1, SETTINGSFILE) == 1);
	SimpleBike = (GetPrivateProfileIntA("OPTIONS", "SimpleBike", 1, SETTINGSFILE) == 1);
	EngDamage = (GetPrivateProfileIntA("OPTIONS", "EngineDamage", 0, SETTINGSFILE) == 1);
	EngStall = (GetPrivateProfileIntA("OPTIONS", "EngineStalling", 0, SETTINGSFILE) == 1);
	EngBrake = (GetPrivateProfileIntA("OPTIONS", "EngineBraking", 0, SETTINGSFILE) == 1);
	ClutchCatching = (GetPrivateProfileIntA("OPTIONS", "ClutchCatching", 0, SETTINGSFILE) == 1);
	ClutchShifting = (GetPrivateProfileIntA("OPTIONS", "ClutchShifting", 0, SETTINGSFILE) == 1);
	DefaultNeutral = (GetPrivateProfileIntA("OPTIONS", "DefaultNeutral", 0, SETTINGSFILE) == 1);
	UITips = (GetPrivateProfileIntA("OPTIONS", "UITips", 1, SETTINGSFILE) == 1);
	UITips_X = GetPrivateProfileIntA("OPTIONS", "UITips_X", 95, SETTINGSFILE) / 100.0f;
	UITips_Y = GetPrivateProfileIntA("OPTIONS", "UITips_Y", 95, SETTINGSFILE) / 100.0f;
	UITips_Size = GetPrivateProfileIntA("OPTIONS", "UITips_Size", 15, SETTINGSFILE) / 100.0f;
	Debug = (GetPrivateProfileIntA("DEBUG", "Info", 0, SETTINGSFILE) == 1);
	ClutchCatchpoint = GetPrivateProfileIntA("OPTIONS", "ClutchCatchpoint", 20, SETTINGSFILE) / 100.0f;
	StallingThreshold = GetPrivateProfileIntA("OPTIONS", "StallingThreshold", 25, SETTINGSFILE) / 100.0f;

	RPMDamage = GetPrivateProfileIntA("OPTIONS", "RPMDamage", 15, SETTINGSFILE) / 100.0f;
	MisshiftDamage = GetPrivateProfileIntA("OPTIONS", "MisshiftDamage", 10, SETTINGSFILE);
	CrossScript = (GetPrivateProfileIntA("OPTIONS", "CrossScript", 0, SETTINGSFILE) == 1);
	HillBrakeWorkaround = (GetPrivateProfileIntA("OPTIONS", "HillBrakeWorkaround", 0, SETTINGSFILE) == 1);

	CheckSettings();

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

	// Start Keyboard section
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Toggle)] = GetPrivateProfileIntA("KEYBOARD", "Toggle", VK_OEM_5, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ToggleH)] = GetPrivateProfileIntA("KEYBOARD", "ToggleH", VK_OEM_6, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftUp)] = GetPrivateProfileIntA("KEYBOARD", "ShiftUp", VK_NUMPAD9, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::ShiftDown)] = GetPrivateProfileIntA("KEYBOARD", "ShiftDown", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Clutch)] = GetPrivateProfileIntA("KEYBOARD", "Clutch", VK_NUMPAD8, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Engine)] = GetPrivateProfileIntA("KEYBOARD", "Engine", 0x45, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HR)] = GetPrivateProfileIntA("KEYBOARD", "HR", VK_NUMPAD0, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H1)] = GetPrivateProfileIntA("KEYBOARD", "H1", VK_NUMPAD1, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H2)] = GetPrivateProfileIntA("KEYBOARD", "H2", VK_NUMPAD2, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H3)] = GetPrivateProfileIntA("KEYBOARD", "H3", VK_NUMPAD3, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H4)] = GetPrivateProfileIntA("KEYBOARD", "H4", VK_NUMPAD4, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H5)] = GetPrivateProfileIntA("KEYBOARD", "H5", VK_NUMPAD5, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H6)] = GetPrivateProfileIntA("KEYBOARD", "H6", VK_NUMPAD6, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H7)] = GetPrivateProfileIntA("KEYBOARD", "H7", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::H8)] = GetPrivateProfileIntA("KEYBOARD", "H8", VK_NUMPAD8, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::HN)] = GetPrivateProfileIntA("KEYBOARD", "HN", VK_NUMPAD9, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Throttle)] = GetPrivateProfileIntA("KEYBOARD", "Throttle", 0x57, SETTINGSFILE);
	scriptControl->KBControl[static_cast<int>(ScriptControls::KeyboardControlType::Brake)] = GetPrivateProfileIntA("KEYBOARD", "Brake", 0x53, SETTINGSFILE);

	// Start wheel section
	WheelEnabled = (GetPrivateProfileIntA("WHEEL", "Enable", 0, SETTINGSFILE) == 1);
	WheelWithoutManual = (GetPrivateProfileIntA("WHEEL", "WheelWithoutManual", 1, SETTINGSFILE) == 1);
	DisableDpad = (GetPrivateProfileIntA("WHEEL", "DisableDpad", 0, SETTINGSFILE) == 1);

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
	scriptControl->ThrottleMin = GetPrivateProfileIntA("WHEELAXIS", "ThrottleMin", -1000, SETTINGSFILE);
	scriptControl->ThrottleMax = GetPrivateProfileIntA("WHEELAXIS", "ThrottleMax", 1000, SETTINGSFILE);
	
	GetPrivateProfileStringA("WHEELAXIS", "Brake", "lRz", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] = buffer;
	scriptControl->BrakeMin = GetPrivateProfileIntA("WHEELAXIS", "BrakeMin", -1000, SETTINGSFILE);
	scriptControl->BrakeMax = GetPrivateProfileIntA("WHEELAXIS", "BrakeMax", 1000, SETTINGSFILE);

	GetPrivateProfileStringA("WHEELAXIS", "Clutch", "rglSlider1", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] = buffer;
	scriptControl->ClutchMin = GetPrivateProfileIntA("WHEELAXIS", "ClutchMin", -1000, SETTINGSFILE);
	scriptControl->ClutchMax = GetPrivateProfileIntA("WHEELAXIS", "ClutchMax", 1000, SETTINGSFILE);

	GetPrivateProfileStringA("WHEELAXIS", "Steer", "lX", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] = buffer;
	scriptControl->SteerLeft = GetPrivateProfileIntA("WHEELAXIS", "SteerLeft", -1000, SETTINGSFILE);
	scriptControl->SteerRight = GetPrivateProfileIntA("WHEELAXIS", "SteerRight", 1000, SETTINGSFILE);
	
	GetPrivateProfileStringA("WHEELAXIS", "FFAxis", "X", buffer, static_cast<DWORD>(24), SETTINGSFILE);
	scriptControl->FFAxis = buffer;

	scriptControl->ClutchDisable = (GetPrivateProfileIntA("WHEELAXIS", "ClutchDisable", 0, SETTINGSFILE) == 1);
	
	WheelRange = GetPrivateProfileIntA("WHEEL", "WheelRange", 180, SETTINGSFILE);
	FFEnable = GetPrivateProfileIntA("WHEEL", "FFEnable", 1, SETTINGSFILE) == 1;
	DamperMax = GetPrivateProfileIntA("WHEEL", "DamperMax", 50, SETTINGSFILE);
	DamperMin = GetPrivateProfileIntA("WHEEL", "DamperMin", 20, SETTINGSFILE);
	TargetSpeed = GetPrivateProfileIntA("WHEEL", "DamperTargetSpeed", 10, SETTINGSFILE);
	FFPhysics = GetPrivateProfileIntA("WHEEL", "PhysicsStrength", 170, SETTINGSFILE) / 100.0f;
	CenterStrength = GetPrivateProfileIntA("WHEEL", "CenterStrength", 100, SETTINGSFILE) / 100.0f;
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
