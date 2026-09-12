#pragma once

#include <map>
#include <msgpack.hpp>
#include <string>

[[nodiscard]] std::string getLastSSLError();

/**
 * @brief Compute the SHA256 hash of @p str
 * 
 * @return String containing the hash
 */
std::string sha256(const std::string &str);