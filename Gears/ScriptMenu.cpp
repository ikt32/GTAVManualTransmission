#define NOMINMAX
#include <string>

#include "inc/main.h"
#include "script.h"
#include "menu.h"

#include "ScriptSettings.hpp"
#include "Input/ScriptControls.hpp"
#include "Input/keyboard.h"
#include "Util/Util.hpp"
#include "Util/Versions.h"
#include "inc/natives.h"

extern ScriptSettings settings;
extern std::string settingsGeneralFile;
extern std::string settingsWheelFile;
extern std::string settingsMenuFile;

extern NativeMenu::Menu menu;

extern ScriptControls controls;
extern ScriptSettings settings;

extern int prevNotification;
extern int speedoIndex;

const std::string escapeKey = "BACKSPACE";
const std::string skipKey = "RIGHT";

// FontName, fontID
std::vector<std::string> fonts{
	{ "Chalet London" },
	{ "Sign Painter" },
	{ "Slab Serif" },
	{ "Chalet Cologne" },
	{ "Pricedown" },
};

std::vector<int> fontIDs{
	0,
	1,
	2,
	4,
	7
};

std::vector<std::string> buttonConfTags{
	{ "TOGGLE_MOD" },
	{ "CHANGE_SHIFTMODE" },
	{ "CLUTCH_BUTTON" },
	{ "SHIFT_UP" },
	{ "SHIFT_DOWN" },
	{ "ENGINE" },
	{ "HANDBRAKE" },
	{ "HORN" },
	{ "LIGHTS" },
	{ "LOOK_BACK" },
	{ "LOOK_LEFT" },
	{ "LOOK_RIGHT" },
	{ "CHANGE_CAMERA" },
	{ "RADIO_NEXT" },
	{ "RADIO_PREVIOUS" },
	{ "INDICATOR_LEFT" },
	{ "INDICATOR_RIGHT" },
	{ "INDICATOR_HAZARD" },
};

std::vector<std::string> keyboardConfTags{
	{ "Toggle" },
	{ "ToggleH" },
	{ "ShiftUp" },
	{ "ShiftDown" },
	{ "Clutch" },
	{ "Engine" },
	{ "Throttle" },
	{ "Brake" },
	{ "HR" },
	{ "H1" },
	{ "H2" },
	{ "H3" },
	{ "H4" },
	{ "H5" },
	{ "H6" },
	{ "H7" },
	{ "HN" },
};

std::vector<std::string> controllerConfTags{
	{ "Toggle" },
	{ "ToggleShift" },
	{ "ShiftUp" },
	{ "ShiftDown" },
	{ "Clutch" },
	{ "Engine" },
	{ "Throttle" },
	{ "Brake" }
};

std::vector<std::string> keyboardConfTagsDetail{
	{ "Toggle mod on/off" },
	{ "Switch shift mode" },
	{ "Shift up" },
	{ "Shift down" },
	{ "Hold for clutch" },
	{ "Toggle engine on/off" },
	{ "Key used for throttle" },
	{ "Key used for brake" },
	{ "H-pattern gear R press" },
	{ "H-pattern gear 1 press" },
	{ "H-pattern gear 2 press" },
	{ "H-pattern gear 3 press" },
	{ "H-pattern gear 4 press" },
	{ "H-pattern gear 5 press" },
	{ "H-pattern gear 6 press" },
	{ "H-pattern gear 7 press" },
	{ "H-pattern Neutral" },
};

std::vector<std::string> controllerConfTagDetail{
	{ "Toggle mod usage: hold" },
	{ "Toggle shift usage: hold" },
	{ "Shift up usage: press" },
	{ "Shift down usage: press" },
	{ "Clutch usage: axis or button" },
	{ "Engine usage: press" },
	{ "Throttle: axis or button" },
	{ "Brake: axis or button" }
};

std::vector<std::string> speedoTypes = {
	"off",
	"kph",
	"mph",
	"ms"
};

///////////////////////////////////////////////////////////////////////////////
//                             Menu stuff
///////////////////////////////////////////////////////////////////////////////
void menuInit() {

}

void menuClose() {
	settings.SaveGeneral();
	settings.SaveWheel(&controls);
	settings.SaveController(&controls);
}

