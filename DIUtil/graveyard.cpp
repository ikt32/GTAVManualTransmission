/*
 * Old DIUtil.cpp code
 */
#if 0
 /*
 * 1. User selects input to be configured
 *		- Show "Press W/A/S/D/Z for steer/throttle etc"
 * 2. User presses WASDZ to config axis
 *		- Show "Press 1-6 for axis etc"
 * 3. User presses 1-6 to choose which game axis is which hw axis
 *		- Show "Steer left/lift foot etc"
 *		- Show "Press Enter to register"
 * 4. User presses RETURN
 *		- Show "Steer right/press pedal to desired max pos"
 *		- Show "Press Enter to register"
 * 5. User presses RETURN
 *		- Save to file
 *		- We done now!
 * 0. User presses ESC
 *		- Cancel at any time, do not save.
 */
 // Why did I make this monstrosity?!

void configAxis(char c) {
	if (c != 'w' && c != 's' && c != 'c' && c != 'a' && c != 'd' && c != 'h') {
		return;
	}
	std::string gameAxis;
	std::string confTag;
	if (c == 'w') {
		gameAxis = "throttle";
		confTag = "THROTTLE";
	}
	if (c == 's') {
		gameAxis = "brake";
		confTag = "BRAKES";
	}
	if (c == 'c') {
		gameAxis = "clutch";
		confTag = "CLUTCH";
	}
	if (c == 'a' || c == 'd') {
		gameAxis = "steering";
		confTag = "STEER";
	}
	if (c == 'h') {
		gameAxis = "handbrake";
		confTag = "HANDBRAKE_ANALOG";
	}

	int progress = 0;
	int devEntry = -1;
	int devAxis = -1;
	std::array<std::string, 8> devAxisH{
		"lX	",
		"lY	",
		"lZ	",
		"lRx",
		"lRy",
		"lRz",
		"rglSlider0",
		"rglSlider1"
	};
	GUID devGUID = {};
	std::wstring wDevName;
	int minVal = -1;
	int maxVal = -1;

	cls();
	while (true) {
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
			if (progress == 0) { // Device selection
				for (int i = 0; i < controls.WheelDI.nEntry; i++) {
					if (c == i + '0') {
						devEntry = i;
						int devNumber = 0;
						for (auto guid : controls.WheelDI.GetGuids()) {
							if (devNumber == devEntry) {
								devGUID = guid;
							}
							devNumber++;
						}
						progress++;
					}
				}
				continue;
			}
			if (progress == 1) { // Axis selection
				for (int i = 1; i <= 8; i++) {
					if (c == i + '0') {
						devAxis = i;
						progress++;
					}
				}
				continue;
			}
			if (progress == 2 || progress == 3) { // Input setting
				if (c == 0x0D) { // Return
					if (progress == 2) { // save min
						minVal = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID);
					}
					if (progress == 3) { // save max
						maxVal = controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID);
					}
					progress++;
				}
				continue;
			}
		}

		switch (progress) {
			case 0: { // Device selection
				setCursorPosition(0, 0);
				int devNumber = 0;
				for (auto guid : controls.WheelDI.GetGuids()) {
					wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
					std::cout << devNumber << ": " << std::string(wDevName.begin(), wDevName.end()) << "\n";
					devNumber++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Select device for %s (0 to %d)", gameAxis.c_str(), controls.WheelDI.nEntry - 1);
			}
					break;
			case 1: { // Axis selection
				setCursorPosition(0, 0);
				std::cout << "Axes: ";
				int pRow = 1;
				for (int i = 0; i < WheelDirectInput::SIZEOF_DIAxis - 1; i++) {
					setCursorPosition(0, pRow);
					std::cout << "                                ";
					setCursorPosition(0, pRow);
					std::cout << std::to_string(i + 1) << ". " << controls.WheelDI.DIAxisHelper[i] << ": " << controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(i), devGUID) << "\n";
					pRow++;
				}

				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Select axis for %s", gameAxis.c_str());
			}
					break;
			case 2: { // Min (no input) register
				setCursorPosition(0, 0);
				for (int i = 0; i < csbi.srWindow.Right; i++) {
					printf(" ");
				}
				setCursorPosition(0, 0);
				printf("Min position: %d", controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID));

				setCursorPosition(0, csbi.srWindow.Bottom - 4);
				printf("Using axis %s", devAxisH[devAxis - 1].c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Min position for %s, Enter to confirm", gameAxis.c_str());
			}
					break;
			case 3: { // Max (full stomp) register
				setCursorPosition(0, 0);
				for (int i = 0; i < csbi.srWindow.Right; i++) {
					printf(" ");
				}
				setCursorPosition(0, 0);
				printf("Max position: %d", controls.WheelDI.GetAxisValue(static_cast<WheelDirectInput::DIAxis>(devAxis - 1), devGUID));

				setCursorPosition(0, csbi.srWindow.Bottom - 4);
				printf("Using axis %s", devAxisH[devAxis - 1].c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 2);
				printf("Using device %d: %s", devEntry, std::string(wDevName.begin(), wDevName.end()).c_str());
				setCursorPosition(0, csbi.srWindow.Bottom - 3);
				printf("Max position for %s, Enter to confirm", gameAxis.c_str());
			}
					break;
			default: { // save all the shit
				int index = settings.SteeringAppendDevice(devGUID, std::string(wDevName.begin(), wDevName.end()).c_str());
				settings.SteeringSaveAxis(confTag, index, devAxisH[devAxis - 1], minVal, maxVal);
				if (gameAxis == "steering") {
					settings.SteeringSaveFFBAxis(confTag, index, devAxisH[devAxis - 1]);
				}
				cls();
				setCursorPosition(0, csbi.srWindow.Bottom);
				printf("Saved changes");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				cls();
				init();
				return;
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s", gameAxis.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

void configButtons(char c) {
	if (c != '+' && c != '-' && c != '=' && c != '_') {
		return;
	}
	std::string gameButton;
	std::string confTag;
	int buttonsActive = 0;
	if (c == '+' || c == '=') {
		gameButton = "shift up";
		confTag = "SHIFT_UP";
	}
	if (c == '-' || c == '_') {
		gameButton = "shift down";
		confTag = "SHIFT_DOWN";
	}
	GUID devGUID = {};
	int button;
	std::string devName;

	cls();
	while (true) {
		controls.UpdateValues(ScriptControls::InputDevices::Wheel, false);
		if (_kbhit()) {
			cls();
			char c = _getch();
			if (c == 0x1B) { // ESC
				return;
			}
			if (c == 0x0D) { // RETURN
				if (buttonsActive > 1) {
					blankLines(0, 5);
					setCursorPosition(0, 0);
					printf("More than 1 button pressed. You can only select one.");
					std::this_thread::yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				else if (buttonsActive == 0) {
					blankLines(0, 5);
					setCursorPosition(0, 0);
					printf("No buttons pressed. Select one.");
					std::this_thread::yield();
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				else {
					int index = settings.SteeringAppendDevice(devGUID, devName.c_str());
					settings.SteeringSaveButton(confTag, index, button);
					cls();
					setCursorPosition(0, csbi.srWindow.Bottom);
					printf("Saved changes");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					cls();
					init();
					return;
				}

			}
		}

		blankLines(0, 5);
		buttonsActive = 0;
		devGUID = {};
		button = -1;
		setCursorPosition(0, 0);
		printf("Button for %s: ", gameButton.c_str());

		for (auto guid : controls.WheelDI.GetGuids()) {
			std::wstring wDevName = controls.WheelDI.FindEntryFromGUID(guid)->diDeviceInstance.tszInstanceName;
			devName = std::string(wDevName.begin(), wDevName.end()).c_str();
			for (int i = 0; i < 255; i++) {
				if (controls.WheelDI.IsButtonPressed(i, guid)) {
					printf("%d @ %s", i, devName.c_str());
					buttonsActive++;
					button = i;
					devGUID = guid;
				}
			}
			//POV hat shit
			std::string directionsStr = "?";
			std::vector<int> directions;
			directions.push_back(WheelDirectInput::POV::N);
			directions.push_back(WheelDirectInput::POV::NE);
			directions.push_back(WheelDirectInput::POV::E);
			directions.push_back(WheelDirectInput::POV::SE);
			directions.push_back(WheelDirectInput::POV::S);
			directions.push_back(WheelDirectInput::POV::SW);
			directions.push_back(WheelDirectInput::POV::W);
			directions.push_back(WheelDirectInput::POV::NW);
			for (auto d : directions) {
				if (controls.WheelDI.IsButtonPressed(d, guid)) {
					if (d == WheelDirectInput::N) directionsStr = "N ";
					if (d == WheelDirectInput::NE) directionsStr = "NE";
					if (d == WheelDirectInput::E) directionsStr = " E";
					if (d == WheelDirectInput::SE) directionsStr = "SE";
					if (d == WheelDirectInput::S) directionsStr = "S ";
					if (d == WheelDirectInput::SW) directionsStr = "SW";
					if (d == WheelDirectInput::W) directionsStr = " W";
					if (d == WheelDirectInput::NW) directionsStr = "NW";
					printf("%s (POV hat) @ %s", directionsStr.c_str(), devName.c_str());
					buttonsActive++;
					button = d;
					devGUID = guid;
				}
			}
		}

		setCursorPosition(0, csbi.srWindow.Bottom - 1);
		printf("Configuring %s", gameButton.c_str());
		setCursorPosition(0, csbi.srWindow.Bottom);
		std::cout << "ESC: Cancel config - ENTER: Confirm selection";
		std::cout.flush();
		std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

#endif
