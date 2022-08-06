#pragma once
#include "VehicleConfig.h"
#include <inc/types.h>
#include <string>
#include <vector>

bool GetKbEntryFloat(float& val);
std::string GetKbEntryStr(const std::string& existingString);

std::vector<std::string> ShowDynamicFfbCurve(float input, float gamma, int responseType);
std::vector<std::string> ShowGammaCurve(const std::string& axis, const float input, const float gamma);

void SetFlags(unsigned& flagArea, std::string& newFlags);

std::vector<std::string> FormatVehicleConfig(const VehicleConfig& config, const std::vector<std::string>& gearboxModes);
void SaveVehicleConfig();

std::vector<std::string> GetAWDInfo(Vehicle vehicle);
