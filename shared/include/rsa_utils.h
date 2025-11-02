#ifndef RSA_UTILS_H
#define RSA_UTILS_H

#include <string>
#include <vector>

class RSACrypto {
public:
    RSACrypto();
    ~RSACrypto();
    
    // Key generation
    bool generateKeyPair(int key_size, std::string& private_key, std::string& public_key);
    bool generateKeyPairToFiles(const std::string& private_key_path, 
                               const std::string& public_key_path, 
                               int key_size = 3072);
    
    // File-based operations
    bool encryptFile(const std::string& public_key_path, 
                    const std::vector<uint8_t>& input_data,
                    const std::string& output_file);
    bool decryptFile(const std::string& private_key_path,
                    const std::vector<uint8_t>& encrypted_data,
                    std::vector<uint8_t>& decrypted_data);
    
    // Memory-based operations
    bool encryptWithPublicKey(const std::string& public_key_pem,
                             const std::string& plaintext,
                             std::string& ciphertext);
    bool decryptWithPrivateKey(const std::string& private_key_pem,
                              const std::string& ciphertext,
                              std::string& plaintext);
    
    // Key management
    static bool validatePublicKey(const std::string& public_key_pem);
    static bool validatePrivateKey(const std::string& private_key_pem);
    static std::string getKeyFingerprint(const std::string& key_pem);
    
    // Utility functions
    static size_t getMaxEncryptionSize(int key_size);
    static bool canEncryptData(size_t data_size, int key_size);
    
private:
    std::string loadKeyFromFile(const std::string& file_path);
    bool saveKeyToFile(const std::string& key_data, const std::string& file_path);
};

#endif // RSA_UTILS_H