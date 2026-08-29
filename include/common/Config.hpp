#pragma once

#include <string>

struct ApplicationConfig {
    std::string hostname;
    std::string hostport;
    long cacheSize;
    long cacheTimeout;
    size_t readBufferSize;
    std::string sharedFolderPath;
};