#include <ClientApplication.hpp>
#include <FileHandler.hpp>
#include <ProtocolHandler.hpp>
#include <Tools.hpp>
#include <UpdateListener.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <msgpack.hpp>
#include <poll.h>
#include <thread>
#include <vector>

namespace filesystem = std::filesystem;

ClientApplication::ClientApplication(const ApplicationConfig &config) : config(config), listener(queue) {
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
  ssl.reset(SSL_new(ctx.get()));
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

  // That initial packet (WE NEED TO SEND THIS)
  SignatureMap localSignatures = FileHandler::generateSignatures(config.sharedFolderPath);
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, localSignatures);
  ProtocolHandler::writeHeaderBytes(ssl.get(), Command::Signature, /*flags=*/0, sbuf.size());
  ProtocolHandler::writeStreamBytes(ssl.get(), sbuf.data(), sbuf.size());

  // Setup file watcher
  fileWatcher.reset(new efsw::FileWatcher());
  // TODO: Add a specific watch for windows, this will only work for linux
  watchID = fileWatcher->addWatch(config.sharedFolderPath, &listener, RECURSIVE_FILE_WATCH);
  if (watchID < 0) {
    std::cerr << "addWatch failed with code: " << watchID << std::endl;
  }
  fileWatcher->watch();

  ProtocolHeader header;
  int fd = SSL_get_fd(ssl.get());
  while (true) {
    if (auto event = queue.pop()) {
      std::cout << event.value().fileName << std::endl;
    }

    pollfd pfd{fd, POLLIN, 0};
    int ret = poll(&pfd, 1, /*timeout ms=*/100);

    if (ret < 0) {
      if (errno == EINTR)
        continue; // interrupted by a signal, just retry
      throw std::runtime_error(std::string("poll failed: ") + std::strerror(errno));
    }
    if (ret == 0) {
      continue; // timed out, nothing to read — loop back to check the queue
    }
    if (pfd.revents & (POLLERR | POLLHUP)) {
      break; // connection dropped
    }

    if (pfd.revents & POLLIN) {
      ProtocolHandler::readHeaderBytes(ssl.get(), header);
      // Read stream bytes
      std::string buffer(header.streamLength, '\0');
      ProtocolHandler::readStreamBytes(ssl.get(), buffer, header.streamLength);
      msgpack::object_handle result;
      msgpack::unpack(result, buffer.data(), header.streamLength);
      switch (header.command) {
      case Command::Signature: {

        break;
      }
      case Command::Update: {

        break;
      }
      default:
        break;
      }
    }
  }

  shutdown();
}

void ClientApplication::patchFiles(SignatureMap &serverDeltas) {
  for (auto &[fileName, delta] : serverDeltas) {
    filesystem::path filePath = config.sharedFolderPath + fileName;
    filesystem::path tmpFilePath = filePath;
    tmpFilePath += ".tmp";
    FilePtr file(std::fopen(filePath.c_str(), "rb"));
    FilePtr tmpFile(std::fopen(tmpFilePath.c_str(), "wb"));
    if (!file || !tmpFile) {
      std::perror("File opening failed");
      throw std::runtime_error("Failed to open file.");
    }
    rs_job_t *job = rs_patch_begin(rs_file_copy_cb, file.get());

    unsigned char out_chunk[CHUNK_SIZE];
    rs_buffers_t buf = {0};
    buf.next_in = delta.data();
    buf.avail_in = delta.size();
    buf.eof_in = 1;

    rs_result result;
    do {
      buf.next_out = (char *)out_chunk;
      buf.avail_out = sizeof(out_chunk);

      result = rs_job_iter(job, &buf);

      size_t written = sizeof(out_chunk) - buf.avail_out;
      if (written > 0) {
        size_t n = std::fwrite(out_chunk, 1, written, tmpFile.get());
        if (n != written) {
          if (std::ferror(tmpFile.get())) {
            tmpFile.reset();
            filesystem::remove(tmpFilePath);
            throw std::runtime_error(std::string("Write failed: ") + std::strerror(errno));
          } else {
            tmpFile.reset();
            filesystem::remove(tmpFilePath);
            throw std::runtime_error("Write failed: short write.");
          }
        }
      }

    } while (result == RS_BLOCKED || (result == RS_DONE && buf.avail_in > 0));

    if (result != RS_DONE) {
      rs_job_free(job);
      tmpFile.reset();
      filesystem::remove(tmpFilePath);
      throw std::runtime_error(std::string("Failed to patch: ") + rs_strerror(result));
    }

    rs_job_free(job);

    // Move tmpFile to original file
    file.reset();
    tmpFile.reset();
    filesystem::rename(tmpFilePath, filePath);
  }
}

void ClientApplication::shutdown() {
  if (!running)
    return;
  running = false;

  if (fileWatcher && watchID > 0) {
    fileWatcher->removeWatch(watchID);
  }
  fileWatcher.reset();

  if (ssl) {
    SSL_shutdown(ssl.get());
  }
}

ClientApplication::~ClientApplication() {
  shutdown();
}