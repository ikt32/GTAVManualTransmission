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

    DiJoyStick();
    ~DiJoyStick();
    void clear();
    void enumerate(LPDIRECTINPUT di,
                   DWORD dwDevType = DI8DEVCLASS_GAMECTRL,
                   LPCDIDATAFORMAT lpdf = &c_dfDIJoystick2,
                   DWORD dwFlags = DIEDFL_ATTACHEDONLY,
                   int maxEntry = 16);
    int getEntryCount() const;
    const Entry* getEntry(int index) const;
    void update() const;

protected:
    static BOOL CALLBACK DIEnumDevicesCallback_static(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
    BOOL DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);

    Entry* entry;
    int maxEntry;
    int nEntry;
    LPDIRECTINPUT di;
    LPCDIDATAFORMAT lpdf;
};
