#pragma once

class Settings;

class Controls
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

	Controls();
	~Controls();
	bool IsKeyPressed(int key);
	bool IsKeyJustPressed(ControlType control);
	void GetButtonState(bool *a, bool *b, bool *up, bool *down, bool *l, bool *r);
private:
	static const int controlSize = SIZEOF_ControlType;
	int controlKeys[controlSize];
	bool controlCurr[controlSize];
	bool controlPrev[controlSize];
};

