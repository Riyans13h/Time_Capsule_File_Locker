#include "rsa_utils.h"
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/files.h>
#include <cryptopp/base64.h>
#include <cryptopp/pem.h>
#include <cryptopp/sha.h>
#include <iostream>
#include <fstream>

using namespace CryptoPP;

RSACrypto::RSACrypto() {
}

RSACrypto::~RSACrypto() {
}

bool RSACrypto::generateKeyPair(int key_size, std::string& private_key, std::string& public_key) {
    try {
        AutoSeededRandomPool rng;
        
        // Generate private key
        RSA::PrivateKey privateKey;
        privateKey.GenerateRandomWithKeySize(rng, key_size);
        
        // Generate public key
        RSA::PublicKey publicKey(privateKey);
        
        // Save keys to strings in PEM format
        private_key.clear();
        public_key.clear();
        
        StringSink private_sink(private_key);
        PEM_Save(private_sink, privateKey);
        
        StringSink public_sink(public_key);
        PEM_Save(public_sink, publicKey);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "RSA key generation error: " << e.what() << std::endl;
        return false;
    }
}

bool RSACrypto::generateKeyPairToFiles(const std::string& private_key_path, 
                                      const std::string& public_key_path, 
                                      int key_size) {
    try {
        std::string private_key, public_key;
        if (!generateKeyPair(key_size, private_key, public_key)) {
            return false;
        }
        
        // Save private key
        std::ofstream priv_file(private_key_path);
        if (!priv_file) {
            std::cerr << "Cannot create private key file: " << private_key_path << std::endl;
            return false;
        }
        priv_file << private_key;
        priv_file.close();
        
        // Save public key
        std::ofstream pub_file(public_key_path);
        if (!pub_file) {
            std::cerr << "Cannot create public key file: " << public_key_path << std::endl;
            return false;
        }
        pub_file << public_key;
        pub_file.close();
        
        std::cout << "RSA key pair generated successfully:" << std::endl;
        std::cout << "  Private key: " << private_key_path << std::endl;
        std::cout << "  Public key:  " << public_key_path << std::endl;
        std::cout << "  Key size:    " << key_size << " bits" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Key file generation error: " << e.what() << std::endl;
        return false;
    }
}

bool RSACrypto::encryptFile(const std::string& public_key_path, 
                           const std::vector<uint8_t>& input_data,
                           const std::string& output_file) {
    try {
        // Load public key
        std::string public_key_pem = loadKeyFromFile(public_key_path);
        if (public_key_pem.empty()) {
            return false;
        }
        
        // Encrypt data
        std::string ciphertext;
        std::string plaintext(input_data.begin(), input_data.end());
        
        if (!encryptWithPublicKey(public_key_pem, plaintext, ciphertext)) {
            return false;
        }
        
        // Save encrypted data
        std::ofstream out_file(output_file, std::ios::binary);
        if (!out_file) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return false;
        }
        
        out_file.write(ciphertext.data(), ciphertext.size());
        out_file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "File encryption error: " << e.what() << std::endl;
        return false;
    }
}

bool RSACrypto::decryptFile(const std::string& private_key_path,
                           const std::vector<uint8_t>& encrypted_data,
                           std::vector<uint8_t>& decrypted_data) {
    try {
        // Load private key
        std::string private_key_pem = loadKeyFromFile(private_key_path);
        if (private_key_pem.empty()) {
            return false;
        }
        
        // Decrypt data
        std::string plaintext;
        std::string ciphertext(encrypted_data.begin(), encrypted_data.end());
        
        if (!decryptWithPrivateKey(private_key_pem, ciphertext, plaintext)) {
            return false;
        }
        
        decrypted_data.assign(plaintext.begin(), plaintext.end());
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "File decryption error: " << e.what() << std::endl;
        return false;
    }
}

bool RSACrypto::encryptWithPublicKey(const std::string& public_key_pem,
                                    const std::string& plaintext,
                                    std::string& ciphertext) {
    try {
        // Load public key from PEM string
        RSA::PublicKey publicKey;
        StringSource public_key_source(public_key_pem, true);
        PEM_Load(public_key