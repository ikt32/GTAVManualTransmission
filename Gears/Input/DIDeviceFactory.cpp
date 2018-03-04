#include "DIDeviceFactory.h"
#include <wbemidl.h>
#include <oleauto.h>
#include <wchar.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x) \
   if(x != NULL)        \
   {                    \
      x->Release();     \
      x = NULL;         \
   }
#endif

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput)
{
    IWbemLocator*           pIWbemLocator = NULL;
    IEnumWbemClassObject*   pEnumDevices = NULL;
    IWbemClassObject*       pDevices[20] = { 0 };
    IWbemServices*          pIWbemServices = NULL;
    BSTR                    bstrNamespace = NULL;
    BSTR                    bstrDeviceID = NULL;
    BSTR                    bstrClassName = NULL;
    DWORD                   uReturned = 0;
    bool                    bIsXinputDevice = false;
    UINT                    iDevice = 0;
    VARIANT                 var;
    HRESULT                 hr;

    // CoInit if needed
    hr = CoInitialize(NULL);
    bool bCleanupCOM = SUCCEEDED(hr);

    // Create WMI
    hr = CoCreateInstance(__uuidof(WbemLocator),
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWbemLocator),
        (LPVOID*)&pIWbemLocator);
    if (FAILED(hr) || pIWbemLocator == NULL)
        goto LCleanup;

    bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2"); if (bstrNamespace == NULL) goto LCleanup;
    bstrClassName = SysAllocString(L"Win32_PNPEntity");   if (bstrClassName == NULL) goto LCleanup;
    bstrDeviceID = SysAllocString(L"DeviceID");          if (bstrDeviceID == NULL)  goto LCleanup;

    // Connect to WMI 
    hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0L,
        0L, NULL, NULL, &pIWbemServices);
    if (FAILED(hr) || pIWbemServices == NULL)
        goto LCleanup;

    // Switch security level to IMPERSONATE. 
    CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, NULL, &pEnumDevices);
    if (FAILED(hr) || pEnumDevices == NULL)
        goto LCleanup;

    // Loop over all devices
    for (;; )
    {
        // Get 20 at a time
        hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned);
        if (FAILED(hr))
            goto LCleanup;
        if (uReturned == 0)
            break;

        for (iDevice = 0; iDevice<uReturned; iDevice++)
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, NULL, NULL);
            if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
            {
                // Check if the device ID contains "IG_".  If it does, then it's an XInput device
                // This information can not be found from DirectInput 
                if (wcsstr(var.bstrVal, L"IG_"))
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr(var.bstrVal, L"VID_");
                    if (strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1)
                        dwVid = 0;
                    WCHAR* strPid = wcsstr(var.bstrVal, L"PID_");
                    if (strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1)
                        dwPid = 0;

                    // Compare the VID/PID to the DInput device
                    DWORD dwVidPid = MAKELONG(dwVid, dwPid);
                    if (dwVidPid == pGuidProductFromDirectInput->Data1)
                    {
                        bIsXinputDevice = true;
                        goto LCleanup;
                    }
                }
            }
            SAFE_RELEASE(pDevices[iDevice]);
        }
    }

LCleanup:
    if (bstrNamespace)
        SysFreeString(bstrNamespace);
    if (bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if (bstrClassName)
        SysFreeString(bstrClassName);
    for (iDevice = 0; iDevice<20; iDevice++)
        SAFE_RELEASE(pDevices[iDevice]);
    SAFE_RELEASE(pEnumDevices);
    SAFE_RELEASE(pIWbemLocator);
    SAFE_RELEASE(pIWbemServices);

    if (bCleanupCOM)
        CoUninitialize();

    return bIsXinputDevice;
}

DIDeviceFactory & DIDeviceFactory::Get() {
    static DIDeviceFactory instance;
    return instance;
}

DIDeviceFactory::DIDeviceFactory() : entry(nullptr),
maxEntry(0),
nEntry(0),
di(nullptr),
lpdf{ nullptr } { }

DIDeviceFactory::~DIDeviceFactory() {
    clear();
}

void DIDeviceFactory::clear() {
    if (entry) {
        delete[] entry;
        entry = nullptr;
    }
    maxEntry = 0;
    nEntry = 0;
    di = nullptr;
}

// All on https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.dideviceinstance(v=vs.85).aspx
// dwDevType = DI8DEVTYPE_JOYSTICK
// dwDevType = DI8DEVTYPE_DRIVING
void DIDeviceFactory::Enumerate(LPDIRECTINPUT di, DWORD dwDevType, LPCDIDATAFORMAT lpdf, DWORD dwFlags, int maxEntry) {
    clear();

    entry = new DIDevice[maxEntry];
    this->di = di;
    this->maxEntry = maxEntry;
    nEntry = 0;
    this->lpdf = lpdf;

    di->EnumDevices(dwDevType, DIEnumDevicesCallback_static, this, dwFlags);

    this->di = nullptr;
}

int DIDeviceFactory::GetEntryCount() const {
    return nEntry;
}

const DIDevice* DIDeviceFactory::GetEntry(int index) const {
    const DIDevice* e = nullptr;
    if (index >= 0 && index < nEntry) {
        e = &entry[index];
    }
    return e;
}

void DIDeviceFactory::Update() const {
    for (int iEntry = 0; iEntry < nEntry; ++iEntry) {
        DIDevice& e = entry[iEntry];
        LPDIRECTINPUTDEVICE8 d = e.diDevice;

        if (FAILED(d->Poll())) {
            HRESULT hr = d->Acquire();
            while (hr == DIERR_INPUTLOST) {
                hr = d->Acquire();
            }
        }
        else {
            d->GetDeviceState(sizeof(DIJOYSTATE2), &e.joystate);
        }
    }
}

BOOL DIDeviceFactory::DIEnumDevicesCallback_static(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
    return reinterpret_cast<DIDeviceFactory*>(pvRef)->DIEnumDevicesCallback(lpddi, pvRef);
}

BOOL DIDeviceFactory::DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
    //if (IsXInputDevice(&lpddi->guidProduct))
    //    return DIENUM_CONTINUE;

    if (nEntry < maxEntry) {
        DIDevice e = { 0 };

        memcpy(&e.diDeviceInstance, lpddi, sizeof(e.diDeviceInstance));
        e.diDevCaps.dwSize = sizeof(e.diDevCaps);

        LPDIRECTINPUTDEVICE8 did = nullptr;

        if (FAILED(di->CreateDevice(lpddi->guidInstance, reinterpret_cast<LPDIRECTINPUTDEVICE*>(&did), nullptr))) {
            return DIENUM_CONTINUE;
        }
        if (FAILED(did->SetDataFormat(lpdf))) {
            return DIENUM_CONTINUE;
        }
        // Second call to this crashes? (G920 + SHVDN)
        //if (FAILED(did->GetCapabilities(&e.diDevCaps))) {
        //    return DIENUM_CONTINUE;
        //}
        e.diDevice = did;
        entry[nEntry++] = e;
    }
    return DIENUM_CONTINUE;
}


