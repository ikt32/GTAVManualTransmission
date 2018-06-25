#pragma once
#include <string>
#include <iostream>
#include <memory>

// https://gist.github.com/Zitrax/a2e0040d301bf4b8ef8101c0b1e3f1d5
// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf

/**
* Convert all std::strings to const char* using constexpr if (C++17)
*/
template<typename T>
auto convert(T&& t) {
    if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) {
        return std::forward<T>(t).c_str();
    }
    else {
        return std::forward<T>(t);
    }
}

/**
* printf like formatting for C++ with std::string
* Original source: https://stackoverflow.com/a/26221725/11722
*/
template<typename ... Args>
std::string stringFormatInternal(const std::string& format, Args&& ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args) ...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}

template<typename ... Args>
std::string fmt(const std::string& fmt, Args&& ... args) {
    return stringFormatInternal(fmt, convert(std::forward<Args>(args))...);
}

bool strfind(const std::string & strHaystack, const std::string & strNeedle);
