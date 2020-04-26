#pragma once

namespace Util {
    struct ColorI {
        int R;
        int G;
        int B;
        int A;
    };

    struct ColorF {
        float R;
        float G;
        float B;
        float A;
    };

    namespace ColorsI {
        const ColorI SolidWhite = { 255, 255, 255, 255 };
        const ColorI SolidBlack = { 0, 0, 0, 255 };

        const ColorI SolidRed = { 255, 0, 0, 255 };
        const ColorI SolidGreen = { 0, 255, 0, 255 };
        const ColorI SolidBlue = { 0, 0, 255, 255 };

        const ColorI SolidPink = { 255, 0, 255, 255 };
        const ColorI SolidYellow = { 255, 255, 0, 255 };
        const ColorI SolidCyan = { 0, 255, 255, 255 };

        const ColorI SolidOrange = { 255, 127, 0, 255 };
        const ColorI SolidLime = { 127, 255, 0, 255 };
        const ColorI SolidPurple = { 127, 0, 255, 255 };

        const ColorI TransparentGray = { 75, 75, 75, 75 };
    }
}