void update_menu() {
	menu.CheckKeys();

	/* Yes hello I am root */
	if (menu.CurrentMenu("mainmenu")) {
		menu.Title("Manual Transmission", 0.90f);
		bool tempEnableRead = settings.EnableManual;

		if (menu.BoolOption("Enable manual transmission", tempEnableRead,
		{ "Enable or disable the entire mod." })) {
			toggleManual();
		}

		int shiftModeTemp = settings.ShiftMode;
		std::vector<std::string> gearboxModes = {
			"Sequential",
			"H-pattern",
			"Automatic"
		};

		menu.StringArray("Gearbox", gearboxModes, shiftModeTemp, { "Choose your gearbox!" });
		if (shiftModeTemp != settings.ShiftMode) {
			settings.ShiftMode = shiftModeTemp;
			setShiftMode(shiftModeTemp);
		}

		menu.MenuOption("Mod options", "optionsmenu", { "Find all mod-related settings here!" });
		menu.MenuOption("Controls", "controlsmenu", { "For your keyboard and controller." });
		menu.MenuOption("Wheel Options", "wheelmenu", { "Set up your steering wheel." });
		menu.MenuOption("HUD Options", "hudmenu", { "Wanna move HUD elements around or",
						"disable things? Imperial or metric?" });
		//menu.MenuOption("Menu Options", "menumenu"); 
		menu.MenuOption("Debug", "debugmenu", { "Technical details and options." });

		int activeIndex = 0;
		std::string activeInputName;
		switch (controls.PrevInput) {
			case ScriptControls::Keyboard:
				activeInputName = "Keyboard";
				break;
			case ScriptControls::Controller:
				activeInputName = "Controller";
				break;
			case ScriptControls::Wheel:
				activeInputName = "Wheel";
				break;
		}
		std::vector<std::string> active = { activeInputName };
		menu.StringArray("Active input", active, activeIndex,
		{ "Active input is automatically detected." });

		int versionIndex = 0;
		std::vector<std::string> version = { DISPLAY_VERSION };
		menu.StringArray("Version", version, versionIndex,
		{ "Thank you for using this mod!","- ikt" });
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("optionsmenu")) {
		menu.Title("Mod options");
		if (menu.BoolOption("Engine Damage", settings.EngDamage,
		{ "Damage the engine when over-revving and when mis-shifting." })) {
		}
		if (menu.BoolOption("Engine Stalling (H)", settings.EngStall,
		{ "Stall the engine when the wheel speed gets too low. Applies to H-pattern shift mode." })) {
		}
		if (menu.BoolOption("Engine Stalling (S)", settings.EngStallS,
		{ "Stall the engine when the wheel speed gets too low. Allies to sequential shift mode." })) {
		}
		if (menu.BoolOption("Engine Braking", settings.EngBrake,
		{ "Help the car braking by slowing down more","at high RPMs" })) {
		}
		if (menu.BoolOption("Clutch Bite", settings.ClutchCatching,
		{ "Simulate clutch biting action and clutch","interaction at near-stop speeds." })) {
		}
		if (menu.BoolOption("Clutch Shift (S)", settings.ClutchShiftingS,
		{ "Require holding the clutch to shift", "in sequential mode." })) {
		}
		if (menu.BoolOption("Clutch Shift (H)", settings.ClutchShiftingH,
		{ "Require holding the clutch to shift", "in H-pattern mode." })) {
		}
		if (menu.BoolOption("Default Neutral", settings.DefaultNeutral,
		{ "The car will be in neutral when you get in." })) {
		}
		if (menu.MenuOption("Fine-tuning", "finetuneoptionsmenu",
		{ "Fine-tune the parameters above" })) {
		}
		if (menu.MenuOption("Misc. options", "miscoptionsmenu",
		{ "Options that don't really have to do with","the gearbox simulation." })) {
		}
	}

	if (menu.CurrentMenu("miscoptionsmenu")) {
		menu.Title("Misc. options");
		if (menu.BoolOption("Simple Bike", settings.SimpleBike,
		{ "Disables bike engine stalling and the","clutch bite simulation." })) {
		}
		if (menu.BoolOption("Hill gravity workaround", settings.HillBrakeWorkaround, 
		{ "Gives the car a push to overcome","the games' default brakes at a stop." })) {}
		if (menu.BoolOption("Auto gear 1", settings.AutoGear1,
		{ "Automatically switch to first gear","when the car reaches a standstill." })) {
		}
		if (menu.BoolOption("Auto look back", settings.AutoLookBack,
		{ "Automatically look back whenever in","reverse gear." })) {
		}
		if (menu.BoolOption("Clutch + throttle start", settings.ThrottleStart,
		{ "Allow to start the engine by pressing","clutch and throttle, like in DiRT Rally." })) {
		}
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("finetuneoptionsmenu")) {
		menu.Title("Fine-tuning");
		if (menu.FloatOption("Clutch bite point", settings.ClutchCatchpoint, 0.0f, 1.0f, 0.05f)) {}
		if (menu.FloatOption("Stalling threshold", settings.StallingThreshold, 0.0f, 1.0f, 0.05f)) {}
		if (menu.FloatOption("RPM Damage", settings.RPMDamage, 0.0f, 10.0f, 0.05f)) {}
		if (menu.IntOption("Misshift Damage", settings.MisshiftDamage, 0, 100, 5)) {}
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("controlsmenu")) {
		menu.Title("Controls");

		menu.MenuOption("Controller", "controllermenu",{"Change controller control assignments."});
		menu.MenuOption("Keyboard", "keyboardmenu",{"Change keyboard control assignments."});
		menu.BoolOption("Non-Xinput controller", controls.UseLegacyController,
		{ "If you needed to set up your controller in","the pause menu, you should enable this." });
		menu.BoolOption("Block car controls", settings.BlockCarControls, { "Blocks car action controls like ducking, switching guns, handbrake, aim."
						" Holding activates the original button again. Experimental! Only works for XInput." } );
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("controllermenu")) {
		menu.Title("Controller controls");

		if (menu.BoolOption("Engine button turns off too", settings.ToggleEngine,
		{ "When checked, the engine button turns off",
			"the engine. Otherwise, the button only", "turns on the engine when it's off." })) {
		}
		if (menu.IntOption("Long press time (ms)", controls.CToggleTime, 100, 5000, 100)) {}

		// wtf ikt
		float currTriggerValue = controls.GetXboxTrigger();
		float prevTriggerValue = currTriggerValue;
		if (menu.FloatOption("Trigger value", currTriggerValue, 0.25, 1.0, 0.05,
		{ "Threshold for an analog input to","be detected as digital input." })) {
		}
		if (currTriggerValue != prevTriggerValue) {
			controls.SetXboxTrigger(currTriggerValue);
		}

		std::vector<std::string> controllerInfo;
		controllerInfo.push_back("Press RIGHT to clear key");
		controllerInfo.push_back("Press RETURN to configure button");
		controllerInfo.push_back("");

		auto it = 0;
		for (auto confTag : controllerConfTags) {
			controllerInfo.back() = controllerConfTagDetail.at(it);
			controllerInfo.push_back("Assigned to " + controls.ConfTagController2Value(confTag));
			if (menu.OptionPlus("Assign " + confTag, controllerInfo, std::bind(clearControllerButton, confTag), nullptr, "Current setting")) {
				bool result = configControllerButton(confTag);
				//showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				WAIT(1000);
			}
			it++;
			controllerInfo.pop_back();
		}
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("keyboardmenu")) {
		menu.Title("Keyboard controls");

		std::vector<std::string> keyboardInfo;
		keyboardInfo.push_back("Press RIGHT to clear key");
		keyboardInfo.push_back("Press RETURN to configure key");
		keyboardInfo.push_back("");

		int it = 0;
		for (auto confTag : keyboardConfTags) {
			keyboardInfo.back() = keyboardConfTagsDetail.at(it);
			keyboardInfo.push_back("Assigned to " + key2str(controls.ConfTagKB2key(confTag)));
			if (menu.OptionPlus("Assign " + confTag, keyboardInfo, std::bind(clearKeyboardKey, confTag), nullptr, "Current setting")) {
				bool result = configKeyboardKey(confTag);
				if (!result) showNotification(("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
				WAIT(1000);
			}
			it++;
			keyboardInfo.pop_back();
		}
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("wheelmenu")) {
		menu.Title("Wheel options");
		if (menu.BoolOption("Enable wheel", settings.EnableWheel,
		{ "Enable usage of a steering wheel." })) {
			settings.SaveWheel(&controls);
		}
		if (menu.BoolOption("Enable wheel without MT", settings.WheelWithoutManual,
		{ "Enable your wheel even when the","Manual Transmission part of this mod is off." })) {
			settings.SaveWheel(&controls);
		}
		if (menu.BoolOption("Enable wheel for boats & planes", settings.AltControls,
		{ "Enable wheel input for boats and planes." })) {
			settings.SaveWheel(&controls);
		}
		if (menu.BoolOption("Patch steering", settings.PatchSteering,
		{ "Patches steering reduction and ", "automatic countersteer for direct control." })) {
			settings.SaveWheel(&controls);
			initSteeringPatches();
		}
		if (menu.BoolOption("Patch steering for all inputs", settings.PatchSteeringAlways,
		{ "Also patch this for keyboard and controller." })) {
			settings.SaveWheel(&controls);
			initSteeringPatches();
		}
		if (menu.BoolOption("Logitech LEDs (can crash!)", settings.LogiLEDs)) { settings.SaveWheel(&controls); }
		menu.MenuOption("Steering wheel axis setup", "axesmenu", 
		{ "Configure analog controls, like throttle,","steering and the like." });
		menu.MenuOption("Steering wheel button setup", "buttonsmenu",
		{ "Set up your buttons on your steering wheel." });
		menu.MenuOption("Force feedback options", "forcefeedbackmenu",
		{ "Fine-tune your force feedback parameters." });
		menu.MenuOption("Steering wheel angles", "anglemenu",
		{ "Steering lock options are here." });
		
		std::vector<std::string> hpatInfo;
		hpatInfo.push_back("Press RIGHT to clear H-pattern shifter");
		hpatInfo.push_back("Active gear:");
		if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) hpatInfo.push_back("Reverse");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) hpatInfo.push_back("Gear 1");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) hpatInfo.push_back("Gear 2");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) hpatInfo.push_back("Gear 3");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) hpatInfo.push_back("Gear 4");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) hpatInfo.push_back("Gear 5");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) hpatInfo.push_back("Gear 6");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) hpatInfo.push_back("Gear 7");

		if (menu.OptionPlus("H-pattern shifter setup", hpatInfo, std::bind(clearHShifter), nullptr, "Input values")) {
			bool result = configHPattern();
			showNotification(result ? "H-pattern shifter saved" : "Cancelled H-pattern shifter setup", &prevNotification);
		}

		menu.BoolOption("Keyboard H-pattern", settings.HPatternKeyboard, 
		{ "This'll allow you to also use the keyboard", "controls for wheel H-shifting. Configure", "controls in the keyboard section." });
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("anglemenu")) {
		menu.Title("Wheel angles");

		if (menu.FloatOption("Physical degrees", settings.SteerAngleMax, 180.0, 1080.0, 60.0)) {
			if (settings.SteerAngleCar > settings.SteerAngleMax) { settings.SteerAngleCar = settings.SteerAngleMax; }
			if (settings.SteerAngleBike > settings.SteerAngleMax) { settings.SteerAngleBike = settings.SteerAngleMax; }
			if (settings.SteerAngleAlt > settings.SteerAngleMax) { settings.SteerAngleAlt = settings.SteerAngleMax; }
		}
		menu.FloatOption("Car soft lock", settings.SteerAngleCar, 180.0, settings.SteerAngleMax, 60.0,
		{"Soft lock for cars, trucks and anything","a kid would call \"car\" (degrees)"});
		menu.FloatOption("Bike soft lock", settings.SteerAngleBike, 180.0, settings.SteerAngleMax, 60.0,
		{ "Soft lock for bikes (degrees)" });
		menu.FloatOption("Boat/Plane soft lock", settings.SteerAngleAlt, 180.0, settings.SteerAngleMax, 60.0,
		{ "Soft lock for boats and planes (in degrees)" });
		if (menu.FloatOption("Steering Multiplier", settings.GameSteerMult, 0.1, 2.0, 0.01, 
		{ "Increase steering lock for all cars." })) {
			updateSteeringMultiplier();
		}
	}


	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("axesmenu")) {
		menu.Title("Configure axes");

		std::vector<std::string> info = {
			"Press RIGHT to clear this axis" ,
			"Steer    : " + std::to_string(controls.SteerVal),
			"Throttle : " + std::to_string(controls.ThrottleVal),
			"Brake    : " + std::to_string(controls.BrakeVal),
			"Clutch   : " + std::to_string(controls.ClutchValRaw),
			"Handbrake: " + std::to_string(controls.HandbrakeVal),
		};

		if (menu.OptionPlus("Calibrate steering", info, std::bind(clearAxis, "STEER"), nullptr, "Input values")) {
			bool result = configAxis("STEER");
			showNotification(result ? "Steering axis saved" : "Cancelled steering axis calibration", &prevNotification);
			if (result) initWheel();
		}
		if (menu.OptionPlus("Calibrate throttle", info, std::bind(clearAxis, "THROTTLE"), nullptr, "Input values")) {
			bool result = configAxis("THROTTLE");
			showNotification(result ? "Throttle axis saved" : "Cancelled throttle axis calibration", &prevNotification);
			if (result) initWheel();
		}
		if (menu.OptionPlus("Calibrate brake", info, std::bind(clearAxis, "BRAKES"), nullptr, "Input values")) {
			bool result = configAxis("BRAKES");
			showNotification(result ? "Brake axis saved" : "Cancelled brake axis calibration", &prevNotification);
			if (result) initWheel();
		}
		if (menu.OptionPlus("Calibrate clutch", info, std::bind(clearAxis, "CLUTCH"), nullptr, "Input values")) {
			bool result = configAxis("CLUTCH");
			showNotification(result ? "Clutch axis saved" : "Cancelled clutch axis calibration", &prevNotification);
			if (result) initWheel();
		}
		if (menu.OptionPlus("Calibrate handbrake", info, std::bind(clearAxis, "HANDBRAKE_ANALOG"), nullptr, "Input values")) {
			bool result = configAxis("HANDBRAKE_ANALOG");
			showNotification(result ? "Handbrake axis saved" : "Cancelled handbrake axis calibration", &prevNotification);
			if (result) initWheel();
		}
		menu.FloatOption("Throttle anti-deadzone",	controls.ADZThrottle, 0.0f, 1.0f, 0.01f, { "GTA V ignores 25% input for throttle by default." });
		menu.FloatOption("Brake anti-deadzone",		controls.ADZBrake, 0.0f, 1.0f, 0.01f, { "GTA V ignores 25% input for brake by default." });

		menu.BoolOption("Invert steer", controls.InvertSteer);
		menu.BoolOption("Invert throttle", controls.InvertThrottle);
		menu.BoolOption("Invert brake", controls.InvertBrake);
		menu.BoolOption("Invert clutch", controls.InvertClutch);
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("forcefeedbackmenu")) {
		menu.Title("Force feedback");

		menu.BoolOption("Enable", settings.EnableFFB);
		menu.FloatOption("Global multiplier", settings.FFGlobalMult, 0.0f, 10.0f, 1.0f);
		menu.IntOption("Damper Max (low speed)", settings.DamperMax, 0, 200, 1);
		menu.IntOption("Damper Min (high speed)", settings.DamperMin, 0, 200, 1);
		menu.FloatOption("Damper Min speed (m/s)", settings.TargetSpeed, 0.0f, 40.0f, 0.2f);
		menu.FloatOption("Physics strength", settings.PhysicsStrength, 0.0f, 10.0f, 0.1f,
		{ "Force feedback effect strength by ","physics events like cornering and collisions." });
		menu.FloatOption("Detail strength", settings.DetailStrength, 0.0f, 10.0f, 0.1f,
		{ "Force feedback effect strength by ","suspension events due to road conditions,",
			"like potholes, speedbumps and peds." });
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("buttonsmenu")) {
		menu.Title("Configure buttons");
		std::vector<std::string> info;
		info.push_back("Press RIGHT to clear this button");
		info.push_back("Active buttons:");
		if (controls.ButtonIn(ScriptControls::WheelControlType::HR)) info.push_back("Gear R");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H1)) info.push_back("Gear 1");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H2)) info.push_back("Gear 2");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H3)) info.push_back("Gear 3");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H4)) info.push_back("Gear 4");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H5)) info.push_back("Gear 5");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H6)) info.push_back("Gear 6");
		if (controls.ButtonIn(ScriptControls::WheelControlType::H7)) info.push_back("Gear 7");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftUp))	info.push_back("ShiftUp");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ShiftDown)) info.push_back("ShiftDown");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Clutch))	info.push_back("ClutchButton");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Engine))	info.push_back("Engine");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Handbrake)) info.push_back("Handbrake");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Horn))		info.push_back("Horn");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Lights))	info.push_back("Lights");
		if (controls.ButtonIn(ScriptControls::WheelControlType::LookBack))	info.push_back("LookBack");
		if (controls.ButtonIn(ScriptControls::WheelControlType::LookLeft))	info.push_back("LookLeft");
		if (controls.ButtonIn(ScriptControls::WheelControlType::LookRight))	info.push_back("LookRight");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Camera))	info.push_back("Camera");
		if (controls.ButtonIn(ScriptControls::WheelControlType::RadioNext)) info.push_back("RadioNext");
		if (controls.ButtonIn(ScriptControls::WheelControlType::RadioPrev)) info.push_back("RadioPrev");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorLeft))		info.push_back("IndicatorLeft");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorRight))	info.push_back("IndicatorRight");
		if (controls.ButtonIn(ScriptControls::WheelControlType::IndicatorHazard))	info.push_back("IndicatorHazard");
		if (controls.ButtonIn(ScriptControls::WheelControlType::Toggle))	info.push_back("ToggleMod");
		if (controls.ButtonIn(ScriptControls::WheelControlType::ToggleH))	info.push_back("ChangeShiftMode");
		for (int i = 0; i < MAX_RGBBUTTONS; i++) {
			if (controls.WheelToKey[i] != -1) {
				if (controls.WheelControl.IsButtonPressed(i, controls.WheelToKeyGUID)) {
					info.push_back(key2str(controls.WheelToKey[i]));
				}
			}
		}

		for (auto confTag : buttonConfTags) {
			if (menu.OptionPlus("Assign " + confTag, info, std::bind(clearButton, confTag), nullptr, "Current inputs")) {
				bool result = configButton(confTag);
				showNotification(result ? (confTag + " saved").c_str() : ("Cancelled " + confTag + " assignment").c_str(), &prevNotification);
			}
		}
	}

	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("hudmenu")) {
		menu.Title("HUD Options");

		menu.BoolOption("Enable", settings.HUD);
		menu.BoolOption("Always enable", settings.AlwaysHUD, { "Display HUD even if manual transmission is off." });
		int fontIndex = static_cast<int>(std::find(fontIDs.begin(), fontIDs.end(), settings.HUDFont) - fontIDs.begin());
		int oldIndex = fontIndex;
		menu.StringArray("Font: ", fonts, fontIndex);
		if (fontIndex != oldIndex) {
			settings.HUDFont = fontIDs.at(fontIndex);
		}

		menu.MenuOption("Gear and shift mode", "geardisplaymenu");
		menu.MenuOption("Speedometer", "speedodisplaymenu");
		menu.MenuOption("RPM Gauge", "rpmdisplaymenu");
		menu.MenuOption("Wheel & Pedal Info", "wheelinfomenu");
	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("geardisplaymenu")) {
		menu.Title("Gear options");

		// prolly gear section
		menu.FloatOption("Gear X", settings.GearXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Gear Y", settings.GearYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Gear Size", settings.GearSize, 0.0f, 3.0f, 0.05f);
		menu.IntOption("Gear Top Color Red", settings.GearTopColorR, 0, 255);
		menu.IntOption("Gear Top Color Green", settings.GearTopColorG, 0, 255);
		menu.IntOption("Gear Top Color Blue", settings.GearTopColorB, 0, 255);

		menu.FloatOption("Shift Mode X", settings.ShiftModeXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Shift Mode Y", settings.ShiftModeYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Shift Mode Size", settings.ShiftModeSize, 0.0f, 3.0f, 0.05f);

	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("speedodisplaymenu")) {
		menu.Title("Speedometer options");
		// prolly speedo section
		ptrdiff_t oldPos = std::find(speedoTypes.begin(), speedoTypes.end(), settings.Speedo) - speedoTypes.begin();
		menu.StringArray("Speedometer", speedoTypes, speedoIndex);
		if (speedoIndex != oldPos) {
			settings.Speedo = speedoTypes.at(speedoIndex);
		}
		menu.BoolOption("Show units", settings.SpeedoShowUnit);

		menu.FloatOption("Speedometer X", settings.SpeedoXpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Speedometer Y", settings.SpeedoYpos, 0.0f, 1.0f, 0.005f);
		menu.FloatOption("Speedometer Size", settings.SpeedoSize, 0.0f, 3.0f, 0.05f);

	}

	/* Yes hello I am root - 2 */
	if (menu.CurrentMenu("rpmdisplaymenu")) {
		menu.Title("RPM Gauge options");
		// prolly RPM section
		menu.BoolOption("RPM Gauge", settings.RPMIndicator);
		menu.FloatOption("RPM Redline", settings.RPMIndicatorRedline, 0.0f, 1.0f, 0.01f);

		menu.FloatOption("RPM X", settings.RPMIndicatorXpos, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Y", settings.RPMIndicatorYpos, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Width", settings.RPMIndicatorWidth, 0.0f, 1.0f, 0.0025f);
		menu.FloatOption("RPM Height", settings.RPMIndicatorHeight, 0.0f, 1.0f, 0.0025f);

		menu.IntOption("RPM Background Red	", settings.RPMIndicatorBackgroundR, 0, 255);
		menu.IntOption("RPM Background Green", settings.RPMIndicatorBackgroundG, 0, 255);
		menu.IntOption("RPM Background Blue	", settings.RPMIndicatorBackgroundB, 0, 255);
		menu.IntOption("RPM Background Alpha", settings.RPMIndicatorBackgroundA, 0, 255);

		menu.IntOption("RPM Foreground Red	", settings.RPMIndicatorForegroundR, 0, 255);
		menu.IntOption("RPM Foreground Green", settings.RPMIndicatorForegroundG, 0, 255);
		menu.IntOption("RPM Foreground Blue	", settings.RPMIndicatorForegroundB, 0, 255);
		menu.IntOption("RPM Foreground Alpha", settings.RPMIndicatorForegroundA, 0, 255);

		menu.IntOption("RPM Redline Red		", settings.RPMIndicatorRedlineR, 0, 255);
		menu.IntOption("RPM Redline Green	", settings.RPMIndicatorRedlineG, 0, 255);
		menu.IntOption("RPM Redline Blue	", settings.RPMIndicatorRedlineB, 0, 255);
		menu.IntOption("RPM Redline Alpha	", settings.RPMIndicatorRedlineA, 0, 255);

		menu.IntOption("RPM Revlimit Red	", settings.RPMIndicatorRevlimitR, 0, 255);
		menu.IntOption("RPM Revlimit Green	", settings.RPMIndicatorRevlimitG, 0, 255);
		menu.IntOption("RPM Revlimit Blue	", settings.RPMIndicatorRevlimitB, 0, 255);
		menu.IntOption("RPM Revlimit Alpha	", settings.RPMIndicatorRevlimitA, 0, 255);
	}

	if (menu.CurrentMenu("wheelinfomenu")) {
		menu.Title("Wheel & Pedal info");
		
		menu.BoolOption("Display steering wheel info", settings.SteeringWheelInfo, { "Show input info graphically." });
		menu.FloatOption("Wheel image X", settings.SteeringWheelTextureX, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Wheel image Y", settings.SteeringWheelTextureY, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Wheel image size", settings.SteeringWheelTextureSz, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals X", settings.PedalInfoX, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals Y", settings.PedalInfoY, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals Height", settings.PedalInfoH	, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals Width", settings.PedalInfoW	, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals Pad X", settings.PedalInfoPadX, 0.0f, 1.0f, 0.01f);
		menu.FloatOption("Pedals Pad Y", settings.PedalInfoPadY, 0.0f, 1.0f, 0.01f);

	}
	/* Yes hello I am root - 1 */
	if (menu.CurrentMenu("debugmenu")) {
		menu.Title("Debug settings");
		if (menu.BoolOption("Display info", settings.DisplayInfo,
		{ "Show all detailed technical info of the gearbox and inputs calculations." })) { }
		if (menu.BoolOption("Display car wheel info", settings.DisplayWheelInfo,
		{ "Show per-wheel debug info with off-ground detection, lockup detection and suspension info." })) { }
		if (menu.BoolOption("Log car address", settings.LogCar,
		{ "Prints the current vehicle address","to Gears.log." })) { }
		if (menu.BoolOption("Expose script variables", settings.CrossScript,
		{ "Shares data like gear, shifting","indicator and Neutral with other mods." })) { }
	}


	menu.EndMenu();
}


///////////////////////////////////////////////////////////////////////////////
//                              Config helpers/util
///////////////////////////////////////////////////////////////////////////////

// Wheel section
// Look at that argument list! :D
bool getConfigAxisWithValues(std::vector<std::tuple<GUID, std::string, int>> startStates, GUID &selectedGUID, std::string &selectedAxis, int hyst, bool &positive, int &startValue_) {
	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			for (auto startState : startStates) {
				std::string axisName = controls.WheelControl.DIAxisHelper[i];
				int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
				int startValue = std::get<2>(startState);
				if (std::get<0>(startState) == guid &&
					std::get<1>(startState) == axisName) {
					startValue_ = startValue;
					if (axisValue > startValue + hyst) { // 0 (min) - 65535 (max)
						selectedGUID = guid;
						selectedAxis = axisName;
						positive = true;
						return true;
					}
					if (axisValue < startValue - hyst) { // 65535 (min) - 0 (max)
						selectedGUID = guid;
						selectedAxis = axisName;
						positive = false;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void saveAxis(const std::string &confTag, GUID devGUID, std::string axis, int min, int max) {
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end());
	auto index = settings.SteeringAppendDevice(devGUID, devName);
	settings.SteeringSaveAxis(confTag, index, axis, min, max);
	if (confTag == "STEER") {
		settings.SteeringSaveFFBAxis(confTag, index, axis);
	}
	settings.Read(&controls);
}

void saveButton(const std::string &confTag, GUID devGUID, int button) {
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
	auto index = settings.SteeringAppendDevice(devGUID, devName.c_str());
	settings.SteeringSaveButton(confTag, index, button);
	settings.Read(&controls);
}

void saveHShifter(const std::string &confTag, GUID devGUID, std::array<int, numGears> buttonArray) {
	std::wstring wDevName = controls.WheelControl.FindEntryFromGUID(devGUID)->diDeviceInstance.tszInstanceName;
	std::string devName = std::string(wDevName.begin(), wDevName.end()).c_str();
	auto index = settings.SteeringAppendDevice(devGUID, devName);
	settings.SteeringSaveHShifter(confTag, index, buttonArray.data());
	settings.Read(&controls);
}

void clearAxis(std::string confTag) {
	settings.SteeringSaveAxis(confTag, -1, "", 0, 0);
	settings.Read(&controls);
	showNotification(("Cleared axis " + confTag).c_str(), &prevNotification);
	initWheel();
}

void clearButton(std::string confTag) {
	settings.SteeringSaveButton(confTag, -1, -1);
	settings.Read(&controls);
	showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

void clearHShifter() {
	int empty[numGears] = {};
	for (int i = 0; i < numGears; i++) {
		empty[i] = -1;
	}
	settings.SteeringSaveHShifter("SHIFTER", -1, empty);
	settings.Read(&controls);
	showNotification("Cleared H-pattern shifter", &prevNotification);
}

// Controller and keyboard
void saveKeyboardKey(std::string confTag, std::string key) {
	settings.KeyboardSaveKey(confTag, key);
	settings.Read(&controls);
	showNotification(("Saved key " + confTag + ": " + key).c_str(), &prevNotification);
}

void saveControllerButton(std::string confTag, std::string button, int btnToBlock) {
	settings.ControllerSaveButton(confTag, button, btnToBlock);
	settings.Read(&controls);
	showNotification(("Saved button " + confTag + ": " + button).c_str(), &prevNotification);
}

void clearKeyboardKey(std::string confTag) {
	settings.KeyboardSaveKey(confTag, "UNKNOWN");
	settings.Read(&controls);
	showNotification(("Cleared key " + confTag).c_str(), &prevNotification);
}

void clearControllerButton(std::string confTag) {
	settings.ControllerSaveButton(confTag, "UNKNOWN", -1);
	settings.Read(&controls);
	showNotification(("Cleared button " + confTag).c_str(), &prevNotification);
}

///////////////////////////////////////////////////////////////////////////////
//                              Config inputs
///////////////////////////////////////////////////////////////////////////////
// Wheel
bool configAxis(std::string str) {

	std::string confTag = str;
	std::string additionalInfo = "Press " + escapeKey + " to exit.";

	if (str == "STEER") {
		additionalInfo += " Steer right to register axis.";
	}
	else if (str == "HANDBRAKE_ANALOG") {
		additionalInfo += " Fully pull and set back handbrake to register axis.";
	}
	else {
		additionalInfo += " Fully press and release the " + confTag + " pedal to register axis.";
	}

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);
	// Save current state
	std::vector<std::tuple<GUID, std::string, int>> startStates;
	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
			std::string axisName = controls.WheelControl.DIAxisHelper[i];
			int axisValue = controls.WheelControl.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), guid);
			startStates.push_back(std::tuple<GUID, std::string, int>(guid, axisName, axisValue));
		}
	}

	// Ignore inputs before selection if they moved less than hyst
	int hyst = 65536 / 8;

	// To track direction of physical <-> digital value.
	bool positive = true;

	GUID selectedGUID;
	std::string selectedAxis;
	int startValue;
	int endValue;

	// going down!
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);
		if (getConfigAxisWithValues(startStates, selectedGUID, selectedAxis, hyst, positive, startValue)) {
			break;
		}
		showSubtitle(additionalInfo);
		WAIT(0);
	}

	// I'll just assume no manufacturer uses inane non-full range steering wheel values
	if (confTag == "STEER") {
		int min = (positive ? 0 : 65535);
		int max = (positive ? 65535 : 0);
		saveAxis(confTag, selectedGUID, selectedAxis, min, max);
		return true;
	}

	int prevAxisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(selectedAxis), selectedGUID);

	// and up again!
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		int axisValue = controls.WheelControl.GetAxisValue(controls.WheelControl.StringToAxis(selectedAxis), selectedGUID);

		if (positive && axisValue < prevAxisValue) {
			endValue = prevAxisValue;
			break;
		}

		if (!positive && axisValue > prevAxisValue) {
			endValue = prevAxisValue;
			break;
		}

		prevAxisValue = axisValue;

		showSubtitle(additionalInfo);
		WAIT(0);
	}

	int min = startValue;
	int max = endValue;
	saveAxis(confTag, selectedGUID, selectedAxis, min, max);
	return true;
}

