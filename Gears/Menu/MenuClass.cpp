/* MenuClass.cpp */

#include "MenuClass.h"
#include <functional>

#include "inc/natives.h"
#include "inc/enums.h"
#include "controls.h"
#include "Util/Util.hpp"

Menu::Menu() {
	//std::string  nothing = "";
	//std::fill(currentmenu, std::end(currentmenu), nothing);
	//std::fill(lastoption, std::end(lastoption), 0);
}

Menu::~Menu() {}
//
//std::string  Menu::stringToChar(std::string string) {
//	return _strdup(string.c_str());
//}

bool Menu::CurrentMenu(std::string  menuname) {
	if (menuname == actualmenu) return true;
	else return false;
}

void Menu::changeMenu(std::string  menuname) {
	currentmenu[menulevel] = actualmenu;
	lastoption[menulevel] = currentoption;
	menulevel++;
	actualmenu = menuname;
	currentoption = 1;
	menuBeep();
	resetButtonStates();
}

void Menu::nextOption() {
	if (currentoption < optioncount)
		currentoption++;
	else
		currentoption = 1;
	if (menulevel > 0) {
		menuBeep();
	}
	resetButtonStates();
}

void Menu::previousOption() {
	if (currentoption > 1)
		currentoption--;
	else
		currentoption = optioncount;
	if (menulevel > 0) {
		menuBeep();
	}
	resetButtonStates();
}

void Menu::backMenu() {
	if (menulevel > 0) {
		menuBeep();
	}
	menulevel--;
	actualmenu = currentmenu[menulevel];
	currentoption = lastoption[menulevel];

}

void Menu::drawText(const std::string  text, int font, float x, float y, float scalex, float scaley, rgba rgba, bool center) {
	UI::SET_TEXT_FONT(font);
	if (font == 0) { // big-ass Chalet London
		scaley *= 0.75f;
		y += 0.0025f;
	}
	UI::SET_TEXT_SCALE(scalex, scaley);
	UI::SET_TEXT_COLOUR(rgba.r, rgba.g, rgba.b, rgba.a);
	UI::SET_TEXT_CENTRE(center);
	UI::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
	UI::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(CharAdapter(text.c_str()));
	UI::END_TEXT_COMMAND_DISPLAY_TEXT(x, y);
};

void Menu::drawRect(float x, float y, float width, float height, rgba rgba) {
	GRAPHICS::DRAW_RECT(x, y, width, height, rgba.r, rgba.g, rgba.b, rgba.a);
};

void Menu::drawSprite(std::string  Streamedtexture, std::string  textureName, float x, float y, float width, float height, float rotation, rgba rgba)
{
	if (!GRAPHICS::HAS_STREAMED_TEXTURE_DICT_LOADED(CharAdapter(Streamedtexture.c_str()))) GRAPHICS::REQUEST_STREAMED_TEXTURE_DICT(CharAdapter(Streamedtexture.c_str()), false);
	else GRAPHICS::DRAW_SPRITE(CharAdapter(Streamedtexture.c_str()), CharAdapter(textureName.c_str()), x, y, width, height, rotation, rgba.r, rgba.g, rgba.b, rgba.a);
};

void Menu::Title(std::string  title) {
	optioncount = 0;
	drawText(title, titleFont, menux, menuy - 0.03f/*0.095f*/, 0.85f, 0.85f, titleText, true);
	drawRect(menux, menuy - 0.0075f/*0.1175f*/, 0.23f, 0.085f, titleRect);
};

