#include "Util.hpp"

STR2INT_ERROR str2int(int &i, char const *s, int base) {
    char *end;
    errno = 0;
    long l = strtol(s, &end, base);
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        return STR2INT_OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return STR2INT_UNDERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return STR2INT_INCONVERTIBLE;
    }
    i = l;
    return STR2INT_SUCCESS;
}

std::string ByteArrayToString(uint8_t *byteArray, size_t length) {
    std::string instructionBytes;
    for (int i = 0; i < length; ++i) {
        char buff[4];
        snprintf(buff, 4, "%02X ", byteArray[i]);
        instructionBytes += buff;
    }
    return instructionBytes;
}

std::string GUID2String(GUID guid) {
    wchar_t szGuidW[40] = {0};
    StringFromGUID2(guid, szGuidW, 40);
    std::wstring wGuid = szGuidW;
    return (std::string(wGuid.begin(), wGuid.end()));
}

GUID String2GUID(std::string guidStr) {
    GUID guid;
    std::wstring clsidStr;
    clsidStr.assign(guidStr.begin(), guidStr.end());
    HRESULT hr = CLSIDFromString(clsidStr.c_str(), &guid);
    if (hr != NOERROR) {
        return GUID();
    }
    return guid;
}
