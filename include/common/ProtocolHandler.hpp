#pragma once

#include <cstdint>
#include <openssl/ssl.h>
#include <string>
#include <map>
#include <vector>

#define FLAG_TEST 0x20

enum class Command : uint8_t {
    Signature = 0, // Sending signatures
    Delta = 1, // Sending deltas
    Update = 2, // Client updating file
    NotImplemented = 3 // Not implemented
};

struct ProtocolHeader {
    Command command;
    uint8_t flags;
    uint32_t streamLength;
};
 
class ProtocolHandler {
public:
    static bool readHeaderBytes(SSL *ssl, ProtocolHeader &outHeader);
    static void writeHeaderBytes(SSL* ssl, Command cmd, uint8_t flags, uint32_t streamLength);
    static void writeStreamBytes(SSL* ssl, const char *bytes, size_t size);
    static void readStreamBytes(SSL *ssl, std::string &buffer, size_t bytesToRead);
private:
    static bool readExact(SSL *ssl, void *destination, size_t bytesToRead);
    static void writeExact(SSL *ssl, const void *source, size_t bytesToWrite);
};