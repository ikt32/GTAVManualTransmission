#include "../../../ScriptHookV_SDK/inc/natives.h"
#include "Util.hpp"

void showText(float x, float y, float scale, const char* text, int font, const Color &rgba) {
	UI::SET_TEXT_FONT(font);
	UI::SET_TEXT_SCALE(scale, scale);
	UI::SET_TEXT_COLOUR(rgba.R, rgba.G, rgba.B, rgba.A);
	UI::SET_TEXT_WRAP(0.0, 1.0);
	UI::SET_TEXT_CENTRE(0);
	UI::SET_TEXT_DROPSHADOW(0, 0, 0, 0, 0);
	UI::SET_TEXT_EDGE(1, 0, 0, 0, 205);
	UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");

	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(CharAdapter(text));
	
	UI::END_TEXT_COMMAND_DISPLAY_TEXT(x, y);
}

void showText(float x, float y, float scale, const char* text) {
	showText(x, y, scale, text, 0, {255, 255, 255, 255});
}

void showText(float x, float y, float scale, const char* text, const Color &rgba) {
	showText(x, y, scale, text, 0, rgba);
}

void showNotification(const char* message, int *prevNotification) {
	if (prevNotification && *prevNotification != 0) {
		UI::_REMOVE_NOTIFICATION(*prevNotification);
	}
	UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");

	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(CharAdapter(message));
	
	*prevNotification = UI::_DRAW_NOTIFICATION(false, false);
}
