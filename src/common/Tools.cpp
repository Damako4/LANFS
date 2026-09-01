#include <Tools.hpp>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <openssl/err.h>
#include <openssl/evp.h>

[[nodiscard]] std::string getLastSSLError() {
  unsigned long errCode = ERR_get_error();
  if (errCode != 0) {
    char errBuf[256];
    ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
    return std::string(errBuf);
  }

  if (errno != 0) {
    return std::string(strerror(errno));
  }

  return "No OpenSSL or system error recorded";
}

std::string sha256(const std::string &str) {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int hash_len = 0;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx) {
    throw std::runtime_error("Failed to create EVP_MD_CTX");
  }

  if (1 != EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) ||
      1 != EVP_DigestUpdate(ctx, str.data(), str.size()) ||
      1 != EVP_DigestFinal_ex(ctx, hash, &hash_len)) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("SHA256 hashing failed");
  }

  EVP_MD_CTX_free(ctx);

  // Convert raw bytes to hex string
  std::stringstream ss;
  for (unsigned int i = 0; i < hash_len; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
  }
  return ss.str();
}

void saveHashMap(const std::string &filename, std::map<std::string, std::string> &fileHashes) {
  msgpack::sbuffer sbuf;
  msgpack::pack(sbuf, fileHashes);

  std::ofstream outFile(filename, std::ios::binary);
  if (!outFile) {
    throw std::runtime_error("Failed to open file for writing: " + filename);
  }

  outFile.write(sbuf.data(), sbuf.size());
}

std::map<std::string, std::string> loadHashMap(const std::string &filename) {
  std::ifstream inFile(filename, std::ios::binary | std::ios::ate);
  if (!inFile) {
    throw std::runtime_error("Failed to open file for writing: " + filename);
  }

  std::streamsize size = inFile.tellg();
  inFile.seekg(0, std::ios::beg);

  std::string buffer(size, '\0');
  if (!inFile.read(&buffer[0], size)) {
    throw std::runtime_error("Failed to read file contents: " + filename);
  }

  msgpack::object_handle result;
  msgpack::unpack(result, buffer.data(), buffer.size());
  std::map<std::string, std::string> map;
  result.get().convert(map);

  return map;
}