#pragma once

#include <cstdint>
#include <openssl/ssl.h>
#include <string>
#include <map>
#include <vector>

#define FLAG_TEST 0x20

/**
 * @brief Thrown when the peer closes the connection cleanly before
 * any bytes of a new message were read.
 *
 * Distinct from std::runtime_error, which represents a genuine
 * protocol/TLS failure (e.g. a connection dropping mid-message).
 */
class ConnectionClosed : public std::exception {
public:
    const char* what() const noexcept override {
        return "Connection closed by peer";
    }
};

/**
 * @brief Enum defining the command being sent / recieved
 */
enum class Command : uint8_t {
    Signature = 0, ///<  Sending / recieving a signature
    Delta = 1, ///< Sending / recieving a delta
    Update = 2, ///< Sending / recieving file update
    NotImplemented = 3 ///< Not implemented
};

/**
 * @brief Header sent before stream bytes
 */
struct ProtocolHeader {
    Command command; ///< Command being sent / received
    uint8_t flags; ///< Flags (not implemented)
    uint32_t streamLength; ///< Size of stream to expect
};
 
class ProtocolHandler {
public:
    /**
     * @brief Read ProtocolHeader from @p ssl into @p outHeader
     * 
     * @param ssl Pointer to the SSL connection to read from
     * @param outHeader A reference to the ProtocolHeader
     */
    static void readHeaderBytes(SSL *ssl, ProtocolHeader &outHeader);
    static void writeHeaderBytes(SSL* ssl, Command cmd, uint8_t flags, uint32_t streamLength);
    static void writeStreamBytes(SSL* ssl, const char *bytes, size_t size);
    static void readStreamBytes(SSL *ssl, std::string &buffer, size_t bytesToRead);
private:
    /**
     * @brief Read exactly @p bytesToRead bytesfrom the @p ssl connection into @p destination 
     * 
     * @param ssl Pointer to the SSL connection to read from
     * @param destination Buffer to read into
     * @param bytesToRead The number of bytes to read
     */
    static void readExact(SSL *ssl, void *destination, size_t bytesToRead);

    /**
     * @brief Write exactly @p bytesToWrite bytes from buffer @p source into connection @p ssl
     * 
     * @param ssl Pointer to the SSL connection to read from
     * @param destination Buffer to read from
     * @param bytesToRead The number of bytes to write 
     */
    static void writeExact(SSL *ssl, const void *source, size_t bytesToWrite);
};