bool configButton(std::string str) {
	int buttonsActive = 0;

	std::array<int, 8> directions = {
		WheelDirectInput::POV::N,
		WheelDirectInput::POV::NE,
		WheelDirectInput::POV::E,
		WheelDirectInput::POV::SE,
		WheelDirectInput::POV::S,
		WheelDirectInput::POV::SW,
		WheelDirectInput::POV::W,
		WheelDirectInput::POV::NW,
	};

	std::string confTag = str;
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	additionalInfo += " Press a button to set " + confTag + ".";

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < 255; i++) {
			if (controls.WheelControl.IsButtonPressed(i, guid)) {
				buttonsActive++;
			}
		}
		for (auto d : directions) {
			if (controls.WheelControl.IsButtonPressed(d, guid)) {
				buttonsActive++;
			}
		}
	}

	if (buttonsActive > 0) {
		showSubtitle("One or more buttons had been pressed on start. Stop pressing and try again.");
		return false;
	}

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		for (auto guid : controls.WheelControl.GetGuids()) {
			for (int i = 0; i < 255; i++) {
				if (controls.WheelControl.IsButtonPressed(i, guid)) {
					saveButton(confTag, guid, i);
					return true;
				}
			}

			//POV hat shit
			for (auto d : directions) {
				if (controls.WheelControl.IsButtonPressed(d, guid)) {
					saveButton(confTag, guid, d);
					return true;
				}
			}
		}
		showSubtitle(additionalInfo.c_str());
		WAIT(0);
	}
}

