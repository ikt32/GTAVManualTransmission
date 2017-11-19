#include "WheelDirectInput.hpp"

#include <winerror.h>

#include <sstream>
#include <chrono>
#include <vector>

#include "../Util/TimeHelper.hpp"
#include "../Util/Logger.hpp"

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


WheelDirectInput::WheelDirectInput() { }

WheelDirectInput::~WheelDirectInput() {
    for (int i = 0; i < djs.getEntryCount(); i++) {
        auto device = djs.getEntry(i);
        if (device)
            device->diDevice->Unacquire();
    }
    
    SAFE_RELEASE(m_cfEffect);
    SAFE_RELEASE(m_dEffect);
    SAFE_RELEASE(m_colEffect);
    SAFE_RELEASE(lpDi);
}

bool WheelDirectInput::PreInit() {
    //logger.Write("WHEEL: Preparing DiJoystick");
    // Just set up to ensure djs can always be used.
    if (FAILED(DirectInput8Create(GetModuleHandle(nullptr),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        reinterpret_cast<void**>(&lpDi),
        nullptr))) {
        return false;
    }
    djs.enumerate(lpDi);
    return true;
}

bool WheelDirectInput::InitWheel() {
    logger.Write(INFO, "WHEEL: Initializing input devices"); 

    logger.Write(INFO, "WHEEL: Setting up DirectInput interface");
    if (!PreInit()) {
        logger.Write(ERROR, "WHEEL: Failed setting up DirectInput interface");
        return false;
    }

    logger.Write(INFO, "WHEEL: Found " + std::to_string(djs.getEntryCount()) + " device(s)");

    if (djs.getEntryCount() < 1) {
        logger.Write(INFO, "WHEEL: No devices detected");
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
    for (int i = 0; i < djs.getEntryCount(); i++) {
        auto device = djs.getEntry(i);
        std::wstring wDevName = device->diDeviceInstance.tszInstanceName;
        logger.Write(INFO, "WHEEL: Device: " + std::string(wDevName.begin(), wDevName.end()));

        GUID guid = device->diDeviceInstance.guidInstance;
        wchar_t szGuidW[40] = { 0 };
        StringFromGUID2(guid, szGuidW, 40);
        std::wstring wGuid = szGuidW;//std::wstring(szGuidW);
        logger.Write(INFO, "WHEEL: GUID:   " + std::string(wGuid.begin(), wGuid.end()));
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
    logger.Write(INFO, "WHEEL: Devices initialized");
    return true;
}

bool WheelDirectInput::InitFFB(GUID guid, DIAxis ffAxis) {
    logger.Write(INFO, "WHEEL: Init FFB device");

    auto e = FindEntryFromGUID(guid);
    
    if (!e) {
        logger.Write(WARN, "WHEEL: FFB device not found");
        return false;
    }

    e->diDevice->Unacquire();
    HRESULT hr;
    if (FAILED(hr = e->diDevice->SetCooperativeLevel(GetForegroundWindow(),
                                                     DISCL_EXCLUSIVE | 
                                                     DISCL_FOREGROUND))) {
        std::string hrStr = formatError(hr);
        logger.Write(ERROR, "WHEEL: Acquire FFB device error");
        logger.Write(ERROR, "WHEEL: HRESULT = " + hrStr);
        std::stringstream ss;
        ss << std::hex << hr;
        logger.Write(ERROR, "WHEEL: ERRCODE = " + ss.str());
        ss.str(std::string());
        ss << std::hex << GetForegroundWindow();
        logger.Write(ERROR, "WHEEL: HWND =    " + ss.str());
        return false;
    }
    logger.Write(INFO, "WHEEL: Init FFB effect on axis " + DIAxisHelper[ffAxis]);
    if (!createEffects(guid, ffAxis)) {
        logger.Write(ERROR, "WHEEL: Init FFB effect failed");
        hasForceFeedback[guid][ffAxis] = false;
        return false;
    } 
    logger.Write(INFO, "WHEEL: Init FFB effect success");
    hasForceFeedback[guid][ffAxis] = true;
    return true;
}

void WheelDirectInput::UpdateCenterSteering(GUID guid, DIAxis steerAxis) {
    djs.update(); // TODO: Figure out why this needs to be called TWICE
    djs.update(); // Otherwise the wheel keeps turning/value is not updated?
    prevTime[guid][steerAxis] = std::chrono::steady_clock::now().time_since_epoch().count(); // 1ns
    prevPosition[guid][steerAxis] = GetAxisValue(steerAxis, guid);
}																																						

/*
 * Return NULL when device isn't found
 */
const DiJoyStick::Entry *WheelDirectInput::FindEntryFromGUID(GUID guid) {
    if (djs.getEntryCount() > 0) {
        if (guid == GUID_NULL) {
            return nullptr;
        }

        for (int i = 0; i < djs.getEntryCount(); i++) {
            auto tempEntry = djs.getEntry(i);
            if (guid == tempEntry->diDeviceInstance.guidInstance) {
                return tempEntry;
            }
        }
    }
    return nullptr;
}

void WheelDirectInput::Update() {
    djs.update();
    updateAxisSpeed();
}

bool WheelDirectInput::IsConnected(GUID device) {
    auto e = FindEntryFromGUID(device);
    if (!e) {
        return false;
    }
    return true;
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
    if (e->joystate.rgbButtons[buttonType])
        return true;
    return false;
}

bool WheelDirectInput::IsButtonJustPressed(int buttonType, GUID device) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        povButtonCurr[device][povIndex] = IsButtonPressed(buttonType,device);

        // raising edge
        if (povButtonCurr[device][povIndex] && !povButtonPrev[device][povIndex]) {
            return true;
        }
        return false;
    }
    rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

    // raising edge
    if (rgbButtonCurr[device][buttonType] && !rgbButtonPrev[device][buttonType]) {
        return true;
    }
    return false;
}

