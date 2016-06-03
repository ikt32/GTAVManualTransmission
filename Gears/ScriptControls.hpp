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
		N,
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
		SIZEOF_LogiControlType
	};
	

public:
	ScriptControls();
	~ScriptControls();
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, KeyboardControlType control);

	int Control[(int)KeyboardControlType::SIZEOF_KeyboardControlType] = {};
	int LogiControl[(int)LogiControlType::SIZEOF_LogiControlType] = {};

	int CToggleTime = 0;
	bool ControlCurr[(int)KeyboardControlType::SIZEOF_KeyboardControlType] = {};
	bool ControlPrev[(int)KeyboardControlType::SIZEOF_KeyboardControlType] = {};

	float Ltvalf = 0.0f;
	float Rtvalf = 0.0f;
	// 1 = Pressed, 0 = Not pressed
	float Clutchvalf = 0.0f;
	int Accelval = 0;
	float Accelvalf = 0.0f;

	std::string ControlXbox[(int)ControllerControlType::SIZEOF_ControllerControlType] = {};

private:
	long long pressTime = 0;
	long long releaseTime = 0;
};