bool configHPattern() {
	int buttonsActive = 0;

	std::string confTag = "SHIFTER";
	std::string additionalInfo = "Press " + escapeKey + " to exit. Press " + skipKey + " to skip gear.";

	controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

	for (auto guid : controls.WheelControl.GetGuids()) {
		for (int i = 0; i < 255; i++) {
			if (controls.WheelControl.IsButtonPressed(i, guid)) {
				buttonsActive++;
			}
		}
	}

	if (buttonsActive > 0) {
		showSubtitle("One or more buttons had been pressed on start. Stop pressing and try again.");
		return false;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////

	GUID devGUID = {};
	std::array<int, numGears> buttonArray; // There are gears 1-7 + R
	std::fill(buttonArray.begin(), buttonArray.end(), -1);

	int progress = 0;

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		if (IsKeyJustUp(str2key(skipKey))) {
			progress++;
		}

		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false, true);

		for (auto guid : controls.WheelControl.GetGuids()) {
			for (int i = 0; i < 255; i++) {
				// only find unregistered buttons
				if (controls.WheelControl.IsButtonPressed(i, guid) &&
					find(begin(buttonArray), end(buttonArray), i) == end(buttonArray)) {
					if (progress == 0) { // also save device info when just started
						devGUID = guid;
						buttonArray[progress] = i;
						progress++;
					}
					else if (guid == devGUID) { // only accept same device onwards
						buttonArray[progress] = i;
						progress++;
					}
				}
			}
		}


		if (progress > 7) {
			break;
		}
		std::string gearDisplay;
		switch (progress) {
			case 0: gearDisplay = "Reverse"; break;
			case 1: gearDisplay = "1st gear"; break;
			case 2: gearDisplay = "2nd gear"; break;
			case 3: gearDisplay = "3rd gear"; break;
			case 4: case 5: case 6:
			case 7: gearDisplay = std::to_string(progress) + "th gear"; break;
			default: gearDisplay = "?"; break;
		}
		showSubtitle("Shift into " + gearDisplay + ". " + additionalInfo);
		WAIT(0);
	}
	saveHShifter(confTag, devGUID, buttonArray);
	return true;
}

