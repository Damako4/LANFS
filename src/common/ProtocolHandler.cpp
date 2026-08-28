#include <ProtocolHandler.hpp>
#include <iostream>

ProtocolHeader ProtocolHandler::readHeaderBytes(SSL* ssl) {
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
  header.flags = (cmdByte >> 2) & 0x20;
  header.fileSize = fileSize;
  return header;
}

void ProtocolHandler::writeHeaderBytes(SSL* ssl, Command command, uint8_t flags,
                                       uint32_t fileSize) {
  uint8_t cmdByte = (flags << 2) | (static_cast<uint8_t>(command) & 0x03);
  size_t nwritten;
 
  if (!SSL_write_ex(ssl, &cmdByte, 1, &nwritten) || nwritten != 1) {
    throw std::runtime_error("Failed to write command header byte");
  }

  if (!SSL_write_ex(ssl, &fileSize, 1, &nwritten) || nwritten != sizeof(fileSize)) {
    throw std::runtime_error("Failed to write file size header byte");
  }
}