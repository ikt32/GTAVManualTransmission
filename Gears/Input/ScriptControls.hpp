#pragma once

#include "XboxController.hpp"
#include "WheelDirectInput.hpp"

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

	enum InputDevices {
		Keyboard = 0,
		Controller = 1,
		Wheel = 2
	};

	ScriptControls();
	~ScriptControls();
	void InitWheel(bool initffb);

	InputDevices GetLastInputDevice(InputDevices previousInput, bool enableWheel = true);
	void UpdateValues(InputDevices prevInput, bool ignoreClutch, bool justPeekingWheelKb);

	float BrakeVal = 0.0f;
	float ThrottleVal = 0.0f;
	float ClutchVal = 0.0f; 	// 1 = Pressed, 0 = Not pressed
	float ClutchValRaw = 0.0f;	// For readout purposes. ClutchVal is for gameplay purposes.
	float SteerVal = 0.5f;
	float HandbrakeVal = 0.0f;

	// Values are filled by ScriptSettings
	std::array<std::string, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXbox = {};
	
	std::array<int, static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)> KBControl = {};
	
	std::array<std::string, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxes = {};
	std::array<GUID, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxesGUIDs = {};

	std::array<int, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelButton = {};
	std::array<GUID, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelButtonGUIDs = {};

	std::array<int, MAX_RGBBUTTONS> WheelToKey = {};
	GUID WheelToKeyGUID = {};
	
	int CToggleTime    = 1000;
	int ThrottleUp	   = 0;
	int ThrottleDown	   = 0;
	int BrakeUp	   = 0;
	int BrakeDown	   = 0;
	int ClutchUp	   = 0;
	int ClutchDown	   = 0;
	int SteerLeft	   = 0;
	int SteerRight	   = 0;
	int HandbrakeUp   = 0;
	int HandbrakeDown   = 0;
	//bool ClutchDisable = false;
	int WButtonHeld = 1000;

	// Add more when desired
	bool ButtonJustPressed(ControllerControlType control);
	bool ButtonJustPressed(KeyboardControlType control);
	bool ButtonJustPressed(WheelControlType control);
	bool ButtonReleased(ControllerControlType control);
	bool ButtonReleased(WheelControlType control);
	bool ButtonHeld(WheelControlType control);
	// Held for specified milliseconds in .ini
	bool ButtonHeld(ControllerControlType control);
	bool ButtonIn(WheelControlType control);
	void CheckCustomButtons(bool justPeeking);
	void CheckGUIDs(const std::vector<_GUID> &guids);
	bool ButtonIn(ControllerControlType control);

	WheelDirectInput WheelControl;

	void SetXboxTrigger(float value);
	float GetXboxTrigger();
	InputDevices PrevInput;


	GUID SteerGUID;
	WheelAxisType SteerAxisType;

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
	int ConfTagKB2key(std::string confTag) {
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
	std::string ConfTagController2Value(std::string confTag) {
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

private:
	bool resolveCombinedPedals(int RawT);
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, KeyboardControlType control);

	long long pressTime = 0;
	long long releaseTime = 0;

	XboxController controller;
	WORD buttonState;

	
	bool KBControlCurr[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
	bool KBControlPrev[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
};

// GUID stuff
bool operator < (const GUID &guid1, const GUID &guid2);
std::string GUID2String(GUID guid);
