#pragma once
#include <string>
#include <vector>

namespace SteeringAnimation {
    struct Animation {
        std::string Dictionary;
        std::string Name;
        float Rotation;
    };
    const std::vector<Animation>& GetAnimations();

    void Update();

    void SetRotation(float wheelDegrees);
}

