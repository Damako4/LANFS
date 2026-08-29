#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <SslDeleters.hpp>
#include <Config.hpp>

class ClientApplication {
public:
    ClientApplication(const ApplicationConfig& config);
    void run();
private:
    ApplicationConfig config;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
};