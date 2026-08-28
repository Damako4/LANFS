#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <Config.hpp>

struct SslCtxDeleter {
    void operator()(SSL_CTX* ptr) const {
        if (ptr) {
            SSL_CTX_free(ptr);
        }
    }
};

struct BioDeleter {
    void operator()(BIO* ptr) const {
        if (ptr) {
            BIO_free_all(ptr);
        }
    }
};

struct SslDeleter {
    void operator()(SSL* ptr) const {
        if (ptr) {
            SSL_free(ptr);
        }
    }
};

class Application {
public:
    Application(const ApplicationConfig& config);
    void run();
    void handleSslSession(SSL* ssl) const;
private:
    ApplicationConfig config;
    std::unique_ptr<BIO, BioDeleter> acceptor;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
};