bool WheelDirectInput::IsButtonJustReleased(int buttonType, GUID device) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        povButtonCurr[device][povIndex] = IsButtonPressed(buttonType,device);

        // falling edge
        if (!povButtonCurr[device][povIndex] && povButtonPrev[device][povIndex]) {
            return true;
        }
        return false;
    }
    rgbButtonCurr[device][buttonType] = IsButtonPressed(buttonType,device);

    // falling edge
    if (!rgbButtonCurr[device][buttonType] && rgbButtonPrev[device][buttonType]) {
        return true;
    }
    return false;
}

bool WheelDirectInput::WasButtonHeldForMs(int buttonType, GUID device, int millis) {
    if (buttonType >= MAX_RGBBUTTONS) { // POV
        int povIndex = povDirectionToIndex(buttonType);
        if (povIndex == -1) return false;
        if (IsButtonJustPressed(buttonType,device)) {
            povPressTime[device][povIndex] = milliseconds_now();
        }
        if (IsButtonJustReleased(buttonType,device)) {
            povReleaseTime[device][povIndex] = milliseconds_now();
        }

        if ((povReleaseTime[device][povIndex] - povPressTime[device][povIndex]) >= millis) {
            povPressTime[device][povIndex] = 0;
            povReleaseTime[device][povIndex] = 0;
            return true;
        }
        return false;
    }
    if (IsButtonJustPressed(buttonType,device)) {
        rgbPressTime[device][buttonType] = milliseconds_now();
    }
    if (IsButtonJustReleased(buttonType,device)) {
        rgbReleaseTime[device][buttonType] = milliseconds_now();
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
    logger.Write(FATAL, "Caught exception 0x%X", code);
    logger.Write(FATAL, "\tException address 0x%p", ex->ExceptionRecord->ExceptionAddress);
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
    diEffect.dwDuration = INFINITE;
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
    LONG rglDirection[1] = { 1 };

    m_damperParams.lDeadBand = 0;
    m_damperParams.lOffset = 0;
    m_damperParams.lPositiveCoefficient = 0;
    m_damperParams.lNegativeCoefficient = 0;
    m_damperParams.dwPositiveSaturation = DI_FFNOMINALMAX;
    m_damperParams.dwNegativeSaturation = DI_FFNOMINALMAX;

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

    diEffect.cAxes = numAxes;
    diEffect.dwStartDelay = 0;
    diEffect.lpEnvelope = nullptr;
    diEffect.cbTypeSpecificParams = diEffect.cAxes * sizeof(DICONDITION);
    diEffect.lpvTypeSpecificParams = &m_damperParams;
}

void WheelDirectInput::createCollisionEffect(DWORD axis, int numAxes, DIEFFECT &diEffect) {
    DWORD rgdwAxes[1] = { axis };
    LONG rglDirection[1] = { 1 };

    m_collisionParams.dwMagnitude = 0;
    m_collisionParams.dwPeriod = static_cast<DWORD>(0.100 * DI_SECONDS);
    m_collisionParams.dwPhase = 0;
    m_collisionParams.lOffset = 0;

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
void logCreateEffectException(const DiJoyStick::Entry *e) {
    logger.Write(FATAL, "CreateEffect (%s) caused an exception!", currentEffectAttempt.c_str());
    GUID guid = e->diDeviceInstance.guidInstance;
    wchar_t szGuidW[40] = { 0 };
    StringFromGUID2(guid, szGuidW, 40);
    char szGuid[40] = { 0 };
    size_t   i;
    wcstombs_s(&i, szGuid, szGuidW, 40);
    logger.Write(FATAL, "\tGUID: %s", szGuid);
}

/*
 * Deal with stl stuff in __try
 */
void logCreateEffectError(HRESULT hr, const char *which) {
    logger.Write(ERROR, "CreateEffect failed: %s (%s)", formatError(hr).c_str(), which);
}

bool WheelDirectInput::createEffects(GUID device, DIAxis ffAxis) {
    const int numEffects = 3;
    int createdEffects = 0;
    auto e = FindEntryFromGUID(device);
    
    if (!e)
        return false;

    HRESULT hr;

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
    int numAxes = 1;
    DIEFFECT cfEffect;
    createConstantForceEffect(axis, numAxes, cfEffect);

    DIEFFECT damperEffect;
    createDamperEffect(axis, numAxes, damperEffect);

    DIEFFECT collisionEffect;
    createCollisionEffect(axis, numAxes, collisionEffect);

    // Call to this crashes? (G920 + SHVDN)
    __try {
        currentEffectAttempt = "constant force";
        hr = e->diDevice->CreateEffect(GUID_ConstantForce, &cfEffect, &m_cfEffect, nullptr);
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
    
    if (createdEffects == 0) return false;

    return true;
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
    m_dEffect->Start(1, 0);
}

void WheelDirectInput::SetDamper(GUID device, DIAxis ffAxis, int force) {
    auto e = FindEntryFromGUID(device);
    if (!e || !m_dEffect || !hasForceFeedback[device][ffAxis])
        return;

    LONG rglDirection[1] = { 1 };

    m_damperParams.lPositiveCoefficient = force;
    m_damperParams.lNegativeCoefficient = force;

    DIEFFECT effect;
    ZeroMemory(&effect, sizeof(effect));
    effect.dwSize = sizeof(DIEFFECT);
    effect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    effect.cAxes = 1;
    effect.rglDirection = rglDirection;
    effect.lpEnvelope = nullptr;
    effect.cbTypeSpecificParams = effect.cAxes * sizeof(DICONDITION);
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

void WheelDirectInput::StopEffects() {
    if (m_cfEffect != nullptr) {
        m_cfEffect->Stop();
    }
    if (m_dEffect != nullptr) {
        m_dEffect->Stop();
    }
    if (m_colEffect != nullptr) {
        m_colEffect->Stop();
    }

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

void WheelDirectInput::PlayLedsDInput(GUID guid, const FLOAT currentRPM, const FLOAT rpmFirstLedTurnsOn, const FLOAT rpmRedLine) {
    auto e = FindEntryFromGUID(guid);

    if (!e)
        return;
    
    WheelData wheelData_;
    ZeroMemory(&wheelData_, sizeof(wheelData_));

    wheelData_.size = sizeof(WheelData);
    wheelData_.versionNbr = LEDS_VERSION_NUMBER;
    wheelData_.rpmData.currentRPM = currentRPM;
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