#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

class HashUtils {
public:
    HashUtils();
    ~HashUtils();
    
    // File-based hashing
    std::string computeFileSHA256(const std::string& file_path);
    std::string computeFileSHA1(const std::string& file_path);
    std::string computeFileMD5(const std::string& file_path);
    
    // Memory-based hashing
    std::string computeSHA256(const std::vector<uint8_t>& data);
    std::string computeSHA256(const std::string& data);
    std::string computeSHA1(const std::vector<uint8_t>& data);
    std::string computeMD5(const std::vector<uint8_t>& data);
    
    // HMAC functions
    std::string computeHMACSHA256(const std::vector<uint8_t>& data, 
                                 const std::vector<uint8_t>& key);
    std::string computeHMACSHA1(const std::vector<uint8_t>& data, 
                               const std::vector<uint8_t>& key);
    
    // Password hashing
    std::string hashPassword(const std::string& password, 
                            const std::vector<uint8_t>& salt,
                            int iterations = 100000);
    bool verifyPassword(const std::string& password, 
                       const std::string& stored_hash,
                       const std::vector<uint8_t>& salt,
                       int iterations = 100000);
    
    // Utility functions
    static std::vector<uint8_t> generateRandomSalt(size_t length = 16);
    static std::string bytesToHex(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> hexToBytes(const std::string& hex);
    static bool compareHashes(const std::string& hash1, const std::string& hash2);
    
    // Progress callback support
    typedef void (*ProgressCallback)(size_t bytes_processed, size_t total_bytes);
    void setProgressCallback(ProgressCallback callback);
    
private:
    ProgressCallback progress_callback_;
    
    // Internal implementations
    std::string computeFileHash(const std::string& file_path, const std::string& algorithm);
    std::string computeHash(const std::vector<uint8_t>& data, const std::string& algorithm);
    std::string computeHMAC(const std::vector<uint8_t>& data, 
                           const std::vector<uint8_t>& key, 
                           const std::string& algorithm);
};

#endif // HASH_UTILS_H