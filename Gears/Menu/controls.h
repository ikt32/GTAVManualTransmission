#pragma once

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
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(ControlType control);
	static const int controlSize = SIZEOF_ControlType;
	int ControlKeys[controlSize];
private:
	bool controlCurr[controlSize];
	bool controlPrev[controlSize];
};

