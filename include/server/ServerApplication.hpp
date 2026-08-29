#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <Config.hpp>
#include <SslDeleters.hpp>
#include <map>
#include <ProtocolHandler.hpp>

class ServerApplication {
public:
    ServerApplication(const ApplicationConfig& config);
    void run();
    void handleSslSession(SSL* ssl) const;
private:
    ApplicationConfig config;
    std::map<std::string, std::string> fileHashes;
    std::unique_ptr<BIO, BioDeleter> acceptor;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
};