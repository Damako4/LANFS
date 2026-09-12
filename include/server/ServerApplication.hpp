#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <Config.hpp>
#include <SslDeleters.hpp>
#include <map>
#include <ProtocolHandler.hpp>

using SignatureMap = std::map<std::string, std::vector<char>>;
using DeltaMap = std::map<std::string, std::vector<char>>;

class ServerApplication {
public:
    /**
     * @brief Initializes the server's SSL context, loads TLS certificates,
     * binds the acceptor socket, and initialize @ref FileHandler with the 
     * shared folder in @p config
     *
     * @param config Application settings
     */
    ServerApplication(const ApplicationConfig& config);

    
    /**
     * @brief Run the acceptor loop for clients, handling each client with 
     * @ref handleSSLSession
     */
    void run();

    /**
     * @brief Handles an incoming SSL session and dispatch commands
     * 
     * Reads protocol headers and dispatches based on Command type
     * @param ssl The active SSL connection to the client
     */
    void handleSSLSession(SSL* ssl) const;
private:
    ApplicationConfig config; ///< ApplicationConfig for the server

    SignatureMap serverSignatures; ///< SignatureMap containing the servers latest signatures
    SignatureMap serverDeltas; ///< @ref DeltaMap containing the servers latest deltas
    std::unique_ptr<BIO, BioDeleter> acceptor;
    std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;    
};