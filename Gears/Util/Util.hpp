#pragma once

#include <string>
#include <vector>
#include <inc/types.h>

enum STR2INT_ERROR {
    STR2INT_SUCCESS, 
    STR2INT_OVERFLOW, 
    STR2INT_UNDERFLOW, 
    STR2INT_INCONVERTIBLE
};

STR2INT_ERROR str2int(int &i, char const *s, int base = 0);

std::string ByteArrayToString(byte *byteArray, size_t length);
