/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

#include "DiJoyStick.h"
#include <array>

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
		UNKNOWN,
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
		"UNKNOWN"
	};

public:
	WheelDirectInput();
	bool InitWheel(std::string ffAxis);
	~WheelDirectInput();

	// Should be called every update()
	void UpdateState();

	bool IsConnected();

	bool IsButtonPressed(int btn);
	bool IsButtonJustPressed(int btn);
	bool IsButtonJustReleased(int btn);
	bool WasButtonHeldForMs(int btn, int millis);
	void UpdateButtonChangeStates();

	HRESULT SetConstantForce(int force) const;

	DIAxis StringToAxis(std::string axisString);
	DIJOYSTATE2 JoyState;

	int GetAxisValue(DIAxis axis);
	float GetAxisSpeed(DIAxis axis);

private:
	DiJoyStick djs;
	LPDIRECTINPUT lpDi = nullptr;

	LPDIRECTINPUTEFFECT pCFEffect;
	LPDIRECTINPUTEFFECT pFREffect;
	bool CreateConstantForceEffect(std::string axis);
	std::array<__int64, MAX_RGBBUTTONS> pressTime;
	std::array<__int64, MAX_RGBBUTTONS> releaseTime;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonCurr;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonPrev;

	int prevPosition = 0;
	long long prevTime = 0;
	float samples[SAMPLES] = { };
	int averageIndex = 0;


};
