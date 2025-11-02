#include "decryptor.h"
#include "utils.h"
#include "../shared/include/huffman.h"
#include "../shared/include/aes_cbc.h"
#include "../shared/include/rsa_utils.h"
#include "../shared/include/hash_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>

Decryptor::Decryptor() {
    // Initialize any required resources
}

Decryptor::~Decryptor() {
    // Cleanup if needed
}

bool Decryptor::downloadAndDecrypt(const DecryptionConfig& config) {
    std::cout << "🔓 Starting decryption process..." << std::endl;
    
    // Validate configuration
    if (!validateConfig(config)) {
        std::cerr << "Configuration validation failed" << std::endl;
        return false;
    }
    
    try {
        // Step 1: Get capsule information
        std::cout << "Step 1: Retrieving capsule information..." << std::endl;
        CapsuleInfo capsule_info;
        if (!getCapsuleInfo(config.server_url, config.capsule_id, capsule_info)) {
            std::cerr << "Failed to get capsule information" << std::endl;
            return false;
        }
        
        // Check if capsule is delivered
        if (capsule_info.status != "delivered") {
            std::cerr << "Capsule is not available for download. Status: " << capsule_info.status << std::endl;
            return false;
        }
        
        std::cout << "📦 Capsule Info:" << std::endl;
        std::cout << "   File: " << capsule_info.original_filename << std::endl;
        std::cout << "   Size: " << ReceiverUtils::formatFileSize(capsule_info.file_size) << std::endl;
        std::cout << "   Sender: " << capsule_info.sender_info << std::endl;
        std::cout << "   Released: " << capsule_info.delivered_at << std::endl;
        
        // Create output directory
        if (!ReceiverUtils::createDirectory(config.output_dir)) {
            std::cerr << "Failed to create output directory: " << config.output_dir << std::endl;
            return false;
        }
        
        // Generate file paths
        std::string encrypted_file_path = config.output_dir + "/encrypted_file.bin";
        std::string encrypted_key_path = config.output_dir + "/encrypted_key.bin";
        std::string compressed_file_path = config.output_dir + "/compressed_file.bin";
        std::string output_file_path = config.output_dir + "/" + capsule_info.original_filename;
        
        // Step 2: Download encrypted file
        std::cout << "Step 2: Downloading encrypted file..." << std::endl;
        std::string file_url = config.server_url + "/api/release/download/file/" + config.capsule_id;
        if (!downloadFile(file_url, encrypted_file_path)) {
            std::cerr << "Failed to download encrypted file" << std::endl;
            return false;
        }
        
        // Step 3: Download encrypted key package
        std::cout << "Step 3: Downloading key package..." << std::endl;
        std::string key_url = config.server_url + "/api/release/download/key/" + config.capsule_id;
        if (!downloadFile(key_url, encrypted_key_path)) {
            std::cerr << "Failed to download key package" << std::endl;
            cleanupDownloadedFiles(config);
            return false;
        }
        
        // Step 4: Decrypt key package
        std::cout << "Step 4: Decrypting key package..." << std::endl;
        std::vector<uint8_t> aes_key, salt, iv;
        if (!decryptKeyPackage(encrypted_key_path, config.private_key_path, aes_key, salt, iv)) {
            std::cerr << "Failed to decrypt key package" << std::endl;
            cleanupDownloadedFiles(config);
            return false;
        }
        
        // If password was provided during encryption, derive the key
        if (!config.password.empty()) {
            std::cout << "Step 4a: Deriving AES key from password..." << std::endl;
            if (!deriveAESKeyFromPassword(salt, config.password, aes_key)) {
                std::cerr << "Failed to derive AES key from password" << std::endl;
                cleanupDownloadedFiles(config);
                return false;
            }
        }
        
        // Step 5: Decrypt the file
        std::cout << "Step 5: Decrypting file..." << std::endl;
        if (!decryptFile(encrypted_file_path, compressed_file_path, aes_key, iv)) {
            std::cerr << "Failed to decrypt file" << std::endl;
            cleanupDownloadedFiles(config);
            return false;
        }
        
        // Step 6: Decompress the file
        std::cout << "Step 6: Decompressing file..." << std::endl;
        if (!decompressFile(compressed_file_path, output_file_path)) {
            std::cerr << "Failed to decompress file" << std::endl;
            cleanupDownloadedFiles(config);
            return false;
        }
        
        // Step 7: Verify file integrity
        std::cout << "Step 7: Verifying file integrity..." << std::endl;
        if (!verifyFileHash(output_file_path, capsule_info.sha256_hash)) {
            std::cerr << "File integrity check failed!" << std::endl;
            std::cerr << "The file may have been tampered with or corrupted." << std::endl;
            cleanupDownloadedFiles(config);
            return false;
        }
        
        // Cleanup temporary files
        cleanupDownloadedFiles(config);
        
        std::cout << "✅ File decrypted successfully!" << std::endl;
        std::cout << "📁 Output file: " << output_file_path << std::endl;
        std::cout << "📊 File size: " << ReceiverUtils::formatFileSize(ReceiverUtils::getFileSize(output_file_path)) << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        cleanupDownloadedFiles(config);
        return false;
    }
}

