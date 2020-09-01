#pragma once
#include <string>

void clearAxis(const std::string& confTag);
void clearButton(const std::string& confTag);
void clearWheelToKey();
void clearHShifter();
void clearASelect();
void clearKeyboardKey(const std::string& confTag);
void clearControllerButton(const std::string& confTag);
void clearLControllerButton(const std::string& confTag);

bool configAxis(const std::string& confTag);
bool configWheelToKey();
bool configButton(const std::string& confTag);
bool configHPattern();
bool configASelect();
bool configKeyboardKey(const std::string& confTag);
bool configControllerButton(const std::string& confTag);
bool configLControllerButton(const std::string& confTag);