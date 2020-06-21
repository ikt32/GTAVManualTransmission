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
    std::string Description;

    // [MT_OPTIONS]
    struct {
        // Only ShiftMode should be changed on-the-fly
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
        bool UsingATCU = false;
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
            float Gamma = 0.8f;
            float MaxSpeed = 80.0f;
        } FFB;

        // [STEER]
        struct {
            float Angle = 720.0f;
            float Gamma = 1.0f;
        } Steering;
    } Wheel;

    // [HUD]
    struct {
        bool Enable = true;

        // Fonts:
        // 0 - Chalet London
        // 1 - Sign Painter
        // 2 - Slab Serif
        // 4 - Chalet Cologne
        // 7 - Pricedown
        int Font = 4;

        bool Outline = true;

        struct {
            bool Enable = true;
            float XPos = 0.9525f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int TopColorR = 255;
            int TopColorG = 63;
            int TopColorB = 63;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } Gear;

        struct {
            bool Enable = true;
            float XPos = 0.935f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } ShiftMode;

        struct {
            // can be kph, mph, or ms
            std::string Speedo = "kph";
            bool ShowUnit = true;
            float XPos = 0.860f;
            float YPos = 0.885f;
            float Size = 0.700f;
            int ColorR = 255;
            int ColorG = 255;
            int ColorB = 255;
        } Speedo;

        struct {
            bool Enable = true;
            float Redline = 0.850f;

            float XPos = 0.120f;
            float YPos = 0.765f;
            float XSz = 0.140f;
            float YSz = 0.005f;

            int BgR = 0;
            int BgG = 0;
            int BgB = 0;
            int BgA = 128;

            int FgR = 255;
            int FgG = 255;
            int FgB = 255;
            int FgA = 255;

            int RedlineR = 255;
            int RedlineG = 92;
            int RedlineB = 0;
            int RedlineA = 255;

            int RevLimitR = 255;
            int RevLimitG = 0;
            int RevLimitB = 0;
            int RevLimitA = 255;
        } RPMBar;

        struct {
            bool Enable = false;
            float XPos = 0.500f;
            float YPos = 0.035f;
            float Size = 1.000f;
        } DashIndicators;
    } HUD;

protected:
    void loadSettings(const ScriptSettings& gSettings,
        const std::string& file);
};
