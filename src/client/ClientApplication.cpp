#include <ClientApplication.hpp>
#include <ProtocolHandler.hpp>
#include <Tools.hpp>
#include <iostream>
#include <map>
#include <msgpack.hpp>
#include <thread>
#include <vector>
#include <FileHandler.hpp>

ClientApplication::ClientApplication(const ApplicationConfig &config) : config(config) {
  ctx.reset(SSL_CTX_new(TLS_client_method()));
  if (!ctx) {
    throw std::runtime_error("Failed to create SSL_CTX: " + getLastSSLError());
  }

  // Abort the handshake if certificate verification fails
  SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_NONE, NULL);
  // TODO: Change this to SSL_VERIFY_PEER for production

  /* Use the default trusted certificate store */
  if (!SSL_CTX_set_default_verify_paths(ctx.get())) {
    throw std::runtime_error(
        "Failed to set the default trusted certificate store: " +
        getLastSSLError());
  }

  if (!SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION)) {
    throw std::runtime_error(
        "Failed to set the minimum TLS protocol version: " + getLastSSLError());
  }
}

void ClientApplication::run() {
  std::string serverName = config.hostname + ":" + config.hostport;
  std::unique_ptr<BIO, BioDeleter> clientBio(BIO_new_connect(serverName.c_str()));
  if (!clientBio) {
    throw std::runtime_error("Error creating connect BIO: " + getLastSSLError());
  }

  std::cout << "Connecting to " << serverName << "..." << std::endl;
  if (BIO_do_connect(clientBio.get()) <= 0) {
    throw std::runtime_error("Failed to connect to server: " + getLastSSLError());
  }
  std::cout << "Connected!" << std::endl;

  // We want to reach out to server everytime that we start up to get latest file updates
  std::unique_ptr<SSL, SslDeleter> ssl(SSL_new(ctx.get()));
  if (!ssl) {
    throw std::runtime_error("Failed to create the SSL object: " + getLastSSLError());
  }
  BIO *raw_client = clientBio.release();
  SSL_set_bio(ssl.get(), raw_client, raw_client);

  if (SSL_connect(ssl.get()) < 1) {
    if (SSL_get_verify_result(ssl.get()) != X509_V_OK) {
      std::string error(X509_verify_cert_error_string(SSL_get_verify_result(ssl.get())));
      throw std::runtime_error("Failed to connect to the server: Verify error: " + error);
    }
    throw std::runtime_error("Failed to connect to the server: " + getLastSSLError());
  }

  // Generate signatures, pack and send
  signatures = FileHandler::generateSignatures(config.sharedFolderPath);
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, signatures);
  ProtocolHandler::writeHeaderBytes(ssl.get(), Command::Signature, 0, sbuf.size());
  ProtocolHandler::writeStreamBytes(ssl.get(), sbuf.data(), sbuf.size());

  // Read deltas back
  ProtocolHeader header;
  ProtocolHandler::readHeaderBytes(ssl.get(), header);
  std::string buffer;
  ProtocolHandler::readStreamBytes(ssl.get(), buffer, header.streamLength);
  SignatureMap serverDeltas;
  msgpack::object_handle result;
  msgpack::unpack(result, buffer.data(), header.streamLength);
  result.get().convert(serverDeltas);
  
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // TODO: Shutdown on server OK response
  // SSL_shutdown(ssl.get());
}