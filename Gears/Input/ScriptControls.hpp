#pragma once
#include <string>

#include "XboxController.hpp"
#include "WheelDirectInput.hpp"
#include "GenDInput.h"
#include "LegacyController.h"

class ScriptControls {
public:
	enum class ControllerControlType {
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Toggle,
		ToggleH,
		Engine,
		SIZEOF_ControllerControlType
	};

	enum class LegacyControlType {
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Toggle,
		ToggleH,
		Engine,
		SIZEOF_LegacyControlType
	};

	enum class KeyboardControlType {
		HR = 0,
		H1,
		H2,
		H3,
		H4,
		H5,
		H6,
		H7,
		HN,
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Engine,
		Toggle,
		ToggleH, 
		SIZEOF_KeyboardControlType
	};

	enum class WheelControlType {
		HR = 0,
		H1,
		H2,
		H3,
		H4,
		H5,
		H6,
		H7,
		HN,
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Engine,
		Toggle,
		ToggleH,
		Handbrake,
		Horn,
		Lights,
		LookBack,
		LookLeft,
		LookRight,
		Camera,
		RadioPrev,
		RadioNext,
		IndicatorLeft,
		IndicatorRight,
		IndicatorHazard,
		SIZEOF_WheelControlType
	};

	enum class WheelAxisType {
		Throttle,
		Brake,
		Clutch,
		Steer,
		Handbrake,
		ForceFeedback,
		SIZEOF_WheelAxisType
	};

	enum class StickAxisType {
		Throttle,
		Brake,
		Pitch,
		Roll,
		RudderL,
		RudderR,
		SIZEOF_StickAxisType
	};

	enum InputDevices {
		Keyboard = 0,
		Controller = 1,
		Wheel = 2,
//		Stick = 3,
	};

	const std::vector<std::pair<std::string, int>> LegacyControlsMap = {
		{ "ControlFrontendDown",		187 },
		{ "ControlFrontendUp",			188 },
		{ "ControlFrontendLeft",		189 },
		{ "ControlFrontendRight",		190 },
		{ "ControlFrontendRdown",		191 },
		{ "ControlFrontendRup",			192 },
		{ "ControlFrontendRleft",		193 },
		{ "ControlFrontendRright",		194 },
		{ "ControlFrontendAxisX",		195 },
		{ "ControlFrontendAxisY",		196 },
		{ "ControlFrontendRightAxisX",	197 },
		{ "ControlFrontendRightAxisY",	198 },
		{ "ControlFrontendPause",		199 },
		{ "ControlFrontendAccept",		201 },
		{ "ControlFrontendCancel",		202 },
		{ "ControlFrontendX",			203 },
		{ "ControlFrontendY",			204 },
		{ "ControlFrontendLb",			205 },
		{ "ControlFrontendRb",			206 },
		{ "ControlFrontendLt",			207 },
		{ "ControlFrontendRt",			208 },
		{ "ControlFrontendLs",			209 },
		{ "ControlFrontendRs",			210 },
		{ "ControlFrontendDelete",		214 },
		{ "ControlFrontendSelect",		217 },
	};

	ScriptControls();
	~ScriptControls();

	void InitWheel(bool initffb);
	void UpdateValues(InputDevices prevInput, bool ignoreClutch, bool justPeekingWheelKb);
	InputDevices GetLastInputDevice(InputDevices previousInput, bool enableWheel = true);
	
	// Add more when desired
	bool ButtonJustPressed(KeyboardControlType control);
private:
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, KeyboardControlType control);


