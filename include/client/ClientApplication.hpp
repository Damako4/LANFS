#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <SslDeleters.hpp>
#include <Config.hpp>
#include <map>
#include <vector>

#define CHUNK_SIZE 65536

using SignatureMap = std::map<std::string, std::vector<char>>;

class ClientApplication {
public:
    ClientApplication(const ApplicationConfig& config);
    void run();
private:
    ApplicationConfig config;
    SignatureMap signatures;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
    void patchFiles(SignatureMap &serverDeltas);
};