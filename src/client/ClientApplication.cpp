#include <ClientApplication.hpp>
#include <ProtocolHandler.hpp>
#include <Tools.hpp>
#include <iostream>
#include <map>
#include <msgpack.hpp>
#include <thread>
#include <vector>

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

  // Load file hashes from file, hello.txt differs
  fileHashes = loadHashMap("./filehashes-client");

  // Send request for an update
  ProtocolHandler::writeHeaderBytes(ssl.get(), Command::UpdateList, 0, 0);

  // Read that stream of bytes back!
  ProtocolHeader header;
  ProtocolHandler::readHeaderBytes(ssl.get(), header);
  std::string buffer;
  ProtocolHandler::readStreamBytes(ssl.get(), buffer, header.fileSize);
  std::map<std::string, std::string> serverFileHashes;
  msgpack::object_handle result;
  msgpack::unpack(result, buffer.data(), header.fileSize);
  result.get().convert(serverFileHashes);

  // Find mismatched file hashes
  std::vector<std::string> mismatched = findDifferentHashes(serverFileHashes, fileHashes);
  for (auto it : mismatched) {
    // Send a request out and wait for a response
    ProtocolHandler::writeHeaderBytes(ssl.get(), Command::RequestList, 0, 0);
    ProtocolHandler::readHeaderBytes(ssl.get(), header);
    // TODO
  }

  std::this_thread::sleep_for(std::chrono::seconds(1));

  // TODO: Shutdown on server OK response
  // SSL_shutdown(ssl.get());
}

using HashMap = std::map<std::string, std::string>;
std::vector<std::string> ClientApplication::findDifferentHashes(HashMap& a, HashMap& b) {
  std::vector<std::string> mismatched;

  for (const auto& [key, val] : a) {
    if (auto it = b.find(key); it != b.end() && it->second != val) {
      mismatched.push_back(key);
    }
  }

  return mismatched;
}