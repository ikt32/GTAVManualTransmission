#include "GUID.h"
#include "Strings.hpp"

std::string GUID2String(GUID guid) {
    wchar_t szGuidW[40] = { 0 };
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
