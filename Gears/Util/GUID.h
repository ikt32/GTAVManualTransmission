#pragma once
#include <dinput.h>
#include <string>

// GUID utils
bool operator < (const GUID& guid1, const GUID& guid2);
std::string GUID2String(GUID guid);
GUID String2GUID(std::string guidStr);
