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
		H8,
		HN,
		Toggle,
		ToggleH,
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Engine,
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
		SIZEOF_WheelAxisType
	};

	enum InputDevices {
		Keyboard = 0,
		Controller = 1,
		Wheel = 2
	};

	ScriptControls();
	~ScriptControls();
	void InitWheel();

	InputDevices GetLastInputDevice(InputDevices previous);
	void UpdateValues(InputDevices prevInput, bool ignoreClutch);

	float BrakeVal = 0.0f;
	float ThrottleVal = 0.0f;
	// 1 = Pressed, 0 = Not pressed
	float ClutchVal = 0.0f;
	float SteerVal = 0.5f;

	// Perceived accelerator value
	int AccelValGTA = 0;

	// Perceived accelerator value, float
	float AccelValGTAf = 0.0f;

	// Values are filled by ScriptSettings
	std::array<std::string, static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)> ControlXbox = {};
	std::array<std::string, static_cast<int>(WheelAxisType::SIZEOF_WheelAxisType)> WheelAxes = {};
	std::array<int, static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)> KBControl = {};
	std::array<int, static_cast<int>(WheelControlType::SIZEOF_WheelControlType)> WheelControl = {};
	std::array<int, MAX_RGBBUTTONS> WheelToKey = {};
	std::string FFAxis;

	int CToggleTime = 1000;
	int ThrottleMin	   = 0;
	int ThrottleMax	   = 0;
	int BrakeMin	   = 0;
	int BrakeMax	   = 0;
	int ClutchMin	   = 0;
	int ClutchMax	   = 0;
	int SteerLeft	   = 0;
	int SteerRight	   = 0;
	bool ClutchDisable = false;

	// Add more when desired
	bool ButtonJustPressed(ControllerControlType control);
	bool ButtonJustPressed(KeyboardControlType control);
	bool ButtonJustPressed(WheelControlType control);
	bool ButtonReleased(ControllerControlType control);
	bool ButtonReleased(WheelControlType control);
	// Held for specified milliseconds in .ini
	bool ButtonHeld(ControllerControlType control);
	bool ButtonIn(WheelControlType control);
	void CheckCustomButtons();
	bool ButtonIn(ControllerControlType control);

	WheelDirectInput WheelDI;

	void SetXboxTrigger(int value);

private:
	long long pressTime = 0;
	long long releaseTime = 0;

	XboxController controller;
	WORD buttonState;

	static bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, KeyboardControlType control);

	bool KBControlCurr[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
	bool KBControlPrev[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
};
