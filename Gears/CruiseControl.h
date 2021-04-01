#pragma once

namespace CruiseControl {
    bool GetActive();
    void SetActive(bool active);

    void Update(float& throttle, float& brake, float& clutch);
}