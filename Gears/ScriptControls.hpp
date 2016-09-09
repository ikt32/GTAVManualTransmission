#pragma once

#include "XboxController.hpp"

class ScriptControls {
public:
	enum class ControllerControlType {
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Toggle,
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

	enum class LogiControlType {
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
		SIZEOF_LogiControlType
	};

	enum InputDevices {
		Keyboard = 0,
		Controller = 1,
		Wheel = 2
	};

public:
	ScriptControls();
	~ScriptControls();

	InputDevices GetLastInputDevice(InputDevices previous);
	void UpdateValues(InputDevices prevInput);

	static bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, KeyboardControlType control);


	int Control[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
	int LogiControl[static_cast<int>(LogiControlType::SIZEOF_LogiControlType)] = {};

	int CToggleTime = 0;
	bool ControlCurr[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};
	bool ControlPrev[static_cast<int>(KeyboardControlType::SIZEOF_KeyboardControlType)] = {};


	float BrakeVal = 0.0f;
	float ThrottleVal = 0.0f;
	// 1 = Pressed, 0 = Not pressed
	float ClutchVal = 0.0f;

	// Perceived accelerator value
	int AccelValGTA = 0;

	// Perceived accelerator value, float
	float AccelValGTAf = 0.0f;

	// Array gets filled by ScriptSettings
	std::string ControlXbox[static_cast<int>(ControllerControlType::SIZEOF_ControllerControlType)] = {};


	// Ddd more when desired

	bool ButtonPressed(ControllerControlType control);

	bool ButtonPressed(KeyboardControlType control);

	bool ButtonHeld(ControllerControlType control);


private:
	long long pressTime = 0;
	long long releaseTime = 0;
	//InputDevices prevInput;

	XboxController controller;
	WORD buttonState;

};
