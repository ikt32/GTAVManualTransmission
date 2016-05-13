#include "ScriptSettings.hpp"
#include "..\..\ScriptHookV_SDK\inc\enums.h"

#include "Logger.hpp"

ScriptSettings::ScriptSettings() {
	EnableManual = false;
	//AutoGear1    = false;
	//AutoReverse  = false;
	RealReverse  = true;
	EngDamage    = false;
	EngStall     = false;
	EngBrake     = false;
	Debug        = false;
	Hshifter     = false;
	LogiWheel    = false;
}


void ScriptSettings::Read(ScriptControls *scriptControl) {
	EnableManual =   (GetPrivateProfileIntA("MAIN", "DefaultEnable",  1, SETTINGSFILE) == 1);
	//AutoGear1 =      (GetPrivateProfileIntA("MAIN", "AutoGear1",      1, SETTINGSFILE) == 1);
	//AutoReverse =    (GetPrivateProfileIntA("MAIN", "AutoReverse",    0, SETTINGSFILE) == 1);
	RealReverse =    (GetPrivateProfileIntA("MAIN", "RealReverse",    1, SETTINGSFILE) == 1);
	SimpleBike =     (GetPrivateProfileIntA("MAIN", "SimpleBike",     1, SETTINGSFILE) == 1);
	EngDamage =      (GetPrivateProfileIntA("MAIN", "EngineDamage",   0, SETTINGSFILE) == 1);
	EngStall =       (GetPrivateProfileIntA("MAIN", "EngineStalling", 0, SETTINGSFILE) == 1);
	EngBrake =       (GetPrivateProfileIntA("MAIN", "EngineBraking",  0, SETTINGSFILE) == 1);
	ClutchCatching = (GetPrivateProfileIntA("MAIN", "ClutchCatching", 0, SETTINGSFILE) == 1);
	DefaultNeutral = (GetPrivateProfileIntA("MAIN", "DefaultNeutral", 0, SETTINGSFILE) == 1);
	UITips =         (GetPrivateProfileIntA("MAIN", "UITips",         1, SETTINGSFILE) == 1);
	UITips_X =       GetPrivateProfileIntA("MAIN", "UITips_X",      95, SETTINGSFILE)/100.0f;
	UITips_Y =       GetPrivateProfileIntA("MAIN", "UITips_Y",      95, SETTINGSFILE)/100.0f;
	UITips_Size =    GetPrivateProfileIntA("MAIN", "UITips_Size",   15, SETTINGSFILE)/100.0f;

	Hshifter =       (GetPrivateProfileIntA("CONTROLS", "EnableH",    0, SETTINGSFILE) == 1);
	LogiWheel =      (GetPrivateProfileIntA("LOGITECHWHEEL", "Enable",0, SETTINGSFILE) == 1);
	Debug =          (GetPrivateProfileIntA("DEBUG", "Info",          0, SETTINGSFILE) == 1);

	Check();

	scriptControl->CToggleTime = GetPrivateProfileIntA("MAIN", "CToggleTime", 500, SETTINGSFILE);

	char buffer[24] = {0};
	GetPrivateProfileStringA("MAIN", "CToggle", "DpadRight", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::CToggle] = buffer; 
	
	GetPrivateProfileStringA("CONTROLLER", "ShiftUp", "A", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::ShiftUp] = buffer;

	GetPrivateProfileStringA("CONTROLLER", "ShiftDown", "X", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::ShiftDown] = buffer;

	GetPrivateProfileStringA("CONTROLLER", "Clutch", "LeftThumbDown", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::Clutch] = buffer;

	GetPrivateProfileStringA("CONTROLLER", "Engine", "DpadDown", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::Engine] = buffer;

	GetPrivateProfileStringA("CONTROLLER", "CThrottle", "RightTrigger", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::CThrottle] = buffer;

	GetPrivateProfileStringA("CONTROLLER", "CBrake", "LeftTrigger", buffer, (DWORD)24, SETTINGSFILE);
	scriptControl->ControlXbox[(int)ScriptControls::ControlType::CBrake] = buffer;


	scriptControl->Control[(int)ScriptControls::ControlType::KToggle] = GetPrivateProfileIntA("MAIN", "KToggle", VK_OEM_5, SETTINGSFILE);

	scriptControl->Control[(int)ScriptControls::ControlType::KShiftUp] = GetPrivateProfileIntA("KEYBOARD", "KShiftUp", VK_NUMPAD9, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::KShiftDown] = GetPrivateProfileIntA("KEYBOARD", "KShiftDown", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::KClutch] = GetPrivateProfileIntA("KEYBOARD", "KClutch", VK_NUMPAD8, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::KEngine] = GetPrivateProfileIntA("KEYBOARD", "KEngine", 0x45, SETTINGSFILE);

	scriptControl->Control[(int)ScriptControls::ControlType::ToggleH] = GetPrivateProfileIntA("KEYBOARD", "ToggleH", VK_OEM_6, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::HR] = GetPrivateProfileIntA("KEYBOARD", "HR", VK_NUMPAD0, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H1] = GetPrivateProfileIntA("KEYBOARD", "H1", VK_NUMPAD1, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H2] = GetPrivateProfileIntA("KEYBOARD", "H2", VK_NUMPAD2, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H3] = GetPrivateProfileIntA("KEYBOARD", "H3", VK_NUMPAD3, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H4] = GetPrivateProfileIntA("KEYBOARD", "H4", VK_NUMPAD4, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H5] = GetPrivateProfileIntA("KEYBOARD", "H5", VK_NUMPAD5, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H6] = GetPrivateProfileIntA("KEYBOARD", "H6", VK_NUMPAD6, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H7] = GetPrivateProfileIntA("KEYBOARD", "H7", VK_NUMPAD7, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::H8] = GetPrivateProfileIntA("KEYBOARD", "H8", VK_NUMPAD8, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::HN] = GetPrivateProfileIntA("KEYBOARD", "HN", VK_NUMPAD9, SETTINGSFILE);

	scriptControl->Control[(int)ScriptControls::ControlType::KThrottle] = GetPrivateProfileIntA("KEYBOARD", "KThrottle", 0x57, SETTINGSFILE);
	scriptControl->Control[(int)ScriptControls::ControlType::KBrake] = GetPrivateProfileIntA("KEYBOARD", "KBrake", 0x53, SETTINGSFILE);

	//Check();

	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::ShiftUp] = GetPrivateProfileIntA("LOGITECHWHEEL", "ShiftUp", 4, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::ShiftDown] = GetPrivateProfileIntA("LOGITECHWHEEL", "ShiftDown", 5, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::HR] = GetPrivateProfileIntA("LOGITECHWHEEL", "HR", 14, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H1] = GetPrivateProfileIntA("LOGITECHWHEEL", "H1", 8, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H2] = GetPrivateProfileIntA("LOGITECHWHEEL", "H2", 9, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H3] = GetPrivateProfileIntA("LOGITECHWHEEL", "H3", 10, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H4] = GetPrivateProfileIntA("LOGITECHWHEEL", "H4", 11, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H5] = GetPrivateProfileIntA("LOGITECHWHEEL", "H5", 12, SETTINGSFILE);
	scriptControl->LogiControl[(int)ScriptControls::LogiControlType::H6] = GetPrivateProfileIntA("LOGITECHWHEEL", "H6", 13, SETTINGSFILE);
}

void ScriptSettings::Save() {
	WritePrivateProfileStringA("MAIN", "DefaultEnable", (EnableManual ? " 1" : " 0"), SETTINGSFILE);
	WritePrivateProfileStringA("CONTROLS", "EnableH", (Hshifter ? " 1" : " 0"), SETTINGSFILE);
}

// Checks for conflicting settings and adjusts them
/*
void ScriptSettings::Check() {
	if (AutoReverse && RealReverse) {
		Logger logger(LOGFILE);
		logger.Write("AutoReverse and RealReverse conflict. Adjusting to RealReverse");
		AutoReverse = false;
		WritePrivateProfileStringA("MAIN", "AutoReverse", " 0", SETTINGSFILE);
	}
}
*/

void ScriptSettings::Check() {
	Logger logger(LOGFILE);
	if (UITips_X > 100) {
		UITips_X = 100;
		logger.Write("UITips_X higher than 100, reverting");
		WritePrivateProfileStringA("MAIN", "UITips_X", "100", SETTINGSFILE);
	}
	if (UITips_Y > 100) {
		UITips_Y = 100;
		logger.Write("UITips_Y higher than 100, reverting");
		WritePrivateProfileStringA("MAIN", "UITips_Y", "100", SETTINGSFILE);
	}
}