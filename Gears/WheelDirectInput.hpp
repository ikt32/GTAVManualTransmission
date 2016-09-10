/*
 * Massive thanks to t-mat on GitHub for having a simple C++ DInput
 * application! https://gist.github.com/t-mat/1391291
 */

#pragma once

//#include "ScriptSettings.hpp"
#include "DiJoyStick.h"

class WheelInput {
public:
	WheelInput();
	~WheelInput();
	// Should be called every update()
	const DIJOYSTATE2* GetState();

	bool IsConnected() const;

	bool IsButtonPressed();
	bool IsButtonJustPressed();
	bool IsButtonJustReleased();
	bool WasButtonHeldForMs();
	void UpdateButtonChangeStates();

private:
	DiJoyStick djs;
	LPDIRECTINPUT lpDi = 0;

};
