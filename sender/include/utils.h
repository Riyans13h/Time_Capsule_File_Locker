#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <chrono>

namespace SenderUtils {
    
    // File operations
    bool fileExists(const std::string& path);
    bool readFile(const std::string& path, std::vector<uint8_t>& data);
    bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    size_t getFileSize(const std::string& path);
    
    // String utilities
    std::string toHexString(const std::vector<uint8_t>& data);
    std::vector<uint8_t> fromHexString(const std::string& hex);
    std::string base64Encode(const std::vector<uint8_t>& data);
    std::vector<uint8_t> base64Decode(const std::string& encoded);
    
    // Time utilities
    std::string getCurrentTimestamp();
    bool isValidTimestamp(const std::string& timestamp);
    std::string formatTimestamp(const std::chrono::system_clock::time_point& time);
    
    // Crypto utilities
    std::vector<uint8_t> generateRandomBytes(size_t length);
    std::string generateUUID();
    
    // HTTP utilities
    std::string urlEncode(const std::string& value);
    std::string buildMultipartForm(const std::string& file_path, 
                                  const std::string& field_name,
                                  const std::vector<std::pair<std::string, std::string>>& fields);
    
    // Validation
    bool isValidEmail(const std::string& email);
    bool isValidFilename(const std::string& filename);
    bool isFutureTimestamp(const std::string& timestamp);
    
    // System info
    std::string getSystemInfo();
    
} // namespace SenderUtils

#endif // UTILS_H