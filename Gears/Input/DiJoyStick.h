/*
* Massive thanks to t-mat on GitHub for having a simple C++ DInput
* application! https://gist.github.com/t-mat/1391291
*/

#pragma once
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

class DiJoyStick {
public:
    struct Entry {
        DIDEVICEINSTANCE diDeviceInstance;
        DIDEVCAPS diDevCaps;
        LPDIRECTINPUTDEVICE8 diDevice;
        DIJOYSTATE2 joystate;
    };

    DiJoyStick() : entry(nullptr),
                   maxEntry(0),
                   nEntry(0),
                   di(nullptr),
                   lpdf{nullptr} {
    }

    ~DiJoyStick() {
        clear();
    }

    void clear() {
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
    void enumerate(
        LPDIRECTINPUT di,
        DWORD dwDevType = DI8DEVCLASS_GAMECTRL, //catch-all
        LPCDIDATAFORMAT lpdf = &c_dfDIJoystick2,
        DWORD dwFlags = DIEDFL_ATTACHEDONLY,
        int maxEntry = 16) {
        clear();

        entry = new Entry[maxEntry];
        this->di = di;
        this->maxEntry = maxEntry;
        nEntry = 0;
        this->lpdf = lpdf;

        di->EnumDevices(dwDevType, DIEnumDevicesCallback_static, this, dwFlags);

        this->di = nullptr;
    }

    int getEntryCount() const {
        return nEntry;
    }

    const Entry* getEntry(int index) const {
        const Entry* e = nullptr;
        if (index >= 0 && index < nEntry) {
            e = &entry[index];
        }
        return e;
    }

    void update() const {
        for (int iEntry = 0; iEntry < nEntry; ++iEntry) {
            Entry& e = entry[iEntry];
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

protected:
    static BOOL CALLBACK DIEnumDevicesCallback_static(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
        return reinterpret_cast<DiJoyStick*>(pvRef)->DIEnumDevicesCallback(lpddi, pvRef);
    }

    BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
        if (nEntry < maxEntry) {
            Entry e = {0};

            memcpy(&e.diDeviceInstance, lpddi, sizeof(e.diDeviceInstance));
            e.diDevCaps.dwSize = sizeof(e.diDevCaps);

            LPDIRECTINPUTDEVICE8 did = nullptr;

            if (SUCCEEDED(di->CreateDevice(lpddi->guidInstance, reinterpret_cast<LPDIRECTINPUTDEVICE*>(&did), nullptr))) {
                if (SUCCEEDED(did->SetDataFormat(lpdf))) {
                    if (SUCCEEDED(did->GetCapabilities(&e.diDevCaps))) {
                        e.diDevice = did;
                        entry[nEntry++] = e;
                    }
                }
            }
        }
        return DIENUM_CONTINUE;
    }

    //
    Entry* entry;
    int maxEntry;
    int nEntry;
    LPDIRECTINPUT di;
    LPCDIDATAFORMAT lpdf;
};
