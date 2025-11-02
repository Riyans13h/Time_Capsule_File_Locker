#include "keygen.h"
#include "../shared/include/rsa_utils.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

KeyGenerator::KeyGenerator() {
    // Initialize any required resources
}

KeyGenerator::~KeyGenerator() {
    // Cleanup if needed
}

bool KeyGenerator::generateKeyPair(const KeyGenConfig& config) {
    std::cout << "🔑 Generating RSA Key Pair..." << std::endl;
    
    try {
        // Create output directory if it doesn't exist
        if (!createDirectory(config.output_dir)) {
            std::cerr << "Failed to create output directory: " << config.output_dir << std::endl;
            return false;
        }
        
        // Generate file paths
        std::string private_key_path = config.output_dir + "/" + config.key_name + "_private.pem";
        std::string public_key_path = config.output_dir + "/" + config.key_name + "_public.pem";
        
        // Check if files already exist
        if (!config.overwrite) {
            if (fileExists(private_key_path) || fileExists(public_key_path)) {
                std::cerr << "Key files already exist. Use --overwrite to replace them." << std::endl;
                return false;
            }
        }
        
        // Generate RSA keys
        std::string private_key, public_key;
        if (!generateRSAKeys(config.key_size, private_key, public_key)) {
            std::cerr << "Failed to generate RSA keys" << std::endl;
            return false;
        }
        
        // Save keys to files
        if (!saveKeyToFile(private_key, private_key_path)) {
            std::cerr << "Failed to save private key" << std::endl;
            return false;
        }
        
        if (!saveKeyToFile(public_key, public_key_path)) {
            std::cerr << "Failed to save public key" << std::endl;
            // Clean up private key file
            std::remove(private_key_path.c_str());
            return false;
        }
        
        // Validate the key pair
        if (!validateKeyPair(private_key_path, public_key_path)) {
            std::cerr << "Key pair validation failed" << std::endl;
            std::remove(private_key_path.c_str());
            std::remove(public_key_path.c_str());
            return false;
        }
        
        if (config.verbose) {
            printKeyInfo(public_key_path, private_key_path);
        }
        
        std::cout << "✅ Key pair generated successfully!" << std::endl;
        std::cout << "📁 Private Key: " << private_key_path << std::endl;
        std::cout << "📁 Public Key:  " << public_key_path << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Key generation error: " << e.what() << std::endl;
        return false;
    }
}

bool KeyGenerator::generateRSAKeys(int key_size, std::string& private_key, std::string& public_key) {
    try {
        RSACrypto rsa;
        return rsa.generateKeyPair(key_size, private_key, public_key);
    } catch (const std::exception& e) {
        std::cerr << "RSA key generation error: " << e.what() << std::endl;
        return false;
    }
}

bool KeyGenerator::saveKeyToFile(const std::string& key_data, const std::string& file_path) {
    try {
        std::ofstream file(file_path);
        if (!file) {
            std::cerr << "Cannot open file for writing: " << file_path << std::endl;
            return false;
        }
        
        file << key_data;
        file.close();
        
        if (file.fail()) {
            std::cerr << "Failed to write key to file: " << file_path << std::endl;
            return false;
        }
        
        // Set secure permissions on private key file
        if (file_path.find("private") != std::string::npos) {
#ifdef _WIN32
            // Windows: set hidden attribute
            SetFileAttributesA(file_path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
            // Unix: set permissions to 600 (owner read/write only)
            chmod(file_path.c_str(), S_IRUSR | S_IWUSR);
#endif
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving key to file: " << e.what() << std::endl;
        return false;
    }
}

bool KeyGenerator::validateKeyPair(const std::string& private_key_path, const std::string& public_key_path) {
    try {
        // Read the keys
        std::ifstream priv_file(private_key_path);
        std::ifstream pub_file(public_key_path);
        
        if (!priv_file || !pub_file) {
            std::cerr << "Cannot open key files for validation" << std::endl;
            return false;
        }
        
        std::string private_key((std::istreambuf_iterator<char>(priv_file)),
                               std::istreambuf_iterator<char>());
        std::string public_key((std::istreambuf_iterator<char>(pub_file)),
                              std::istreambuf_iterator<char>());
        
        // Basic validation - check PEM headers
        if (private_key.find("-----BEGIN PRIVATE KEY-----") == std::string::npos ||
            private_key.find("-----END PRIVATE KEY-----") == std::string::npos) {
            std::cerr << "Invalid private key format" << std::endl;
            return false;
        }
        
        if (public_key.find("-----BEGIN PUBLIC KEY-----") == std::string::npos ||
            public_key.find("-----END PUBLIC KEY-----") == std::string::npos) {
            std::cerr << "Invalid public key format" << std::endl;
            return false;
        }
        
        // Test encryption/decryption with the key pair
        RSACrypto rsa;
        std::string test_message = "TimeCapsule Key Validation Test";
        std::string encrypted, decrypted;
        
        if (!rsa.encryptWithPublicKey(public_key, test_message, encrypted)) {
            std::cerr << "Encryption test failed" << std::endl;
            return false;
        }
        
        if (!rsa.decryptWithPrivateKey(private_key, encrypted, decrypted)) {
            std::cerr << "Decryption test failed" << std::endl;
            return false;
        }
        
        if (test_message != decrypted) {
            std::cerr << "Key pair validation failed - decrypted message doesn't match" << std::endl;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Key validation error: " << e.what() << std::endl;
        return false;
    }
}

std::string KeyGenerator::getKeyFingerprint(const std::string& key_data) {
    // Create SHA256 fingerprint of the key
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, key_data.c_str(), key_data.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
        if (i < SHA256_DIGEST_LENGTH - 1) {
            ss << ":";
        }
    }
    
    return ss.str();
}

void KeyGenerator::printKeyInfo(const std::string& public_key_path, const std::string& private_key_path) {
    std::cout << "\n📋 Key Information:" << std::endl;
    std::cout << "──────────────────" << std::endl;
    
    // Read public key
    std::ifstream pub_file(public_key_path);
    if (pub_file) {
        std::string public_key((std::istreambuf_iterator<char>(pub_file)),
                             std::istreambuf_iterator<char>());
        
        std::string fingerprint = getKeyFingerprint(public_key);
        std::cout << "🔑 Public Key Fingerprint: " << fingerprint << std::endl;
        std::cout << "📍 Public Key Path: " << public_key_path << std::endl;
    }
    
    // Read private key
    std::ifstream priv_file(private_key_path);
    if (priv_file) {
        std::string private_key((std::istreambuf_iterator<char>(priv_file)),
                              std::istreambuf_iterator<char>());
        
        std::string fingerprint = getKeyFingerprint(private_key);
        std::cout << "🔒 Private Key Fingerprint: " << fingerprint << std::endl;
        std::cout << "📍 Private Key Path: " << private_key_path << std::endl;
    }
    
    std::cout << "💡 Next Steps:" << std::endl;
    std::cout << "   1. Upload the public key to the Time Capsule server" << std::endl;
    std::cout << "   2. Keep the private key secure and never share it" << std::cout << std::endl;
    std::cout << "   3. Back up your private key in a secure location" << std::endl;
}

bool KeyGenerator::fileExists(const std::string& path) {
    return ReceiverUtils::fileExists(path);
}

bool KeyGenerator::createDirectory(const std::string& path) {
    return ReceiverUtils::createDirectory(path);
}

std::string KeyGenerator::generateKeyId() {
    // Generate a unique key identifier
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::stringstream ss;
    ss << "key_" << timestamp << "_" << std::rand() % 10000;
    return ss.str();
}