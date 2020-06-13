#include <curl/curl.h>
#include <fmt/format.h>

#include "Constants.h"
#include "Util/Logger.hpp"
#include "UpdateChecker.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "GDI32.LIB")
#pragma comment(lib, "ADVAPI32.LIB")
#pragma comment(lib, "CRYPT32.LIB")
#pragma comment(lib, "USER32.LIB")
#pragma comment(lib, "Wldap32.Lib")
#pragma comment(lib, "Normaliz.lib")

// Use after error buffer has been set. Returns out of calling func on failure.
#define CHECK_CURL_CODE(result, msg, errBuff, retValue) \
    if ((result) != CURLE_OK) {\
        logger.Write(ERROR, "[CURL] %s [%d - %s]", msg, result, errBuff);\
        return retValue;\
    }



size_t cbWriter(char *data, size_t size, size_t nmemb, std::string *writerData) {
    if (writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

bool init(CURL *&curl, std::string& dataBuffer, char* errBuff, const RepoInfo& repo) {
    curl = curl_easy_init();

    if (curl == nullptr) {
        logger.Write(ERROR, "[CURL] Failed to create CURL connection");
        return false;
    }

    CURLcode result = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errBuff);
    if (result != CURLE_OK) {
        logger.Write(ERROR, "[CURL] Failed to set error buffer [%d]", result);
        return false;
    }

    result = curl_easy_setopt(curl, CURLOPT_HEADER, repo.Header.c_str());
    CHECK_CURL_CODE(result, "Failed to set request header", errBuff, false);

    result = curl_easy_setopt(curl, CURLOPT_URL, repo.URL.c_str());
    CHECK_CURL_CODE(result, "Failed to set URL", errBuff, false);

    result = curl_easy_setopt(curl, CURLOPT_USERAGENT, repo.UserAgent.c_str());
    CHECK_CURL_CODE(result, "Failed to set user agent", errBuff, false);

    result = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CHECK_CURL_CODE(result, "Failed to set redirect option", errBuff, false);

    result = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cbWriter);
    CHECK_CURL_CODE(result, "Failed to set writer cb", errBuff, false);

    result = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dataBuffer);
    CHECK_CURL_CODE(result, "Failed to write buffer", errBuff, false);

    return true;
}

// E66666666, GTAVManualTransmission
ReleaseInfo GetLatestReleaseInfo(const RepoInfo& repoInfo) {
    const char* jTagName = "tag_name";
    const char* jPublished = "published_at";
    const char* jBody = "body";

    char errBuff[CURL_ERROR_SIZE];
    std::string dataBuffer;

    CURL* curl;

    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    CHECK_CURL_CODE(result, "Failed initializing CURL lib", errBuff, ReleaseInfo());

    if (!init(curl, dataBuffer, errBuff, repoInfo)) {
        return ReleaseInfo();
    }

    result = curl_easy_perform(curl);
    CHECK_CURL_CODE(result, "Failed performing request", errBuff, ReleaseInfo());

    curl_easy_cleanup(curl);

    auto offsetTagname = dataBuffer.find(jTagName);
    if (offsetTagname == std::string::npos) {
        logger.Write(ERROR, "Did not find %s in buffer. Dumping json:", jTagName);
        logger.Write(ERROR, dataBuffer);
        return ReleaseInfo();
    }

    auto offsetPublished = dataBuffer.find(jPublished);
    if (offsetPublished == std::string::npos) {
        logger.Write(ERROR, "Did not find %s in buffer. Dumping json:", jPublished);
        logger.Write(ERROR, dataBuffer);
        return ReleaseInfo();
    }

    auto offsetBody = dataBuffer.find(jBody);
    if (offsetBody == std::string::npos) {
        logger.Write(ERROR, "Did not find %s in buffer. Dumping json:", jBody);
        logger.Write(ERROR, dataBuffer);
        return ReleaseInfo();
    }

    // 4 comes from ":"X where we want index of X.
    offsetTagname = offsetTagname + strlen(jTagName) + 3;
    offsetPublished = offsetPublished + strlen(jPublished) + 3;
    offsetBody = offsetBody + strlen(jBody) + 3;

    std::string tagName = dataBuffer.substr(offsetTagname);
    tagName = tagName.substr(0, tagName.find('\"'));

    std::string published = dataBuffer.substr(offsetPublished);
    published = published.substr(0, published.find('\"'));

    std::string body = dataBuffer.substr(offsetBody);
    body = body.substr(0, body.find('\"'));

    unsigned char major, minor, patch;
    char suffix[32] = "";
    // Parse tag into versions. Suffix indicates alpha/beta/other stuff I could've added.
    int argsScanned = sscanf_s(tagName.c_str(), "v%hhu.%hhu.%hhu%s", &major, &minor, &patch, suffix, 32);

    if (argsScanned < 3) {
        logger.Write(ERROR, "Failed to parse tag_name [%s]", tagName.c_str());
        return ReleaseInfo();
    }

    ReleaseInfo relInfo{ major, minor, patch, suffix, tagName, published, body };

    return relInfo;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

bool CheckUpdate(ReleaseInfo& relInfo) {
    logger.Write(INFO, "Checking for updates");
    std::string url = fmt::format("https://api.github.com/repos/{}/{}/releases/latest", "E66666666", "GTAVManualTransmission");
    std::string ua = fmt::format("E66666666/MT{}", Constants::DisplayVersion);
    std::string header = "Accept: application/vnd.github.v3+json";

    relInfo = GetLatestReleaseInfo({ url, ua, header });

    if (!relInfo.MajorVersion) {
        logger.Write(ERROR, "Aborting version check");
        return false;
    }

    bool majorg = relInfo.MajorVersion > VERSION_MAJOR;
    bool minorg = relInfo.MinorVersion > VERSION_MINOR;
    bool patchg = relInfo.PatchVersion > VERSION_PATCH;

    bool majore = relInfo.MajorVersion == VERSION_MAJOR;
    bool minore = relInfo.MinorVersion == VERSION_MINOR;
    bool patche = relInfo.PatchVersion == VERSION_PATCH;

    relInfo.Body = ReplaceAll(relInfo.Body, R"(\r\n)", "\n");

    if (majorg ||
        majore && minorg ||
        majore && minore && patchg) {
        logger.Write(WARN, "Update available: [%s]", relInfo.Version.c_str());
        logger.Write(WARN, "Published at [%s]", relInfo.TimestampPublished.c_str());
        logger.Write(WARN, "Update description:\n%s", relInfo.Body.c_str());
        return true;
    }

    logger.Write(INFO, "No update available, latest version: [%s]", relInfo.Version.c_str());
    return false;
}
