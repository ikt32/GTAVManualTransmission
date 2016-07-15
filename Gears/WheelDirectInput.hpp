/*
 * I probably stole half of this from some MS FFB example code
 */

#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "..\..\ScriptHookV_SDK\inc\types.h"
#include "ScriptSettings.hpp"

enum class InputType {
	Wheel,
	Throttle,
	Brake,
	Clutch
};

class WheelDirectInput {
public:

	/*	
	bool Update();
	void Steer(float steerVal);
	void PlayEffects(
		Vector3 &accelVals,
		ScriptSettings &settings);
	float GetValue(InputType type);
	int GetPov();
	BYTE Buttons[128];
	*/
private:
	LPDIRECTINPUT8          g_pDI = nullptr;
	LPDIRECTINPUTDEVICE8    g_pDevice = nullptr;
	LPDIRECTINPUTEFFECT     g_pEffect = nullptr;
	BOOL                    g_bActive = TRUE;
	DWORD                   g_dwNumForceFeedbackAxis = 0;
	INT                     g_nXForce;
	INT                     g_nYForce;
	DWORD                   g_dwLastEffectSet;
	BOOL CALLBACK			EnumFFDevicesCallback(const DIDEVICEINSTANCE* pInst,
							VOID* pContext);
};