bool Decryptor::getCapsuleInfo(const std::string& server_url, const std::string& capsule_id, CapsuleInfo& info) {
    try {
        std::string url = server_url + "/api/release/metadata/" + capsule_id;
        std::string response = ReceiverUtils::httpGet(url);
        
        if (response.empty()) {
            std::cerr << "Empty response from server" << std::endl;
            return false;
        }
        
        // Parse JSON response
        Json::CharReaderBuilder reader;
        Json::Value root;
        std::string errors;
        
        std::istringstream response_stream(response);
        if (!Json::parseFromStream(reader, response_stream, &root, &errors)) {
            std::cerr << "Failed to parse JSON response: " << errors << std::endl;
            return false;
        }
        
        // Check for error
        if (root.isMember("error")) {
            std::cerr << "Server error: " << root["error"].asString() << std::endl;
            return false;
        }
        
        // Extract capsule information
        Json::Value capsule = root["capsule"];
        info.capsule_id = capsule["capsule_id"].asString();
        info.sender_info = capsule["sender_info"].asString();
        info.original_filename = capsule["original_filename"].asString();
        info.file_size = capsule["file_size"].asUInt64();
        info.sha256_hash = capsule["sha256_hash"].asString();
        info.release_time = capsule["release_time"].asString();
        info.status = capsule["status"].asString();
        info.created_at = capsule["created_at"].asString();
        info.delivered_at = capsule["delivered_at"].asString();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting capsule info: " << e.what() << std::endl;
        return false;
    }
}

bool Decryptor::downloadFile(const std::string& url, const std::string& output_path) {
    return ReceiverUtils::downloadFromUrl(url, output_path);
}

bool Decryptor::decryptKeyPackage(const std::string& encrypted_key_path, 
                                 const std::string& private_key_path,
                                 std::vector<uint8_t>& aes_key,
                                 std::vector<uint8_t>& salt,
                                 std::vector<uint8_t>& iv) {
    try {
        // Read encrypted key package
        std::vector<uint8_t> encrypted_data;
        if (!ReceiverUtils::readFile(encrypted_key_path, encrypted_data)) {
            std::cerr << "Failed to read encrypted key package" << std::endl;
            return false;
        }
        
        // Decrypt with RSA
        RSACrypto rsa;
        std::vector<uint8_t> decrypted_data;
        if (!rsa.decryptFile(private_key_path, encrypted_data, decrypted_data)) {
            std::cerr << "Failed to decrypt key package with RSA" << std::endl;
            return false;
        }
        
        // Parse key package structure
        std::vector<uint8_t> key_package = ReceiverUtils::parseKeyPackage(decrypted_data);
        if (key_package.empty()) {
            std::cerr << "Failed to parse key package" << std::endl;
            return false;
        }
        
        // Extract components (structure: key_size(1) + key + salt_size(1) + salt + iv_size(1) + iv)
        size_t pos = 0;
        
        // Extract key
        uint8_t key_size = key_package[pos++];
        aes_key.assign(key_package.begin() + pos, key_package.begin() + pos + key_size);
        pos += key_size;
        
        // Extract salt
        uint8_t salt_size = key_package[pos++];
        salt.assign(key_package.begin() + pos, key_package.begin() + pos + salt_size);
        pos += salt_size;
        
        // Extract IV
        uint8_t iv_size = key_package[pos++];
        iv.assign(key_package.begin() + pos, key_package.begin() + pos + iv_size);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Key package decryption error: " << e.what() << std::endl;
        return false;
    }
}

