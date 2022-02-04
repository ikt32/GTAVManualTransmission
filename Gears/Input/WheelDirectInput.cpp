#include "WheelDirectInput.hpp"
#include "DirectInputError.h"

#include "../Util/Logger.hpp"
#include "../Util/Strings.hpp"
#include "../Util/GUID.h"
#include "../Util/MathExt.h"

#ifdef _DEBUG
#include "../Dump.h"
#endif

#include <winerror.h>

#include <algorithm>
#include <chrono>
#include <vector>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

// 0 - 10000 (inclusive)
constexpr uint32_t outputLutSize = 10001;
HWND g_windowHandle;

WheelDirectInput::WheelDirectInput() {}

WheelDirectInput::~WheelDirectInput() {
    FreeDirectInput();
}

void WheelDirectInput::FreeDirectInput() {
    logger.Write(DEBUG, "[Wheel] Freeing DirectInput");

    // GTAV.exe does not close when this is called during unload,
    // when the effects' dwDuration is INFINITE. (Could be Fanatec-specific)
    // Workaround: Leave the mess around. DLL is successfully unloaded without
    // all this.
    return;

    // Here, have some unreachable code.
    for (auto& instance : mDirectInputDeviceInstances) {
        logger.Write(DEBUG, "[Wheel] Processing %s", GUID2String(instance.first).c_str());
        logger.Write(DEBUG, "[Wheel] Sending FFB Reset");
        instance.second.Device->SendForceFeedbackCommand(DISFFC_RESET);
        logger.Write(DEBUG, "[Wheel] Unacquire device");
        instance.second.Device->Unacquire();
    }

    logger.Write(DEBUG, "[Wheel] Releasing ConstantForceEffect");
    SAFE_RELEASE(ffbEffectInfo.ConstantForceEffectInterface);
    logger.Write(DEBUG, "[Wheel] Releasing DamperEffect");
    SAFE_RELEASE(ffbEffectInfo.DamperEffectInterface);
    logger.Write(DEBUG, "[Wheel] Releasing CollisionEffect");
    SAFE_RELEASE(ffbEffectInfo.CollisionEffectInterface);

    for (auto& instance : mDirectInputDeviceInstances) {
        logger.Write(DEBUG, "[Wheel] Releasing %s", GUID2String(instance.first).c_str());
        SAFE_RELEASE(instance.second.Device);
    }

    logger.Write(DEBUG, "[Wheel] Clearing mDirectInputDeviceInstances");
    mDirectInputDeviceInstances.clear();

    logger.Write(DEBUG, "[Wheel] Releasing mDirectInput");
    SAFE_RELEASE(mDirectInput);

    logger.Write(DEBUG, "[Wheel] Finished freeing DirectInput resources");
}

void WheelDirectInput::ClearLut() {
    mLut.clear();
}

void WheelDirectInput::AssignLut(const std::map<float, float>& rawLut) {
    ClearLut();

    if (rawLut.size() == 0) {
        logger.Write(WARN, "[Wheel] Empty LUT, skipping");
        return;
    }

    if (!Math::Near(rawLut.begin()->second, 0.0f, 0.01f)) {
        logger.Write(WARN, "[Wheel] LUT[0] is %f, should be 0.0, skipping", rawLut.begin()->second);
        return;
    }

    if (!Math::Near(rawLut.rbegin()->second, 1.0f, 0.01f)) {
        logger.Write(WARN, "[Wheel] LUT[back] is %f, should be 1.0, skipping", rawLut.begin()->second);
        return;
    }

    mLut = std::vector<int>(outputLutSize);

    for (const auto& [inputRaw, outputRaw] : rawLut) {
        uint32_t input = (int)(inputRaw * 10000.0f);
        int output = (int)(outputRaw * 10000.0f);
        mLut[input] = output;
    }

    uint32_t prevNonZeroIdx = 0;
    for (uint32_t i = 1; i < outputLutSize; ++i) {
        if (mLut[i] != 0) {
            prevNonZeroIdx = i;
        }
        if (mLut[i] == 0) {
            int prevVal = mLut[prevNonZeroIdx];

            // Find next non-zero value
            uint32_t nextIndex = i + 1;
            for (; nextIndex < outputLutSize; ++nextIndex) {
                if (mLut[nextIndex] != 0) {
                    break;
                }
            }
            int nextVal = mLut[nextIndex];
            mLut[i] = static_cast<int>(
                map(static_cast<float>(i),
                    static_cast<float>(prevNonZeroIdx),
                    static_cast<float>(nextIndex),
                    static_cast<float>(prevVal),
                    static_cast<float>(nextVal)));
        }
    }

    logger.Write(DEBUG, "[Wheel] Expanded LUT with %d values to %d values", rawLut.size(), outputLutSize);
}

