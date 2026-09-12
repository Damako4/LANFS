#include <FileHandler.hpp>
#include <ProtocolHandler.hpp>
#include <ServerApplication.hpp>
#include <Tools.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <librsync.h>
#include <msgpack.hpp>
#include <openssl/err.h>
#include <vector>

namespace filesystem = std::filesystem;

void ServerApplication::handleSSLSession(SSL *ssl) const {
  ProtocolHeader header;
  while (true) {
    // Read header bytes
    ProtocolHandler::readHeaderBytes(ssl, header);

    // Read stream bytes
    std::string buffer(header.streamLength, '\0');
    ProtocolHandler::readStreamBytes(ssl, buffer, header.streamLength);
    msgpack::object_handle result;
    msgpack::unpack(result, buffer.data(), header.streamLength);
    switch (header.command) {
    case Command::Signature: {
      SignatureMap clientSignatures;
      result.get().convert(clientSignatures);

      // Generate file deltas and send over
      SignatureMap deltas = FileHandler::generateDeltas(serverSignatures, clientSignatures);
      msgpack::sbuffer sbuf;
      msgpack::pack(sbuf, deltas);
      ProtocolHandler::writeHeaderBytes(ssl, Command::Delta, 0, sbuf.size());
      ProtocolHandler::writeStreamBytes(ssl, sbuf.data(), sbuf.size());
      break;
    }
    case Command::Update: {
      // File name to update is stored in buffer
      std::string fileName;
      result.get().convert(fileName);
      std::vector<char> signature;

      // Get signature for that filename
      if (auto search = serverSignatures.find(fileName); search != serverSignatures.end()) {
        signature = search->second;
      } else {
        throw std::runtime_error("Failed to find client file name key in server map.");
      }

      // Send signature and then wait for a delta
      msgpack::sbuffer sbuf;
      auto fileNameSignature = std::make_pair(fileName, signature);
      msgpack::pack(sbuf, fileNameSignature);
      ProtocolHandler::writeHeaderBytes(ssl, Command::Signature, 0, sbuf.size());
      ProtocolHandler::writeStreamBytes(ssl, sbuf.data(), sbuf.size());
      break;
    }
    case Command::Delta: {
      // Apply delta
      FileDeltaPair fileDeltaPair;
      result.get().convert(fileDeltaPair);
      FileHandler::patchFile(fileDeltaPair);
      std::cout << "Updating File!" << std::endl;
      break;
    }
    default:
      break;
    }
  }
}

void ServerApplication::run() {
  while (1) {
    ERR_clear_error(); // Before each new connection
    if (BIO_do_accept(acceptor.get()) <= 0) {
      /* Client went away before we accepted the connection */
      continue;
    }

    // Pop off acceptor chain and reset its state
    std::unique_ptr<BIO, BioDeleter> client(BIO_pop(acceptor.get()));
    std::cout << "New client connection" << std::endl;

    /* Associate new SSL handle */
    std::unique_ptr<SSL, SslDeleter> ssl(SSL_new(ctx.get()));
    if (!ssl) {
      std::cerr << "Error creating SSL handle for new connection: " << getLastSSLError() << std::endl;
      continue;
    }
    BIO *raw_client = client.release();
    SSL_set_bio(ssl.get(), raw_client, raw_client);

    /* Attempt an SSL handshake with the client */
    if (SSL_accept(ssl.get()) <= 0) {
      std::cerr << "Error performing SSL handshake with client: " << getLastSSLError() << std::endl;
      continue;
    }

    try {
      handleSSLSession(ssl.get());
    } catch (const ConnectionClosed &) {
      std::cout << "Client connection closed." << std::endl;
      SSL_shutdown(ssl.get());
      continue;
    } catch (const std::exception &e) {
      std::cout << "Client connection closed." << std::endl;
      std::cerr << "Error: " << e.what() << std::endl;
      continue;
    }    
  }
}

ServerApplication::ServerApplication(const ApplicationConfig &config) : config(config) {
  ctx.reset(SSL_CTX_new(TLS_server_method()));
  if (!ctx) {
    throw std::runtime_error("Failed to create SSL_CTX: " + getLastSSLError());
  }

  if (!SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION)) {
    throw std::runtime_error("Failed to set the minimum TLS protocol version: " + getLastSSLError());
  }

  SSL_CTX_set_options(ctx.get(), SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION);

  // Load the server's certificate *chain* file (PEM format)
  if (SSL_CTX_use_certificate_chain_file(ctx.get(), "chain.pem") <= 0) {
    throw std::runtime_error("Failed to load the server certificate chain file: " + getLastSSLError());
  }

  // Load corresponding private key
  if (SSL_CTX_use_PrivateKey_file(ctx.get(), "pkey.pem", SSL_FILETYPE_PEM) <= 0) {
    throw std::runtime_error("Error loading the server private key file, possible key/cert mismatch: " + getLastSSLError());
  }

  // Enable session caching
  const unsigned char cache_id[] = "application"; // Can be anything
  SSL_CTX_set_session_id_context(ctx.get(), cache_id, sizeof cache_id);
  SSL_CTX_set_session_cache_mode(ctx.get(), SSL_SESS_CACHE_SERVER);
  SSL_CTX_sess_set_cache_size(ctx.get(), config.cacheSize); // Set server cache size
  SSL_CTX_set_timeout(ctx.get(), config.cacheTimeout);

  // Don't require mTLS (Certificate Based Authentication)
  SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_NONE, NULL);

  // Create acceptor BIO for clients
  acceptor.reset(BIO_new_accept(config.hostport.c_str()));
  if (!acceptor) {
    throw std::runtime_error("Error creating acceptor bio: " + getLastSSLError());
  }

  BIO_set_bind_mode(acceptor.get(), BIO_BIND_REUSEADDR);
  if (BIO_do_accept(acceptor.get()) <= 0) {
    throw std::runtime_error("Error setting up acceptor socket: " + getLastSSLError());
  }

  FileHandler::init(config.sharedFolderPath);
}