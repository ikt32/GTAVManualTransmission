#include "ScriptMenuUtils.h"

#include "script.h"
#include "AWD.h"
#include "Compatibility.h"
#include "Constants.h"
#include "WheelInput.h"

#include "Memory/VehicleExtensions.hpp"

#include "Util/AddonSpawnerCache.h"
#include "Util/Logger.hpp"
#include "Util/Paths.h"
#include "Util/Strings.hpp"
#include "Util/UIUtils.h"

#include <inc/natives.h>
#include <fmt/format.h>
#include <unordered_map>
#include <filesystem>

using VExt = VehicleExtensions;

extern ScriptSettings g_settings;

namespace {
    std::string getASCachedModelName(Hash model) {
        const std::unordered_map<Hash, std::string>& cache = ASCache::Get();
        if (!cache.empty()) {
            auto bla = cache.find(model);
            if (bla != cache.end())
                return bla->second;
        }
        return {};
    }
}

std::vector<std::string> ShowDynamicFfbCurve(float input, float gamma, int responseType) {
    std::string larr = "< ";
    std::string rarr = " >";
    if (gamma >= 5.0f - 0.01f) rarr = "";
    if (gamma <= 0.1f + 0.01f) larr = "";

    std::string printVar = fmt::format("{:.{}f}", gamma, 2);

    std::vector<std::string> info{
        "FFB response curve:",
        fmt::format("{}{}{}", larr, printVar, rarr)
    };

    const int max_samples = 100;
    std::vector<std::pair<float, float>> points;
    for (int i = 0; i < max_samples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(max_samples);
        float y = WheelInput::GetProfiledFFBValue(x, gamma, responseType);
        points.emplace_back(x, y);
    }

    float rectX = 0.5f;
    float rectY = 0.5f;
    float rectW = 0.40f / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float rectH = 0.40f;
    float blockW = rectW / max_samples;//0.001f * (16.0f / 9.0f) / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::GET_ASPECT_RATIO(FALSE);

    GRAPHICS::DRAW_RECT({ rectX, rectY },
        rectW + 3.0f * blockW, rectH + 3.0f * blockH,
        255, 255, 255, 191, 0);
    GRAPHICS::DRAW_RECT({ rectX, rectY },
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 239, 0);

    for (auto point : points) {
        float pointX = rectX - 0.5f * rectW + point.first * rectW;
        float pointY = rectY + 0.5f * rectH - point.second * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
            blockW, blockH,
            255, 255, 255, 255, 0);
    }

    std::pair<float, float> currentPoint = {
        input,
        WheelInput::GetProfiledFFBValue(input, gamma, responseType)
    };
    float pointX = rectX - 0.5f * rectW + currentPoint.first * rectW;
    float pointY = rectY + 0.5f * rectH - currentPoint.second * rectH;
    GRAPHICS::DRAW_RECT({ pointX, pointY },
        3.0f * blockW, 3.0f * blockH,
        255, 0, 0, 255, 0);

    return info;
}

std::vector<std::string> ShowGammaCurve(const std::string& axis, const float input, const float gamma) {
    std::string larr = "< ";
    std::string rarr = " >";
    if (gamma >= 5.0f - 0.01f) rarr = "";
    if (gamma <= 0.1f + 0.01f) larr = "";

    std::string printVar = fmt::format("{:.{}f}", gamma, 2);

    std::vector<std::string> info{
        fmt::format("{} gamma:", axis),
        fmt::format("{}{}{}", larr, printVar, rarr)
    };

    const int max_samples = 100;
    std::vector<std::pair<float, float>> points;
    for (int i = 0; i < max_samples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(max_samples);
        float y = pow(x, gamma);
        points.emplace_back(x, y);
    }

    float rectX = 0.5f;
    float rectY = 0.5f;
    float rectW = 0.40f / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float rectH = 0.40f;
    float blockW = rectW / max_samples;//0.001f * (16.0f / 9.0f) / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::GET_ASPECT_RATIO(FALSE);

    GRAPHICS::DRAW_RECT({ rectX, rectY },
        rectW + 3.0f * blockW, rectH + 3.0f * blockH,
        255, 255, 255, 191, 0);
    GRAPHICS::DRAW_RECT({ rectX, rectY },
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 239, 0);

    for (auto point : points) {
        float pointX = rectX - 0.5f * rectW + point.first * rectW;
        float pointY = rectY + 0.5f * rectH - point.second * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
            blockW, blockH,
            255, 255, 255, 255, 0);
    }

    std::pair<float, float> currentPoint = { input, pow(input, gamma) };
    float pointX = rectX - 0.5f * rectW + currentPoint.first * rectW;
    float pointY = rectY + 0.5f * rectH - currentPoint.second * rectH;
    GRAPHICS::DRAW_RECT({ pointX, pointY },
        3.0f * blockW, 3.0f * blockH,
        255, 0, 0, 255, 0);

    return info;
}

