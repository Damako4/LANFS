#pragma once

#include <cstdint>
#include <openssl/ssl.h>
#include <string>
#include <optional>

#define FLAG_TEST 0x20

enum class Command : uint8_t {
    UpdateList = 0, // Client asking for file hashes to compare
    RequestList = 1, // Client asking for specific files
    DataStream = 2, // Server sending data
    NotImplemented = 3 // Not implemented
};

struct ProtocolHeader {
    Command command;
    uint8_t flags;
    uint32_t fileSize;
};
 
class ProtocolHandler {
public:
    static bool readHeaderBytes(SSL *ssl, ProtocolHeader &outHeader);
    static void writeHeaderBytes(SSL* ssl, Command cmd, uint8_t flags, uint32_t fileSize);
    static void writeStreamBytes(SSL* ssl, const char *bytes, size_t size);
    static void readStreamBytes(SSL *ssl, std::string &buffer, size_t bytesToRead);
private:
    static bool readExact(SSL *ssl, void *destination, size_t bytesToRead);
    static void writeExact(SSL *ssl, const void *source, size_t bytesToWrite);
};