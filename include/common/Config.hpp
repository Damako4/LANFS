#pragma once

#include <string>

/**
 * @brief Runtime configuration for either the client or server application
 */
struct ApplicationConfig {
    std::string hostname; ///< Server hostname or IP to bind / connect to
    std::string hostport; ///< Port number as string
    std::string sharedFolderPath; ///< Path to shared folder
    long cacheSize; ///< SSL session cache size (server only)
    long cacheTimeout; ///< SSL session cache timeout in seconds
    size_t readBufferSize; ///< Size of read buffer
};