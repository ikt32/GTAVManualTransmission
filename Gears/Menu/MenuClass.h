/* MenuClass.h */ /* Taken from SudoMod*/

#include <string>
#include <windows.h>
#include <vector>
#include <functional>

class Controls;

struct rgba {
	int r, g, b, a;
};

extern float menux;
extern rgba titleText;
extern rgba titleRect;
extern rgba scroller;
extern rgba options;
extern rgba optionsrect;

class Menu {
public:
	Menu();
	~Menu();

	void Title(char* title);
	bool Option(char* option);
	void drawAdditionalInfoBox(std::vector<std::string> &extra, size_t infoLines);
	bool OptionPlus(char *option, std::vector<std::string> &extra, bool *highlighted, std::function<void()> onRight, std::function<void()> onLeft);
	bool MenuOption(char* option, char* menu);
	bool IntOption(char* option, int *var, int min, int max, int step = 1);
	bool FloatOption(char* option, float *var, float min, float max, float step = 0.1);
	bool DoubleOption(char *option, double *var, double min, double max, double step);
	bool BoolOption(char* option, bool *b00l);
	bool BoolSpriteOption(char* option, bool b00l, char* category, char* spriteOn, char* spriteOff);
	bool IntArray(char* option, int display[], int *PlaceHolderInt);
	bool FloatArray(char* option, float display[], int *PlaceHolderInt);
	bool CharArray(char* option, char* display[], int *PlaceHolderInt);
	void TeleportOption(char* option, float x, float y, float z);

	bool CurrentMenu(char* menuname);

	void IniWriteInt(LPCWSTR file, LPCWSTR section, LPCWSTR key, int value);
	int IniReadInt(LPCWSTR file, LPCWSTR section, LPCWSTR key);

	void LoadMenuTheme(LPCWSTR file);
	void SaveMenuTheme(LPCWSTR file);

	void EndMenu();
	void CheckKeys(Controls* controls, std::function<void(void) > onMain, std::function<void(void) > onExit);
	void CloseMenu();

private:
	int optionsFont = 6;
	int titleFont = 7;
	float menux = 0.2f;
	rgba titleText = { 0, 0, 0, 255 };
	rgba titleRect = { 255, 200, 0, 255 };
	rgba scroller = { 80, 80, 80, 200 };
	rgba options = { 0, 0, 0, 255 };
	rgba optionsrect = { 255, 220, 30, 60 };

	int optioncount = 0;
	int currentoption = 0;
	bool optionpress = false;
	bool leftpress = false;
	bool rightpress = false;
	bool uppress = false;
	bool downpress = false;

	char* currentmenu[100];
	char* actualmenu = "";
	int lastoption[100];
	int menulevel = 0;
	int infocount = 0;
	int delay = GetTickCount();

	void drawText(const char *text, int font, float x, float y, float scalex, float scaley, rgba rgba, bool center);
	void drawRect(float x, float y, float width, float height, rgba rgba);
	void drawSprite(char* Streamedtexture, char* textureName, float x, float y, float width, float height, float rotation, rgba rgba);
	void changeMenu(char* menuname);
	void nextOption();
	void previousOption();
	void backMenu();
	void menuBeep();
	void resetButtonStates();

};