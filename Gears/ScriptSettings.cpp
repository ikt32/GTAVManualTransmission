#include "ScriptSettings.hpp"

#include <string>
#include "Input/keyboard.h"
#include "Util/simpleini/SimpleIni.h"
#include "Input/ScriptControls.hpp"

// TODO: This Device<->GUID->Control thing is very broken pls fix

//ScriptSettings::ScriptSettings(): nDevices(0) {
//	// Defaults
//}

ScriptSettings::ScriptSettings(std::string general,
	                           std::string wheel,
	                           Logger &logger) : logger(logger),
                                                 nDevices(0),
                                                 settingsGeneralFile(general),
                                                 settingsWheelFile(wheel) {
	//settingsGeneralFile = general;
	//settingsWheelFile = wheel;
}

void ScriptSettings::Read(ScriptControls* scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA settingsGeneral;
	settingsGeneral.SetUnicode();
	settingsGeneral.LoadFile(settingsGeneralFile.c_str());

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
	ini.LoadFile(settingsGeneralFile.c_str());
	ini.SetBoolValue("OPTIONS", "Enable", EnableManual);
	ini.SetLongValue("OPTIONS", "ShiftMode", ShiftMode);
	ini.SaveFile(settingsGeneralFile.c_str());
}

void ScriptSettings::IsCorrectVersion() const {
	if (settings_general_version != CORRECTVGENERAL && settings_wheel_version != CORRECTVWHEEL)
		throw std::runtime_error("Wrong settings_general.ini and \nsettings_wheel.ini version");
	if (settings_general_version != CORRECTVGENERAL)
		throw std::runtime_error("Wrong settings_general.ini version");
	if (settings_wheel_version != CORRECTVWHEEL)
		throw std::runtime_error("Wrong settings_wheel.ini version");
}

std::vector<GUID> ScriptSettings::GetGuids() {
	return reggdGuids;
}

