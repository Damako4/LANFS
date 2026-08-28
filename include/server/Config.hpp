#pragma once

#include <string>

struct ApplicationConfig {
    std::string hostport;
    long cacheSize;
    long timeout;
    size_t readBufferSize;
};