DirectInputDeviceInfo* WheelDirectInput::GetDeviceInfo(GUID guid) {
    auto it = mDirectInputDeviceInstances.find(guid);

    if (it != mDirectInputDeviceInstances.end())
        return &it->second;
    return nullptr;
}

const std::unordered_map<GUID, DirectInputDeviceInfo>& WheelDirectInput::GetDevices() {
    return mDirectInputDeviceInstances;
}

bool WheelDirectInput::InitWheel() {
    logger.Write(INFO, "[Wheel] Initializing input devices"); 
    logger.Write(INFO, "[Wheel] Setting up DirectInput interface");

    if (!initDirectInput()) {
        return false;
    }

    enumerateDevices();
    logger.Write(INFO, "[Wheel] Found %d device(s)", mDirectInputDeviceInstances.size());

    if (mDirectInputDeviceInstances.size() < 1) {
        logger.Write(INFO, "[Wheel] No devices detected");
        return false;
    }

    rgbPressTime.clear();
    rgbReleaseTime.clear();
    rgbButtonCurr.clear();
    rgbButtonPrev.clear();
    povPressTime.clear();
    povReleaseTime.clear();
    povButtonCurr.clear();
    povButtonPrev.clear();

    for (const auto& [guid, device] : mDirectInputDeviceInstances) {
        std::string devName = device.DeviceInstance.tszInstanceName;
        
        logger.Write(INFO, "[Wheel]     Name:   %s", devName.c_str());
        logger.Write(INFO, "[Wheel]     GUID:   %s", GUID2String(guid).c_str());

        rgbPressTime.insert(	std::pair<GUID, std::array<__int64, MAX_RGBBUTTONS>>(guid, {}));
        rgbReleaseTime.insert(	std::pair<GUID, std::array<__int64, MAX_RGBBUTTONS>>(guid, {}));
        rgbButtonCurr.insert(	std::pair<GUID, std::array<bool,	MAX_RGBBUTTONS>>(guid, {}));
        rgbButtonPrev.insert(	std::pair<GUID, std::array<bool,	MAX_RGBBUTTONS>>(guid, {}));
        povPressTime.insert(	std::pair<GUID, std::array<__int64, POVDIRECTIONS>>(guid, {}));
        povReleaseTime.insert(	std::pair<GUID, std::array<__int64, POVDIRECTIONS>>(guid, {}));
        povButtonCurr.insert(	std::pair<GUID, std::array<bool, POVDIRECTIONS>>(guid, {}));
        povButtonPrev.insert(	std::pair<GUID, std::array<bool, POVDIRECTIONS>>(guid, {}));
    }
    logger.Write(INFO, "[Wheel] Devices initialized");
    return true;
}

bool WheelDirectInput::InitFFB(GUID guid, DIAxis ffAxis) {
    if (ffbDevice == guid && ffbAxis == ffAxis) {
        logger.Write(DEBUG, "[Wheel] FFB already initialized");
        logger.Write(INFO, "    GUID: %s", GUID2String(guid).c_str());

        if (ffAxis >= UNKNOWN_AXIS) {
            logger.Write(ERROR, "    But axis is unknown? Axis: %d", ffAxis);
            ffbEffectInfo = FFBEffects();
            return false;
        }

        logger.Write(INFO, "    Axis: %s", DIAxisHelper[ffAxis].c_str());
        return true;
    }
    else {
        ffbEffectInfo = FFBEffects();
        if (ffbDevice == GUID_NULL) {
            logger.Write(INFO, "[Wheel] Initializing force feedback");
        }
        else {
            logger.Write(INFO, "[Wheel] Reinitializing force feedback on another device");
        }
    }
    logger.Write(INFO, "    GUID: %s", GUID2String(guid).c_str());

    if (ffAxis >= UNKNOWN_AXIS) {
        logger.Write(ERROR, "[Wheel] Skipping FFB initialization (unknown axis: %d)", ffAxis);
        return false;
    }

    logger.Write(INFO, "    Axis: %s", DIAxisHelper[ffAxis].c_str());

    auto e = GetDeviceInfo(guid);
    
    if (!e) {
        logger.Write(WARN, "[Wheel] Device assigned to FFB not found (%s)", GUID2String(guid).c_str());
        return false;
    }

    logger.Write(INFO, "[Wheel] Initializing FFB effects");
    if (!createEffects(guid, ffAxis)) {
        logger.Write(ERROR, "[Wheel] FFB effect initialization failed");
        return false;
    }
    logger.Write(INFO, "[Wheel] Initializing force feedback success");

    ffbDevice = guid;
    ffbAxis = ffAxis;
    return true;
}