void ScriptSettings::parseSettingsWheel(ScriptControls *scriptControl) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...
	CSimpleIniA settingsWheel;
	settingsWheel.SetUnicode();
	settingsWheel.LoadFile(settingsWheelFile.c_str());

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
	int it = 0;
	reggdGuids.clear();
	while (true) {
		std::string currDevIndex = std::string("DEV") + std::to_string(it);
		std::string currGuidIndex = std::string("GUID") + std::to_string(it);

		std::string currDevice = settingsWheel.GetValue("INPUT_DEVICES", currDevIndex.c_str(), "");
		if (currDevice == "")
			break;
		std::string currGuid = settingsWheel.GetValue("INPUT_DEVICES", currGuidIndex.c_str(), "");
		if (currGuid == "")
			break;

		std::wstring clsidStr;
		clsidStr.assign(currGuid.begin(), currGuid.end());
		GUID guid;
		HRESULT hr = CLSIDFromString(clsidStr.c_str(), &guid);
		if (hr != NOERROR) {
			std::string errStr;
			switch (hr) {
				case CO_E_CLASSSTRING:
					errStr = "The class string was improperly formatted.";
					break;
				case REGDB_E_CLASSNOTREG:
					errStr = "The CLSID corresponding to the class string was not found in the registry.";
					break;
				case REGDB_E_READREGDB:
					errStr = "The registry could not be opened for reading.";
					break;
				default:
					errStr = "Something went terribly wrong.";
					break;
			}
			logger.Write("CLSIDFromString error: " + errStr);
		}
		reggdGuids.push_back(guid);		
		it++;
	}
	nDevices = it;

	// [TOGGLE_MOD]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("TOGGLE_MOD", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Toggle)] =
		settingsWheel.GetLongValue("TOGGLE_MOD", "BUTTON", -1);

	// [CHANGE_SHIFTMODE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ToggleH)] =
		settingsWheel.GetLongValue("CHANGE_SHIFTMODE", "BUTTON", -1);


	// [STEER]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("STEER", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Steer)] =
		settingsWheel.GetValue("STEER", "AXLE", "");
	scriptControl->SteerLeft = settingsWheel.GetLongValue("STEER", "MIN", -1);
	scriptControl->SteerRight = settingsWheel.GetLongValue("STEER", "MAX", -1);
	SteerAngleMax = settingsWheel.GetDoubleValue("STEER", "SteerAngleMax", 900.0);
	SteerAngleCar = settingsWheel.GetDoubleValue("STEER", "SteerAngleCar", 720.0);
	SteerAngleBike = settingsWheel.GetDoubleValue("STEER", "SteerAngleBike", 180.0);
	SteerAngleAlt = settingsWheel.GetDoubleValue("STEER", "SteerAngleAlt", 180.0);

	// [THROTTLE]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("THROTTLE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Throttle)] =
		settingsWheel.GetValue("THROTTLE", "AXLE", "");
	scriptControl->ThrottleUp = settingsWheel.GetLongValue("THROTTLE", "MIN", -1);
	scriptControl->ThrottleDown = settingsWheel.GetLongValue("THROTTLE", "MAX", -1);

	// [BRAKES]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("BRAKES", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Brake)] =
		settingsWheel.GetValue("BRAKES", "AXLE", "");
	scriptControl->BrakeUp = settingsWheel.GetLongValue("BRAKES", "MIN", -1);
	scriptControl->BrakeDown = settingsWheel.GetLongValue("BRAKES", "MAX", -1);

	// [CLUTCH]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CLUTCH", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Clutch)] =
		settingsWheel.GetValue("CLUTCH", "AXLE", "");
	scriptControl->ClutchUp = settingsWheel.GetLongValue("CLUTCH", "MIN", -1);
	scriptControl->ClutchDown = settingsWheel.GetLongValue("CLUTCH", "MAX", -1);

	// [HANDBRAKE_ANALOG]
	scriptControl->WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "DEVICE", -1), reggdGuids);
	scriptControl->WheelAxes[static_cast<int>(ScriptControls::WheelAxisType::Handbrake)] =
		settingsWheel.GetValue("HANDBRAKE_ANALOG", "AXLE", "");
	scriptControl->HandbrakeDown = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MIN", -1);
	scriptControl->HandbrakeUp = settingsWheel.GetLongValue("HANDBRAKE_ANALOG", "MAX", -1);

	// [SHIFTER]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H1)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H2)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H3)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H4)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H5)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H6)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::H7)] =
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::HR)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFTER", "DEVICE", -1), reggdGuids);
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
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_UP", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftUp)] =
		settingsWheel.GetLongValue("SHIFT_UP", "BUTTON", -1);

	// [SHIFT_DOWN]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("SHIFT_DOWN", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::ShiftDown)] =
		settingsWheel.GetLongValue("SHIFT_DOWN", "BUTTON", -1);

	// [HANDBRAKE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HANDBRAKE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Handbrake)] =
		settingsWheel.GetLongValue("HANDBRAKE", "BUTTON", -1);

	// [ENGINE]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("ENGINE", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Engine)] =
		settingsWheel.GetLongValue("ENGINE", "BUTTON", -1);

	// [LIGHTS]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LIGHTS", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Lights)] =
		settingsWheel.GetLongValue("LIGHTS", "BUTTON", -1);

	// [HORN]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("HORN", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Horn)] =
		settingsWheel.GetLongValue("HORN", "BUTTON", -1);

	// [LOOK_BACK]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("LOOK_BACK", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::LookBack)] =
		settingsWheel.GetLongValue("LOOK_BACK", "BUTTON", -1);

	// [CHANGE_CAMERA]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("CHANGE_CAMERA", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::Camera)] =
		settingsWheel.GetLongValue("CHANGE_CAMERA", "BUTTON", -1);

	// [RADIO_NEXT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_NEXT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioNext)] =
		settingsWheel.GetLongValue("RADIO_NEXT", "BUTTON", -1);

	// [RADIO_PREVIOUS]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("RADIO_PREVIOUS", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::RadioPrev)] =
		settingsWheel.GetLongValue("RADIO_PREVIOUS", "BUTTON", -1);

	// [INDICATOR_LEFT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_LEFT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorLeft)] =
		settingsWheel.GetLongValue("INDICATOR_LEFT", "BUTTON", -1);

	// [INDICATOR_RIGHT]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_RIGHT", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorRight)] =
		settingsWheel.GetLongValue("INDICATOR_RIGHT", "BUTTON", -1);

	// [INDICATOR_HAZARD]
	scriptControl->WheelButtonGUIDs[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		DeviceIndexToGUID(settingsWheel.GetLongValue("INDICATOR_HAZARD", "DEVICE", -1), reggdGuids);
	scriptControl->WheelButton[static_cast<int>(ScriptControls::WheelControlType::IndicatorHazard)] =
		settingsWheel.GetLongValue("INDICATOR_HAZARD", "BUTTON", -1);
	
	// [TO_KEYBOARD]
	scriptControl->WheelToKeyGUID = 
		DeviceIndexToGUID(settingsWheel.GetLongValue("TO_KEYBOARD", "DEVICE", -1), reggdGuids);
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
	settings_wheel_version = settingsWheel.GetLongValue("FILEVERSION", "VERSION", 0);
#pragma warning(pop)

}

GUID ScriptSettings::DeviceIndexToGUID(int device, std::vector<GUID> guids) {
	if (device < 0) {
		return{};
	}
	if (device > nDevices - 1) {
		return{};
	}
	return guids[device];
}
