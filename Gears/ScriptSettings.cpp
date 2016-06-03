#include "ScriptSettings.hpp"
#include "..\..\ScriptHookV_SDK\inc\enums.h"

#include "Logger.hpp"

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
	Hshifter = false;
	LogiWheel = false;
	WheelRange = 180;
	FFEnable = true;
	FFDamperStationary = 60;
	FFDamperMoving = 30;
	FFPhysics = 1.0f;
	FFCenterSpring = 1.0f;
	DisableDpad = false;

	Debug = false;

	UITips_X = 0.0f;
	UITips_Y = 0.0f;
	UITips_Size = 0.0f;

	ClutchCatchpoint = 0.0f;
	StallingThreshold = 0.0f;
	RPMDamage = 0.0f;
	MisshiftDamage = 0;
}


void ScriptSettings::Read(ScriptControls *scriptControl) {
	EnableManual =   (GetPrivateProfileIntA("OPTIONS", "Enable",         1, SETTINGSFILE) == 1);
	Hshifter =       (GetPrivateProfileIntA("OPTIONS", "EnableH",        0, SETTINGSFILE) == 1);
	RealReverse =    (GetPrivateProfileIntA("OPTIONS", "RealReverse",    1, SETTINGSFILE) == 1);
	SimpleBike =     (GetPrivateProfileIntA("OPTIONS", "SimpleBike",     1, SETTINGSFILE) == 1);
	EngDamage =      (GetPrivateProfileIntA("OPTIONS", "EngineDamage",   0, SETTINGSFILE) == 1);
	EngStall =       (GetPrivateProfileIntA("OPTIONS", "EngineStalling", 0, SETTINGSFILE) == 1);
	EngBrake =       (GetPrivateProfileIntA("OPTIONS", "EngineBraking",  0, SETTINGSFILE) == 1);
	ClutchCatching = (GetPrivateProfileIntA("OPTIONS", "ClutchCatching", 0, SETTINGSFILE) == 1);
	ClutchShifting = (GetPrivateProfileIntA("OPTIONS", "ClutchShifting", 0, SETTINGSFILE) == 1);
	DefaultNeutral = (GetPrivateProfileIntA("OPTIONS", "DefaultNeutral", 0, SETTINGSFILE) == 1);
	UITips =         (GetPrivateProfileIntA("OPTIONS", "UITips",         1, SETTINGSFILE) == 1);
	UITips_X =       GetPrivateProfileIntA("OPTIONS", "UITips_X",       95, SETTINGSFILE)/100.0f;
	UITips_Y =       GetPrivateProfileIntA("OPTIONS", "UITips_Y",       95, SETTINGSFILE)/100.0f;
	UITips_Size =    GetPrivateProfileIntA("OPTIONS", "UITips_Size",    15, SETTINGSFILE)/100.0f;
	Debug =          (GetPrivateProfileIntA("DEBUG", "Info",          0, SETTINGSFILE) == 1);
	ClutchCatchpoint = GetPrivateProfileIntA("OPTIONS", "ClutchCatchpoint", 20, SETTINGSFILE) / 100.0f;
	StallingThreshold = GetPrivateProfileIntA("OPTIONS", "StallingThreshold", 25, SETTINGSFILE) / 100.0f;

	RPMDamage = GetPrivateProfileIntA("OPTIONS", "RPMDamage", 15, SETTINGSFILE) / 100.0f;
	MisshiftDamage = GetPrivateProfileIntA("OPTIONS", "MisshiftDamage", 10, SETTINGSFILE);

	CheckSettings();

	// Start Controller section
	char buffer[24] = {0};
	GetPrivateProfileStringA("CONTROLLER", "Toggle", "DpadRight", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::Toggle] = buffer; 
	scriptControl->CToggleTime = GetPrivateProfileIntA("CONTROLLER", "ToggleTime", 500, SETTINGSFILE);
	GetPrivateProfileStringA("CONTROLLER", "ShiftUp", "A", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::ShiftUp] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "ShiftDown", "X", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::ShiftDown] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Clutch", "LeftThumbDown", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::Clutch] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Engine", "DpadDown", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::Engine] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Throttle", "RightTrigger", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::Throttle] = buffer;
	GetPrivateProfileStringA("CONTROLLER", "Brake", "LeftTrigger", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControllerControlType::Brake] = buffer;

	// Start Keyboard section
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::Toggle]  = GetPrivateProfileIntA("KEYBOARD", "Toggle",  VK_OEM_5, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::ToggleH] = GetPrivateProfileIntA("KEYBOARD", "ToggleH", VK_OEM_6, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::ShiftUp]   = GetPrivateProfileIntA("KEYBOARD", "ShiftUp",   VK_NUMPAD9, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::ShiftDown] = GetPrivateProfileIntA("KEYBOARD", "ShiftDown", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::Clutch]    = GetPrivateProfileIntA("KEYBOARD", "Clutch",    VK_NUMPAD8, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::Engine]    = GetPrivateProfileIntA("KEYBOARD", "Engine",    0x45,       SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::HR] = GetPrivateProfileIntA("KEYBOARD", "HR", VK_NUMPAD0, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H1] = GetPrivateProfileIntA("KEYBOARD", "H1", VK_NUMPAD1, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H2] = GetPrivateProfileIntA("KEYBOARD", "H2", VK_NUMPAD2, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H3] = GetPrivateProfileIntA("KEYBOARD", "H3", VK_NUMPAD3, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H4] = GetPrivateProfileIntA("KEYBOARD", "H4", VK_NUMPAD4, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H5] = GetPrivateProfileIntA("KEYBOARD", "H5", VK_NUMPAD5, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H6] = GetPrivateProfileIntA("KEYBOARD", "H6", VK_NUMPAD6, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H7] = GetPrivateProfileIntA("KEYBOARD", "H7", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::H8] = GetPrivateProfileIntA("KEYBOARD", "H8", VK_NUMPAD8, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::HN] = GetPrivateProfileIntA("KEYBOARD", "HN", VK_NUMPAD9, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::Throttle] = GetPrivateProfileIntA("KEYBOARD", "Throttle", 0x57, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::KeyboardControlType::Brake] = GetPrivateProfileIntA("KEYBOARD", "Brake", 0x53, SETTINGSFILE);

	// Start wheel section
	LogiWheel = (GetPrivateProfileIntA("LOGITECHWHEEL", "Enable", 0, SETTINGSFILE) == 1);
	DisableDpad = (GetPrivateProfileIntA("LOGITECHWHEEL", "DisableDpad", 0, SETTINGSFILE) == 1);

	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Toggle] = GetPrivateProfileIntA("LOGITECHWHEEL", "Toggle", 17, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::ToggleH] = GetPrivateProfileIntA("LOGITECHWHEEL", "ToggleH", 6, SETTINGSFILE);

	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::ShiftUp]   = GetPrivateProfileIntA("LOGITECHWHEEL", "ShiftUp", 4, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::ShiftDown] = GetPrivateProfileIntA("LOGITECHWHEEL", "ShiftDown", 5, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::HR]        = GetPrivateProfileIntA("LOGITECHWHEEL", "HR", 14, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H1]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H1", 8, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H2]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H2", 9, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H3]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H3", 10, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H4]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H4", 11, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H5]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H5", 12, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H6]        = GetPrivateProfileIntA("LOGITECHWHEEL", "H6", 13, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Handbrake] = GetPrivateProfileIntA("LOGITECHWHEEL", "Handbrake", 19, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Engine]    = GetPrivateProfileIntA("LOGITECHWHEEL", "Engine", 21, SETTINGSFILE);

	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Horn]      = GetPrivateProfileIntA("LOGITECHWHEEL", "Horn", 20, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Lights]    = GetPrivateProfileIntA("LOGITECHWHEEL", "Lights", 7, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::LookBack]  = GetPrivateProfileIntA("LOGITECHWHEEL", "LookBack", 22, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::Camera]    = GetPrivateProfileIntA("LOGITECHWHEEL", "Camera", 0, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::RadioPrev] = GetPrivateProfileIntA("LOGITECHWHEEL", "RadioPrev", 1, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::RadioNext] = GetPrivateProfileIntA("LOGITECHWHEEL", "RadioNext", 2, SETTINGSFILE);

	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::IndicatorLeft] = GetPrivateProfileIntA("LOGITECHWHEEL", "IndicatorLeft", 19, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::IndicatorRight] = GetPrivateProfileIntA("LOGITECHWHEEL", "IndicatorRight", 21, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::IndicatorHazard] = GetPrivateProfileIntA("LOGITECHWHEEL", "IndicatorHazard", 15, SETTINGSFILE);

	WheelRange = GetPrivateProfileIntA("LOGITECHWHEEL", "WheelRange", 180, SETTINGSFILE);
	FFEnable = GetPrivateProfileIntA("LOGITECHWHEEL", "FFEnable", 1, SETTINGSFILE) == 1;
	FFDamperStationary = GetPrivateProfileIntA("LOGITECHWHEEL", "FFDamperStationary", 60, SETTINGSFILE);
	FFDamperMoving = GetPrivateProfileIntA("LOGITECHWHEEL", "FFDamperMoving", 30, SETTINGSFILE);
	FFPhysics = GetPrivateProfileIntA("LOGITECHWHEEL", "FFPhysics", 150, SETTINGSFILE)/100.0f;
	FFCenterSpring = GetPrivateProfileIntA("LOGITECHWHEEL", "FFCenterSpring", 100, SETTINGSFILE)/100.0f;
}

void ScriptSettings::Save() {
	WritePrivateProfileStringA("OPTIONS", "Enable", (EnableManual ? " 1" : " 0"), SETTINGSFILE);
	WritePrivateProfileStringA("OPTIONS", "EnableH", (Hshifter ? " 1" : " 0"), SETTINGSFILE);
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
