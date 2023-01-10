#pragma once
#include <string>
#include <vector>

enum EShiftMode : int {
    Sequential = 0,
    HPattern = 1,
    Automatic = 2,
};

EShiftMode Next(EShiftMode mode);

template <typename T>
class Tracked {
    T mValue;
    T mInitialValue;
    bool mChanged = false;
public:
    Tracked(T val) : mValue(val), mInitialValue(val), mChanged(false) { }
    Tracked& operator=(T v) { mChanged = true; mValue = v; return *this; }
 
    T Value() const { return mValue; }

    operator T() const { return mValue; }
    operator T&() { if (mValue != mInitialValue) { mChanged = true; } return mValue; }

    bool operator==(const T& rhs) { return mValue == rhs; }
    bool operator!=(const T& rhs) { return !(mValue == rhs); }

    bool Changed() const { return mChanged || mValue != mInitialValue; }
    void Set(T val) { mValue = mInitialValue = val; mChanged = false; }
    void Reset() { mInitialValue = mValue; mChanged = false; }
};

class VehicleConfig {
public:
    VehicleConfig();
    void SetFiles(VehicleConfig* baseConfig, const std::string& file);

    void LoadSettings();
    void SaveSettings();
    void SaveSettings(VehicleConfig* baseConfig, const std::string& customPath);

    std::string Name;

    // ID
    std::vector<std::string> ModelNames;
    std::vector<std::string> Plates;
    std::string Description;

    // [MT_OPTIONS]
    struct {
        Tracked<EShiftMode> ShiftMode = EShiftMode::Sequential;
        Tracked<bool> ClutchCreep = false;
        Tracked<bool> ClutchShiftH = true;
        Tracked<bool> ClutchShiftS = false;
        struct {
            Tracked<bool> Enable = false;
            // m/s -> default 120 kph-ish
            Tracked<float> Speed = 33.3f;
        } SpeedLimiter;
    } MTOptions;

    // [MT_PARAMS]
    struct {
        Tracked<float> EngBrakePower = 0.0f;
        Tracked<float> EngBrakeThreshold = 0.75f;
        // Clutch _lift_ distance
        Tracked<float> ClutchThreshold = 0.15f;
        Tracked<float> StallingRPM = 0.09f;
        Tracked<float> StallingRate = 3.75f;
        Tracked<float> StallingSlip = 0.40f;
        Tracked<float> RPMDamage = 0.1f;
        Tracked<float> MisshiftDamage = 5.0f;
        Tracked<float> CreepIdleThrottle = 0.1f;
        Tracked<float> CreepIdleRPM = 0.1f;
    } MTParams;

    // [DRIVING_ASSISTS]
    struct {
        struct {
            Tracked<bool> Enable = false;
            // true: only applied to abs-less vehicles
            Tracked<bool> Filter = true;
            Tracked<bool> Flash = false;

            // tyre speed threshold, ratio with actual speed
            // Should be < 1.0
            Tracked<float> SlipMin = 0.9f;
            Tracked<float> SlipMax = 0.8f;
        } ABS;

        struct {
            Tracked<bool> Enable = false;
            // 0 Brake, 1 Throttle 
            Tracked<int> Mode = 1;
            // tyre speed threshold, ratio with actual speed
            // Should be > 1.0
            Tracked<float> SlipMin = 1.20f;
            Tracked<float> SlipMax = 1.40f;
            // Multiplier for fBrakeForce for how hard TCS may brake a wheel.
            Tracked<float> BrakeMult = 1.0f;
        } TCS;

        struct {
            Tracked<bool> Enable = false;

            Tracked<float> OverMin = 05.0f; // deg
            Tracked<float> OverMax = 15.0f; // deg

            Tracked<float> OverMinComp = 0.0f; // brake mult
            Tracked<float> OverMaxComp = 1.0f; // brake mult

            Tracked<float> UnderMin = 5.0f; // deg
            Tracked<float> UnderMax = 10.0f; // deg

            Tracked<float> UnderMinComp = 0.0f; // brake mult
            Tracked<float> UnderMaxComp = 1.0f; // brake mult

        } ESP;

        struct {
            Tracked<bool> Enable = false;
            Tracked<float> Viscosity = 10.0f;
        } LSD;

        struct {
            // Only active for cars with
            // fDriveBiasFront >= 0.1 &&
            // fDriveBiasFront <= 0.9 &&
            // fDriveBiasFront != 0.5
            Tracked<bool> Enable = false;

