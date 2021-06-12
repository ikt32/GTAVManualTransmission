#pragma once

#include <dinput.h>
#include <array>
#include <vector>
#include <unordered_map>
#include <optional>

// https://stackoverflow.com/questions/24113864/what-is-the-right-way-to-use-a-guid-as-the-key-in-stdhash-map
namespace std {
    template<> struct hash<GUID> {
        size_t operator()(const GUID& guid) const noexcept {
            const std::uint64_t* p = reinterpret_cast<const std::uint64_t*>(&guid);
            std::hash<std::uint64_t> hash;
            return hash(p[0]) ^ hash(p[1]);
        }
    };
}

struct DirectInputDeviceInfo {
    DIDEVICEINSTANCE DeviceInstance;
    DIDEVCAPS DeviceCapabilities;
    LPDIRECTINPUTDEVICE8 Device;
    DIJOYSTATE2 Joystate;
};

const int MAX_RGBBUTTONS = 128;
const int AVGSAMPLES = 2;
const int POVDIRECTIONS = 8;

class WheelDirectInput {
public:
    enum DIAxis {
        lX,
        lY,
        lZ,
        lRx,
        lRy,
        lRz,
        rglSlider0,
        rglSlider1,
        UNKNOWN_AXIS,
        SIZEOF_DIAxis
    };

    enum POV {
        N = 3600,
        NE = 4500,
        E = 9000,
        SE = 13500,
        S = 18000,
        SW = 22500,
        W = 27000,
        NW = 31500,
        SIZEOF_POV,
    };

    inline static const std::array<POV, POVDIRECTIONS> POVDirections {
        N, NE, E, SE, S, SW, W, NW
    };

    const std::array<std::string, SIZEOF_DIAxis> DIAxisHelper {
        "lX",
        "lY",
        "lZ",
        "lRx",
        "lRy",
        "lRz",
        "rglSlider0",
        "rglSlider1",
        "UNKNOWN_AXIS"
    };

    WheelDirectInput();
    ~WheelDirectInput();

    std::optional<DirectInputDeviceInfo> GetDeviceInfo(GUID guid);
    const std::unordered_map<GUID, DirectInputDeviceInfo>& GetDevices();

    bool InitWheel();
    bool InitFFB(GUID guid, DIAxis ffAxis);
    void Acquire();

    // Should be called every update()
    void Update();

    bool IsConnected(GUID device);
    bool IsButtonPressed(int buttonType, GUID device);
    bool IsButtonJustPressed(int buttonType, GUID device);
    bool IsButtonJustReleased(int buttonType, GUID device);
    bool WasButtonHeldForMs(int buttonType, GUID device, int millis);
    void UpdateButtonChangeStates();

    void SetConstantForce(GUID device, DIAxis ffAxis, int force);
    void SetDamper(GUID device, DIAxis ffAxis, int force);
    void SetCollision(GUID device, DIAxis ffAxis, int force);

    DIAxis StringToAxis(std::string& axisString);

    int GetAxisValue(DIAxis axis, GUID device);
    float GetAxisSpeed(DIAxis axis, GUID device);
    void PlayLedsDInput(GUID guid, float currentRPM, float rpmFirstLedTurnsOn, float rpmRedLine);

private:
    bool initDirectInput();
    bool isDirectInputInitialized();
    bool enumerateDevices();
    static BOOL CALLBACK enumerateDevicesCallbackS(const DIDEVICEINSTANCE* instance, VOID* context);
    BOOL enumerateDevicesCallback(const DIDEVICEINSTANCE* instance, VOID* context);

    void updateAxisSpeed();
    void createConstantForceEffect(GUID device, DIAxis axis, DWORD rawAxis);
    void createDamperEffect(GUID device, DIAxis axis, DWORD rawAxis);
    void createCollisionEffect(GUID device, DIAxis axis, DWORD rawAxis);
    bool createEffects(GUID device, DIAxis ffAxis);
    int povDirectionToIndex(int povDirection);

    LPDIRECTINPUT lpDi = nullptr;

    struct FFBEffects {
        ~FFBEffects();
        bool Enabled = false;

        DIEFFECT ConstantForceEffect{};
        DICONSTANTFORCE ConstantForceParams{};
        LPDIRECTINPUTEFFECT ConstantForceEffectInterface = nullptr;

        DIEFFECT DamperEffect{};
        DICONDITION DamperParams{};
        LPDIRECTINPUTEFFECT DamperEffectInterface = nullptr;

        DIEFFECT CollisionEffect{};
        DIPERIODIC CollisionParams{};
        LPDIRECTINPUTEFFECT CollisionEffectInterface = nullptr;
    };

    std::unordered_map<GUID, std::array<__int64, MAX_RGBBUTTONS>> rgbPressTime   { 0 };
    std::unordered_map<GUID, std::array<__int64, MAX_RGBBUTTONS>> rgbReleaseTime { 0 };
    std::unordered_map<GUID, std::array<bool, MAX_RGBBUTTONS>>	rgbButtonCurr { 0 };
    std::unordered_map<GUID, std::array<bool, MAX_RGBBUTTONS>>	rgbButtonPrev { 0 };
    std::unordered_map<GUID, std::array<__int64, POVDIRECTIONS>>	povPressTime   { 0 };
    std::unordered_map<GUID, std::array<__int64, POVDIRECTIONS>>	povReleaseTime { 0 };
    std::unordered_map<GUID, std::array<bool, POVDIRECTIONS>>		povButtonCurr { 0 };
    std::unordered_map<GUID, std::array<bool, POVDIRECTIONS>>		povButtonPrev { 0 };

    std::unordered_map<GUID, std::array<int, SIZEOF_DIAxis>> prevPosition { 0 };
    std::unordered_map<GUID, std::array<__int64, SIZEOF_DIAxis>> prevTime { 0 };
    std::unordered_map<GUID, std::array<std::array<float, AVGSAMPLES>, SIZEOF_DIAxis>> samples { 0 };

    std::unordered_map<GUID, std::array<int, SIZEOF_DIAxis>> averageIndex { 0 };
    std::unordered_map<GUID, std::array<FFBEffects, SIZEOF_DIAxis>> ffbEffectInfo;

    LPDIRECTINPUT mDirectInput = nullptr;

    std::unordered_map<GUID, DirectInputDeviceInfo> mDirectInputDeviceInstances;
    std::unordered_map<GUID, DirectInputDeviceInfo> mDirectInputDeviceInstancesNew;
};

bool isSupportedDrivingDevice(DWORD dwDevType);