bool Menu::Option(std::string  option) {
	optioncount++;

	bool thisOption = false;
	if (currentoption == optioncount) thisOption = true;

	if (currentoption <= 16 && optioncount <= 16)
	{
		drawText(option, optionsFont, menux - 0.1f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, false);
		drawRect(menux, ((optioncount * 0.035f) + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, optionsrect);
		if (thisOption) drawRect(menux, ((optioncount * 0.035f) + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, scroller);
	}

	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
	{
		drawText(option, optionsFont, menux - 0.1f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, false);
		drawRect(menux, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, optionsrect);
		if (thisOption) drawRect(menux, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, scroller);
	}

	if (optionpress && currentoption == optioncount) return true;
	else return false;
};

void Menu::drawAdditionalInfoBox(std::vector<std::string> &extra, size_t infoLines) {
	for (int i = 0; i < infoLines; i++) {
		drawText(extra[i].c_str(), optionsFont, menux + 0.125f, i * 0.035f + (menuy + 0.035f)/*0.160f*/, 0.5f, 0.5f, options, false);
		drawRect(menux + 0.23f, i * 0.035f + (menuy + 0.0515f)/*0.1765f*/, 0.23f, 0.035f, optionsrect);
	}
}

bool Menu::OptionPlus(std::string  option, std::vector<std::string> &extra, bool *highlighted, std::function<void(void) > onRight, std::function<void(void) > onLeft) {
	optioncount++;
	size_t infoLines = extra.size();
	bool thisOption = false;
	if (currentoption == optioncount) {
		thisOption = true;
		if (highlighted != nullptr) {
			*highlighted = true;
		}
		if (onLeft && leftpress) {
			onLeft();
			leftpress = false;
			return false;
		}
		if (onRight && rightpress) {
			onRight();
			rightpress = false;
			return false;
		}
	}

	if (currentoption <= 16 && optioncount <= 16) {
		drawText(option, optionsFont, menux - 0.1f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, false);
		drawRect(menux, ((optioncount * 0.035f) + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, optionsrect);
		if (thisOption) {
			drawRect(menux, ((optioncount * 0.035f) + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, scroller); // Highlighted line
			drawAdditionalInfoBox(extra, infoLines);
		}
	}

	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
	{
		drawText(option, optionsFont, menux - 0.1f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, false);
		drawRect(menux, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, optionsrect);
		if (thisOption) {
			drawRect(menux, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, scroller);
			drawAdditionalInfoBox(extra, infoLines);
		}
	}

	if (optionpress && currentoption == optioncount) return true;
	else return false;
};



bool Menu::MenuOption(std::string  option, std::string  menu) {
	Option(option);

	if (currentoption <= 16 && optioncount <= 16)
		drawText(">>", optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(">>", optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (optionpress && currentoption == optioncount) {
		optionpress = false;
		changeMenu(menu);
		return true;
	}
	else return false;
}

bool Menu::IntOption(std::string  option, int *var, int min, int max, int step) {
	Option(option);

	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + std::to_string(*var) + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + std::to_string(*var) + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*var <= min) *var = max;
			else *var -= step;
			leftpress = false;
			return true;
		}
		if (*var < min) *var = max;
		if (rightpress) {
			if (*var >= max) *var = min;
			else *var += step;
			rightpress = false;
			return true;
		}
		if (*var > max) *var = min;
	}

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

bool Menu::FloatOption(std::string  option, float *var, float min, float max, float step) {
	Option(option);

	char buf[100];
	_snprintf_s(buf, sizeof(buf), "%.2f", *var);

	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*var <= min) *var = max;
			else *var -= step;
			leftpress = false;
			return true;
		}
		if (*var < min) *var = max;
		if (rightpress) {
			if (*var >= max) *var = min;
			else *var += step;
			rightpress = false;
			return true;
		}
		if (*var > max) *var = min;
	}

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

bool Menu::DoubleOption(std::string  option, double *var, double min, double max, double step) {
	Option(option);

	char buf[100];
	_snprintf_s(buf, sizeof(buf), "%.5f", *var);

	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*var <= min) *var = max;
			else *var -= step;
			leftpress = false;
			return true;
		}
		if (*var < min) *var = max;

		if (rightpress) {
			if (*var >= max) *var = min;
			else *var += step;
			rightpress = false;
			return true;
		}
		if (*var > max) *var = min;
	}

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

bool Menu::BoolOption(std::string  option, bool *b00l) {
	Option(option);
	if (currentoption <= 16 && optioncount <= 16)
		drawSprite("commonmenu", *b00l ? "shop_box_tick" : "shop_box_blank",
				   menux + 0.068f, (optioncount * 0.035f + (menuy + 0.016f)/*0.141f*/), 0.03f, 0.05f, 0, options);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawSprite("commonmenu", *b00l ? "shop_box_tick" : "shop_box_blank",
				   menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.016f)/*0.141f*/), 0.03f, 0.05f, 0, options);
	if (optionpress && currentoption == optioncount) {
		*b00l ^= 1;
		return true;
	}
	else return false;
}

