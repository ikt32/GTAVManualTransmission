#include "StringFormat.h"
#include <locale>
#include <algorithm>

bool strfind(const std::string & strHaystack, const std::string & strNeedle) {
    auto it = std::search(
        strHaystack.begin(), strHaystack.end(),
        strNeedle.begin(), strNeedle.end(),
        [](char ch1, char ch2) { return tolower(ch1) == tolower(ch2); }
    );
    return (it != strHaystack.end());
}