// Keyboard
bool isMenuControl(int control) {
	if (control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuKey] ||
		control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuSelect] ||
		control == menu.GetControls().ControlKeys[NativeMenu::MenuControls::ControlType::MenuCancel]) {
		return true;
	}
	return false;
}

bool configKeyboardKey(const std::string &confTag) {
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		auto keymap = createKeyMap();
		for (auto k : keymap) {
			if (isMenuControl(k.second))
				continue;
			if (IsKeyJustUp(k.second)) {
				saveKeyboardKey(confTag, k.first);
				return true;
			}
		}

		// Key limits from keyboard.cpp
		// a-z
		for (int i = 0x30; i <= 0x39; i++) {
			if (isMenuControl(i))
				continue;
			if (IsKeyJustUp(i)) {
				std::string str;
				str = static_cast<char>(i);
				saveKeyboardKey(confTag, str);
				return true;
			}
		}
		// A-Z
		for (int i = 0x41; i <= 0x5A; i++) {
			if (isMenuControl(i))
				continue;
			if (IsKeyJustUp(i)) {
				std::string str;
				str = static_cast<char>(i);
				saveKeyboardKey(confTag, str);
				return true;
			}
		}

		showSubtitle("Press " + confTag + ". Menu controls can't be chosen." + additionalInfo);
		WAIT(0);
	}
}

