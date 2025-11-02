#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <string>
#include <vector>

struct EncryptionConfig {
    std::string input_file;
    std::string receiver_id;
    std::string receiver_public_key_path;
    std::string release_time;
    std::string password;
    std::string server_url;
    std::string sender_info;
    
    // Output files
    std::string compressed_file;
    std::string encrypted_file;
    std::string key_package_file;
    
    // Crypto parameters
    int aes_key_size = 32; // 256 bits
    int salt_size = 16;
    int iv_size = 16;
    int pbkdf2_iterations = 100000;
};

class Encryptor {
public:
    Encryptor();
    ~Encryptor();
    
    // Main encryption workflow
    bool encryptAndUpload(const EncryptionConfig& config);
    
    // Individual steps
    bool compressFile(const std::string& input_file, const std::string& output_file);
    bool generateAESKey(const std::string& password, std::vector<uint8_t>& key, 
                       std::vector<uint8_t>& salt, std::vector<uint8_t>& iv);
    bool encryptFile(const std::string& input_file, const std::string& output_file,
                    const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    bool createKeyPackage(const std::vector<uint8_t>& key, const std::vector<uint8_t>& salt,
                         const std::vector<uint8_t>& iv, const std::string& public_key_path,
                         const std::string& output_file);
    bool uploadToServer(const EncryptionConfig& config, const std::string& sha256_hash);
    
    // Utility functions
    std::string computeSHA256(const std::string& file_path);
    size_t getFileSize(const std::string& file_path);
    
private:
    void cleanupTempFiles(const EncryptionConfig& config);
    bool validateConfig(const EncryptionConfig& config);
};

#endif // ENCRYPTOR_H