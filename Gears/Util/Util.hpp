#pragma once

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#include <algorithm>
#endif
#include <dinput.h>

#include <string>
#include <vector>
#include <sstream>

enum STR2INT_ERROR {
    STR2INT_SUCCESS, 
    STR2INT_OVERFLOW, 
    STR2INT_UNDERFLOW, 
    STR2INT_INCONVERTIBLE
};

STR2INT_ERROR str2int(int &i, char const *s, int base = 0);

std::string ByteArrayToString(uint8_t *byteArray, size_t length);

constexpr unsigned long joaat(const char* s) {
    unsigned long hash = 0;
    for (; *s != '\0'; ++s) {
        auto c = *s;
        if (c >= 0x41 && c <= 0x5a) {
            c += 0x20;
        }
        hash += c;
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

template<typename E>
constexpr auto EToInt(E e) -> typename std::underlying_type<E>::type {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

// GUID utils
bool operator < (const GUID& guid1, const GUID& guid2);
std::string GUID2String(GUID guid);
GUID String2GUID(std::string guidStr);

namespace StrUtil {
    template<typename Out>
    void split(const std::string& s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    std::vector<std::string> split(const std::string& s, char delim);

    std::string toLower(std::string s);

    //https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
    // Convert a wide Unicode string to an UTF8 string
    std::string utf8_encode(const std::wstring& wstr);

    // Convert an UTF8 string to a wide Unicode String
    std::wstring utf8_decode(const std::string& str);
}

namespace SysUtil {
    bool IsWindowFocused();
}
