#pragma once

#include <vector>
#include <string>
#include <map>
#include <librsync.h>
#include <future>
#include <memory>
#include <vector>

struct FileCloser { void operator()(FILE* f) const { if (f) std::fclose(f); } };
using FilePtr = std::unique_ptr<FILE, FileCloser>;

struct SignatureDeleter { void operator()(rs_signature_t *sumset) const { if (sumset) rs_free_sumset(sumset); } };
using SignaturePtr = std::unique_ptr<rs_signature_t, SignatureDeleter>;

using SignatureMap = std::map<std::string, std::vector<char>>;
using FileDeltaPair = std::pair<std::string, std::vector<char>>;

class FileHandler {
public:
    static SignatureMap generateSignatures(const std::string &sharedFolderPath);
    static std::vector<char> generateSignature(const std::string &fileName);
    static SignatureMap generateDeltas(const SignatureMap &serverSignatures, const SignatureMap &clientSignatures, const std::string &sharedFolderPath);
private:
    static SignaturePtr loadSignatureFromBuffer(const std::vector<char> &buffer);
    static FileDeltaPair threadComputeDelta(const std::vector<char> &signatureBuffer, const std::string &filePath, const std::string &fileName);
};