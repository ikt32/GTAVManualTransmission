/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

#include "DiJoyStick.h"
#include <array>

#define MAX_RGBBUTTONS 128

class WheelInput {
public:
	WheelInput();
	~WheelInput();
	// Should be called every update()
	const DIJOYSTATE2* GetState();

	bool IsConnected() const;

	bool IsButtonPressed(int btn);
	bool IsButtonJustPressed(int btn);
	bool IsButtonJustReleased(int btn);
	bool WasButtonHeldForMs(int btn, int millis);
	void UpdateButtonChangeStates();

	HRESULT SetForce(int force);
private:
	DiJoyStick djs;
	LPDIRECTINPUT lpDi = 0;
	DIJOYSTATE2 joyState;

	LPDIRECTINPUTEFFECT g_pEffect;

	bool CreateEffect();
	std::array<__int64, MAX_RGBBUTTONS> pressTime;
	std::array<__int64, MAX_RGBBUTTONS> releaseTime;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonCurr;
	std::array<bool, MAX_RGBBUTTONS> rgbButtonPrev;
};