bool Menu::BoolSpriteOption(std::string  option, bool b00l, std::string  category, std::string  spriteOn, std::string  spriteOff) {
	Option(option);

	if (currentoption <= 16 && optioncount <= 16)
		drawSprite(category, b00l ? spriteOn : spriteOff,
				   menux + 0.068f, (optioncount * 0.035f + (menuy + 0.016f)/*0.141f*/), 0.03f, 0.05f, 0, options);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawSprite(category, b00l ? spriteOn : spriteOff,
				   menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + (menuy + 0.016f)/*0.141f*/), 0.03f, 0.05f, 0, options);

	if (optionpress && currentoption == optioncount) return true;
	else return false;
}

bool Menu::IntArray(std::string  option, int display[], int *PlaceHolderInt) {
	Option(option);

	int min = 0;
	int max = sizeof(display) / sizeof(*display);

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*PlaceHolderInt <= min) *PlaceHolderInt = max;
			else PlaceHolderInt -= 1;
			leftpress = false;
			return true;
		}
		if (*PlaceHolderInt < min) *PlaceHolderInt = max;
		if (rightpress) {
			if (*PlaceHolderInt >= max) *PlaceHolderInt = min;
			else *PlaceHolderInt += 1;
			rightpress = false;
			return true;
		}
		if (*PlaceHolderInt > max) *PlaceHolderInt = min;
	}
	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + std::to_string(display[*PlaceHolderInt]) + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + std::to_string(display[*PlaceHolderInt]) + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

bool Menu::FloatArray(std::string  option, float display[], int *PlaceHolderInt) {
	Option(option);

	int min = 0;
	int max = sizeof(display) / sizeof(*display);

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*PlaceHolderInt <= min) *PlaceHolderInt = max;
			else *PlaceHolderInt -= 1;
			leftpress = false;
			return true;
		}
		if (*PlaceHolderInt < min) *PlaceHolderInt = max;
		if (rightpress) {
			if (*PlaceHolderInt >= max) *PlaceHolderInt = min;
			else *PlaceHolderInt += 1;
			rightpress = false;
			return true;
		}
		if (*PlaceHolderInt > max) *PlaceHolderInt = min;
	}

	char buf[30];
	_snprintf_s(buf, sizeof(buf), "%.2f", display[*PlaceHolderInt]);

	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + (std::string)buf + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

