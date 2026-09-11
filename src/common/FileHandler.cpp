#include <FileHandler.hpp>
#include <ThreadPool.hpp>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace filesystem = std::filesystem;

void FileHandler::patchFile(FileDeltaPair &delta, const std::string &sharedFolderPath) {
  std::string filePath = sharedFolderPath + delta.first;
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
  buf.next_in = delta.second.data();
  buf.avail_in = delta.second.size();
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

/*
void FileHandler::patchFiles(SignatureMap &deltas) {
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
*/

// Generate a signature for a given file
FileSignaturePair FileHandler::generateSignature(const std::string &sharedFolderPath, const std::string &fileName) {
  std::string filePath = sharedFolderPath + fileName;
  FilePtr file(std::fopen(filePath.data(), "rb"));
  if (!file) {
    std::perror("File opening failed");
    throw std::runtime_error("Failed to open file.");
  }

  // Get size of file
  std::fseek(file.get(), 0, SEEK_END);
  rs_long_t file_size = std::ftell(file.get());
  std::fseek(file.get(), 0, SEEK_SET);

  // Determine best arguments
  rs_magic_number magic = (rs_magic_number)0;
  size_t block_len = 0;
  size_t strong_len = 0;
  rs_result res;
  if ((res = rs_sig_args(file_size, &magic, &block_len, &strong_len)) != RS_DONE) {
    throw std::runtime_error(std::string("Failed to generate signature arguments: ") + rs_strerror(res));
  }

  FilePtr sig_file(std::tmpfile());
  if ((res = rs_sig_file(file.get(), sig_file.get(), block_len, strong_len, magic, nullptr)) != RS_DONE) {
    throw std::runtime_error(std::string("Failed to generate signature file: ") + rs_strerror(res));
  }

  // Get size of sig_file
  std::fseek(sig_file.get(), 0, SEEK_END);
  rs_long_t sig_file_size = std::ftell(sig_file.get());
  if (sig_file_size < 0) {
    throw std::runtime_error("ftell failed on signature file");
  }
  std::fseek(sig_file.get(), 0, SEEK_SET);

  std::vector<char> buffer(sig_file_size);
  size_t bytesRead = std::fread(buffer.data(), 1, sig_file_size, sig_file.get());
  if (std::ferror(sig_file.get())) {
    throw std::runtime_error("Failed to read signature file.");
  } else if (std::feof(sig_file.get()) && bytesRead < sig_file_size) {
    throw std::runtime_error("Failed to read all signature file bytes.");
  }

  return {fileName, std::move(buffer)};
}

// Generate map of file name -> librsync signatures for a given directory path
SignatureMap FileHandler::generateSignatures(const std::string &sharedFolderPath) {
  std::map<std::string, std::vector<char>> signatures;
  std::vector<std::string> fileNames;
  std::cout << sharedFolderPath << std::endl;

  if (filesystem::exists(sharedFolderPath) && filesystem::is_directory(sharedFolderPath)) {
    for (const auto &entry : filesystem::directory_iterator(sharedFolderPath)) {
      std::string fileName = entry.path().filename().string();

      fileNames.push_back(fileName);
    }
  } else {
    throw std::runtime_error("Directory not found: " + sharedFolderPath);
  }

  for (auto &fileName : fileNames) {
    signatures.insert(generateSignature(sharedFolderPath, fileName));
  }

  return signatures;
}

SignaturePtr FileHandler::loadSignatureFromBuffer(const std::vector<char> &buffer) {
  rs_signature_t *sumset = nullptr;
  rs_job_t *job = rs_loadsig_begin(&sumset);

  rs_buffers_t buf;
  buf.next_in = const_cast<char *>(buffer.data());
  buf.avail_in = buffer.size();
  buf.eof_in = 1;
  buf.next_out = nullptr;
  buf.avail_out = 0;

  rs_result res;
  do {
    res = rs_job_iter(job, &buf);
  } while (res == RS_BLOCKED);

  rs_job_free(job);

  if (res != RS_DONE) {
    if (sumset)
      rs_free_sumset(sumset);
    throw std::runtime_error(std::string("Failed to load signature: ") + rs_strerror(res));
  }

  res = rs_build_hash_table(sumset);
  if (res != RS_DONE) {
    rs_free_sumset(sumset);
    throw std::runtime_error(std::string("Failed to build hash table: ") + rs_strerror(res));
  }

  return std::unique_ptr<rs_signature_t, SignatureDeleter>(sumset);
}

FileDeltaPair FileHandler::threadComputeDelta(const std::vector<char> &signatureBuffer, const std::string &filePath, const std::string &fileName) {
  FilePtr file(std::fopen(filePath.c_str(), "rb"));
  if (!file) {
    std::perror("File opening failed");
    throw std::runtime_error("Failed to open file.");
  }

  SignaturePtr clientSignature = loadSignatureFromBuffer(signatureBuffer);
  rs_result res;
  FilePtr deltaFile(std::tmpfile());
  res = rs_delta_file(clientSignature.get(), file.get(), deltaFile.get(), nullptr);
  if (res != RS_DONE) {
    throw std::runtime_error(std::string("Failed to generate delta file: ") + rs_strerror(res));
  }

  // Get size of deltaFile and allocate buffer for it
  std::fseek(deltaFile.get(), 0, SEEK_END);
  rs_long_t deltaBufferSize = std::ftell(deltaFile.get());
  if (deltaBufferSize < 0) {
    throw std::runtime_error("ftell failed on delta file");
  }
  std::fseek(deltaFile.get(), 0, SEEK_SET);
  std::vector<char> deltaBuffer(static_cast<size_t>(deltaBufferSize));
  size_t bytesRead = std::fread(deltaBuffer.data(), 1, deltaBufferSize, deltaFile.get());
  if (std::ferror(deltaFile.get())) {
    throw std::runtime_error("Failed to read signature file.");
  } else if (std::feof(deltaFile.get()) && bytesRead < deltaBufferSize) {
    throw std::runtime_error("Failed to read all signature file bytes.");
  }

  return {fileName, std::move(deltaBuffer)};
}

FileDeltaPair FileHandler::generateDelta(const FileDeltaPair &authoritativeSignature, const FileSignaturePair &toPatchSignature, const std::string &sharedFolderPath) {
  if (authoritativeSignature.first.compare(toPatchSignature.first) == 0) {
    std::string filePath = sharedFolderPath + toPatchSignature.first;
    return FileHandler::threadComputeDelta(toPatchSignature.second, filePath, toPatchSignature.first);
  } else {
    // TODO: Handle this error
    throw std::runtime_error("Failed to find client file name key in server map.");
  }
}

// Computes patches for toPatchSignature against authoritativeSignature (source of truth), what will be in the delta
SignatureMap FileHandler::generateDeltas(const SignatureMap &authoritativeSignature, const SignatureMap &toPatchSignature, const std::string &sharedFolderPath) {
  SignatureMap deltas;
  ThreadPool pool(CPU_CORES);

  std::vector<std::future<FileDeltaPair>> fileToDeltaPairs;

  for (auto &[fileName, signatureBuffer] : toPatchSignature) {
    if (auto search = authoritativeSignature.find(fileName); search != authoritativeSignature.end()) {
      std::string filePath = sharedFolderPath + fileName;
      fileToDeltaPairs.push_back(pool.submit(&FileHandler::threadComputeDelta, signatureBuffer, filePath, fileName));
    } else {
      // TODO: Handle this error
      throw std::runtime_error("Failed to find client file name key in server map.");
    }
  }

  for (auto &fut : fileToDeltaPairs) {
    FileDeltaPair result = fut.get();
    deltas.insert(std::move(result));
  }

  return deltas;
}