bool GetKbEntryInt(int& val) {
    UI::Notify(INFO, "Enter value");
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        fmt::format("{}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify(INFO, "Cancelled value entry");
        return false;
    }

    std::string intStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (intStr.empty()) {
        UI::Notify(INFO, "Cancelled value entry");
        return false;
    }

    char* pEnd;
    int parsedValue = strtol(intStr.c_str(), &pEnd, 10);

    if (parsedValue == 0 && *pEnd != 0) {
        UI::Notify(INFO, "Failed to parse entry.");
        return false;
    }

    val = parsedValue;
    return true;
}

bool GetKbEntryFloat(float& val) {
    UI::Notify(INFO, "Enter value");
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        fmt::format("{:f}", val).c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify(INFO, "Cancelled value entry");
        return false;
    }

    std::string floatStr = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (floatStr.empty()) {
        UI::Notify(INFO, "Cancelled value entry");
        return false;
    }

    char* pEnd;
    float parsedValue = strtof(floatStr.c_str(), &pEnd);

    if (parsedValue == 0.0f && *pEnd != 0) {
        UI::Notify(INFO, "Failed to parse entry.");
        return false;
    }

    val = parsedValue;
    return true;
}

std::string GetKbEntryStr(const std::string& existingString) {
    std::string val;
    UI::Notify(INFO, "Enter value");
    MISC::DISPLAY_ONSCREEN_KEYBOARD(LOCALIZATION::GET_CURRENT_LANGUAGE() == 0, "FMMC_KEY_TIP8", "",
        existingString.c_str(), "", "", "", 64);
    while (MISC::UPDATE_ONSCREEN_KEYBOARD() == 0) {
        WAIT(0);
    }
    if (!MISC::GET_ONSCREEN_KEYBOARD_RESULT()) {
        UI::Notify(INFO, "Cancelled value entry");
        return {};
    }

    std::string enteredVal = MISC::GET_ONSCREEN_KEYBOARD_RESULT();
    if (enteredVal.empty()) {
        UI::Notify(INFO, "Cancelled value entry");
        return {};
    }

    return enteredVal;
}

void SetFlags(unsigned& flagArea, std::string& newFlags) {
    if (!newFlags.empty()) {
        try {
            flagArea = std::stoul(newFlags, nullptr, 16);
        }
        catch (std::invalid_argument&) {
            UI::Notify(ERROR, "Error: Couldn't convert entered value to int.");
        }
        catch (std::out_of_range&) {
            UI::Notify(ERROR, "Error: Entered value out of range.");
        }
    }
    else {
        UI::Notify(ERROR, "Error: No flags entered.");
    }
}

std::vector<std::string> FormatVehicleConfig(const VehicleConfig& config, const std::vector<std::string>& gearboxModes) {
    std::string modelNames;
    for (auto it = config.ModelNames.begin(); it != config.ModelNames.end(); ++it) {
        modelNames += fmt::format("[{}]", *it);
        if (std::next(it) != config.ModelNames.end())
            modelNames += " ";
    }
    if (config.ModelNames.empty())
        modelNames = "None";

    std::string plates;
    for (auto it = config.Plates.begin(); it != config.Plates.end(); ++it) {
        plates += fmt::format("[{}]", *it);
        if (std::next(it) != config.Plates.end())
            plates += " ";
    }
    if (config.Plates.empty())
        plates = "None";

    std::string shiftAssist;
    if (config.ShiftOptions.UpshiftCut)
        shiftAssist += "Up";
    if (config.ShiftOptions.DownshiftBlip)
        shiftAssist += " & Down";
    if (shiftAssist.empty())
        shiftAssist = "None";

    EShiftMode shiftMode = config.MTOptions.ShiftMode;
    bool clutchCreep = config.MTOptions.ClutchCreep;

    bool absEn = config.DriveAssists.ABS.Enable;
    bool tcsEn = config.DriveAssists.TCS.Enable;
    bool espEn = config.DriveAssists.ESP.Enable;

    bool lsdEn = config.DriveAssists.LSD.Enable;
    bool lcEn = config.DriveAssists.LaunchControl.Enable;

    std::vector<std::string> extras{
        fmt::format("{}", config.Description),
        "Compatible cars:",
        fmt::format("\tModels: {}", modelNames),
        fmt::format("\tPlates: {}", plates),
        "Shifting options:",
        fmt::format("\tShift mode: {}", gearboxModes[shiftMode]),
        fmt::format("\tClutch creep: {}", clutchCreep),
        fmt::format("\tSequential assist: {}", shiftAssist),
        "Driving assists:",
        fmt::format("\t{}ABS {}TCS {}ESP",
            absEn ? "~g~" : "~r~",
            tcsEn ? "~g~" : "~r~",
            espEn ? "~g~" : "~r~"),
        fmt::format("\t{}LSD {}Launch",
            lsdEn ? "~g~" : "~r~",
            lcEn ? "~g~" : "~r~"),
        "Steering wheel:",
        fmt::format("\tSoft lock: {:.0f}", config.Steering.Wheel.SoftLock)
    };
    return extras;
}

