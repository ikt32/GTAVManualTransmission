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
    return StrUtil::utf8_encode(wGuid);
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

std::vector<std::string> StrUtil::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    StrUtil::split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string StrUtil::toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string StrUtil::utf8_encode(const std::wstring& wstr) {
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring StrUtil::utf8_decode(const std::string& str) {
    if (str.empty())
        return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

bool SysUtil::IsWindowFocused() {
    auto foregroundHwnd = GetForegroundWindow();
    DWORD foregroundProcId;
    GetWindowThreadProcessId(foregroundHwnd, &foregroundProcId);
    auto currentProcId = GetCurrentProcessId();
    return foregroundProcId == currentProcId;
}