std::vector<eControl> controlsToBeBlocked = {
	ControlVehicleAim,
	ControlVehicleAttack,
	ControlVehicleDuck,
	ControlVehicleHandbrake,
	ControlVehicleSelectNextWeapon,
};

// Controller
int lastVehicleButtonPressed() {
	for (auto control : controlsToBeBlocked) {
		if (CONTROLS::IS_CONTROL_JUST_PRESSED(0, control))
			return control;
	}
	return -1;
}

bool configControllerButton(const std::string &confTag) {
	std::string additionalInfo = "Press " + escapeKey + " to exit.";
	XboxController* rawController = controls.GetRawController();
	if (rawController == nullptr)
		return false;

	int buttonToBlock = -1;

	while (true) {
		if (IsKeyJustUp(str2key(escapeKey))) {
			return false;
		}
		controls.UpdateValues(ScriptControls::InputDevices::Controller, false, true);
		if (buttonToBlock == -1) {
			buttonToBlock = lastVehicleButtonPressed();
		}
		for (std::string buttonHelper : rawController->XboxButtonsHelper) {
			auto button = rawController->StringToButton(buttonHelper);
			if (rawController->IsButtonJustPressed(button, controls.GetButtonState())) {
				saveControllerButton(confTag, buttonHelper, buttonToBlock);
				return true;
			}
		}
		showSubtitle("Press " + confTag + ". " + additionalInfo);
		WAIT(0);
	}
}