void SaveVehicleConfig() {
    const std::string vehConfigDir = Paths::GetModPath() + "\\Vehicles";

    // Amazing, I haven't referred to this ever since throwing a menu in...
    extern Vehicle g_playerVehicle;
    // Pre-fill with actual model name, if Add-on Spawner is present.
    const std::string modelName = getASCachedModelName(ENTITY::GET_ENTITY_MODEL(g_playerVehicle));

    UI::ShowHelpText("Enter configuration name.");
    std::string fileName = GetKbEntryStr(modelName);
    if (fileName.empty()) {
        UI::Notify(INFO, "Cancelled configuration save.");
        return;
    }

    uint32_t saveFileSuffix = 0;
    std::string saveFile = fileName;
    bool duplicate;
    do {
        duplicate = false;
        for (const auto& p : std::filesystem::directory_iterator(vehConfigDir)) {
            if (p.path().stem() == saveFile) {
                duplicate = true;
                saveFile = fmt::format("{}_{:02d}", fileName.c_str(), saveFileSuffix++);
            }
        }
    } while (duplicate);

    if (saveFile != fileName) {
        UI::Notify(WARN, fmt::format("Duplicate filename(s) detected. Actual filename: {}", saveFile.c_str()));
    }

    std::string finalFile = fmt::format("{}\\{}.ini", vehConfigDir, saveFile);

    UI::ShowHelpText("Enter model name. Multiple models can be entered, separated by a space.");

    std::string userModels = GetKbEntryStr(modelName);

    if (userModels.empty()) {
        UI::Notify(INFO, "Cancelled configuration save.");
        return;
    }

    VehicleConfig config;
    config.SetFiles(&g_settings(), finalFile);
    config.LoadSettings();
    config.ModelNames = StrUtil::split(userModels, ' ');
    config.SaveSettings();
    loadConfigs();
    UI::Notify(INFO, fmt::format("Stored new configuration as {}", finalFile.c_str()));
}

std::vector<std::string> GetAWDInfo(Vehicle vehicle) {
    namespace HR = HandlingReplacement;

    void* handlingDataOrig = nullptr;
    void* handlingDataReplace = nullptr;

    if (!HR::Available()) {
        return { "HandlingReplacement.asi missing" };
    }

    bool replaced = HR::GetHandlingData(vehicle, &handlingDataOrig, &handlingDataReplace);

    if (handlingDataOrig == nullptr) {
        return {
            "Could not find handling data.",
            "Something went wrong within HandlingReplacement.asi.",
            "You shouldn't even be able to see this message."
        };
    }

    float driveBiasFOriginal = AWD::GetDriveBiasFront(handlingDataOrig);
    float driveBiasFCustom = g_settings().DriveAssists.AWD.CustomBaseBias;

    if (VExt::GetNumWheels(vehicle) != 4) {
        return { "Only vehicles with 4 wheels are supported." };
    }

    if (driveBiasFOriginal <= 0.0f || driveBiasFOriginal >= 1.0f) {
        return {
            (fmt::format("Drive layout: {}", driveBiasFOriginal == 0.0f ? "RWD" : "FWD")),
            "Not AWD compatible.",
            "Set fDriveBiasFront in handling data to between 0.10 and 0.90 for AWD support."
        };
    }

    if (driveBiasFCustom <= 0.0f || driveBiasFCustom >= 1.0f) {
        return {
            (fmt::format("Custom drive layout corrupt: Full {}", driveBiasFOriginal == 0.0f ? "RWD" : "FWD")),
            "This shouldn't happen.",
            "Reset custom drive layout to between 0.01 and 0.99."
        };
    }

    std::vector<std::string> awdInfo;

    float driveBiasFrontBase;
    float driveBiasFrontLive;
    if (replaced) {
        driveBiasFrontBase = g_settings().DriveAssists.AWD.UseCustomBaseBias ? driveBiasFCustom : driveBiasFOriginal;
        driveBiasFrontLive = AWD::GetDriveBiasFront(handlingDataReplace);
    }
    else {
        driveBiasFrontBase = driveBiasFOriginal;
    }

    awdInfo.emplace_back("Drive layout: AWD");
    std::string baseDistrStr = fmt::format("Base distribution: F{:.0f}/R{:.0f}", driveBiasFrontBase * 100.0f, (1.0f - driveBiasFrontBase) * 100.0f);
    if (replaced) {
        awdInfo.emplace_back("AWD mode: Adaptive");
        awdInfo.push_back(baseDistrStr);
        awdInfo.push_back(fmt::format("Live distribution: F{:.0f}/R{:.0f}", driveBiasFrontLive * 100.0f, (1.0f - driveBiasFrontLive) * 100.0f));
        awdInfo.push_back(fmt::format("Live transfer percentage: {:.0f}%", AWD::GetTransferValue() * 100.0f));
    }
    else {
        awdInfo.emplace_back("AWD mode: Static");
        awdInfo.push_back(baseDistrStr);
    }
    return awdInfo;
}
