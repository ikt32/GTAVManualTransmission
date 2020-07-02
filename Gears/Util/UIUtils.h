#pragma once
#include <string>
#include <vector>
#include <inc/types.h>

#include "Color.h"

void showText(float x, float y, float scale, const std::string &text, 
    int font = 0, 
    const Util::ColorI &rgba = Util::ColorsI::SolidWhite, 
    bool outline = true);

void showDebugInfo3D(Vector3 location, const std::vector<std::string> &textLines,
    const Util::ColorI& backgroundColor = Util::ColorsI::TransparentGray,
    const Util::ColorI& fontColor = Util::ColorsI::SolidWhite);

void showDebugInfo3DColors(Vector3 location, const std::vector<std::pair<std::string, Util::ColorI>> &textLines,
    const Util::ColorI& backgroundColor = Util::ColorsI::TransparentGray);

void showNotification(const std::string &message, int *prevNotification);
void showSubtitle(const std::string &message, int duration = 2500);

void drawSphere(Vector3 p, float scale, const Util::ColorI& c);

namespace UI {
    void Notify(int level, const std::string& message);
    void Notify(int level, const std::string& message, bool removePrevious);
    float GetStringWidth(const std::string& text, float scale, int font);
}
