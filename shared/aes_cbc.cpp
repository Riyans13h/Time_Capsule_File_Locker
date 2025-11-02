#include "aes_cbc.h"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <iostream>
#include <fstream>

using namespace CryptoPP;

AESCrypto::AESCrypto() {
}

AESCrypto::~AESCrypto() {
}

bool AESCrypto::encryptFile(const std::string& input_file, const std::string& output_file,
                           const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    try {
        // Read input file
        std::ifstream in_file(input_file, std::ios::binary);
        if (!in_file) {
            std::cerr << "Cannot open input file: " << input_file << std::endl;
            return false;
        }
        
        std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(in_file)),
                                      std::istreambuf_iterator<char>());
        in_file.close();
        
        if (input_data.empty()) {
            std::cerr << "Input file is empty: " << input_file << std::endl;
            return false;
        }
        
        // Encrypt data
        std::vector<uint8_t> encrypted_data;
        if (!encryptData(input_data, encrypted_data, key, iv)) {
            std::cerr << "Encryption failed" << std::endl;
            return false;
        }
        
        // Write encrypted file
        std::ofstream out_file(output_file, std::ios::binary);
        if (!out_file) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return false;
        }
        
        out_file.write(reinterpret_cast<const char*>(encrypted_data.data()), 
                      encrypted_data.size());
        out_file.close();
        
        if (out_file.fail()) {
            std::cerr << "Failed to write encrypted file" << std::endl;
            return false;
        }
        
        std::cout << "Encryption successful: " << input_file << " -> " << output_file << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return false;
    }
}

bool AESCrypto::decryptFile(const std::string& input_file, const std::string& output_file,
                           const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    try {
        // Read encrypted file
        std::ifstream in_file(input_file, std::ios::binary);
        if (!in_file) {
            std::cerr << "Cannot open input file: " << input_file << std::endl;
            return false;
        }
        
        std::vector<uint8_t> encrypted_data((std::istreambuf_iterator<char>(in_file)),
                                          std::istreambuf_iterator<char>());
        in_file.close();
        
        if (encrypted_data.empty()) {
            std::cerr << "Encrypted file is empty: " << input_file << std::endl;
            return false;
        }
        
        // Decrypt data
        std::vector<uint8_t> decrypted_data;
        if (!decryptData(encrypted_data, decrypted_data, key, iv)) {
            std::cerr << "Decryption failed" << std::endl;
            return false;
        }
        
        // Write decrypted file
        std::ofstream out_file(output_file, std::ios::binary);
        if (!out_file) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return false;
        }
        
        out_file.write(reinterpret_cast<const char*>(decrypted_data.data()), 
                      decrypted_data.size());
        out_file.close();
        
        if (out_file.fail()) {
            std::cerr << "Failed to write decrypted file" << std::endl;
            return false;
        }
        
        std::cout << "Decryption successful: " << input_file << " -> " << output_file << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        return false;
    }
}

bool AESCrypto::encryptData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                           const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    try {
        // Validate key size
        if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
            std::cerr << "Invalid AES key size: " << key.size() << std::endl;
            return false;
        }
        
        if (iv.size() != AES::BLOCKSIZE) {
            std::cerr << "Invalid IV size: " << iv.size() << std::endl;
            return false;
        }
        
        // Prepare data with padding
        std::vector<uint8_t> padded_data = input;
        addPadding(padded_data);
        
        // Encrypt using Crypto++
        CBC_Mode<AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
        
        output.resize(padded_data.size());
        ArraySource as(padded_data.data(), padded_data.size(), true,
            new StreamTransformationFilter(encryptor,
                new ArraySink(output.data(), output.size())
            )
        );
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return false;
    }
}

bool AESCrypto::decryptData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output,
                           const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    try {
        // Validate key size
        if (key.size() != 16 && key.size() != 24 && key.size() != 32) {
            std::cerr << "Invalid AES key size: " << key.size() << std::endl;
            return false;
        }
        
        if (iv.size() != AES::BLOCKSIZE) {
            std::cerr << "Invalid IV size: " << iv.size() << std::endl;
            return false;
        }
        
        // Decrypt using Crypto++
        CBC_Mode<AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());
        
        output.resize(input.size());
        ArraySource as(input.data(), input.size(), true,
            new StreamTransformationFilter(decryptor,
                new ArraySink(output.data(), output.size())
            )
        );
        
        // Remove padding
        removePadding(output);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint8_t> AESCrypto::deriveKeyPBKDF2(const std::string& password,
                                               const std::vector<uint8_t>& salt,
                                               size_t key_size, int iterations) {
    try {
        std::vector<uint8_t> key(key_size);
        
        PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
        pbkdf.DeriveKey(key.data(), key.size(), 0,
                       reinterpret_cast<const byte*>(password.data()), password.size(),
                       salt.data(), salt.size(), iterations);
        
        return key;
        
    } catch (const std::exception& e) {
        std::cerr << "PBKDF2 error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<uint8_t> AESCrypto::generateRandomIV() {
    AutoSeededRandomPool rng;
    std::vector<uint8_t> iv(AES::BLOCKSIZE);
    rng.GenerateBlock(iv.data(), iv.size());
    return iv;
}

std::vector<uint8_t> AESCrypto::generateRandomKey(size_t size) {
    if (size != 16 && size != 24 && size != 32) {
        size = 32; // Default to 256-bit
    }
    
    AutoSeededRandomPool rng;
    std::vector<uint8_t> key(size);
    rng.GenerateBlock(key.data(), key.size());
    return key;
}

bool AESCrypto::validateKey(const std::vector<uint8_t>& key) {
    return (key.size() == 16 || key.size() == 24 || key.size() == 32);
}

void AESCrypto::addPadding(std::vector<uint8_t>& data) {
    size_t block_size = AES::BLOCKSIZE;
    size_t padding = block_size - (data.size() % block_size);
    
    for (size_t i = 0; i < padding; i++) {
        data.push_back(static_cast<uint8_t>(padding));
    }
}

void AESCrypto::removePadding(std::vector<uint8_t>& data) {
    if (data.empty()) return;
    
    uint8_t padding = data.back();
    if (padding > 0 && padding <= AES::BLOCKSIZE) {
        data.resize(data.size() - padding);
    }
}

std::vector<uint8_t> AESCrypto::xorWithIV(const std::vector<uint8_t>& data, 
                                         const std::vector<uint8_t>& iv) {
    std::vector<uint8_t> result(data.size());
    
    for (size_t i = 0; i < data.size(); i++) {
        result[i] = data[i] ^ iv[i % iv.size()];
    }
    
    return result;
}