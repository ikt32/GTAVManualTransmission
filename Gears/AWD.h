#pragma once

namespace AWD {
    enum EAWDSpecialFlags : int {
        AWD_REMAP_DIAL_Y97Y_R32 = 1,
        AWD_REMAP_DIAL_WANTED188_R32 = 2,
    };

    enum class EAWDMode {
        Automatic,
        Manual
    };

    float& GetDisplayValue();

    void Update();
    float GetTransferValue();
    float GetDriveBiasFront(void* pHandlingDataOrig);
}
