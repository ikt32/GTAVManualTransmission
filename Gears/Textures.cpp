#include "Textures.h"

#include "Util/Logger.hpp"
#include "Util/Paths.h"

#include <inc/main.h>

// Oof...
int g_textureWheelId;
int g_textureAbsId;
int g_textureTcsId;
int g_textureEspId;
int g_textureBrkId;
int g_textureDsProtId;

void Textures::Init() {
    std::string absoluteModPath = Paths::GetModPath();
    std::string textureWheelFile = absoluteModPath + "\\texture_wheel.png";
    std::string textureABSFile = absoluteModPath + "\\texture_abs.png";
    std::string textureTCSFile = absoluteModPath + "\\texture_tcs.png";
    std::string textureESPFile = absoluteModPath + "\\texture_esp.png";
    std::string textureBRKFile = absoluteModPath + "\\texture_handbrake.png";
    std::string textureDsProtFile = absoluteModPath + "\\texture_downshift_protection.png";

    if (Paths::FileExists(textureWheelFile)) {
        g_textureWheelId = createTexture(textureWheelFile.c_str());
    }
    else {
        logger.Write(ERROR, textureWheelFile + " does not exist.");
        g_textureWheelId = -1;
    }

    if (Paths::FileExists(textureABSFile)) {
        g_textureAbsId = createTexture(textureABSFile.c_str());
    }
    else {
        logger.Write(ERROR, textureABSFile + " does not exist.");
        g_textureAbsId = -1;
    }

    if (Paths::FileExists(textureTCSFile)) {
        g_textureTcsId = createTexture(textureTCSFile.c_str());
    }
    else {
        logger.Write(ERROR, textureTCSFile + " does not exist.");
        g_textureTcsId = -1;
    }

    if (Paths::FileExists(textureESPFile)) {
        g_textureEspId = createTexture(textureESPFile.c_str());
    }
    else {
        logger.Write(ERROR, textureESPFile + " does not exist.");
        g_textureEspId = -1;
    }

    if (Paths::FileExists(textureBRKFile)) {
        g_textureBrkId = createTexture(textureBRKFile.c_str());
    }
    else {
        logger.Write(ERROR, textureBRKFile + " does not exist.");
        g_textureBrkId = -1;
    }

    if (Paths::FileExists(textureDsProtFile)) {
        g_textureDsProtId = createTexture(textureDsProtFile.c_str());
    }
    else {
        logger.Write(ERROR, textureDsProtFile + " does not exist.");
        g_textureDsProtId = -1;
    }
}
