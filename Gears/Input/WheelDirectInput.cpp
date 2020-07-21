#include "WheelDirectInput.hpp"

#include "../Util/Logger.hpp"
#include "../Util/Strings.hpp"
#include "../Util/GUID.h"

#ifdef _DEBUG
#include "../Dump.h"
#endif

#include <winerror.h>

#include <chrono>
#include <vector>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

std::string formatError(HRESULT hr) {
    switch (hr) {
    case DI_OK:                     return "DI_OK";
    case DIERR_INVALIDPARAM:        return "DIERR_INVALIDPARAM";
    case DIERR_NOTINITIALIZED:      return "DIERR_NOTINITIALIZED";
    case DIERR_ALREADYINITIALIZED:  return "DIERR_ALREADYINITIALIZED";
    case DIERR_INPUTLOST:           return "DIERR_INPUTLOST";
    case DIERR_ACQUIRED:            return "DIERR_ACQUIRED";
    case DIERR_NOTACQUIRED:         return "DIERR_NOTACQUIRED";
    case E_HANDLE:                  return "E_HANDLE";
    case DIERR_DEVICEFULL:          return "DIERR_DEVICEFULL";
    case DIERR_DEVICENOTREG:        return "DIERR_DEVICENOTREG";
    default:                        return "UNKNOWN";
    }
}

WheelDirectInput::WheelDirectInput()
    : m_constantForceParams()
    , m_damperParams()
    , m_collisionParams() { }

WheelDirectInput::~WheelDirectInput() {
    for (int i = 0; i < DIDeviceFactory::Get().GetEntryCount(); i++) {
        auto device = DIDeviceFactory::Get().GetEntry(i);
        if (device) {
            HRESULT hr = device->diDevice->Unacquire();
            if (FAILED(hr)) {
                logger.Write(ERROR, "[Wheel] Failed unacquiring device: %s", 
                    formatError(hr).c_str());
                logger.Write(ERROR, "[Wheel]     GUID: %s",
                    GUID2String(device->diDeviceInstance.guidInstance).c_str());
            }
        }
    }
    
    SAFE_RELEASE(m_cfEffect);
    SAFE_RELEASE(m_dEffect);
    SAFE_RELEASE(m_colEffect);
    SAFE_RELEASE(lpDi);
}

bool WheelDirectInput::InitDI() {
    HRESULT result = DirectInput8Create(GetModuleHandle(nullptr),
                                        DIRECTINPUT_VERSION,
                                        IID_IDirectInput8,
                                        reinterpret_cast<void**>(&lpDi),
                                        nullptr);

    if (FAILED(result)) {
        logger.Write(ERROR, "[Wheel] Failed setting up DirectInput interface");
        return false;
    }
    DIDeviceFactory::Get().Enumerate(lpDi);
    return true;
}

