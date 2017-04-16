/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

#include "DiJoyStick.h"
#include <array>
#include "../Util/Logger.hpp"
#include <vector>

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
	//bool NoFeedback = false;
	
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

	WheelDirectInput(Logger &logAlt);
	~WheelDirectInput();

	bool InitWheel();

	bool InitFFB(GUID guid, DIAxis ffAxis);
	void UpdateCenterSteering(GUID guid, DIAxis steerAxis);
	const DiJoyStick::Entry *FindEntryFromGUID(GUID guid);

	// Should be called every update()
	void Update();

	bool IsConnected(GUID device);
	bool IsButtonPressed(int btn, GUID device);
	bool IsButtonJustPressed(int btn, GUID device);
	bool IsButtonJustReleased(int btn, GUID device);
	bool WasButtonHeldForMs(int btn, GUID device, int millis);
	void UpdateButtonChangeStates();

	void SetConstantForce(GUID device, int force);

	DIAxis StringToAxis(std::string& axisString);

	int GetAxisValue(DIAxis axis, GUID device);
	float GetAxisSpeed(DIAxis axis, GUID device);
	std::vector<GUID> GetGuids();
	void PlayLedsDInput(GUID guid, const FLOAT currentRPM, const FLOAT rpmFirstLedTurnsOn, const FLOAT rpmRedLine);

private:
	Logger logger;

	bool createConstantForceEffect(const DiJoyStick::Entry *e, DIAxis ffAxis);
	void formatError(HRESULT hr, std::string &hrStr);

	DiJoyStick djs;
	LPDIRECTINPUT lpDi = nullptr;
	LPDIRECTINPUTEFFECT pCFEffect;
	LPDIRECTINPUTEFFECT pFREffect;
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
	std::vector<GUID> foundGuids;
	bool hasForceFeedback = false;
};