bool Decryptor::decryptFile(const std::string& encrypted_file_path,
                           const std::string& output_file_path,
                           const std::vector<uint8_t>& key,
                           const std::vector<uint8_t>& iv) {
    try {
        AESCrypto aes;
        return aes.decryptFile(encrypted_file_path, output_file_path, key, iv);
    } catch (const std::exception& e) {
        std::cerr << "File decryption error: " << e.what() << std::endl;
        return false;
    }
}

bool Decryptor::decompressFile(const std::string& compressed_file_path, 
                              const std::string& output_file_path) {
    try {
        HuffmanCompressor compressor;
        return compressor.decompressFile(compressed_file_path, output_file_path);
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return false;
    }
}

bool Decryptor::verifyFileHash(const std::string& file_path, const std::string& expected_hash) {
    try {
        HashUtils hasher;
        std::string actual_hash = hasher.computeFileSHA256(file_path);
        
        if (actual_hash != expected_hash) {
            std::cerr << "Hash mismatch!" << std::endl;
            std::cerr << "Expected: " << expected_hash << std::endl;
            std::cerr << "Actual:   " << actual_hash << std::endl;
            return false;
        }
        
        std::cout << "✅ File integrity verified successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Hash verification error: " << e.what() << std::endl;
        return false;
    }
}

bool Decryptor::cleanupDownloadedFiles(const DecryptionConfig& config) {
    bool success = true;
    
    // Remove temporary downloaded files
    std::vector<std::string> temp_files = {
        config.encrypted_file_path,
        config.encrypted_key_path,
        config.output_dir + "/compressed_file.bin"
    };
    
    for (const auto& file : temp_files) {
        if (ReceiverUtils::fileExists(file)) {
            if (std::remove(file.c_str()) != 0) {
                std::cerr << "Warning: Failed to remove temporary file: " << file << std::endl;
                success = false;
            }
        }
    }
    
    return success;
}

std::string Decryptor::getCapsuleStatus(const std::string& server_url, const std::string& capsule_id) {
    try {
        CapsuleInfo info;
        if (getCapsuleInfo(server_url, capsule_id, info)) {
            return info.status;
        }
        return "unknown";
    } catch (...) {
        return "error";
    }
}

bool Decryptor::validateConfig(const DecryptionConfig& config) {
    if (config.capsule_id.empty()) {
        std::cerr << "Capsule ID cannot be empty" << std::endl;
        return false;
    }
    
    if (config.private_key_path.empty() || !ReceiverUtils::fileExists(config.private_key_path)) {
        std::cerr << "Private key file does not exist: " << config.private_key_path << std::endl;
        return false;
    }
    
    if (!ReceiverUtils::isValidPrivateKey(config.private_key_path)) {
        std::cerr << "Invalid private key file: " << config.private_key_path << std::endl;
        return false;
    }
    
    if (config.server_url.empty()) {
        std::cerr << "Server URL cannot be empty" << std::endl;
        return false;
    }
    
    return true;
}

bool Decryptor::deriveAESKeyFromPassword(const std::vector<uint8_t>& salt, 
                                        const std::string& password,
                                        std::vector<uint8_t>& key) {
    try {
        key = AESCrypto::deriveKeyPBKDF2(password, salt, 32, 100000);
        return !key.empty();
    } catch (const std::exception& e) {
        std::cerr << "Password derivation error: " << e.what() << std::endl;
        return false;
    }
}