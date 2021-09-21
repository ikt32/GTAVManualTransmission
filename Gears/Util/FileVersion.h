#pragma once
#include <string>

struct SVersion {
    int Minor;
    int Build;
};

bool operator==(const SVersion& a, const SVersion& b);

bool operator<=(const SVersion& a, const SVersion& b);

SVersion getExeVersion(const std::string& exe);
SVersion getExeInfo();