// why?!
bool Menu::CharArray(std::string  option, std::string  display[], int *PlaceHolderInt) {
	Option(option);

	int min = 0;
	int max = sizeof(display) / sizeof(*display) + 1;

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*PlaceHolderInt <= min) *PlaceHolderInt = max;
			else *PlaceHolderInt -= 1;
			leftpress = false;
		}
		if (*PlaceHolderInt < min) *PlaceHolderInt = max;
		if (rightpress) {
			if (*PlaceHolderInt >= max) *PlaceHolderInt = min;
			else *PlaceHolderInt += 1;
			rightpress = false;
		}
		if (*PlaceHolderInt > max) *PlaceHolderInt = min;
	}
	if (currentoption <= 16 && optioncount <= 16)
		drawText(("<" + (std::string)display[*PlaceHolderInt] + ">").c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText(("<" + (std::string)display[*PlaceHolderInt] + ">").c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

// :ok_hand:
bool Menu::StringArray(std::string  option, std::vector<std::string>display, int *PlaceHolderInt) {
	Option(option);

	int min = 0;
	int max = static_cast<int>(display.size()) - 1;

	if (currentoption == optioncount) {
		if (leftpress) {
			if (*PlaceHolderInt <= min) *PlaceHolderInt = max;
			else *PlaceHolderInt -= 1;
			leftpress = false;
		}
		if (*PlaceHolderInt < min) *PlaceHolderInt = max;
		if (rightpress) {
			if (*PlaceHolderInt >= max) *PlaceHolderInt = min;
			else *PlaceHolderInt += 1;
			rightpress = false;
		}
		if (*PlaceHolderInt > max) *PlaceHolderInt = min;
	}
	std::string leftArrow = "<";
	std::string rightArrow = ">";
	if (max == 0) {
		leftArrow = rightArrow = "";
	}
	if (currentoption <= 16 && optioncount <= 16)
		drawText((leftArrow + (std::string)display[*PlaceHolderInt] + rightArrow).c_str(), optionsFont, menux + 0.068f, (optioncount * 0.035f + menuy), 0.5f, 0.5f, options, true);
	else if ((optioncount > (currentoption - 16)) && optioncount <= currentoption)
		drawText((leftArrow + (std::string)display[*PlaceHolderInt] + rightArrow).c_str(), optionsFont, menux + 0.068f, ((optioncount - (currentoption - 16)) * 0.035f + menuy), 0.5f, 0.5f, options, true);

	if (optionpress && currentoption == optioncount)
		return true;
	else return false;
}

void Menu::TeleportOption(std::string  option, float x, float y, float z) {
	Option(option);

	if (currentoption == optioncount && optionpress) {
		Entity handle = PLAYER::PLAYER_PED_ID();
		if (PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false)) handle = PED::GET_VEHICLE_PED_IS_USING(PLAYER::PLAYER_PED_ID());
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(handle, x, y, z, false, false, false);
	}
}

void Menu::IniWriteInt(LPCWSTR file, LPCWSTR section, LPCWSTR key, int value)
{
	wchar_t newValue[256];
	wsprintfW(newValue, L"%d", value);
	WritePrivateProfileStringW(section, key, newValue, file);
}

int Menu::IniReadInt(LPCWSTR file, LPCWSTR section, LPCWSTR key)
{
	int returning = GetPrivateProfileIntW(section, key, NULL, file);
	return returning;
}

void Menu::LoadMenuTheme(LPCWSTR file)
{
	// Title Text
	titleText.r = IniReadInt(file, L"Title Text", L"Red");
	titleText.g = IniReadInt(file, L"Title Text", L"Green");
	titleText.b = IniReadInt(file, L"Title Text", L"Blue");
	titleText.a = IniReadInt(file, L"Title Text", L"Alpha");
	titleFont = IniReadInt(file, L"Title Text", L"Font");
	// Title Rect
	titleRect.r = IniReadInt(file, L"Title Rect", L"Red");
	titleRect.g = IniReadInt(file, L"Title Rect", L"Green");
	titleRect.b = IniReadInt(file, L"Title Rect", L"Blue");
	titleRect.a = IniReadInt(file, L"Title Rect", L"Alpha");

	// Scroller
	scroller.r = IniReadInt(file, L"Scroller", L"Red");
	scroller.g = IniReadInt(file, L"Scroller", L"Green");
	scroller.b = IniReadInt(file, L"Scroller", L"Blue");
	scroller.a = IniReadInt(file, L"Scroller", L"Alpha");

	// Option Text
	options.r = IniReadInt(file, L"Options Text", L"Red");
	options.g = IniReadInt(file, L"Options Text", L"Green");
	options.b = IniReadInt(file, L"Options Text", L"Blue");
	options.a = IniReadInt(file, L"Options Text", L"Alpha");
	optionsFont = IniReadInt(file, L"Options Text", L"Font");

	// Option Rect
	optionsrect.r = IniReadInt(file, L"Options Rect", L"Red");
	optionsrect.g = IniReadInt(file, L"Options Rect", L"Green");
	optionsrect.b = IniReadInt(file, L"Options Rect", L"Blue");
	optionsrect.a = IniReadInt(file, L"Options Rect", L"Alpha");
}

void Menu::SaveMenuTheme(LPCWSTR file)
{
	// Title Text
	IniWriteInt(file, L"Title Text", L"Red", titleText.r);
	IniWriteInt(file, L"Title Text", L"Green", titleText.g);
	IniWriteInt(file, L"Title Text", L"Blue", titleText.b);
	IniWriteInt(file, L"Title Text", L"Alpha", titleText.a);
	IniWriteInt(file, L"Title Text", L"Font", titleFont);

	// Title Rect
	IniWriteInt(file, L"Title Rect", L"Red", titleRect.r);
	IniWriteInt(file, L"Title Rect", L"Green", titleRect.g);
	IniWriteInt(file, L"Title Rect", L"Blue", titleRect.b);
	IniWriteInt(file, L"Title Rect", L"Alpha", titleRect.a);

	// Scroller 
	IniWriteInt(file, L"Scroller", L"Red", scroller.r);
	IniWriteInt(file, L"Scroller", L"Green", scroller.g);
	IniWriteInt(file, L"Scroller", L"Blue", scroller.b);
	IniWriteInt(file, L"Scroller", L"Alpha", scroller.a);

	// Options Text
	IniWriteInt(file, L"Options Text", L"Red", options.r);
	IniWriteInt(file, L"Options Text", L"Green", options.g);
	IniWriteInt(file, L"Options Text", L"Blue", options.b);
	IniWriteInt(file, L"Options Text", L"Alpha", options.a);
	IniWriteInt(file, L"Options Text", L"Font", optionsFont);

	// Options Rect
	IniWriteInt(file, L"Options Rect", L"Red", optionsrect.r);
	IniWriteInt(file, L"Options Rect", L"Green", optionsrect.g);
	IniWriteInt(file, L"Options Rect", L"Blue", optionsrect.b);
	IniWriteInt(file, L"Options Rect", L"Alpha", optionsrect.a);
}

void Menu::EndMenu() {
	if (menulevel > 0)
	{
		if (optioncount > 16)
		{
			drawText((std::to_string(currentoption) + "/" + std::to_string(optioncount)).c_str(),
					 optionsFont, menux - 0.1f, (17 * 0.035f + menuy), 0.5f, 0.5f, titleText, false);
			drawRect(menux, (17 * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, titleRect);

			if (currentoption == 1) {
				drawSprite("commonmenu", "arrowright", menux, ((16 + 1) * 0.035f + (menuy + 0.0175f)/*0.145f*/), 0.02f, 0.02f, 90, titleText);
			}
			else if (currentoption == optioncount) {
				drawSprite("commonmenu", "arrowright", menux, ((16 + 1) * 0.035f + (menuy + 0.0175f)/*0.145f*/), 0.02f, 0.02f, 270, titleText);
			}
			else {
				drawSprite("commonmenu", "arrowright", menux, ((16 + 1) * 0.035f + (menuy + 0.0125f)/*0.14f*/), 0.02f, 0.02f, 270, titleText);
				drawSprite("commonmenu", "arrowright", menux, ((16 + 1) * 0.035f + (menuy + 0.0225f)/*0.15f*/), 0.02f, 0.02f, 90, titleText);
			}
		}
		else
		{
			drawText((std::to_string(currentoption) + "/" + std::to_string(optioncount)).c_str(),
					 optionsFont, menux - 0.1f, ((optioncount + 1) * 0.035f + menuy), 0.5f, 0.5f, titleText, false);
			drawRect(menux, ((optioncount + 1) * 0.035f + (menuy + 0.0165f)/*0.1415f*/), 0.23f, 0.035f, titleRect);

			if (currentoption == 1 && optioncount > 1) {
				drawSprite("commonmenu", "arrowright", menux, ((optioncount + 1) * 0.035f + (menuy + 0.0175f)/*0.145f*/), 0.02f, 0.02f, 90, titleText);
			}
			else if (currentoption == optioncount && optioncount > 1) {
				drawSprite("commonmenu", "arrowright", menux, ((optioncount + 1) * 0.035f + (menuy + 0.0175f)/*0.145f*/), 0.02f, 0.02f, 270, titleText);
			}
			else if (optioncount > 1) {
				drawSprite("commonmenu", "arrowright", menux, ((optioncount + 1) * 0.035f + (menuy + 0.0125f)/*0.14f*/), 0.02f, 0.02f, 270, titleText);
				drawSprite("commonmenu", "arrowright", menux, ((optioncount + 1) * 0.035f + (menuy + 0.0225f)/*0.15f*/), 0.02f, 0.02f, 90, titleText);
			}
		}


		UI::HIDE_HELP_TEXT_THIS_FRAME();
		CAM::SET_CINEMATIC_BUTTON_ACTIVE(0);
		UI::HIDE_HUD_COMPONENT_THIS_FRAME(10);
		UI::HIDE_HUD_COMPONENT_THIS_FRAME(6);
		UI::HIDE_HUD_COMPONENT_THIS_FRAME(7);
		UI::HIDE_HUD_COMPONENT_THIS_FRAME(9);
		UI::HIDE_HUD_COMPONENT_THIS_FRAME(8);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlNextCamera, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlPhone, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehicleCinCam, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlSelectCharacterMichael, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlSelectCharacterFranklin, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlSelectCharacterTrevor, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlSelectCharacterMultiplayer, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlCharacterWheel, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlMeleeAttackLight, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlMeleeAttackHeavy, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlMeleeAttackAlternate, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlMultiplayerInfo, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlMapPointOfInterest, true);

		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlRadioWheelLeftRight, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehicleNextRadio, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehiclePrevRadio, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehicleDuck, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehicleSelectNextWeapon, true);
		CONTROLS::DISABLE_CONTROL_ACTION(2, ControlVehicleSelectPrevWeapon, true);


		if (currentoption > optioncount) currentoption = optioncount;
		if (currentoption < 1) currentoption = 1;
	}
}

