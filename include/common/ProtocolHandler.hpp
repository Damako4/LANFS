#pragma once

#include <cstdint>
#include <openssl/ssl.h>

#define FLAG_TEST 0x20

enum class Command : uint8_t {
    UpdateList = 0,
    RequestList = 1,
    Ping = 2,
    Status = 3
};

struct ProtocolHeader {
    Command command;
    uint8_t flags;
    uint8_t fileSize;
};
 
class ProtocolHandler {
public:
    static ProtocolHeader readHeaderBytes(SSL* ssl);
    static void writeHeaderBytes(SSL* ssl, Command cmd, uint8_t flags, uint32_t fileSize);
};