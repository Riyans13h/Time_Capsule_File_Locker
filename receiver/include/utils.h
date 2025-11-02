#ifndef RECEIVER_UTILS_H
#define RECEIVER_UTILS_H

#include <string>
#include <vector>

namespace ReceiverUtils {
    
    // File operations
    bool fileExists(const std::string& path);
    bool readFile(const std::string& path, std::vector<uint8_t>& data);
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    bool createDirectory(const std::string& path);
    std::string getFileExtension(const std::string& filename);
    std::string getFileName(const std::string& path);
    
    // Crypto utilities
    bool isValidPrivateKey(const std::string& key_path);
    bool isValidPublicKey(const std::string& key_path);
    std::string getKeyFingerprint(const std::string& key_path);
    
    // Network utilities
    bool downloadFromUrl(const std::string& url, const std::string& output_path);
    std::string httpGet(const std::string& url);
    bool httpPost(const std::string& url, const std::vector<std::pair<std::string, std::string>>& data);
    
    // String utilities
    std::vector<uint8_t> parseKeyPackage(const std::vector<uint8_t>& decrypted_data);
    std::string formatFileSize(size_t bytes);
    std::string getTimestampString();
    
    // Validation
    bool isValidCapsuleId(const std::string& capsule_id);
    bool isValidReceiverId(const std::string& receiver_id);
    
    // System utilities
    std::string getHomeDirectory();
    std::string getConfigDirectory();
    bool saveConfig(const std::string& key, const std::string& value);
    std::string loadConfig(const std::string& key);
    
} // namespace ReceiverUtils

#endif // RECEIVER_UTILS_H