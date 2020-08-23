#pragma once

enum class EShiftMode : int {
    Sequential = 0,
    HPattern = 1,
    Automatic = 2,
};

EShiftMode Next(EShiftMode mode);

#include <string>
#include <vector>

class VehicleConfig {
public:
    VehicleConfig();
    VehicleConfig(VehicleConfig* baseConfig,
        const std::string& file);

    void LoadSettings(const std::string& file);
    void SaveSettings();

    std::string Name;

    // ID
    std::vector<std::string> ModelNames;
    std::vector<std::string> Plates;
    std::string Description;

    // [MT_OPTIONS]
    struct {
        EShiftMode ShiftMode = EShiftMode::Sequential;

        bool ClutchCreep = false;
        bool ClutchShiftH = true;
        bool ClutchShiftS = false;
    } MTOptions;

    // [MT_PARAMS]
    struct {
        float EngBrakePower = 0.0f;
        float EngBrakeThreshold = 0.75f;
        // Clutch _lift_ distance
        float ClutchThreshold = 0.15f;
        float StallingRPM = 0.09f;
        float StallingRate = 3.75f;
        float StallingSlip = 0.40f;
        float RPMDamage = 0.1f;
        float MisshiftDamage = 5.0f;
        float CreepIdleThrottle = 0.1f;
        float CreepIdleRPM = 0.1f;
    } MTParams;

    // [DRIVING_ASSISTS]
    struct {
        struct {
            bool Enable = false;
            // true: only applied to abs-less vehicles
            bool Filter = true;
        } ABS;

        struct {
            bool Enable = false;
            // 0 Brake, 1 Throttle 
            int Mode = 0;
            // tyre speed threshold, m/s
            float SlipMax = 2.5f;
        } TCS;

        struct {
            bool Enable = false;

            float OverMin = 05.0f; // deg
            float OverMax = 15.0f; // deg

            float OverMinComp = 0.0f; // brake mult
            float OverMaxComp = 2.0f; // brake mult

            float UnderMin = 5.0f; // deg
            float UnderMax = 10.0f; // deg

            float UnderMinComp = 0.0f; // brake mult
            float UnderMaxComp = 1.0f; // brake mult

        } ESP;

        struct {
            bool Enable = false;
            float Viscosity = 1.0f;
        } LSD;

        struct {
            // Only active for cars with
            // fDriveBiasFront >= 0.1 &&
            // fDriveBiasFront <= 0.9 &&
            // fDriveBiasFront != 0.5
            bool Enable = false;

            // Bias when bias transfer is maximized
            float BiasAtMaxTransfer = 0.5f;

            // AWD::EAWDMode Mode = AWD::EAWDMode::Automatic;

            bool UseCustomBaseBias = false;
            float CustomBaseBias = 0.01f;
            float CustomMin = 0.0f;
            float CustomMax = 1.0f;

            bool UseTraction = false;
            float TractionLossMin = 1.05f; // "Strong" axle is  5% faster than "weak" axle 
            float TractionLossMax = 1.50f; // "Strong" axle is 50% faster than "weak" axle

            // Should only be used for RWD-biased cars
            bool UseOversteer = false;
            float OversteerMin = 5.0f; // degrees
            float OversteerMax = 15.0f; // degrees

            // Should only be used for FWD-biased cars
            bool UseUndersteer = false;
            float UndersteerMin = 5.0f; // degrees
            float UndersteerMax = 15.0f; // degrees

            // See AWD.h for currently supported flags
            uint32_t SpecialFlags = 0;
        } AWD;
    } DriveAssists;

    // [SHIFT_OPTIONS]
    struct {
        bool UpshiftCut = true;
        bool DownshiftBlip = true;
        float ClutchRateMult = 1.0f;
        float RPMTolerance = 0.2f;
    } ShiftOptions;

    // [AUTO_PARAMS]
    struct {
        // Lower = upshift later
        float UpshiftLoad = 0.12f;
        // Higher = downshift later
        float DownshiftLoad = 0.60f;
        // Don't upshift until next gears' RPM is over this value.
        float NextGearMinRPM = 0.33f;
        // Downshift when RPM drops below this value.
        float CurrGearMinRPM = 0.27f;
        // Lower = keep in low gear longer // eco - 0.33
        float EcoRate = 0.05f;
        // Timeout mult for not downshifting after an upshift
        float DownshiftTimeoutMult = 1.0f;
        // Experimental new tcu
        bool UsingATCU = false;
    } AutoParams;

    struct {
        // [STEER]
        struct {
            float SoftLock = 720.0f;
        } Steering;
    } Wheel;

    // [CUSTOM_STEERING]
    struct {
        float CustomRotationDegrees = 720.0f;
    } CustomSteering;

    // [MISC]
    struct {
        // [CAM]
        struct {
            bool Enable = true;
            int AttachId = 0; // 0: Head, 1: Vehicle, 2: FPV Offset?
            bool RemoveHead = true;

            struct {
                bool Follow = true;
                float RotationDirectionMult = 0.750f;
                float RotationRotationMult = 0.15f;
                float RotationMaxAngle = 45.0f;

                float LongForwardMult = 0.05f;
                float LongBackwardMult = 0.10f;
                float LongDeadzone = 0.95f;
                float LongGamma = 1.0f;
                float LongForwardLimit = 0.10f;
                float LongBackwardLimit = 0.12f;
            } Movement;

            float FOV = 55.0f;
            float OffsetHeight = 0.04f;
            float OffsetForward = 0.05f;
            float OffsetSide = 0.0f;
            float Pitch = 0.0f;
            float LookTime = 0.000010f;
            float MouseLookTime = 0.000001f;
            int MouseCenterTimeout = 750;
            float MouseSensitivity = 0.3f;
            struct {
                bool Disable = false;
                int AttachId = 0; // 0: Head, 1: Vehicle, 2: FPV Offset?
                float FOV = 55.0f;
                float OffsetHeight = -0.05f;
                float OffsetForward = -0.08f;
                float OffsetSide = 0.0f;
                float Pitch = -11.0f;
            } Bike;
        } Camera;
    } Misc;

protected:
    void saveGeneral();

    std::string mFile;

    // Reference to one unique "master" instance.
    VehicleConfig* mBaseConfig;
};
