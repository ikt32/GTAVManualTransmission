#pragma once
#include <string>
#include <vector>

namespace SteeringAnimation {
    struct Animation {
        std::string Dictionary;
        std::string Name;
        float Rotation{ 0.0f };
        std::vector<std::string> Layouts;
    };

    void Load();

    const std::vector<Animation>& GetAnimations();
    size_t GetAnimationIndex();
    void SetAnimationIndex(size_t index);
    void SetRotation(float wheelRadians);

    void Update();
    void SetFile(const std::string& cs);
    bool FileProblem();

    void Toggle(bool enable);

    // Only call on detach
    void CancelAnimation();
}

