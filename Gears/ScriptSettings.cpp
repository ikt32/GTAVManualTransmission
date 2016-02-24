#include "ScriptSettings.hpp"
#include "..\..\ScriptHookV_SDK\inc\enums.h"

ScriptSettings::ScriptSettings() {
	EnableManual = false;
	AutoGear1    = false;
	AutoReverse  = false;
	OldReverse   = false;
	EngDamage    = false;
	EngStall     = false;
	EngBrake     = false;
	Debug        = false;
	Hshifter     = false;
}


void ScriptSettings::Read(ScriptControls *scriptControl) {
	EnableManual = (GetPrivateProfileInt(L"MAIN", L"DefaultEnable", 1, L"./Gears.ini") == 1);
	AutoGear1 = (GetPrivateProfileInt(L"MAIN", L"AutoGear1", 0, L"./Gears.ini") == 1);
	AutoReverse = (GetPrivateProfileInt(L"MAIN", L"AutoReverse", 0, L"./Gears.ini") == 1);
	OldReverse = (GetPrivateProfileInt(L"MAIN", L"OldReverse", 0, L"./Gears.ini") == 1);
	EngDamage = (GetPrivateProfileInt(L"MAIN", L"EngineDamage", 0, L"./Gears.ini") == 1);
	EngStall = GetPrivateProfileInt(L"MAIN", L"EngineStalling", 0, L"./Gears.ini");
	EngBrake = (GetPrivateProfileInt(L"MAIN", L"EngineBraking", 0, L"./Gears.ini") == 1);
	ClutchCatching = (GetPrivateProfileInt(L"MAIN", L"ClutchCatching", 0, L"./Gears.ini") == 1);
	DefaultNeutral = (GetPrivateProfileInt(L"MAIN", L"DefaultNeutral", 0, L"./Gears.ini") == 1);
	UITips = (GetPrivateProfileInt(L"MAIN", L"UITips", 1, L"./Gears.ini") == 1);
	Hshifter = (GetPrivateProfileInt(L"CONTROLS", L"EnableH", 0, L"./Gears.ini") == 1);
	Debug = (GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini") == 1);

	scriptControl->Control[ScriptControls::ControlType::Toggle] = GetPrivateProfileInt(L"MAIN", L"Toggle", VK_OEM_5, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::CToggle] = GetPrivateProfileInt(L"MAIN", L"CToggle", ControlScriptPadRight, L"./Gears.ini");
	scriptControl->CToggleTime = GetPrivateProfileInt(L"MAIN", L"CToggleTime", 500, L"./Gears.ini");

	scriptControl->Control[ScriptControls::ControlType::ShiftUp] = GetPrivateProfileInt(L"CONTROLS", L"ShiftUp", ControlFrontendAccept, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::ShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"ShiftDown", ControlFrontendX, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::Clutch] = GetPrivateProfileInt(L"CONTROLS", L"Clutch", ControlFrontendLb, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::Engine] = GetPrivateProfileInt(L"CONTROLS", L"Engine", ControlFrontendLs, L"./Gears.ini");

	scriptControl->Control[ScriptControls::ControlType::KShiftUp] = GetPrivateProfileInt(L"CONTROLS", L"KShiftUp", VK_NUMPAD9, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KShiftDown] = GetPrivateProfileInt(L"CONTROLS", L"KShiftDown", VK_NUMPAD7, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KClutch] = GetPrivateProfileInt(L"CONTROLS", L"KClutch", VK_NUMPAD8, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KEngine] = GetPrivateProfileInt(L"CONTROLS", L"KEngine", 0x45, L"./Gears.ini");

	scriptControl->Control[ScriptControls::ControlType::ToggleH] = GetPrivateProfileInt(L"CONTROLS", L"ToggleH", VK_OEM_6, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::HR] = GetPrivateProfileInt(L"CONTROLS", L"HR", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H1] = GetPrivateProfileInt(L"CONTROLS", L"H1", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H2] = GetPrivateProfileInt(L"CONTROLS", L"H2", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H3] = GetPrivateProfileInt(L"CONTROLS", L"H3", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H4] = GetPrivateProfileInt(L"CONTROLS", L"H4", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H5] = GetPrivateProfileInt(L"CONTROLS", L"H5", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H6] = GetPrivateProfileInt(L"CONTROLS", L"H6", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H7] = GetPrivateProfileInt(L"CONTROLS", L"H7", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::H8] = GetPrivateProfileInt(L"CONTROLS", L"H8", VK_NUMPAD0, L"./Gears.ini");
	scriptControl->Control[ScriptControls::KEngageNeutral] = GetPrivateProfileInt(L"CONTROLS", L"KNeutral", VK_DECIMAL, L"./Gears.ini");

	scriptControl->Control[ScriptControls::ControlType::CThrottle] = GetPrivateProfileInt(L"CONTROLS", L"CThrottle", ControlFrontendRt, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::CBrake] = GetPrivateProfileInt(L"CONTROLS", L"CBrake", ControlFrontendLt, L"./Gears.ini");

	scriptControl->Control[ScriptControls::ControlType::KThrottle] = GetPrivateProfileInt(L"CONTROLS", L"KThrottle", 0x57, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KBrake] = GetPrivateProfileInt(L"CONTROLS", L"KBrake", 0x53, L"./Gears.ini");
}

void ScriptSettings::Save() {
	WritePrivateProfileString(L"MAIN", L"DefaultEnable", (EnableManual ? L" 1" : L" 0"), L"./Gears.ini");
	WritePrivateProfileString(L"CONTROLS", L"EnableH", (Hshifter ? L" 1" : L" 0"), L"./Gears.ini");
}
