#pragma once

namespace CruiseControl {
    bool GetActive();
    void SetActive(bool active);

    bool GetAdaptiveActive();

    void Update(float& throttle, float& brake, float& clutch);
}