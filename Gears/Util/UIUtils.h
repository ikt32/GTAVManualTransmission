#pragma once
#include <string>
#include <vector>
#include <inc/types.h>

#include "Color.h"

namespace UI {
    float GetStringWidth(const std::string& text, float scale, int font);

    void Notify(int level, const std::string& message);
    void Notify(int level, const std::string& message, bool removePrevious);

    void ShowSubtitle(const std::string& message, int duration = 2500);

    void ShowHelpText(const std::string& message);

    void ShowText(float x, float y, float scale, const std::string& text,
        int font = 0,
        const Util::ColorI& rgba = Util::ColorsI::SolidWhite,
        bool outline = true);

    void ShowText3D(Vector3 location, const std::vector<std::string>& textLines,
        const Util::ColorI& backgroundColor = Util::ColorsI::TransparentGray,
        const Util::ColorI& fontColor = Util::ColorsI::SolidWhite);

    void ShowText3DColors(Vector3 location, const std::vector<std::pair<std::string, Util::ColorI>>& textLines,
        const Util::ColorI& backgroundColor = Util::ColorsI::TransparentGray);

    void DrawSphere(Vector3 p, float scale, const Util::ColorI& c);

    void DrawBar(float x, float y, float width, float height, Util::ColorI fg, Util::ColorI bg, float value);
}
