#pragma once

#include <string>
#include <vector>

struct ReleaseInfo {
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    unsigned char PatchVersion;
    std::string Suffix;

    std::string Version;
    std::string TimestampPublished;
    std::string Body;
};

struct RepoInfo {
    std::string URL;
    std::string UserAgent;
    std::string Header;
};

ReleaseInfo GetLatestReleaseInfo(const RepoInfo& repoInfo);

bool CheckUpdate(ReleaseInfo& relInfo);
