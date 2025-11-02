#ifndef DECRYPTOR_H
#define DECRYPTOR_H

#include <string>
#include <vector>

struct DecryptionConfig {
    std::string capsule_id;
    std::string receiver_id;
    std::string private_key_path;
    std::string output_dir = ".";
    std::string server_url = "http://localhost:3000";
    std::string password; // Only if password was used during encryption
    
    // Downloaded files
    std::string encrypted_file_path;
    std::string encrypted_key_path;
    std::string output_file_path;
};

struct CapsuleInfo {
    std::string capsule_id;
    std::string sender_info;
    std::string original_filename;
    size_t file_size;
    std::string sha256_hash;
    std::string release_time;
    std::string status;
    std::string created_at;
    std::string delivered_at;
};

class Decryptor {
public:
    Decryptor();
    ~Decryptor();
    
    // Main decryption workflow
    bool downloadAndDecrypt(const DecryptionConfig& config);
    
    // Individual steps
    bool getCapsuleInfo(const std::string& server_url, const std::string& capsule_id, CapsuleInfo& info);
    bool downloadFile(const std::string& url, const std::string& output_path);
    bool decryptKeyPackage(const std::string& encrypted_key_path, 
                          const std::string& private_key_path,
                          std::vector<uint8_t>& aes_key,
                          std::vector<uint8_t>& salt,
                          std::vector<uint8_t>& iv);
    bool decryptFile(const std::string& encrypted_file_path,
                    const std::string& output_file_path,
                    const std::vector<uint8_t>& key,
                    const std::vector<uint8_t>& iv);
    bool decompressFile(const std::string& compressed_file_path, 
                       const std::string& output_file_path);
    bool verifyFileHash(const std::string& file_path, const std::string& expected_hash);
    
    // Utility functions
    bool cleanupDownloadedFiles(const DecryptionConfig& config);
    std::string getCapsuleStatus(const std::string& server_url, const std::string& capsule_id);
    
private:
    bool validateConfig(const DecryptionConfig& config);
    bool deriveAESKeyFromPassword(const std::vector<uint8_t>& salt, 
                                 const std::string& password,
                                 std::vector<uint8_t>& key);
};

#endif // DECRYPTOR_H