#pragma once
#include <vector>

namespace DrivingAssists {
    struct ABSData {
        bool Use;
    };

    struct TCSData {
        bool Use;
        // How much it spins faster/slower than the suspension component. Ratio.
        std::vector<float> LinearSlipRatio;
        float AverageSlipRatio;
    };

    struct ESPData {
        bool Use;
        // Whether it passed the ESP Understeer threshold
        bool Understeer;

        // average front wheels slip angle
        float UndersteerAngle; // rad

        bool Oversteer;
        // average rear wheels slip angle
        float OversteerAngle; // rad
        bool OppositeLock;
    };

    struct LSDData {
        bool Use;
        float BrakeLF;
        float BrakeRF;
        float BrakeLR;
        float BrakeRR;
        float FDD; // debug, front diff speeddiff
        float RDD; // debug, rear  diff speeddiff
    };

    ABSData GetABS();
    TCSData GetTCS();
    ESPData GetESP();

    // Technically not an assist since the ESP-ish "braked wheel sends power to the other side"
    // doesn't apply, but putting it here anyway since we negative-brake to simulate power transfer.
    LSDData GetLSD();

    std::vector<float> GetESPBrakes(ESPData espData);
    std::vector<float> GetTCSBrakes(TCSData tcsData);
    std::vector<float> GetABSBrakes(ABSData absData);
    std::vector<float> GetLSDBrakes(LSDData lsdData);
}
