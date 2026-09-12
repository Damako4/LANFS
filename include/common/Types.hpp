#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <librsync.h>

/**
 * @brief Maps a file's relative path to its current librsync signature.
 */
using SignatureMap = std::map<std::string, std::vector<char>>;

/**
 * @brief A file name paired with its delta bytes (librsync patch).
 */
using FileDeltaPair = std::pair<std::string, std::vector<char>>;

/**
 * @brief A file name paired with its signature bytes.
 */
using FileSignaturePair = std::pair<std::string, std::vector<char>>;

struct FileCloser { void operator()(FILE* f) const { if (f) std::fclose(f); } };
using FilePtr = std::unique_ptr<FILE, FileCloser>;

struct SignatureDeleter { void operator()(rs_signature_t *sumset) const { if (sumset) rs_free_sumset(sumset); } };
using SignaturePtr = std::unique_ptr<rs_signature_t, SignatureDeleter>;
