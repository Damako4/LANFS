#pragma once

#include <Config.hpp>
#include <SslDeleters.hpp>
#include <map>
#include <memory>
#include <openssl/ssl.h>
#include <vector>
#include <efsw/efsw.hpp>
#include <UpdateListener.hpp>
#include <FileEventQueue.hpp>

#define CHUNK_SIZE 65536
#define RECURSIVE_FILE_WATCH 0

using SignatureMap = std::map<std::string, std::vector<char>>;

class ClientApplication {
public:
  explicit ClientApplication(const ApplicationConfig &config);
  ~ClientApplication();
  void run();
  void shutdown();

private:
  ApplicationConfig config;
  SignatureMap signatures;
  std::unique_ptr<SSL_CTX, SslCtxDeleter> ctx;
  std::unique_ptr<SSL, SslDeleter> ssl;
  void patchFiles(SignatureMap &serverDeltas);

  FileEventQueue queue;
  UpdateListener listener;
  std::unique_ptr<efsw::FileWatcher> fileWatcher;
  efsw::WatchID watchID = -1;
  bool running = false;
};