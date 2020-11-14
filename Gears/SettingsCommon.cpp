#include "SettingsCommon.h"

void SetValue(CSimpleIniA& ini, const char* section, const char* key, int val) {
    ini.SetLongValue(section, key, val);
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, const std::string& val) {
    ini.SetValue(section, key, val.c_str());
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, bool val) {
    ini.SetBoolValue(section, key, val);
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, float val) {
    ini.SetDoubleValue(section, key, static_cast<double>(val));
}

int GetValue(CSimpleIniA& ini, const char* section, const char* key, int val) {
    return ini.GetLongValue(section, key, val);
}

std::string GetValue(CSimpleIniA& ini, const char* section, const char* key, const std::string& val) {
    return ini.GetValue(section, key, val.c_str());
}

bool GetValue(CSimpleIniA& ini, const char* section, const char* key, bool val) {
    return ini.GetBoolValue(section, key, val);
}

float GetValue(CSimpleIniA& ini, const char* section, const char* key, float val) {
    return static_cast<float>(ini.GetDoubleValue(section, key, val));
}
