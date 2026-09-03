#pragma once

#include <map>
#include <msgpack.hpp>
#include <string>

[[nodiscard]] std::string getLastSSLError();

std::string sha256(const std::string &str);