/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

#include "DiJoyStick.h"
#include <array>

#define MAX_RGBBUTTONS 128

class WheelDirectInput {
public:
	WheelDirectInput();
	~WheelDirectInput();

	// Should be called every update()
	void UpdateState();

	bool IsConnected();

	bool IsButtonPressed(int btn);
	bool IsButtonJustPressed(int btn);
	bool IsButtonJustReleased(int btn);
	bool WasButtonHeldForMs(int btn, int millis);
	void UpdateButtonChangeStates();

	HRESULT SetForce(int force) const;

	DIJOYSTATE2 JoyState;

private:
	DiJoyStick djs;
	LPDIRECTINPUT lpDi = nullptr;

	LPDIRECTINPUTEFFECT g_pEffect;

	bool CreateEffect();
	std::array<__int64, MAX_RGBBUTTONS> pressTime;
	std::array<__int64, MAX_RGBBUTTONS> releaseTime;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonCurr;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonPrev;
};
