#pragma once
#include "ScriptSettings.hpp"

class VehicleConfig {
public:
    VehicleConfig(const ScriptSettings& gSettings,
        const std::string& file);

    std::string Name;

    // ID
    std::vector<std::string> ModelNames;
    std::vector<std::string> Plates;

    // [MT_OPTIONS]
    struct {
        // Only ShiftMode should be changed on-the-fly
        EShiftMode ShiftMode = EShiftMode::Sequential;
        bool ClutchCreep = false;
    } MTOptions;

    // [MT_PARAMS]
    struct {
        float EngBrakePower = 0.0f;
        float EngBrakeThreshold = 0.75f;
        // Clutch _lift_ distance
        float ClutchThreshold = 0.15f;
        // Clutch _lift_ distance
        float StallingThreshold = 0.35f;
        float StallingRPM = 0.1f;
        float RPMDamage = 0.1f;
        float MisshiftDamage = 5.0f;
    } MTParams;

    // [DRIVING_ASSISTS]
    struct {
        struct {
            bool Enable = false;
            // true: only applied to abs-less vehicles
            bool Filter = true;
        } ABS;

        struct {
            // 0 Disabled, 1 Brake, 2 Throttle (patch) 
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
        float UpshiftLoad = 0.05f;
        float DownshiftLoad = 0.60f;
        float NextGearMinRPM = 0.33f;
        float CurrGearMinRPM = 0.27f;
        float EcoRate = 0.05f;
        float DownshiftTimeoutMult = 1.0f;
    } AutoParams;

    struct {
        // [FORCE_FEEDBACK]
        struct {
            bool Enable = true;
            bool Scale = true;
            int AntiDeadForce = 0;
            float SATAmpMult = 1.0f;
            int SATMax = 10000;
            float SATFactor = 0.66f;
            int DamperMax = 50;
            int DamperMin = 10;
            float DamperMinSpeed = 1.2f; // TargetSpeed in m/s
            float DetailMult = 2.5f;
            int DetailLim = 20000;
            int DetailMAW = 1;
            float CollisionMult = 1.0f;
        } FFB;

        // [STEER]
        struct {
            float Angle = 720.0f;
        } Steering;
    } Wheel;

protected:
    void loadSettings(const ScriptSettings& gSettings,
        const std::string& file);
};