void Menu::CheckKeys(MenuControls* controls, std::function<void(void) > onMain, std::function<void(void) > onExit) {
	optionpress = false;
	if (GetTickCount() - delay > menuTime) {
		//if (getKeyPressed(VK_MULTIPLY) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendLb) &&
		//	CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendRb)) {
		if (controls->IsKeyJustPressed(MenuControls::MenuKey)) {
			if (menulevel > 0) { menuTime = menuTimeSlow; }
			if (menulevel == 0) {
				changeMenu("mainmenu");
				if (onMain) onMain();
			}
			else if (menulevel == 1) {
				backMenu();
				if (onExit) {
					CAM::SET_CINEMATIC_BUTTON_ACTIVE(1);
					onExit();
				}

			}
			delay = GetTickCount();
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuCancel) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendCancel)) {
			if (menulevel > 0) { menuTime = menuTimeSlow; }
			if (menulevel > 0) {
				if (menulevel == 1) {
					if (onExit) {
						CAM::SET_CINEMATIC_BUTTON_ACTIVE(1);
						onExit();
					}
				}
				backMenu();

			}
			delay = GetTickCount();
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuSelect) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendAccept)) {
			if (menulevel > 0) { menuTime = menuTimeSlow; }
			if (menulevel > 0) {
				menuBeep();
			}
			optionpress = true;
			delay = GetTickCount();
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuDown) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendDown)) {
			if (menulevel > 0) { menuTime = menuTimeMedium; }
			nextOption();
			delay = GetTickCount();
			downpress = true;
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuUp) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlFrontendUp)) {
			if (menulevel > 0) { menuTime = menuTimeMedium; }
			previousOption();
			delay = GetTickCount();
			uppress = true;
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuLeft) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlPhoneLeft)) {
			if (menulevel > 0) { menuTime = menuTimeFast; }
			if (menulevel > 0) {
				menuBeep();
			}
			leftpress = true;
			delay = GetTickCount();
		}
		if (controls->IsKeyJustPressed(MenuControls::MenuRight) || CONTROLS::IS_DISABLED_CONTROL_PRESSED(0, ControlPhoneRight)) {
			if (menulevel > 0) { menuTime = menuTimeFast; }
			if (menulevel > 0) {
				menuBeep();
			}
			rightpress = true;
			delay = GetTickCount();
		}
	}
}

void Menu::menuBeep()
{
	AUDIO::PLAY_SOUND_FRONTEND(-1, "NAV_UP_DOWN", "HUD_FRONTEND_DEFAULT_SOUNDSET", 0);
}

void Menu::resetButtonStates() {
	optionpress = false;
	leftpress = false;
	rightpress = false;
	uppress = false;
	downpress = false;
}

void Menu::CloseMenu() {
	while (menulevel > 0) {
		backMenu();
	}
}
