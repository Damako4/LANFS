#include <ProtocolHandler.hpp>
#include <iostream>

ProtocolHeader ProtocolHandler::readHeaderBytes(SSL *ssl) {
  uint8_t cmdByte;
  uint32_t fileSize;
  size_t nread;

  if (!SSL_read_ex(ssl, &cmdByte, 1, &nread) || nread != 1) {
    throw std::runtime_error("Failed to read command header byte");
  }

  if (!SSL_read_ex(ssl, &fileSize, sizeof(fileSize), &nread) ||
      nread != sizeof(fileSize)) {
    throw std::runtime_error("Failed to read payload size bytes");
  }

  ProtocolHeader header;
  header.command = static_cast<Command>(cmdByte & 0x03);
  header.flags = (cmdByte >> 2) & 0x3F;
  header.fileSize = fileSize;
  return header;
}

void ProtocolHandler::writeHeaderBytes(SSL *ssl, Command command, uint8_t flags,
                                       uint32_t fileSize) {
  uint8_t cmdByte = (flags << 2) | (static_cast<uint8_t>(command) & 0x03);
  size_t nwritten;

  if (!SSL_write_ex(ssl, &cmdByte, 1, &nwritten) || nwritten != 1) {
    throw std::runtime_error("Failed to write command header byte");
  }

  if (!SSL_write_ex(ssl, &fileSize, sizeof fileSize, &nwritten) || nwritten != sizeof(fileSize)) {
    throw std::runtime_error("Failed to write file size header byte");
  }
}

void ProtocolHandler::writeStreamBytes(SSL *ssl, const char *bytes, size_t size) {
  size_t nwritten;
  if (!SSL_write_ex(ssl, bytes, size, &nwritten) || nwritten != size) {
    throw std::runtime_error("Failed to write data stream bytes");
  }
}

void ProtocolHandler::readStreamBytes(SSL *ssl, std::string &buffer, size_t bytesToRead) {
  size_t nread;
  buffer.resize(bytesToRead);
  if (!SSL_read_ex(ssl, buffer.data(), bytesToRead, &nread) || nread != bytesToRead) {
    throw std::runtime_error("Failed to read payload size bytes");
  }
}