#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);

bool IsKeyDown(DWORD key);
bool IsKeyJustUp(DWORD key, bool exclusive = true);

DWORD GetKeyFromName(const std::string& name);
std::string GetNameFromKey(DWORD key);
const std::unordered_map<std::string, int>& GetKeyMap();
