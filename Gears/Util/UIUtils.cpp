#include "UIUtils.h"
#include <inc/natives.h>
#include <algorithm>

float getStringWidth(const std::string &text, float scale, int font) {
    UI::_BEGIN_TEXT_COMMAND_WIDTH("STRING");
    UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)text.c_str());
    UI::SET_TEXT_FONT(font);
    UI::SET_TEXT_SCALE(scale, scale);
    return UI::_END_TEXT_COMMAND_GET_WIDTH(true);
}

void showText(float x, float y, float scale, const std::string &text, int font, const Color &rgba, bool outline) {
    UI::SET_TEXT_FONT(font);
    UI::SET_TEXT_SCALE(scale, scale);
    UI::SET_TEXT_COLOUR(rgba.R, rgba.G, rgba.B, rgba.A);
    UI::SET_TEXT_WRAP(0.0, 1.0);
    UI::SET_TEXT_CENTRE(0);
    if (outline) UI::SET_TEXT_OUTLINE();
    UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)text.c_str());
    UI::END_TEXT_COMMAND_DISPLAY_TEXT(x, y);
}

void showDebugInfo3D(Vector3 location, const std::vector<std::string> &textLines, Color backgroundColor, Color fontColor) {
    float height = 0.0125f;

    GRAPHICS::SET_DRAW_ORIGIN(location.x, location.y, location.z, 0);
    int i = 0;

    float szX = 0.060f;
    for (auto line : textLines) {
        showText(0, 0 + height * i, 0.2f, line.c_str(), 0, fontColor, true);
        float currWidth = getStringWidth(line, 0.2f, 0);
        if (currWidth > szX) {
            szX = currWidth;
        }
        i++;
    }

    float szY = (height * i) + 0.02f;
    GRAPHICS::DRAW_RECT(0.027f, (height * i) / 2.0f, szX, szY,
        backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}

void showDebugInfo3DColors(Vector3 location, const std::vector<std::pair<std::string, Color>> &textLines, Color backgroundColor) {
    float height = 0.0125f;

    GRAPHICS::SET_DRAW_ORIGIN(location.x, location.y, location.z, 0);
    int i = 0;

    float szX = 0.060f;
    for (auto line : textLines) {
        showText(0, 0 + height * i, 0.2f, line.first.c_str(), 0, line.second, true);
        float currWidth = getStringWidth(line.first, 0.2f, 0);
        if (currWidth > szX) {
            szX = currWidth;
        }
        i++;
    }

    float szY = (height * i) + 0.02f;
    GRAPHICS::DRAW_RECT(0.027f, (height * i) / 2.0f, szX, szY,
        backgroundColor.R, backgroundColor.G, backgroundColor.B, backgroundColor.A);
    GRAPHICS::CLEAR_DRAW_ORIGIN();
}

void showNotification(const std::string &message, int *prevNotification) {
    if (prevNotification != nullptr && *prevNotification != 0) {
        UI::_REMOVE_NOTIFICATION(*prevNotification);
    }
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");

    UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)message.c_str());

    int id = UI::_DRAW_NOTIFICATION(false, false);
    if (prevNotification != nullptr) {
        *prevNotification = id;
    }
}

void showSubtitle(const std::string &message, int duration) {
    UI::BEGIN_TEXT_COMMAND_PRINT("CELL_EMAIL_BCON");

    const int maxStringLength = 99;

    for (int i = 0; i < message.size(); i += maxStringLength) {
        int npos = std::min(maxStringLength, static_cast<int>(message.size()) - i);
        UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME((char*)message.substr(i, npos).c_str());
    }

    UI::END_TEXT_COMMAND_PRINT(duration, 1);
}
