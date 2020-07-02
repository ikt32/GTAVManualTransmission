#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#endif

#include <string>

// GUID utils
bool operator < (const GUID& guid1, const GUID& guid2);
std::string GUID2String(GUID guid);
GUID String2GUID(std::string guidStr);
