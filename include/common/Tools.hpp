#pragma once

#include <string>

[[nodiscard]] std::string getLastSSLError();

std::string sha256(const std::string& str);