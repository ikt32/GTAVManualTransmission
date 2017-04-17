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

	InputDevices GetLastInputDevice(InputDevices previous);
	void UpdateValues(InputDevices prevInput, bool ignoreClutch, bool justPeekingWheelKb);

	float BrakeVal = 0.0f;
	float ThrottleVal = 0.0f;
	// 1 = Pressed, 0 = Not pressed
	float ClutchVal = 0.0f;
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

	void SetXboxTrigger(int value);
	InputDevices PrevInput;


	GUID SteerGUID;
	WheelAxisType SteerAxisType;

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