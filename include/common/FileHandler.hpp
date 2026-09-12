#pragma once

#include <vector>
#include <string>
#include <map>
#include <librsync.h>
#include <future>
#include <memory>
#include <vector>
#include <Types.hpp>

#define CHUNK_SIZE 65536

class FileHandler {
public:
    /**
     * @brief Initialize @ref FileHandler to use @p sharedFolderPath for all subsequent calls
     * 
     * @param sharedFolderPath The path of the shared folder
     */
    static void init(const std::string &sharedFolderPath);

    /**
     * @brief Patch file in @p delta with the delta contained
     * 
     * @param delta @ref FileDeltaPair containing the fileName and delta
     */
    static void patchFile(FileDeltaPair &delta);
    
    /**
     * @brief Generate signature for @p fileName
     * 
     * @param fileName The name of the file
     * @return A FileSignaturePair containing @p fileName and signature
     */
    static FileSignaturePair generateSignature(const std::string &fileName);

    /**
     * @brief Generate a @ref FileDeltaPair for a file
     * 
     * Computes the delta needed to bring the file described by @p toPatchSignature
     * up to date with @p authoritativeSignature (the current source of truth).
     * @param authoritativeSignature The newest signature, the source of truth
     * @param toPatchSignature The signature for the file that is to be patched
     * @return A FileDeltaPair containing the file name and the delta to patch with
     */
    static FileDeltaPair generateDelta(const FileDeltaPair &authoritativeSignature, const FileSignaturePair &toPatchSignature);

    /**
     * @brief Like @ref generateDelta but for a SignatureMap
     * 
     * Generates a SignatureMap containing deltas for every file in @p toPatchSignature,
    * computed in parallel using a thread pool.
     * @param authoritativeSignature The newest signature map, the source of truth
     * @param toPatchSignature The signature map for the files that are to be patched
     * @return A SignatureMap containing the file names and deltas to patch with
     */
    static SignatureMap generateDeltas(const SignatureMap &authoritativeSignature, const SignatureMap &toPatchSignature);
private:
    static std::string sharedFolderPath;
    /**
     * @brief Return a pointer to a signature contained in a @p buffer
     * 
     * @param buffer The buffer containing the signature
     * @return A smart pointer to the signature
     */
    static SignaturePtr loadSignatureFromBuffer(const std::vector<char> &buffer);

    /**
     * @brief Compute the delta for @p signatureBuffer against @p fileName
     * 
     * @param signatureBuffer The buffer containing the signature
     * @param filePath The folder containing the file
     * @param fileName The name of the file
     * @return A FileDeltaPair containing the file name and the delta to apply
     */
    static FileDeltaPair threadComputeDelta(const std::vector<char> &signatureBuffer, const std::string &filePath, const std::string &fileName);
};