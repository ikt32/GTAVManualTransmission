/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

#include "DiJoyStick.h"
#include <array>
#include "../Util/Logger.hpp"
#include <vector>
#include <unordered_set> 

#define MAX_RGBBUTTONS 128
#define SAMPLES 4

class WheelDirectInput {
public:
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
	bool NoFeedback = false;

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

	WheelDirectInput();
	bool InitWheel(DIAxis ffAxis);
	const DiJoyStick::Entry *findEntryFromGUID(GUID guid);
	const DiJoyStick::Entry *findEntryFromGUID();
	bool InitFFB(GUID guid, DIAxis ffAxis);

	// Should be called every update()
	void UpdateState();

	bool IsConnected() const;
	bool IsButtonPressed(int btn);
	bool IsButtonJustPressed(int btn);
	bool IsButtonJustReleased(int btn);
	bool WasButtonHeldForMs(int btn, int millis);
	void UpdateButtonChangeStates();

	HRESULT SetConstantForce(int force) const;

	DIAxis StringToAxis(std::string& axisString);
	DIJOYSTATE2 JoyStates;

	int GetAxisValue(DIAxis axis, int device);
	float GetAxisSpeed(DIAxis axis, int device);

private:
	Logger logger;// (LOGFILE);
	DiJoyStick djs;


	LPDIRECTINPUT lpDi = nullptr;
	LPDIRECTINPUTEFFECT pCFEffect;
	LPDIRECTINPUTEFFECT pFREffect;
	bool CreateConstantForceEffect(const DiJoyStick::Entry *e, DIAxis ffAxis);
	std::array<__int64, MAX_RGBBUTTONS> rgbPressTime;
	std::array<__int64, MAX_RGBBUTTONS> rgbReleaseTime;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonCurr;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonPrev;

	// hooooo boi a big array I only use 8 values of
	std::array<__int64, SIZEOF_POV> povPressTime;
	std::array<__int64, SIZEOF_POV> povReleaseTime;
	std::array<bool, SIZEOF_POV> povButtonCurr;
	std::array<bool, SIZEOF_POV> povButtonPrev;

	int prevPosition = 0;
	long long prevTime = 0;
	std::array<float, SAMPLES> samples = {};
	int averageIndex = 0;
};
