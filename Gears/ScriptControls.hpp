#pragma once

#include "..\..\ScriptHookV_SDK\inc\natives.h"

class ScriptControls
{
public:
	enum ControlType {
		HR = 0,
		H1,
		H2,
		H3,
		H4,
		H5,
		H6,
		H7,
		H8,
		ShiftUp,
		ShiftDown,
		Clutch,
		Toggle,
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
		KEngageNeutral,
		SIZE_OF_ARRAY
	};

public:
	ScriptControls();
	~ScriptControls();
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(int key, ControlType control);

	bool WasControlPressedForMs(int control, int ms);

	int Control[SIZE_OF_ARRAY];
	int CToggleTime;
	bool ControlCurr[SIZE_OF_ARRAY];
	bool ControlPrev[SIZE_OF_ARRAY];

	float Ltvalf = 0.0f;
	float Rtvalf = 0.0f;
	float Clutchvalf = 0.0f;

	int Accelval;
	float Accelvalf = 0.0f;

private:
	long long pressTime;
	long long releaseTime;
};

