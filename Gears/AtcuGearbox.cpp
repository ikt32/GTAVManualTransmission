#include <numeric>
#include <vector>
#include <algorithm>
#include "AtcuGearbox.h"

float AtcuGearbox::parsePowertrainRatioThreshold() {
    auto avg = (std::accumulate(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end(), 0.0f)) / PowertrainHistoryDistribution.size();
    auto max = *std::max_element(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end());
    if (max > avg) {
        std::vector<float> gapElements = {};
        auto gap = max - avg;
        for (auto& c : PowertrainHistoryDistribution)
            if (c > avg) gapElements.push_back(c);
        auto ratio = avg - ((gap * ((gapElements.size() + 0.0f) / (PowertrainHistoryDistribution.size() + 0.00f))) * 2.0f);
        auto integralityIndex = (PowertrainHistoryDistribution.size() + 1.0f) / (PowertrainHistorySize + 1.0f);
        if (integralityIndex > 1.0f) integralityIndex = 1.0f;
        return ratio * integralityIndex;
    }
    else return avg;
}

float AtcuGearbox::parsePowertrainRatio() {
    auto avg = (std::accumulate(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end(), 0.0f)) / PowertrainHistoryDistribution.size();
    auto max = *std::max_element(PowertrainHistoryDistribution.begin(), PowertrainHistoryDistribution.end());
    if (max > avg) {
        std::vector<float> gapElements = {};
        auto gap = max - avg;
        for (auto& c : PowertrainHistoryDistribution)
            if (c > avg) gapElements.push_back(c);
        return avg + (gap * ((gapElements.size() + 0.0f) / (PowertrainHistoryDistribution.size() + 0.00f)));
    }
    else return avg;
}

void AtcuGearbox::updatePowertrainRatioDistribution(float ratio) {
    PowertrainHistoryDistribution.push_back(ratio);
    while (PowertrainHistoryDistribution.size() > PowertrainHistorySize || PowertrainHistoryDistribution[0] == 0.000123f)
        PowertrainHistoryDistribution.erase(PowertrainHistoryDistribution.begin());
}

bool AtcuGearbox::isPowertrainRatioTrustworthy() {
    auto ratio = parsePowertrainRatio();
    return PowertrainHistoryDistribution.size() > (PowertrainHistorySize * 0.5) && (abs(ratio - parsePowertrainRatioThreshold()) / ratio) < 0.08f;
}
