#include "ScriptSettings.hpp"
#include "..\..\ScriptHookV_SDK\inc\enums.h"

ScriptSettings::ScriptSettings() {
	EnableManual = false;
	AutoGear1    = false;
	AutoReverse  = false;
	RealReverse  = true;
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
	RealReverse = (GetPrivateProfileInt(L"MAIN", L"RealReverse", 0, L"./Gears.ini") == 1);
	SimpleBike = (GetPrivateProfileInt(L"MAIN", L"SimpleBike", 0, L"./Gears.ini") == 1);
	EngDamage = (GetPrivateProfileInt(L"MAIN", L"EngineDamage", 0, L"./Gears.ini") == 1);
	EngStall = GetPrivateProfileInt(L"MAIN", L"EngineStalling", 0, L"./Gears.ini");
	EngBrake = (GetPrivateProfileInt(L"MAIN", L"EngineBraking", 0, L"./Gears.ini") == 1);
	ClutchCatching = (GetPrivateProfileInt(L"MAIN", L"ClutchCatching", 0, L"./Gears.ini") == 1);
	DefaultNeutral = (GetPrivateProfileInt(L"MAIN", L"DefaultNeutral", 0, L"./Gears.ini") == 1);
	UITips = (GetPrivateProfileInt(L"MAIN", L"UITips", 1, L"./Gears.ini") == 1);
	Hshifter = (GetPrivateProfileInt(L"CONTROLS", L"EnableH", 0, L"./Gears.ini") == 1);
	Debug = (GetPrivateProfileInt(L"DEBUG", L"Info", 0, L"./Gears.ini") == 1);

	char buffer[24] = {0};
	GetPrivateProfileStringA("MAIN", "CToggle", "DpadRight", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::CToggle] = buffer; 
	
	GetPrivateProfileStringA("CONTROLS", "ShiftUp", "A", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::ShiftUp] = buffer;

	GetPrivateProfileStringA("CONTROLS", "ShiftDown", "X", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::ShiftDown] = buffer;

	GetPrivateProfileStringA("CONTROLS", "Clutch", "LeftThumbDown", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::Clutch] = buffer;

	GetPrivateProfileStringA("CONTROLS", "Engine", "DpadDown", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::Engine] = buffer;

	GetPrivateProfileStringA("CONTROLS", "CThrottle", "RightTrigger", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::CThrottle] = buffer;

	GetPrivateProfileStringA("CONTROLS", "CBrake", "LeftTrigger", buffer, (DWORD)24, "./Gears.ini");
	scriptControl->ControlXbox[ScriptControls::ControlType::CBrake] = buffer;

	scriptControl->CToggleTime = GetPrivateProfileInt(L"MAIN", L"CToggleTime", 500, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KToggle] = GetPrivateProfileInt(L"MAIN", L"KToggle", VK_OEM_5, L"./Gears.ini");

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

	scriptControl->Control[ScriptControls::ControlType::KThrottle] = GetPrivateProfileInt(L"CONTROLS", L"KThrottle", 0x57, L"./Gears.ini");
	scriptControl->Control[ScriptControls::ControlType::KBrake] = GetPrivateProfileInt(L"CONTROLS", L"KBrake", 0x53, L"./Gears.ini");
}

void ScriptSettings::Save() {
	WritePrivateProfileString(L"MAIN", L"DefaultEnable", (EnableManual ? L" 1" : L" 0"), L"./Gears.ini");
	WritePrivateProfileString(L"CONTROLS", L"EnableH", (Hshifter ? L" 1" : L" 0"), L"./Gears.ini");
}
