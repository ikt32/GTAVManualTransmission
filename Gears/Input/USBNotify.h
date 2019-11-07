#pragma once
#include <functional>

namespace USB {
    void Init(const std::function<void()>& func);
    void Update();
}
