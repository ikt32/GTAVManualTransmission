#include "WheelDirectInput.hpp"


//-----------------------------------------------------------------------------
// Function prototypes 
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback(const DIDEVICEINSTANCE* pInst, VOID* pContext);
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext);
HRESULT InitDirectInput(HWND hDlg);
VOID FreeDirectInput();
HRESULT SetDeviceForcesXY();
HRESULT UpdateInputState(HWND hDlg);


//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }


LPDIRECTINPUT8          g_pDI = nullptr;
LPDIRECTINPUTDEVICE8    g_pDevice = nullptr;
LPDIRECTINPUTEFFECT     g_pEffect = nullptr;
BOOL                    g_bActive = TRUE;
DWORD                   g_dwNumForceFeedbackAxis = 0;
INT                     g_nXForce;
INT                     g_nYForce;
DWORD                   g_dwLastEffectSet; // Time of the previous force feedback effect set

//-----------------------------------------------------------------------------
// Name: InitDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
HRESULT InitDirectInput(HWND hDlg)
{
	DIPROPDWORD dipdw;
	HRESULT hr;

	// Register with the DirectInput subsystem and get a pointer
	// to a IDirectInput interface we can use.
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&g_pDI, nullptr)))
	{
		return hr;
	}

	// Look for a force feedback device we can use
	if (FAILED(hr = g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL,
		EnumFFDevicesCallback, nullptr,
		DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)))
	{
		return hr;
	}

	if (!g_pDevice)
	{
		return S_FALSE;
	}

	// Set the data format to "simple joystick" - a predefined data format. A
	// data format specifies which controls on a device we are interested in,
	// and how they should be reported.
	//
	// This tells DirectInput that we will be passing a DIJOYSTATE structure to
	// IDirectInputDevice8::GetDeviceState(). Even though we won't actually do
	// it in this sample. But setting the data format is important so that the
	// DIJOFS_* values work properly.
	if (FAILED(hr = g_pDevice->SetDataFormat(&c_dfDIJoystick)))
		return hr;

	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.
	// Exclusive access is required in order to perform force feedback.
	if (FAILED(hr = g_pDevice->SetCooperativeLevel(hDlg,
		DISCL_EXCLUSIVE |
		DISCL_FOREGROUND)))
	{
		return hr;
	}

	// Since we will be playing force feedback effects, we should disable the
	// auto-centering spring.
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = FALSE;

	if (FAILED(hr = g_pDevice->SetProperty(DIPROP_AUTOCENTER, &dipdw.diph)))
		return hr;

	// Enumerate and count the axes of the joystick 
	if (FAILED(hr = g_pDevice->EnumObjects(EnumAxesCallback,
		(VOID*)&g_dwNumForceFeedbackAxis, DIDFT_AXIS)))
		return hr;

	// This simple sample only supports one or two axis joysticks
	if (g_dwNumForceFeedbackAxis > 2)
		g_dwNumForceFeedbackAxis = 2;

	// This application needs only one effect: Applying raw forces.
	DWORD rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
	LONG rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf = { 0 };

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.dwDuration = INFINITE;
	eff.dwSamplePeriod = 0;
	eff.dwGain = DI_FFNOMINALMAX;
	eff.dwTriggerButton = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes = g_dwNumForceFeedbackAxis;
	eff.rgdwAxes = rgdwAxes;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	// Create the prepared effect
	if (FAILED(hr = g_pDevice->CreateEffect(GUID_ConstantForce,
		&eff, &g_pEffect, nullptr)))
	{
		return hr;
	}

	if (!g_pEffect)
		return E_FAIL;

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//       each force feedback enabled axis
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback(const DIDEVICEOBJECTINSTANCE* pdidoi,
	VOID* pContext)
{
	auto pdwNumForceFeedbackAxis = reinterpret_cast<DWORD*>(pContext);

	if ((pdidoi->dwFlags & DIDOI_FFACTUATOR) != 0)
		(*pdwNumForceFeedbackAxis)++;

	return DIENUM_CONTINUE;
}




//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//       one, create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback(const DIDEVICEINSTANCE* pInst,
	VOID* pContext)
{
	LPDIRECTINPUTDEVICE8 pDevice;
	HRESULT hr;

	// Obtain an interface to the enumerated force feedback device.
	hr = g_pDI->CreateDevice(pInst->guidInstance, &pDevice, nullptr);

	// If it failed, then we can't use this device for some
	// bizarre reason.  (Maybe the user unplugged it while we
	// were in the middle of enumerating it.)  So continue enumerating
	if (FAILED(hr))
		return DIENUM_CONTINUE;

	// We successfully created an IDirectInputDevice8.  So stop looking 
	// for another one.
	g_pDevice = pDevice;

	return DIENUM_STOP;
}




//-----------------------------------------------------------------------------
// Name: FreeDirectInput()
// Desc: Initialize the DirectInput variables.
//-----------------------------------------------------------------------------
VOID FreeDirectInput()
{
	// Unacquire the device one last time just in case 
	// the app tried to exit while the device is still acquired.
	if (g_pDevice)
		g_pDevice->Unacquire();

	// Release any DirectInput objects.
	SAFE_RELEASE(g_pEffect);
	SAFE_RELEASE(g_pDevice);
	SAFE_RELEASE(g_pDI);
}

//-----------------------------------------------------------------------------
// Name: SetDeviceForcesXY()
// Desc: Apply the X and Y forces to the effect we prepared.
//-----------------------------------------------------------------------------
HRESULT SetDeviceForcesXY()
{
	// Modifying an effect is basically the same as creating a new one, except
	// you need only specify the parameters you are modifying
	LONG rglDirection[2] = { 0, 0 };

	DICONSTANTFORCE cf;

	if (g_dwNumForceFeedbackAxis == 1)
	{
		// If only one force feedback axis, then apply only one direction and 
		// keep the direction at zero
		cf.lMagnitude = g_nXForce;
		rglDirection[0] = 0;
	}
	else
	{
		// If two force feedback axis, then apply magnitude from both directions 
		rglDirection[0] = g_nXForce;
		rglDirection[1] = g_nYForce;
		cf.lMagnitude = (DWORD)sqrt((double)g_nXForce * (double)g_nXForce +
			(double)g_nYForce * (double)g_nYForce);
	}

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.cAxes = g_dwNumForceFeedbackAxis;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	// Now set the new parameters and start the effect immediately.
	return g_pEffect->SetParameters(&eff, DIEP_DIRECTION |
		DIEP_TYPESPECIFICPARAMS |
		DIEP_START);
}
