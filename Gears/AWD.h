#pragma once

namespace AWD {
    enum EAWDSpecialFlags : int {
        AWD_REMAP_DIAL_Y97Y_R32 = 1,
    };

    enum class EAWDMode {
        Automatic,
        Manual
    };

    void Update();
}