public:
	bool ButtonJustPressed(ControllerControlType control);
	bool ButtonReleased(ControllerControlType control);
	bool ButtonReleasedAfter(ControllerControlType control, int time);
	bool ButtonHeld(ControllerControlType control);
	bool ButtonHeldOver(ControllerControlType control, int millis);
	XboxController::TapState ButtonTapped(ControllerControlType control);
	bool ButtonIn(ControllerControlType control);
	void SetXboxTrigger(float value);
	float GetXboxTrigger();

	bool ButtonJustPressed(LegacyControlType control);
	bool ButtonReleased(LegacyControlType control);
	bool ButtonHeld(LegacyControlType control);
	bool ButtonHeldOver(LegacyControlType control, int millis);
	bool ButtonIn(LegacyControlType control);
	LegacyController::TapState ButtonTapped(LegacyControlType control);
	bool ButtonReleasedAfter(LegacyControlType control, int time);

	bool ButtonJustPressed(WheelControlType control);
	bool ButtonReleased(WheelControlType control);
	bool ButtonHeld(WheelControlType control);
	bool ButtonIn(WheelControlType control);
	void CheckCustomButtons(bool justPeeking);
	void CheckGUIDs(const std::vector<_GUID> &guids);

	InputDevices PrevInput;

	float ThrottleVal = 0.0f;
	float BrakeVal = 0.0f;
	float ClutchVal = 0.0f; 	// 1 = Pressed, 0 = Not pressed
	float ClutchValRaw = 0.0f;	// For readout purposes. ClutchVal is for gameplay purposes.
	float SteerVal = 0.5f;
	float HandbrakeVal = 0.0f;

	int CToggleTime = 1000;
	int ThrottleUp = 0;
	int ThrottleDown = 0;
	int BrakeUp = 0;
	int BrakeDown = 0;
	int ClutchUp = 0;
	int ClutchDown = 0;
	int SteerLeft = 0;
	int SteerRight = 0;
	int HandbrakeUp = 0;
	int HandbrakeDown = 0;
	int WButtonHeld = 1000;

	bool InvertSteer = false;
	bool InvertThrottle = false;
	bool InvertBrake = false;
	bool InvertClutch = false;

	float ADZThrottle = 0.25f;
	float ADZBrake = 0.25f;
	float ADZSteer = 0.25f;

	float PlaneThrottle;
	float PlaneBrake;
	float PlanePitch;
	float PlaneRoll;
	float PlaneRudderL;
	float PlaneRudderR;

	int PlaneThrottleMin;
	int PlaneThrottleMax;
	int PlaneBrakeMin;
	int PlaneBrakeMax;
	int PlanePitchMin;
	int PlanePitchMax;
	int PlaneRollMin;
	int PlaneRollMax;
	int PlaneRudderLMin;
	int PlaneRudderLMax;
	int PlaneRudderRMin;
	int PlaneRudderRMax;


	std::array<std::string, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXbox = {};
	std::array<int, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXboxBlocks = {};

	bool UseLegacyController = false;
	std::array<int, static_cast<int>(LegacyControlType::SIZEOF_LegacyControlType)> LegacyControls = {};
	std::array<int, static_cast<int>(LegacyControlType::SIZEOF_LegacyControlType)> ControlNativeBlocks = {};

	std::array<int, static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)> KBControl = {};
	
	std::array<std::string, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxes = {};
	std::array<GUID, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxesGUIDs = {};
	std::array<int, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelButton = {};
	std::array<GUID, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelButtonGUIDs = {};
	std::array<int, WheelDirectInput::MAX_RGBBUTTONS> WheelToKey = {};

	GUID SteerGUID;
	GUID WheelToKeyGUID = {};
	WheelDirectInput WheelControl;
	WheelAxisType SteerAxisType;

	std::array<std::string, static_cast<int>(StickAxisType::SIZEOF_StickAxisType)> StickAxes = {};
	std::array<GUID, static_cast<int>(StickAxisType::SIZEOF_StickAxisType)> StickAxesGUIDs = {};
	std::array<int, static_cast<int>(StickAxisType::SIZEOF_StickAxisType)> StickButton = {};
	std::array<GUID, static_cast<int>(StickAxisType::SIZEOF_StickAxisType)> StickButtonGUIDs = {};
	//GenDInput StickControl;

	// blergh
	XboxController *GetRawController() {
		return &controller;
	}
	// BLERGH
	WORD GetButtonState() const {
		return buttonState;
	}

	// BLEURGHGHHHGGGKKKKKGGGG
	// Uh, like, I abstracted the controls to numbers @ read and use those values
	// but now I want to show them again...
	int ConfTagKB2key(const std::string &confTag) {
		if (confTag == "Toggle") return KBControl[static_cast<int>(KeyboardControlType::Toggle)];
		if (confTag == "ToggleH") return KBControl[static_cast<int>(KeyboardControlType::ToggleH)];
		if (confTag == "ShiftUp") return KBControl[static_cast<int>(KeyboardControlType::ShiftUp)];
		if (confTag == "ShiftDown") return KBControl[static_cast<int>(KeyboardControlType::ShiftDown)];
		if (confTag == "Clutch") return KBControl[static_cast<int>(KeyboardControlType::Clutch)];
		if (confTag == "Engine") return KBControl[static_cast<int>(KeyboardControlType::Engine)];
		if (confTag == "Throttle") return KBControl[static_cast<int>(KeyboardControlType::Throttle)];
		if (confTag == "Brake") return KBControl[static_cast<int>(KeyboardControlType::Brake)];
		if (confTag == "HR") return KBControl[static_cast<int>(KeyboardControlType::HR)];
		if (confTag == "H1") return KBControl[static_cast<int>(KeyboardControlType::H1)];
		if (confTag == "H2") return KBControl[static_cast<int>(KeyboardControlType::H2)];
		if (confTag == "H3") return KBControl[static_cast<int>(KeyboardControlType::H3)];
		if (confTag == "H4") return KBControl[static_cast<int>(KeyboardControlType::H4)];
		if (confTag == "H5") return KBControl[static_cast<int>(KeyboardControlType::H5)];
		if (confTag == "H6") return KBControl[static_cast<int>(KeyboardControlType::H6)];
		if (confTag == "H7") return KBControl[static_cast<int>(KeyboardControlType::H7)];
		if (confTag == "HN") return KBControl[static_cast<int>(KeyboardControlType::HN)];

		return -1;
	}

	// Same sentiment applies here
	std::string ConfTagController2Value(const std::string &confTag) {
		if (confTag == "Toggle") return ControlXbox[static_cast<int>(ControllerControlType::Toggle)];
		if (confTag == "ToggleShift") return ControlXbox[static_cast<int>(ControllerControlType::ToggleH)];
		if (confTag == "ShiftUp") return ControlXbox[static_cast<int>(ControllerControlType::ShiftUp)];
		if (confTag == "ShiftDown") return ControlXbox[static_cast<int>(ControllerControlType::ShiftDown)];
		if (confTag == "Clutch") return ControlXbox[static_cast<int>(ControllerControlType::Clutch)];
		if (confTag == "Engine") return ControlXbox[static_cast<int>(ControllerControlType::Engine)];
		if (confTag == "Throttle") return ControlXbox[static_cast<int>(ControllerControlType::Throttle)];
		if (confTag == "Brake") return ControlXbox[static_cast<int>(ControllerControlType::Brake)];

		return "UNKNOWN";
	}

	int ConfTagLController2Value(const std::string &confTag) {
		if (confTag == "Toggle")		return LegacyControls[static_cast<int>(LegacyControlType::Toggle)];
		if (confTag == "ToggleShift")	return LegacyControls[static_cast<int>(LegacyControlType::ToggleH)];
		if (confTag == "ShiftUp")		return LegacyControls[static_cast<int>(LegacyControlType::ShiftUp)];
		if (confTag == "ShiftDown")		return LegacyControls[static_cast<int>(LegacyControlType::ShiftDown)];
		if (confTag == "Clutch")		return LegacyControls[static_cast<int>(LegacyControlType::Clutch)];
		if (confTag == "Engine")		return LegacyControls[static_cast<int>(LegacyControlType::Engine)];
		if (confTag == "Throttle")		return LegacyControls[static_cast<int>(LegacyControlType::Throttle)];
		if (confTag == "Brake")			return LegacyControls[static_cast<int>(LegacyControlType::Brake)];

		return -1;
	}

	std::string NativeControl2Text(int nativeControl) const {
		for (auto mapItem : LegacyControlsMap) {
			if (mapItem.second == nativeControl) {
				return mapItem.first;
			}
		}
		return std::to_string(nativeControl);
	}

	// Oh ikt, what the HELL are ya doing?
	int ConfTagWheel2Value(const std::string &confTag) {
		if (confTag == "TOGGLE_MOD"			) return WheelButton[static_cast<int>(WheelControlType::Toggle)];
		if (confTag == "CHANGE_SHIFTMODE"	) return WheelButton[static_cast<int>(WheelControlType::ToggleH)];
		if (confTag == "CLUTCH_BUTTON"		) return WheelButton[static_cast<int>(WheelControlType::Clutch)];
		if (confTag == "SHIFT_UP"			) return WheelButton[static_cast<int>(WheelControlType::ShiftUp)];
		if (confTag == "SHIFT_DOWN"			) return WheelButton[static_cast<int>(WheelControlType::ShiftDown)];
		if (confTag == "ENGINE"				) return WheelButton[static_cast<int>(WheelControlType::Engine)];
		if (confTag == "HANDBRAKE"			) return WheelButton[static_cast<int>(WheelControlType::Handbrake)];
		if (confTag == "HORN"				) return WheelButton[static_cast<int>(WheelControlType::Horn)];
		if (confTag == "LIGHTS"				) return WheelButton[static_cast<int>(WheelControlType::Lights)];
		if (confTag == "LOOK_BACK"			) return WheelButton[static_cast<int>(WheelControlType::LookBack)];
		if (confTag == "LOOK_LEFT"			) return WheelButton[static_cast<int>(WheelControlType::LookLeft)];
		if (confTag == "LOOK_RIGHT"			) return WheelButton[static_cast<int>(WheelControlType::LookRight)];
		if (confTag == "CHANGE_CAMERA"		) return WheelButton[static_cast<int>(WheelControlType::Camera)];
		if (confTag == "RADIO_NEXT"			) return WheelButton[static_cast<int>(WheelControlType::RadioNext)];
		if (confTag == "RADIO_PREVIOUS"		) return WheelButton[static_cast<int>(WheelControlType::RadioPrev)];
		if (confTag == "INDICATOR_LEFT"		) return WheelButton[static_cast<int>(WheelControlType::IndicatorLeft)];
		if (confTag == "INDICATOR_RIGHT"	) return WheelButton[static_cast<int>(WheelControlType::IndicatorRight)];
		if (confTag == "INDICATOR_HAZARD"	) return WheelButton[static_cast<int>(WheelControlType::IndicatorHazard)];

		return -1;
	}


private:
	long long pressTime = 0;
	long long releaseTime = 0;
	LegacyController lcontroller;
	XboxController controller;
	WORD buttonState;

	bool KBControlCurr[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
	bool KBControlPrev[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};

};

// GUID stuff
bool operator < (const GUID &guid1, const GUID &guid2);
std::string GUID2String(GUID guid);
