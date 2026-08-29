#pragma once

#include <openssl/ssl.h>
#include <openssl/bio.h>

struct SslDeleter {
    void operator()(SSL* ptr) const {
        if (ptr) {
            SSL_free(ptr);
        }
    }
};

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