#include <vector>

struct AtcuGearbox {
    // TCU stuff
    std::vector<float> PowertrainHistoryDistribution = { 0.000123f };
    int PowertrainHistorySize = 240;
    int PowertrainSecondCounter = 0;
    int PowertrainTimer = 0;
    std::vector<float> PowertrainHistoryCrossingPoint = { 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f };
    float parsePowertrainRatioThreshold();
    float parsePowertrainRatio();
    void updatePowertrainRatioDistribution(float ratio);
    bool isPowertrainRatioTrustworthy();
    float CurrentRpm = 0.00f;
};