bool WheelDirectInput::InitWheel() {
    logger.Write(INFO, "[Wheel] Initializing input devices"); 
    logger.Write(INFO, "[Wheel] Setting up DirectInput interface");
    if (!InitDI()) {
        return false;
    }

    logger.Write(INFO, "[Wheel] Found %d device(s)", DIDeviceFactory::Get().GetEntryCount());

    if (DIDeviceFactory::Get().GetEntryCount() < 1) {
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

    foundGuids.clear();
    for (int i = 0; i < DIDeviceFactory::Get().GetEntryCount(); i++) {
        auto device = DIDeviceFactory::Get().GetEntry(i);
        std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
        GUID guid = device->diDeviceInstance.guidInstance;
        
        logger.Write(INFO, "[Wheel]     Name:   %s", StrUtil::utf8_encode(wDevName).c_str());
        logger.Write(INFO, "[Wheel]     GUID:   %s", GUID2String(guid).c_str());
        foundGuids.push_back(guid);

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
    logger.Write(INFO, "[Wheel] Initializing force feedback");

    auto e = FindEntryFromGUID(guid);
    
    if (!e) {
        logger.Write(WARN, "[Wheel] FFB device not found");
        return false;
    }

    e->diDevice->Unacquire();
    HRESULT hr;
    if (FAILED(hr = e->diDevice->SetCooperativeLevel(GetForegroundWindow(),
                                                     DISCL_EXCLUSIVE | 
                                                     DISCL_FOREGROUND))) {
        logger.Write(ERROR, "[Wheel] Acquire FFB device error");
        logger.Write(ERROR, "[Wheel]     HRESULT = %s", formatError(hr).c_str());
        logger.Write(ERROR, "[Wheel]     ERRCODE = %X", hr);
        logger.Write(ERROR, "[Wheel]     HWND =    %X", GetForegroundWindow());
        return false;
    }
    logger.Write(INFO, "[Wheel] Initializing FFB effects (axis: %s)", DIAxisHelper[ffAxis].c_str());
    if (!createEffects(guid, ffAxis)) {
        logger.Write(ERROR, "[Wheel] Init FFB effect failed, disabling force feedback");
        hasForceFeedback[guid][ffAxis] = false;
        return false;
    } 
    logger.Write(INFO, "[Wheel] Initializing force feedback success");
    hasForceFeedback[guid][ffAxis] = true;
    return true;
}

void WheelDirectInput::UpdateCenterSteering(GUID guid, DIAxis steerAxis) {
    DIDeviceFactory::Get().Update(); // TODO: Figure out why this needs to be called TWICE
    DIDeviceFactory::Get().Update(); // Otherwise the wheel keeps turning/value is not updated?
    prevTime[guid][steerAxis] = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
    prevPosition[guid][steerAxis] = GetAxisValue(steerAxis, guid);
}																																						

/*
 * Return NULL when device isn't found
 */
const DIDevice *WheelDirectInput::FindEntryFromGUID(GUID guid) {
    if (DIDeviceFactory::Get().GetEntryCount() > 0) {
        if (guid == GUID_NULL) {
            return nullptr;
        }

        for (int i = 0; i < DIDeviceFactory::Get().GetEntryCount(); i++) {
            auto tempEntry = DIDeviceFactory::Get().GetEntry(i);
            if (guid == tempEntry->diDeviceInstance.guidInstance) {
                return tempEntry;
            }
        }
    }
    return nullptr;
}

void WheelDirectInput::Update() {
    DIDeviceFactory::Get().Update();
    updateAxisSpeed();
}

bool WheelDirectInput::IsConnected(GUID device) {
    auto e = FindEntryFromGUID(device);
    return e != nullptr;
}

bool WheelDirectInput::IsButtonPressed(int buttonType, GUID device) {
    auto e = FindEntryFromGUID(device);
    if (!e) {
        return false;
    }

    if (buttonType >= MAX_RGBBUTTONS) {
        switch (buttonType) {
            case N:
                if (e->joystate.rgdwPOV[0] == 0) {
                    return true;
                }
            case NE:
            case E:
            case SE:
            case S:
            case SW:
            case W:
            case NW:
                if (buttonType == e->joystate.rgdwPOV[0])
                    return true;
            default:
                return false;
        }
    }
    return e->joystate.rgbButtons[buttonType] != 0;
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
    for (auto device : foundGuids) {
        for (int i = 0; i < MAX_RGBBUTTONS; i++) {
            rgbButtonPrev[device][i] = rgbButtonCurr[device][i];
        }
        for (auto pov : POVDirections) {
            int povIndex = povDirectionToIndex(pov);
            if (povIndex == -1) continue;
            povButtonPrev[device][povIndex] = povButtonCurr[device][povIndex];
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

void WheelDirectInput::createConstantForceEffect(DWORD axis, int numAxes, DIEFFECT &diEffect) {
    DWORD rgdwAxes[1] = { axis };
    LONG rglDirection[1] = { 0 };
    m_constantForceParams.lMagnitude = 0;

    ZeroMemory(&diEffect, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwDuration = 50 * 1000; // 50ms
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = numAxes;
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = diEffect.cAxes * sizeof(DICONSTANTFORCE);
    diEffect.lpvTypeSpecificParams = &m_constantForceParams;
    diEffect.dwStartDelay = 0;
}

void WheelDirectInput::createDamperEffect(DWORD axis, int numAxes, DIEFFECT &diEffect) {
    DWORD rgdwAxes[1] = { axis };
    LONG rglDirection[1] = { 0 };

    m_damperParams.lDeadBand = 0;
    m_damperParams.lOffset = 0;
    m_damperParams.lPositiveCoefficient = 0;
    m_damperParams.lNegativeCoefficient = 0;
    m_damperParams.dwPositiveSaturation = DI_FFNOMINALMAX;
    m_damperParams.dwNegativeSaturation = DI_FFNOMINALMAX;

    ZeroMemory(&diEffect, sizeof(DIEFFECT));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwDuration = 50 * 1000; // 50ms
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = numAxes;
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = sizeof(DICONDITION);
    diEffect.lpvTypeSpecificParams = &m_damperParams;
}

void WheelDirectInput::createCollisionEffect(DWORD axis, int numAxes, DIEFFECT &diEffect) {
    DWORD rgdwAxes[1] = { axis };
    LONG rglDirection[1] = { 0 };

    m_collisionParams.dwMagnitude = 0;
    m_collisionParams.dwPeriod = static_cast<DWORD>(0.100 * DI_SECONDS);
    m_collisionParams.dwPhase = 0;
    m_collisionParams.lOffset = 0;

    ZeroMemory(&diEffect, sizeof(diEffect));
    diEffect.dwSize = sizeof(DIEFFECT);
    diEffect.rgdwAxes = rgdwAxes;
    diEffect.rglDirection = rglDirection;
    diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    diEffect.dwSamplePeriod = 0;
    diEffect.dwGain = DI_FFNOMINALMAX;
    diEffect.dwTriggerButton = DIEB_NOTRIGGER;
    diEffect.dwTriggerRepeatInterval = 0;

    diEffect.cAxes = numAxes;
    diEffect.dwDuration = static_cast<ULONG>(200 * 1000);
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = diEffect.cAxes * sizeof(DIPERIODIC);
    diEffect.lpvTypeSpecificParams = &m_collisionParams;
}

/*
 * Deal with stl stuff in __try
 */
std::string currentEffectAttempt = "";
void logCreateEffectException(const DIDevice *e) {
    logger.Write(FATAL, "[Wheel] CreateEffect (%s) caused an exception!", currentEffectAttempt.c_str());
    logger.Write(FATAL, "[Wheel]     GUID: %s", GUID2String(e->diDeviceInstance.guidInstance).c_str());
}

/*
 * Deal with stl stuff in __try
 */
void logCreateEffectError(HRESULT hr, const char *which) {
    logger.Write(ERROR, "[Wheel] CreateEffect [] failed: [0x%08X] %s (%s)", hr, formatError(hr).c_str(), which);
}

bool WheelDirectInput::createEffects(GUID device, DIAxis ffAxis) {
    int createdEffects = 0;
    auto e = FindEntryFromGUID(device);
    
    if (!e)
        return false;

    DWORD axis;
    if		(ffAxis == lX)	{ axis = DIJOFS_X; }
    else if (ffAxis == lY)	{ axis = DIJOFS_Y; }
    else if (ffAxis == lZ)	{ axis = DIJOFS_Z; }
    else if (ffAxis == lRx) { axis = DIJOFS_RX; }
    else if (ffAxis == lRy) { axis = DIJOFS_RY; }
    else if (ffAxis == lRz) { axis = DIJOFS_RZ; }
    else { return false; }

    // TODO: Make joystickable
    // I'm focusing on steering wheels, so you get one axis.
    const int numAxes = 1;
    DIEFFECT cfEffect;
    createConstantForceEffect(axis, numAxes, cfEffect);

    DIEFFECT damperEffect;
    createDamperEffect(axis, numAxes, damperEffect);

    DIEFFECT collisionEffect;
    createCollisionEffect(axis, numAxes, collisionEffect);

    // Call to this crashes? (G920 + SHVDN)
    __try {
        currentEffectAttempt = "constant force";
        HRESULT hr = e->diDevice->CreateEffect(GUID_ConstantForce, &cfEffect, &m_cfEffect, nullptr);
        if (FAILED(hr) || !m_cfEffect) {
            logCreateEffectError(hr, "constant force");
        }
        else {
            createdEffects++;
        }

        currentEffectAttempt = "damper";
        hr = e->diDevice->CreateEffect(GUID_Damper, &damperEffect, &m_dEffect, nullptr);
        if (FAILED(hr) || !m_dEffect) {
            logCreateEffectError(hr, "damper");
        }
        else {
            createdEffects++;
        }

        currentEffectAttempt = "collision";
        hr = e->diDevice->CreateEffect(GUID_Square, &collisionEffect, &m_colEffect, nullptr);
        if (FAILED(hr) || !m_colEffect) {
            logCreateEffectError(hr, "collision");
        }
        else {
            createdEffects++;
        }

        currentEffectAttempt = "";
    }
    __except (filterException(GetExceptionCode(), GetExceptionInformation())) {
        logCreateEffectException(e);
        currentEffectAttempt = "";
        return false;
    }

    return createdEffects != 0;
}

void WheelDirectInput::SetConstantForce(GUID device, DIAxis ffAxis, int force) {
    // As per Microsoft's DirectInput example:
    // Modifying an effect is basically the same as creating a new one, except
    // you need only specify the parameters you are modifying
    auto e = FindEntryFromGUID(device);
    if (!e || !m_cfEffect || !hasForceFeedback[device][ffAxis])
        return;

    LONG rglDirection[1] = { 0 };

    m_constantForceParams.lMagnitude = force;

    DIEFFECT effect;
    ZeroMemory(&effect, sizeof(effect));
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = effect.cAxes * sizeof(DICONSTANTFORCE);
    effect.lpvTypeSpecificParams = &m_constantForceParams;
    effect.dwStartDelay = 0;

    e->diDevice->Acquire();
    m_cfEffect->SetParameters(&effect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
    m_cfEffect->Start(1, 0);
}

void WheelDirectInput::SetDamper(GUID device, DIAxis ffAxis, int force) {
    auto e = FindEntryFromGUID(device);
    if (!e || !m_dEffect || !hasForceFeedback[device][ffAxis])
        return;

    LONG rglDirection[1] = { 0 };

    m_damperParams.lPositiveCoefficient = force;
    m_damperParams.lNegativeCoefficient = force;

    DIEFFECT effect;
    ZeroMemory(&effect, sizeof(DIEFFECT));
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = sizeof(DICONDITION);
    effect.lpvTypeSpecificParams = &m_damperParams;
    effect.dwStartDelay = 0;

    e->diDevice->Acquire();
    m_dEffect->SetParameters(&effect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
    m_dEffect->Start(1, 0);
}

void WheelDirectInput::SetCollision(GUID device, DIAxis ffAxis, int force) {
    auto e = FindEntryFromGUID(device);
    if (!e || !m_colEffect || !hasForceFeedback[device][ffAxis])
        return;

    LONG rglDirection[1] = { 0 };

    m_collisionParams.dwMagnitude = force;

    DIEFFECT effect;
    ZeroMemory(&effect, sizeof(effect));
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = effect.cAxes * sizeof(DIPERIODIC);
    effect.lpvTypeSpecificParams = &m_collisionParams;
    effect.dwStartDelay = 0;

    e->diDevice->Acquire();
    m_colEffect->SetParameters(&effect,
        DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
    m_colEffect->Start(1, 0);
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
    auto e = FindEntryFromGUID(device);
    if (!e)
        return -1;
    switch (axis) {
        case lX: return  e->joystate.lX;
        case lY: return  e->joystate.lY;
        case lZ: return  e->joystate.lZ;
        case lRx: return e->joystate.lRx;
        case lRy: return e->joystate.lRy;
        case lRz: return e->joystate.lRz;
        case rglSlider0: return e->joystate.rglSlider[0];
        case rglSlider1: return e->joystate.rglSlider[1];
        default: return 0;
    }
}

void WheelDirectInput::updateAxisSpeed() {
    for (GUID device : foundGuids) {
        for (int i = 0; i < SIZEOF_DIAxis; i++) {
            DIAxis axis = static_cast<DIAxis>(i);

            auto time = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
            auto position = GetAxisValue(axis, device);
            auto result = (position - prevPosition[device][i]) / ((time - prevTime[device][i]) / 1e9f);

            prevTime[device][i] = time;
            prevPosition[device][i] = position;

            samples[device][i][averageIndex[device][i]] = result;
            averageIndex[device][i] = (averageIndex[device][i] + 1) % (AVGSAMPLES - 1);
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

std::vector<GUID> WheelDirectInput::GetGuids() {
    return foundGuids;
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
    auto e = FindEntryFromGUID(guid);

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

    e->diDevice->Escape(&data_);
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
