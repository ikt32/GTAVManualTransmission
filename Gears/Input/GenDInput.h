#pragma once
#include <array>
#include <vector>
#include "DiJoyStick.h"

/*
 * Okay, so you're probably looking at this class and are trying to figure out
 * why the hell I copy-pasted 98% of the WheelDirectInput stuff. I'm not a 
 * smart person, so I put a bit too much wheel stuff into the initialization
 * of the WheelDirectInput class. This class COULD be seen as a more generic
 * version, so in some far future when I care enough (never) both these
 * classes could be "merged".
 * 
 * I really don't like this, but oh well :/
 */

class Axis {

};

class GenDInput {
public:
	static const int MAX_RGBBUTTONS = 128;
	static const int AVGSAMPLES = 2;
	static const int numAxes = 9;

	enum DIAxis {
		lX,
		lY,
		lZ,
		lRx,
		lRy,
		lRz,
		rglSlider0,
		rglSlider1,
		UNKNOWN_AXIS,
		SIZEOF_DIAxis
	};

	std::array<std::string, SIZEOF_DIAxis> DIAxisHelper{
		"lX",
		"lY",
		"lZ",
		"lRx",
		"lRy",
		"lRz",
		"rglSlider0",
		"rglSlider1",
		"UNKNOWN_AXIS"
	};

	enum POV {
		N = 3600,
		NE = 4500,
		E = 9000,
		SE = 13500,
		S = 18000,
		SW = 22500,
		W = 27000,
		NW = 31500,
		UNKNOWN_POV,
		SIZEOF_POV
	};

	GenDInput();
	~GenDInput();

	bool PreInit();

	bool InitDevice();

	const DiJoyStick::Entry *FindEntryFromGUID(GUID guid);

	void Update();

	bool IsConnected(GUID device);
	bool IsButtonPressed(GUID device, int button);
	bool IsButtonJustPressed(GUID device, int button);
	bool IsButtonJustReleased(GUID device, int button);
	bool WasButtonHeldForMs(GUID device, int button, int millis);
	void UpdateButtonChangeStates();

	DIAxis StringToAxis(std::string& axisString);

	int GetAxisValue(GUID device, DIAxis axis);
	float GetAxisSpeed(GUID device, DIAxis axis);

	std::vector<GUID> GetGUIDs();

private:
	DiJoyStick djs;
	LPDIRECTINPUT lpDi = nullptr;
	std::array<__int64, MAX_RGBBUTTONS> rgbPressTime;
	std::array<__int64, MAX_RGBBUTTONS> rgbReleaseTime;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonCurr;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonPrev;

	// hooooo boi a big array I only use 8 values of
	std::array<__int64, SIZEOF_POV> povPressTime;
	std::array<__int64, SIZEOF_POV> povReleaseTime;
	std::array<bool, SIZEOF_POV> povButtonCurr;
	std::array<bool, SIZEOF_POV> povButtonPrev;

	std::array<float, AVGSAMPLES> samples = {};
	std::vector<GUID> foundGuids;

};
