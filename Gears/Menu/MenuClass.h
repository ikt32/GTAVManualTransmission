#pragma once

/*
* Menu system that was originally from sudomod, but with a bunch of
* changes to make working with it easier.
*/

#include <string>
#include <windows.h>
#include <vector>
#include <functional>
#include <array>

class MenuControls;

struct rgba {
	int r, g, b, a;
};

class Menu {
public:
	Menu();
	~Menu();

	void Title(std::string  title);
	bool Option(std::string  option);
	void drawAdditionalInfoBox(std::vector<std::string> &extra, size_t infoLines);
	bool OptionPlus(std::string option, std::vector<std::string> &extra, bool *highlighted, std::function<void()> onRight, std::function<void()> onLeft);
	bool MenuOption(std::string  option, std::string  menu);
	bool IntOption(std::string  option, int *var, int min, int max, int step = 1);
	bool FloatOption(std::string  option, float *var, float min, float max, float step = 0.1);
	bool DoubleOption(std::string option, double *var, double min, double max, double step);
	bool BoolOption(std::string  option, bool *b00l);
	bool BoolSpriteOption(std::string  option, bool b00l, std::string  category, std::string  spriteOn, std::string  spriteOff);
	bool IntArray(std::string  option, int display[], int *PlaceHolderInt);
	bool FloatArray(std::string  option, float display[], int *PlaceHolderInt);
	bool StringArray(std::string option, std::vector<std::string> display, int *PlaceHolderInt);
	void TeleportOption(std::string  option, float x, float y, float z);

	bool CurrentMenu(std::string  menuname);

	void IniWriteInt(LPCWSTR file, LPCWSTR section, LPCWSTR key, int value);
	int IniReadInt(LPCWSTR file, LPCWSTR section, LPCWSTR key);

	void LoadMenuTheme(LPCWSTR file);
	void SaveMenuTheme(LPCWSTR file);

	void EndMenu();
	void CheckKeys(MenuControls* controls, std::function<void(void) > onMain, std::function<void(void) > onExit);
	void CloseMenu();

	int optionsFont = 6;
	int titleFont = 7;
	float menux = 0.2f;
	float menuy = 0.125f;
	rgba titleText = { 0, 0, 0, 255 };
	rgba titleRect = { 255, 200, 0, 255 };
	rgba scroller = { 80, 80, 80, 200 };
	rgba options = { 0, 0, 0, 255 };
	rgba optionsrect = { 255, 220, 30, 60 };

private:
	int optioncount = 0;
	int currentoption = 0;
	bool optionpress = false;
	bool leftpress = false;
	bool rightpress = false;
	bool uppress = false;
	bool downpress = false;

	std::array<std::string, 100> currentmenu;
	std::string actualmenu;
	int lastoption[100];
	int menulevel = 0;
	int infocount = 0;
	unsigned int delay = GetTickCount();

	const unsigned int menuTimeRepeat = 240;
	const unsigned int menuTimeSlow = 120;
	const unsigned int menuTimeMedium = 75;
	const unsigned int menuTimeFast = 40;
	unsigned int menuTime = menuTimeRepeat;

	void drawText(const std::string text, int font, float x, float y, float scalex, float scaley, rgba rgba, bool center);
	void drawRect(float x, float y, float width, float height, rgba rgba);
	void drawSprite(std::string  Streamedtexture, std::string  textureName, float x, float y, float width, float height, float rotation, rgba rgba);
	void changeMenu(std::string  menuname);
	void nextOption();
	void previousOption();
	void backMenu();
	void menuBeep();
	void resetButtonStates();
	bool useNative = true;

};
