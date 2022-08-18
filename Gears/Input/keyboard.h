#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

bool IsKeyDown(DWORD key);
bool IsKeyJustUp(DWORD key);

DWORD GetKeyFromName(const std::string& name);
std::string GetNameFromKey(DWORD key);
const std::unordered_map<std::string, int>& GetKeyMap();
