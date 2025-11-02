#ifndef AES_CBC_H
#define AES_CBC_H

#include <string>
#include <vector>

class AESCrypto {
public:
    AESCrypto();
    ~AESCrypto();
    
    // File-based operations
    bool encryptFile(const std::string& input_file, const std::string& output_file,
                    const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    bool decryptFile(const std::string& input_file, const std::string& output_file,
                    const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    
    // Memory-based operations
    bool encryptData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                    const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    bool decryptData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                    const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv);
    
    // Key derivation
    static std::vector<uint8_t> deriveKeyPBKDF2(const std::string& password,
                                               const std::vector<uint8_t>& salt,
                                               size_t key_size = 32,
                                               int iterations = 100000);
    
    // Utility functions
    static std::vector<uint8_t> generateRandomIV();
    static std::vector<uint8_t> generateRandomKey(size_t size = 32);
    bool validateKey(const std::vector<uint8_t>& key);
    
private:
    void addPadding(std::vector<uint8_t>& data);
    void removePadding(std::vector<uint8_t>& data);
    std::vector<uint8_t> xorWithIV(const std::vector<uint8_t>& data, 
                                  const std::vector<uint8_t>& iv);
};

#endif // AES_CBC_H