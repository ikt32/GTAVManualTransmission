#include "Color.h"
#include <algorithm>
#include <cmath>

Util::ColorF Util::RGB2HSV(ColorF rgb) {
    ColorF hsv{};
    auto fR = rgb.R;
    auto fG = rgb.G;
    auto fB = rgb.B;

    auto fCMax = std::max(std::max(fR, fG), fB);
    auto fCMin = std::min(std::min(fR, fG), fB);
    auto fDelta = fCMax - fCMin;

    if (fDelta > 0.0f) {
        if (fCMax == fR) {
            hsv.R = 60.0f * (std::fmod(((fG - fB) / fDelta), 6.0f));
        }
        else if (fCMax == fG) {
            hsv.R = 60.0f * (((fB - fR) / fDelta) + 2.0f);
        }
        else if (fCMax == fB) {
            hsv.R = 60.0f * (((fR - fG) / fDelta) + 4.0f);
        }

        if (fCMax > 0.0f) {
            hsv.G = fDelta / fCMax;
        }
        else {
            hsv.G = 0.0f;
        }

        hsv.B = fCMax;
    }
    else {
        hsv.R = 0.0f;
        hsv.G = 0.0f;
        hsv.B = fCMax;
    }

    if (hsv.R < 0.0) {
        hsv.R = 360.0f + hsv.R;
    }
    return hsv;
}

Util::ColorF Util::HSV2RGB(ColorF hsv) {
    ColorF rgb{};
    auto fH = hsv.R;
    auto fS = hsv.G;
    auto fV = hsv.B;

    auto fC = fV * fS; // Chroma
    auto fHPrime = std::fmod(fH / 60.0f, 6.0f);
    auto fX = fC * (1.0f - std::abs(fmod(fHPrime, 2.0f) - 1.0f));
    auto fM = fV - fC;

    if (0.0f <= fHPrime && fHPrime < 1.0f) {
        rgb.R = fC;
        rgb.G = fX;
        rgb.B = 0.0f;
    }
    else if (1.0f <= fHPrime && fHPrime < 2.0f) {
        rgb.R = fX;
        rgb.G = fC;
        rgb.B = 0.0f;
    }
    else if (2.0f <= fHPrime && fHPrime < 3.0f) {
        rgb.R = 0.0f;
        rgb.G = fC;
        rgb.B = fX;
    }
    else if (3.0f <= fHPrime && fHPrime < 4.0f) {
        rgb.R = 0.0f;
        rgb.G = fX;
        rgb.B = fC;
    }
    else if (4.0f <= fHPrime && fHPrime < 5.0f) {
        rgb.R = fX;
        rgb.G = 0.0f;
        rgb.B = fC;
    }
    else if (5.0 <= fHPrime && fHPrime < 6.0) {
        rgb.R = fC;
        rgb.G = 0.0;
        rgb.B = fX;
    }
    else {
        rgb.R = 0.0f;
        rgb.G = 0.0f;
        rgb.B = 0.0f;
    }

    rgb.R += fM;
    rgb.G += fM;
    rgb.B += fM;

    return rgb;
}
