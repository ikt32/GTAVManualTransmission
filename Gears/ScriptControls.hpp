#pragma once

#include "..\..\ScriptHookV_SDK\inc\natives.h"
#include <string>

class ScriptControls
{
public:
	enum class ControllerControlType {
		ShiftUp,
		ShiftDown,
		Clutch,
		CThrottle,
		CBrake,
		CToggle,
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
		KToggle,
		ToggleH,
		KShiftUp,
		KShiftDown,
		KClutch,
		KThrottle,
		KBrake,
		KEngine,
		SIZEOF_KeyboardControlType
	};

	// todo: Something about these enums and scopes
	enum class ControlType {
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
		ShiftUp,
		ShiftDown,
		Clutch,
		KToggle,
		ToggleH,
		KShiftUp,
		KShiftDown,
		KClutch,
		CThrottle,
		CBrake,
		KThrottle,
		KBrake,
		Engine,
		KEngine,
		CToggle,
		SIZEOF_ControlType
	};
	
	enum class LogiControlType {
		HR = 0,
		H1,
		H2,
		H3,
		H4,
		H5,
		H6,
		N,
		ShiftUp,
		ShiftDown,
		Clutch,
		Throttle,
		Brake,
		Engine,
		Toggle,
		SIZEOF_LogiControlType
	};
	

public:
	ScriptControls();
	~ScriptControls();
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, ControlType control);

	bool WasControlPressedForMs(int control, int ms);

	int Control[(int)ControlType::SIZEOF_ControlType];
	int LogiControl[(int)LogiControlType::SIZEOF_LogiControlType];

	int CToggleTime;
	bool ControlCurr[(int)ControlType::SIZEOF_ControlType];
	bool ControlPrev[(int)ControlType::SIZEOF_ControlType];

	float Ltvalf = 0.0f;
	float Rtvalf = 0.0f;
	float Clutchvalf = 0.0f;
	int Accelval;
	float Accelvalf = 0.0f;

	std::string ControlXbox[(int)ControlType::SIZEOF_ControlType];

private:
	long long pressTime;
	long long releaseTime;
};