            // Bias when bias transfer is maximized
            Tracked<float> BiasAtMaxTransfer = 0.5f;

            // AWD::EAWDMode Mode = AWD::EAWDMode::Automatic;

            Tracked<bool> UseCustomBaseBias = false;
            Tracked<float> CustomBaseBias = 0.01f;
            Tracked<float> CustomMin = 0.0f;
            Tracked<float> CustomMax = 1.0f;

            Tracked<bool> UseTraction = false;
            Tracked<float> TractionLossMin = 1.05f; // "Strong" axle is  5% faster than "weak" axle 
            Tracked<float> TractionLossMax = 1.50f; // "Strong" axle is 50% faster than "weak" axle

            // Should only be used for RWD-biased cars
            Tracked<bool> UseOversteer = false;
            Tracked<float> OversteerMin = 5.0f; // degrees
            Tracked<float> OversteerMax = 15.0f; // degrees

            // Should only be used for FWD-biased cars
            Tracked<bool> UseUndersteer = false;
            Tracked<float> UndersteerMin = 5.0f; // degrees
            Tracked<float> UndersteerMax = 15.0f; // degrees

            // See AWD.h for currently supported flags
            Tracked<uint32_t> SpecialFlags = 0;
        } AWD;

        struct {
            Tracked<bool> Enable = false;
            Tracked<float> RPM = 0.625f;
            // tyre speed threshold, m/s
            Tracked<float> SlipMin = 1.0f;
            Tracked<float> SlipMax = 2.0f;
        } LaunchControl;

        struct {
            Tracked<bool> Enable = false;
            Tracked<float> Speed = 36.0f;           // m/s -> default 130 kph-ish
            Tracked<float> MaxAcceleration = 4.0f;  // m/s²

            Tracked<bool> Adaptive = false; 
            Tracked<float> MinFollowDistance = 5.0f; // m
            Tracked<float> MaxFollowDistance = 100.0f; // m

            Tracked<float> MinDistanceSpeedMult = 1.00f; // 
            Tracked<float> MaxDistanceSpeedMult = 2.00f; //

            Tracked<float> MinDeltaBrakeMult = 2.4f; // 
            Tracked<float> MaxDeltaBrakeMult = 1.2f; // 
        } CruiseControl;
    } DriveAssists;

    // [SHIFT_OPTIONS]
    struct {
        Tracked<bool> UpshiftCut = true;
        Tracked<bool> DownshiftBlip = true;
        Tracked<bool> DownshiftProtect = false;
        Tracked<float> ClutchRateMult = 1.0f;
        Tracked<float> RPMTolerance = 0.2f;
    } ShiftOptions;

    // [AUTO_PARAMS]
    struct {
        // Lower = upshift later
        Tracked<float> UpshiftLoad = 0.12f;
        // Higher = downshift later
        Tracked<float> DownshiftLoad = 0.50f;
        // Don't upshift until next gears' RPM is over this value.
        Tracked<float> NextGearMinRPM = 0.33f;
        // Downshift when RPM drops below this value.
        Tracked<float> CurrGearMinRPM = 0.27f;
        // Lower = keep in low gear longer // eco - 0.33
        Tracked<float> EcoRate = 0.05f;
        // Timeout mult for not upshifting right after an upshift
        Tracked<float> UpshiftTimeoutMult = 1.0f;
        // Timeout mult for not downshifting after an upshift
        Tracked<float> DownshiftTimeoutMult = 1.0f;
        // Experimental new tcu
        Tracked<bool> UsingATCU = false;
    } AutoParams;

    // [STEERING]
    struct {
        struct {
            Tracked<bool> UseCustomLock = true;
            Tracked<float> SoftLock = 720.0f;
            Tracked<float> SteeringMult = 1.0f;
            Tracked<float> SteeringReduction = 0.9f;
        } CustomSteering;

        struct {
            Tracked<float> SATMult = 1.0f;
            Tracked<float> CurveMult = 1.0f;
            Tracked<float> SoftLock = 720.0f;
            Tracked<float> SteeringMult = 1.0f;
        } Wheel;
    } Steering;

private:
    void saveGeneral();

    std::string mFile;

    // Reference to one unique "master" instance.
    VehicleConfig* mBaseConfig;
};
