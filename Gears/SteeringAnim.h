#pragma once
#include <string>
#include <vector>

namespace SteeringAnimation {
    struct Animation {
        std::string Dictionary;
        std::string Name;
        float Rotation;
        std::vector<std::string> Layouts;
    };

    void Load(const std::string& path);

    const std::vector<Animation>& GetAnimations();
    size_t GetAnimationIndex();
    void SetAnimationIndex(size_t index);
    void SetRotation(float wheelDegrees);

    void Update();
}

