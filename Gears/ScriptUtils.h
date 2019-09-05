#pragma once
#include "inc/enums.h"
#include <string>

/**
 * Script-specific utilities that put script-specific info into utility functions.
 * Thought this was cleaner than just pulling info into the more generic utility classes.
 */

namespace UI {
    void Notify(const std::string& message);
}

namespace Controls {
    void SetControlADZ(eControl control, float value, float adz);
}