void WheelDirectInput::Acquire() {
    std::string guidcppStr = GUID2String(ffbDevice);
    const char* guidStr = guidcppStr.c_str();
    if (ffbDevice == GUID_NULL) {
        logger.Write(DEBUG, "[Wheel] No FFB device registered to re-acquire");
        return;
    }
    else {
        logger.Write(DEBUG, "[Wheel] Re-acquiring FFB device %s", guidStr);
    }

    DirectInputDeviceInfo* e = GetDeviceInfo(ffbDevice);
    if (!e) {
        logger.Write(ERROR, "[Wheel] FFB device %s not found", guidStr);
        return;
    }

    HRESULT hr = e->Device->Unacquire();
    if (FAILED(hr)) {
        logger.Write(ERROR, "[Wheel] Unacquire failed with %x for %s", hr, guidStr);
    }
    else {
        logger.Write(DEBUG, "[Wheel] Unacquire success for %s", guidStr);
    }

    logger.Write(DEBUG, "[Wheel] Getting GetDeviceState before acquiring");
    
    hr = e->Device->GetDeviceState(sizeof(DIJOYSTATE2), &e->Joystate);
    if (FAILED(hr)) {
        logger.Write(DEBUG, "[Wheel] Failed GetDeviceState: %x %s", hr, GetDIError(hr).c_str());

        /*HRESULT */hr = e->Device->Acquire();
        if (hr == DI_OK) {
            logger.Write(DEBUG, "[Wheel] Re-acquired %s", guidStr);
        }
        else if (hr == S_FALSE) {
            logger.Write(DEBUG, "[Wheel] Re-acquired %s (already acquired)", guidStr);
        }
        else {
            logger.Write(ERROR, "[Wheel] Re-acquire failed with %x (%s) for %s", hr, GetDIError(hr).c_str(), guidStr);
        }

        if (ffbEffectInfo.CollisionEffectInterface)
            ffbEffectInfo.CollisionEffectInterface->Start(1, 0);

        if (ffbEffectInfo.DamperEffectInterface)
            ffbEffectInfo.DamperEffectInterface->Start(1, 0);

        if (ffbEffectInfo.ConstantForceEffectInterface)
            ffbEffectInfo.ConstantForceEffectInterface->Start(1, 0);
    }
    else {
        logger.Write(DEBUG, "[Wheel] No need to reacquire %s", guidStr);
    }
}

void WheelDirectInput::Update() {
    for (auto& [guid, device] : mDirectInputDeviceInstances) {
        if (FAILED(device.Device->Poll())) {
            HRESULT hr = device.Device->Acquire();
            while (hr == DIERR_INPUTLOST) {
                hr = device.Device->Acquire();
            }
        }
        else {
            device.Device->GetDeviceState(sizeof(DIJOYSTATE2), &device.Joystate);
        }
    }
    updateAxisSpeed();
}

bool WheelDirectInput::IsConnected(GUID device) {
    return GetDeviceInfo(device) != nullptr;
}

bool WheelDirectInput::IsButtonPressed(int buttonType, GUID device) {
    auto e = GetDeviceInfo(device);
    if (!e) {
        return false;
    }

    if (buttonType >= MAX_RGBBUTTONS) {
        switch (buttonType) {
            case N:
                if (e->Joystate.rgdwPOV[0] == 0) {
                    return true;
                }
            case NE:
            case E:
            case SE:
            case S:
            case SW:
            case W:
            case NW:
                if (buttonType == e->Joystate.rgdwPOV[0])
                    return true;
            default:
                return false;
        }
    }
    return e->Joystate.rgbButtons[buttonType] != 0;
}

