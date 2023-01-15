#pragma once
#include "Util/Enums.hpp"

#include <map>
#include <string>

struct SShakeParams {
    float Amplitude;
    float Frequency;
};

class CShakeData {
public:
    CShakeData(const std::string& shakeFile);

    void Load();

    // Base rates
    float MinRateModSpd = 25.0f;
    float MaxRateModSpd = 50.0f;
    float MinRateModTrn = 12.0f;
    float MaxRateModTrn = 24.0f;

    std::map<eMaterial, SShakeParams> MaterialReactionMap;

private:
    std::string mShakeFile;
};
