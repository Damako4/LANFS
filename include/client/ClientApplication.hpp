#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <SslDeleters.hpp>
#include <Config.hpp>
#include <map>
#include <vector>

using HashMap = std::map<std::string, std::string>;

class ClientApplication {
public:
    ClientApplication(const ApplicationConfig& config);
    void run();
private:
    ApplicationConfig config;
    std::map<std::string, std::string> fileHashes;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
    std::vector<std::string> findDifferentHashes(HashMap& a, HashMap& b);
};