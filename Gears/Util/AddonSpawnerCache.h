#pragma once
#include <inc/types.h>
#include <string>
#include <unordered_map>

namespace ASCache {
    std::unordered_map<Hash, std::string> Get();
}