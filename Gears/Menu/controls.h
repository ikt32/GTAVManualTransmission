#pragma once
#include "inc/enums.h"

const int eControlSize = 338;

class Settings;

class MenuControls
{
public:
	friend Settings;

	enum ControlType {
		MenuKey = 0,
		MenuUp,
		MenuDown,
		MenuLeft,
		MenuRight,
		MenuSelect,
		MenuCancel,
		SIZEOF_ControlType
	};

	MenuControls();
	~MenuControls();
	bool IsKeyPressed(ControlType control);
	bool IsKeyJustPressed(ControlType control);
	bool IsKeyJustReleased(ControlType control);
	bool IsKeyDownFor(ControlType control, int millis);
	void Update();
	bool IsControlDownFor(eControl control, int millis);
	static const int controlSize = SIZEOF_ControlType;
	int ControlKeys[controlSize];
private:

	bool controlCurr[controlSize];
	bool controlPrev[controlSize];

	unsigned long long pressTime[controlSize];
	unsigned long long releaseTime[controlSize];

	bool nControlCurr[eControlSize];
	bool nControlPrev[eControlSize];

	unsigned long long nPressTime[eControlSize];
	unsigned long long nReleaseTime[eControlSize];
};