bool WheelDirectInput::IsButtonJustPressed(int buttonType, GUID device) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        povButtonCurr[device][povIndex] = IsButtonPressed(buttonType,device);

        // raising edge
        return povButtonCurr[device][povIndex] && !povButtonPrev[device][povIndex];
    }
    rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

    // raising edge
    return rgbButtonCurr[device][buttonType] && !rgbButtonPrev[device][buttonType];
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType, GUID device) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        povButtonCurr[device][povIndex] = IsButtonPressed(buttonType,device);

        // falling edge
        return !povButtonCurr[device][povIndex] && povButtonPrev[device][povIndex];
    }
    rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

    // falling edge
    return !rgbButtonCurr[device][buttonType] && rgbButtonPrev[device][buttonType];
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, GUID device, int millis) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        if (IsButtonJustPressed(buttonType,device)) {
            povPressTime[device][povIndex] = GetTickCount64();
        }
        if (IsButtonJustReleased(buttonType,device)) {
            povReleaseTime[device][povIndex] = GetTickCount64();
        }

        if ((povReleaseTime[device][povIndex] - povPressTime[device][povIndex]) >= millis) {
            povPressTime[device][povIndex] = 0;
            povReleaseTime[device][povIndex] = 0;
            return true;
        }
        return false;
    }
    if (IsButtonJustPressed(buttonType,device)) {
        rgbPressTime[device][buttonType] = GetTickCount64();
    }
    if (IsButtonJustReleased(buttonType,device)) {
        rgbReleaseTime[device][buttonType] = GetTickCount64();
    }

    if ((rgbReleaseTime[device][buttonType] - rgbPressTime[device][buttonType]) >= millis) {
        rgbPressTime[device][buttonType] = 0;
        rgbReleaseTime[device][buttonType] = 0;
        return true;
    }
    return false;
}

void WheelDirectInput::UpdateButtonChangeStates() {
    for (const auto& [guid, device] : mDirectInputDeviceInstances) {
        for (int i = 0; i < MAX_RGBBUTTONS; i++) {
            rgbButtonPrev[guid][i] = rgbButtonCurr[guid][i];
        }
        for (auto pov : POVDirections) {
            int povIndex = povDirectionToIndex(pov);
            if (povIndex == -1) continue;
            povButtonPrev[guid][povIndex] = povButtonCurr[guid][povIndex];
        }
    }
}

int filterException(int code, PEXCEPTION_POINTERS ex) {
    logger.Write(FATAL, "[Wheel] Caught exception 0x%X", code);
    logger.Write(FATAL, "[Wheel]     Exception address 0x%p", ex->ExceptionRecord->ExceptionAddress);
#ifdef _DEBUG
    DumpStackTrace(ex);
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}

