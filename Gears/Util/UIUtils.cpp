#include "UIUtils.h"

#include "inc/natives.h"
#include "inc/enums.h"
#include "fmt/format.h"
#include <algorithm>

namespace {
    float getStringWidth(const std::string& text, float scale, int font) {
        HUD::_BEGIN_TEXT_COMMAND_GET_WIDTH("STRING");
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
        HUD::SET_TEXT_FONT(font);
        HUD::SET_TEXT_SCALE(scale, scale);
        return HUD::_END_TEXT_COMMAND_GET_WIDTH(true);
    }
}

void showText(float x, float y, float scale, const std::string &text, 
    int font, const Util::ColorI &rgba, bool outline) {
    HUD::SET_TEXT_FONT(font);
    HUD::SET_TEXT_SCALE(scale, scale);
    HUD::SET_TEXT_COLOUR(rgba.R, rgba.G, rgba.B, rgba.A);
    HUD::SET_TEXT_WRAP(0.0, 1.0);
    HUD::SET_TEXT_CENTRE(0);
    if (outline) HUD::SET_TEXT_OUTLINE();
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(text.c_str());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(x, y, 0);
}

void showDebugInfo3D(Vector3 location, const std::vector<std::string> &textLines, 
    const Util::ColorI& backgroundColor, const Util::ColorI& fontColor) {
    float height = 0.0125f;

    GRAPHICS::SET_DRAW_ORIGIN(location.x, location.y, location.z, 0);
    int i = 0;

    float szX = 0.060f;
    for (const auto& line : textLines) {
        showText(0, 0 + height * static_cast<float>(i), 0.2f, line, 0, fontColor, true);
        float currWidth = getStringWidth(line, 0.2f, 0);
        if (currWidth > szX) {
            szX = currWidth;
        }
        i++;
    }

    float szY = (height * static_cast<float>(i)) + 0.02f;
    GRAPHICS::DRAW_RECT(0.027f, (height * static_cast<float>(i)) / 2.0f, szX, szY,
        backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A, 0);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}

void showDebugInfo3DColors(Vector3 location, const std::vector<std::pair<std::string, Util::ColorI>> &textLines, 
    const Util::ColorI& backgroundColor) {
    float height = 0.0125f;

    GRAPHICS::SET_DRAW_ORIGIN(location.x, location.y, location.z, 0);
    int i = 0;

    float szX = 0.060f;
    for (const auto& line : textLines) {
        showText(0, 0 + height * static_cast<float>(i), 0.2f, line.first, 0, line.second, true);
        float currWidth = getStringWidth(line.first, 0.2f, 0);
        if (currWidth > szX) {
            szX = currWidth;
        }
        i++;
    }

    float szY = (height * static_cast<float>(i)) + 0.02f;
    GRAPHICS::DRAW_RECT(0.027f, (height * static_cast<float>(i)) / 2.0f, szX, szY,
        backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A, 0);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}

void showNotification(const std::string &message, int *prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        HUD::THEFEED_REMOVE_ITEM(*prevNotification);
    }
    HUD::BEGIN_TEXT_COMMAND_THEFEED_POST("STRING");

    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.c_str());

    int id = HUD::END_TEXT_COMMAND_THEFEED_POST_TICKER(false, false);
    if (prevNotification != nullptr) {
        *prevNotification = id;
    }
}

void showSubtitle(const std::string &message, int duration) {
    HUD::BEGIN_TEXT_COMMAND_PRINT("CELL_EMAIL_BCON");

    const size_t maxStringLength = 99;

    for (size_t i = 0; i < message.size(); i += maxStringLength) {
        size_t npos = std::min(maxStringLength, static_cast<int>(message.size()) - i);
        HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(message.substr(i, npos).c_str());
    }

    HUD::END_TEXT_COMMAND_PRINT(duration, 1);
}

void drawSphere(Vector3 p, float scale, const Util::ColorI& c) {
    GRAPHICS::DRAW_MARKER(eMarkerType::MarkerTypeDebugSphere,
                          p.x, p.y, p.z,
                          0.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.0f,
                          scale, scale, scale,
                          c.R, c.G, c.B, c.A,
                          false, false, 2, false, nullptr, nullptr, false);
}
