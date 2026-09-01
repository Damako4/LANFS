#pragma once

#include <map>
#include <msgpack.hpp>
#include <string>

[[nodiscard]] std::string getLastSSLError();

std::string sha256(const std::string &str);

void saveHashMap(const std::string &filename, std::map<std::string, std::string> &fileHashes);

std::map<std::string, std::string> loadHashMap(const std::string &filename);