void WheelDirectInput::createConstantForceEffect(GUID device, DIAxis axis, DWORD rawAxis) {
    DWORD rgdwAxes[1] = { rawAxis };
    LONG rglDirection[1] = { 0 };

    ffbEffectInfo.ConstantForceParams.lMagnitude = 0;

    auto& diEffect = ffbEffectInfo.ConstantForceEffect;
    ZeroMemory(&diEffect, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwDuration = INFINITE;
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = 1;
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = 1 * sizeof(DICONSTANTFORCE);
    diEffect.lpvTypeSpecificParams = &ffbEffectInfo.ConstantForceParams;
    diEffect.dwStartDelay = 0;
}

void WheelDirectInput::createDamperEffect(GUID device, DIAxis axis, DWORD rawAxis) {
    DWORD rgdwAxes[1] = { rawAxis };
    LONG rglDirection[1] = { 0 };

    auto& damperParams = ffbEffectInfo.DamperParams;
    damperParams.lDeadBand = 0;
    damperParams.lOffset = 0;
    damperParams.lPositiveCoefficient = 0;
    damperParams.lNegativeCoefficient = 0;
    damperParams.dwPositiveSaturation = DI_FFNOMINALMAX;
    damperParams.dwNegativeSaturation = DI_FFNOMINALMAX;

    auto& diEffect = ffbEffectInfo.DamperEffect;
    ZeroMemory(&diEffect, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwDuration = INFINITE;
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = 1;
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = sizeof(DICONDITION);
    diEffect.lpvTypeSpecificParams = &ffbEffectInfo.DamperParams;
}

void WheelDirectInput::createCollisionEffect(GUID device, DIAxis axis, DWORD rawAxis) {
    DWORD rgdwAxes[1] = { rawAxis };
    LONG rglDirection[1] = { 0 };

    auto& collisionParams = ffbEffectInfo.CollisionParams;
    collisionParams.dwMagnitude = 0;
    collisionParams.dwPeriod = static_cast<DWORD>(0.100 * DI_SECONDS);
    collisionParams.dwPhase = 0;
    collisionParams.lOffset = 0;

    auto& diEffect = ffbEffectInfo.CollisionEffect;
    ZeroMemory(&diEffect, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = 1;
    diEffect.dwDuration = static_cast<ULONG>(200 * 1000);
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = ffbEffectInfo.CollisionEffect.cAxes * sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &ffbEffectInfo.CollisionParams;
}

/*
 * Deal with stl stuff in __try
 */
std::string currentEffectAttempt = "";
void logCreateEffectException(const DirectInputDeviceInfo& e) {
    logger.Write(FATAL, "[Wheel] CreateEffect (%s) caused an exception!", currentEffectAttempt.c_str());
    logger.Write(FATAL, "[Wheel]     GUID: %s", GUID2String(e.DeviceInstance.guidInstance).c_str());
}

/*
 * Deal with stl stuff in __try
 */
void logCreateEffectError(HRESULT hr, const char *which) {
    logger.Write(ERROR, "[Wheel] CreateEffect failed: [0x%08X] %s (%s)", hr, GetDIError(hr).c_str(), which);
}

bool WheelDirectInput::createEffects(GUID device, DIAxis ffAxis) {
    int createdEffects = 0;
    auto e = GetDeviceInfo(device);

    if (!e) {
        logger.Write(ERROR, "[Wheel] No force feedback device while creating effects");
        return false;
    }

    DWORD axisOffset;
    switch (ffAxis) {
        case lX : axisOffset = DIJOFS_X;  break;
        case lY : axisOffset = DIJOFS_Y;  break;
        case lZ : axisOffset = DIJOFS_Z;  break;
        case lRx: axisOffset = DIJOFS_RX; break;
        case lRy: axisOffset = DIJOFS_RY; break;
        case lRz: axisOffset = DIJOFS_RZ; break;
        default: {
            logger.Write(ERROR, "[Wheel] Invalix axis %d, bailing out", ffAxis);
            return false;
        }
    }

    createConstantForceEffect(device, ffAxis, axisOffset);
    logger.Write(DEBUG, "[Wheel] Created Constant Force Effect struct");

    createDamperEffect(device, ffAxis, axisOffset);
    logger.Write(DEBUG, "[Wheel] Created Damper Effect struct");

    createCollisionEffect(device, ffAxis, axisOffset);
    logger.Write(DEBUG, "[Wheel] Created Collision Effect struct");

    // Call to this crashes? (G920 + SHVDN)
    __try {
        currentEffectAttempt = "constant force";
        HRESULT hr = e->Device->CreateEffect(
            GUID_ConstantForce,
            &ffbEffectInfo.ConstantForceEffect,
            &ffbEffectInfo.ConstantForceEffectInterface,
            nullptr);

        if (FAILED(hr) || !ffbEffectInfo.ConstantForceEffectInterface) {
            logCreateEffectError(hr, "constant force");
        }
        else {
            createdEffects++;
            logger.Write(DEBUG, "[Wheel] Created Constant Force Effect on device");
        }

        currentEffectAttempt = "damper";
        hr = e->Device->CreateEffect(
            GUID_Friction,
            &ffbEffectInfo.DamperEffect,
            &ffbEffectInfo.DamperEffectInterface,
            nullptr);
        if (FAILED(hr) || !ffbEffectInfo.DamperEffectInterface) {
            logCreateEffectError(hr, "damper");
        }
        else {
            createdEffects++;
            logger.Write(DEBUG, "[Wheel] Created Damper Effect on device");
        }
        
        currentEffectAttempt = "collision";
        hr = e->Device->CreateEffect(
            GUID_Square,
            &ffbEffectInfo.CollisionEffect,
            &ffbEffectInfo.CollisionEffectInterface,
            nullptr);
        if (FAILED(hr) || !ffbEffectInfo.CollisionEffectInterface) {
            logCreateEffectError(hr, "collision");
        }
        else {
            createdEffects++;
            logger.Write(DEBUG, "[Wheel] Created Collision Effect on device");
        }

        currentEffectAttempt = "";
    }
    __except (filterException(GetExceptionCode(), GetExceptionInformation())) {
        logCreateEffectException(*e);
        currentEffectAttempt = "";
        return false;
    }

    return createdEffects != 0;
}

void WheelDirectInput::SetConstantForce(GUID device, DIAxis ffAxis, int force) {
    if (ffAxis >= UNKNOWN_AXIS ||
        !ffbEffectInfo.ConstantForceEffectInterface)
        return;

    force = std::clamp(force, -10000, 10000);

    if (mLut.size() == outputLutSize) {
        force = mLut[abs(force)] * sgn(force);
    }

    LONG rglDirection[1] = { 0 };

    ffbEffectInfo.ConstantForceParams.lMagnitude = force;

    auto& effect = ffbEffectInfo.ConstantForceEffect;
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = effect.cAxes * sizeof(DICONSTANTFORCE);
    effect.lpvTypeSpecificParams = &ffbEffectInfo.ConstantForceParams;
    effect.dwStartDelay = 0;

    ffbEffectInfo.ConstantForceEffectInterface->SetParameters(
        &ffbEffectInfo.ConstantForceEffect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
}

void WheelDirectInput::SetDamper(GUID device, DIAxis ffAxis, int force) {
    if (ffAxis >= UNKNOWN_AXIS ||
        !ffbEffectInfo.DamperEffectInterface)
        return;

    LONG rglDirection[1] = { 0 };

    ffbEffectInfo.DamperParams.lPositiveCoefficient = force;
    ffbEffectInfo.DamperParams.lNegativeCoefficient = force;

    auto& effect = ffbEffectInfo.DamperEffect;
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = sizeof(DICONDITION);
    effect.lpvTypeSpecificParams = &ffbEffectInfo.DamperParams;
    effect.dwStartDelay = 0;

    ffbEffectInfo.DamperEffectInterface->SetParameters(
        &ffbEffectInfo.DamperEffect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
}

void WheelDirectInput::SetCollision(GUID device, DIAxis ffAxis, int force) {
    if (ffAxis >= UNKNOWN_AXIS ||
        !ffbEffectInfo.CollisionEffectInterface)
        return;

    LONG rglDirection[1] = { 0 };

    ffbEffectInfo.CollisionParams.dwMagnitude = force;

    auto& effect = ffbEffectInfo.CollisionEffect;
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = effect.cAxes * sizeof(DIPERIODIC);
    effect.lpvTypeSpecificParams = &ffbEffectInfo.CollisionParams;
    effect.dwStartDelay = 0;

    ffbEffectInfo.CollisionEffectInterface->SetParameters(
        &ffbEffectInfo.CollisionEffect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
}

WheelDirectInput::DIAxis WheelDirectInput::StringToAxis(std::string &axisString) {
    for (int i = 0; i < SIZEOF_DIAxis; i++) {
        if (axisString == DIAxisHelper[i]) {
            return static_cast<DIAxis>(i);
        }
    }
    return UNKNOWN_AXIS;
}

// -1 means device not accessible
int WheelDirectInput::GetAxisValue(DIAxis axis, GUID device) {
    if (!IsConnected(device)) {
        return -1;
    }
    auto e = GetDeviceInfo(device);
    if (!e)
        return -1;
    switch (axis) {
        case lX: return  e->Joystate.lX;
        case lY: return  e->Joystate.lY;
        case lZ: return  e->Joystate.lZ;
        case lRx: return e->Joystate.lRx;
        case lRy: return e->Joystate.lRy;
        case lRz: return e->Joystate.lRz;
        case rglSlider0: return e->Joystate.rglSlider[0];
        case rglSlider1: return e->Joystate.rglSlider[1];
        default: return 0;
    }
}

bool WheelDirectInput::initDirectInput() {
    if (isDirectInputInitialized()) {
        logger.Write(DEBUG, "[Wheel] DirectInput interface already exists");
        return true;
    }
    
    // Create HWND for SetCooperativeMode for force feedback
    g_windowHandle = CreateWindowA("STATIC", "FFB Window", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

    HRESULT result = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
        IID_IDirectInput8, reinterpret_cast<void**>(&mDirectInput), nullptr);

    if (FAILED(result)) {
        logger.Write(ERROR, "[Wheel] Failed setting up DirectInput interface");
        return false;
    }

    return true;
}

bool WheelDirectInput::isDirectInputInitialized() {
    return mDirectInput != nullptr;
}

bool WheelDirectInput::enumerateDevices() {
    if (!isDirectInputInitialized()) {
        logger.Write(DEBUG, "[Wheel] DirectInput interface not ready for enumeration");
        return false;
    }
    const DWORD devType = DI8DEVCLASS_GAMECTRL;
    const DWORD flags = DIEDFL_ATTACHEDONLY;

    mDirectInputDeviceInstancesNew.clear();
    HRESULT result = mDirectInput->EnumDevices(devType, enumerateDevicesCallbackS,
        this, flags);

    if (FAILED(result)) {
        logger.Write(ERROR, "[Wheel] Failed to enumerate devices");
        return false;
    }

    std::erase_if(mDirectInputDeviceInstances, [&](const auto& instanceOld) {
        auto it = std::find_if(mDirectInputDeviceInstancesNew.begin(), mDirectInputDeviceInstancesNew.end(), [&](const auto& instanceNew) {
            return instanceNew.first == instanceOld.first;
            });

        if (it == mDirectInputDeviceInstancesNew.end()) {
            // TODO: De-init and release associated FFB?
            logger.Write(DEBUG, "[Wheel] Removing stale device with GUID %s",
                GUID2String(instanceOld.first).c_str());
        }

        return it == mDirectInputDeviceInstancesNew.end();
        });

    for (auto& instanceNew : mDirectInputDeviceInstancesNew) {
        auto existingInstanceIt = std::find_if(mDirectInputDeviceInstances.begin(), mDirectInputDeviceInstances.end(),
            [&](const auto& instanceOld) {
                return instanceNew.first == instanceOld.first;
            });

        if (existingInstanceIt == mDirectInputDeviceInstances.end()) {
            auto hr = mDirectInput->CreateDevice(
                instanceNew.first,
                reinterpret_cast<LPDIRECTINPUTDEVICE*>(&instanceNew.second.Device),
                nullptr);

            if (FAILED(hr)) {
                logger.Write(ERROR, "[Wheel] Failed to create device with GUID %s",
                    GUID2String(instanceNew.first).c_str());
                continue;
            }

            hr = instanceNew.second.Device->SetDataFormat(&c_dfDIJoystick2);

            if (FAILED(hr)) {
                logger.Write(ERROR, "[Wheel] Failed to set data format for device with GUID %s",
                    GUID2String(instanceNew.first).c_str());
                return false;
            }

            if (FAILED(hr = instanceNew.second.Device->SetCooperativeLevel(g_windowHandle,
                                                                            DISCL_EXCLUSIVE |
                                                                            DISCL_BACKGROUND))) {
                logger.Write(ERROR, "[Wheel] SetCooperativeLevel FFB device error");
                logger.Write(ERROR, "[Wheel]     HRESULT = %s", GetDIError(hr).c_str());
                logger.Write(ERROR, "[Wheel]     ERRCODE = %X", hr);
                logger.Write(ERROR, "[Wheel]     HWND =    %X", g_windowHandle);
                return false;
            }

            mDirectInputDeviceInstances.emplace(instanceNew);
            logger.Write(DEBUG, "[Wheel] Added new device with GUID %s",
                GUID2String(instanceNew.first).c_str());
        }
    }

    return true;
}

BOOL WheelDirectInput::enumerateDevicesCallbackS(const DIDEVICEINSTANCE* instance, VOID* context) {
    return reinterpret_cast<WheelDirectInput*>(context)->enumerateDevicesCallback(instance, context);
}

BOOL WheelDirectInput::enumerateDevicesCallback(const DIDEVICEINSTANCE* instance, VOID* context) {
    DirectInputDeviceInfo deviceInfo{};
    deviceInfo.DeviceInstance = *instance;
    deviceInfo.DeviceCapabilities.dwSize = sizeof(deviceInfo.DeviceCapabilities);
    mDirectInputDeviceInstancesNew.emplace(instance->guidInstance, deviceInfo);
    return DIENUM_CONTINUE;
}

void WheelDirectInput::updateAxisSpeed() {
    for (auto& [guid, device] : mDirectInputDeviceInstances) {
        for (int i = 0; i < SIZEOF_DIAxis; i++) {
            DIAxis axis = static_cast<DIAxis>(i);

            auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
            auto position = GetAxisValue(axis, guid);
            auto result = (position - prevPosition[guid][i]) / ((time - prevTime[guid][i]) / 1e9f);

            prevTime[guid][i] = time;
            prevPosition[guid][i] = position;

            samples[guid][i][averageIndex[guid][i]] = result;
            averageIndex[guid][i] = (averageIndex[guid][i] + 1) % (AVGSAMPLES - 1);
        }
    }
}

// Returns in units/s
float WheelDirectInput::GetAxisSpeed(DIAxis axis, GUID device) {
    auto sum = 0.0f;
    for (auto i = 0; i < AVGSAMPLES; i++) {
        sum += samples[device][axis][i];
    }
    return sum / AVGSAMPLES;
}

CONST DWORD ESCAPE_COMMAND_LEDS = 0;
CONST DWORD LEDS_VERSION_NUMBER = 0x00000001;

struct LedsRpmData {
    FLOAT currentRPM;
    FLOAT rpmFirstLedTurnsOn;
    FLOAT rpmRedLine;
};

struct WheelData {
    DWORD size;
    DWORD versionNbr;
    LedsRpmData rpmData;
};

void WheelDirectInput::PlayLedsDInput(GUID guid, float currentRPM, float rpmFirstLedTurnsOn, float rpmRedLine) {
    auto e = GetDeviceInfo(guid);

    if (!e)
        return;
    
    WheelData wheelData_{};
    ZeroMemory(&wheelData_, sizeof(wheelData_));

    float rpm = std::clamp(currentRPM, rpmFirstLedTurnsOn, 1.0f);

    wheelData_.size = sizeof(WheelData);
    wheelData_.versionNbr = LEDS_VERSION_NUMBER;
    wheelData_.rpmData.currentRPM = rpm;
    wheelData_.rpmData.rpmFirstLedTurnsOn = rpmFirstLedTurnsOn;
    wheelData_.rpmData.rpmRedLine = rpmRedLine;

    DIEFFESCAPE data_;
    ZeroMemory(&data_, sizeof(data_));

    data_.dwSize = sizeof(DIEFFESCAPE);
    data_.dwCommand = ESCAPE_COMMAND_LEDS;
    data_.lpvInBuffer = &wheelData_;
    data_.cbInBuffer = sizeof(wheelData_);

    e->Device->Escape(&data_);
}

int WheelDirectInput::povDirectionToIndex(int povDirection) {
    switch (povDirection) {
        case N:		return 0;
        case NE:	return 1;
        case E:		return 2;
        case SE:	return 3;
        case S:		return 4;
        case SW:	return 5;
        case W:		return 6;
        case NW:	return 7;
        default:
            return -1;
    }
}

bool operator < (const GUID &guid1, const GUID &guid2) {
    if (guid1.Data1 != guid2.Data1) {
        return guid1.Data1 < guid2.Data1;
    }
    if (guid1.Data2 != guid2.Data2) {
        return guid1.Data2 < guid2.Data2;
    }
    if (guid1.Data3 != guid2.Data3) {
        return guid1.Data3 < guid2.Data3;
    }
    for (int i = 0; i<8; i++) {
        if (guid1.Data4[i] != guid2.Data4[i]) {
            return guid1.Data4[i] < guid2.Data4[i];
        }
    }
    return false;
}

bool isSupportedDrivingDevice(DWORD dwDevType) {
    switch (GET_DIDEVICE_TYPE(dwDevType)) {
        case DI8DEVTYPE_DRIVING: // Wheels
        case DI8DEVTYPE_SUPPLEMENTAL: // Pedal sets, shifters, etc
        case DI8DEVTYPE_FLIGHT: // Yay HOTAS
            return true;
        default:
            return false;
    }
}
