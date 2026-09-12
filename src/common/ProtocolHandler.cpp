#include <ProtocolHandler.hpp>
#include <cstring>
#include <iostream>

void ProtocolHandler::readExact(SSL *ssl, void *destination, size_t bytesToRead) {
  uint8_t *ptr = static_cast<uint8_t *>(destination);
  size_t totalRead = 0;

  while (totalRead < bytesToRead) {
    size_t nread = 0;
    int ret = SSL_read_ex(ssl, ptr + totalRead, bytesToRead - totalRead, &nread);

    if (ret <= 0) {
      int err = SSL_get_error(ssl, ret);
      if (totalRead == 0 && (err == SSL_ERROR_ZERO_RETURN || (err == SSL_ERROR_SYSCALL && nread == 0))) {
        throw ConnectionClosed();
      }
      throw std::runtime_error("TLS read failure or unexpected connection drop");
    }
    totalRead += nread;
  }
}

void ProtocolHandler::writeExact(SSL *ssl, const void *source, size_t bytesToWrite) {
  const uint8_t *ptr = static_cast<const uint8_t *>(source);
  size_t totalWritten = 0;

  while (totalWritten < bytesToWrite) {
    size_t nwritten = 0;
    if (!SSL_write_ex(ssl, ptr + totalWritten, bytesToWrite - totalWritten, &nwritten)) {
      throw std::runtime_error("TLS write failure");
    }
    totalWritten += nwritten;
  }
}

void ProtocolHandler::readHeaderBytes(SSL *ssl, ProtocolHeader &outHeader) {
  uint8_t raw[5];

  readExact(ssl, raw, sizeof(raw));

  outHeader.command = static_cast<Command>(raw[0] & 0x03);
  outHeader.flags = (raw[0] >> 2) & 0x3F;
  std::memcpy(&outHeader.streamLength, &raw[1], sizeof(outHeader.streamLength));
}

void ProtocolHandler::writeHeaderBytes(SSL *ssl, Command command, uint8_t flags, uint32_t streamLength) {
  uint8_t raw[5];

  raw[0] = (flags << 2) | (static_cast<uint8_t>(command) & 0x03);
  std::memcpy(&raw[1], &streamLength, sizeof(streamLength));

  writeExact(ssl, raw, sizeof(raw));
}

void ProtocolHandler::readStreamBytes(SSL *ssl, std::string &buffer, size_t bytesToRead) {
  buffer.resize(bytesToRead);
  if (bytesToRead > 0) {
    readExact(ssl, buffer.data(), bytesToRead);
  }
}

void ProtocolHandler::writeStreamBytes(SSL *ssl, const char *bytes, size_t size) {
  if (size > 0) {
    writeExact(ssl, bytes, size);
  }
}