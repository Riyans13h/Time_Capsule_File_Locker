#include "encryptor.h"
#include "utils.h"
#include "../shared/include/huffman.h"
#include "../shared/include/aes_cbc.h"
#include "../shared/include/rsa_utils.h"
#include "../shared/include/hash_utils.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <curl/curl.h>

Encryptor::Encryptor() {
    // Initialize Crypto++ if needed
}

Encryptor::~Encryptor() {
    // Cleanup if needed
}

bool Encryptor::encryptAndUpload(const EncryptionConfig& config) {
    std::cout << "Starting encryption and upload process..." << std::endl;
    
    // Validate configuration
    if (!validateConfig(config)) {
        std::cerr << "Configuration validation failed" << std::endl;
        return false;
    }
    
    try {
        // Step 1: Compress the file
        std::cout << "Step 1: Compressing file..." << std::endl;
        if (!compressFile(config.input_file, config.compressed_file)) {
            std::cerr << "File compression failed" << std::endl;
            return false;
        }
        std::cout << "Compression completed: " << config.compressed_file << std::endl;
        
        // Step 2: Generate AES key
        std::cout << "Step 2: Generating encryption keys..." << std::endl;
        std::vector<uint8_t> aes_key, salt, iv;
        if (!generateAESKey(config.password, aes_key, salt, iv)) {
            std::cerr << "AES key generation failed" << std::endl;
            return false;
        }
        
        // Step 3: Encrypt the compressed file
        std::cout << "Step 3: Encrypting file..." << std::endl;
        if (!encryptFile(config.compressed_file, config.encrypted_file, aes_key, iv)) {
            std::cerr << "File encryption failed" << std::endl;
            return false;
        }
        std::cout << "Encryption completed: " << config.encrypted_file << std::endl;
        
        // Step 4: Compute SHA256 hash
        std::cout << "Step 4: Computing file hash..." << std::endl;
        std::string sha256_hash = computeSHA256(config.encrypted_file);
        if (sha256_hash.empty()) {
            std::cerr << "SHA256 computation failed" << std::endl;
            return false;
        }
        std::cout << "File hash: " << sha256_hash << std::endl;
        
        // Step 5: Create key package
        std::cout << "Step 5: Creating key package..." << std::endl;
        if (!createKeyPackage(aes_key, salt, iv, config.receiver_public_key_path, config.key_package_file)) {
            std::cerr << "Key package creation failed" << std::endl;
            return false;
        }
        std::cout << "Key package created: " << config.key_package_file << std::endl;
        
        // Step 6: Upload to server
        std::cout << "Step 6: Uploading to server..." << std::endl;
        if (!uploadToServer(config, sha256_hash)) {
            std::cerr << "Upload to server failed" << std::endl;
            return false;
        }
        
        // Cleanup temporary files
        cleanupTempFiles(config);
        
        std::cout << "Encryption and upload completed successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error during encryption: " << e.what() << std::endl;
        cleanupTempFiles(config);
        return false;
    }
}

bool Encryptor::compressFile(const std::string& input_file, const std::string& output_file) {
    try {
        HuffmanCompressor compressor;
        return compressor.compressFile(input_file, output_file);
    } catch (const std::exception& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return false;
    }
}

bool Encryptor::generateAESKey(const std::string& password, std::vector<uint8_t>& key, 
                              std::vector<uint8_t>& salt, std::vector<uint8_t>& iv) {
    try {
        // Generate random salt and IV
        salt = SenderUtils::generateRandomBytes(16);
        iv = SenderUtils::generateRandomBytes(16);
        
        if (!password.empty()) {
            // Use PBKDF2 to derive key from password
            key = AESCrypto::deriveKeyPBKDF2(password, salt, 32, 100000);
        } else {
            // Generate random AES key
            key = SenderUtils::generateRandomBytes(32);
        }
        
        return !key.empty() && !salt.empty() && !iv.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "Key generation error: " << e.what() << std::endl;
        return false;
    }
}

bool Encryptor::encryptFile(const std::string& input_file, const std::string& output_file,
                           const std::vector<uint8_t>& key, const std::vector<uint8_t>& iv) {
    try {
        AESCrypto aes;
        return aes.encryptFile(input_file, output_file, key, iv);
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return false;
    }
}

bool Encryptor::createKeyPackage(const std::vector<uint8_t>& key, const std::vector<uint8_t>& salt,
                                const std::vector<uint8_t>& iv, const std::string& public_key_path,
                                const std::string& output_file) {
    try {
        // Create key package structure: key_size(1) + key + salt_size(1) + salt + iv_size(1) + iv
        std::vector<uint8_t> key_package;
        
        // Add key
        key_package.push_back(static_cast<uint8_t>(key.size()));
        key_package.insert(key_package.end(), key.begin(), key.end());
        
        // Add salt
        key_package.push_back(static_cast<uint8_t>(salt.size()));
        key_package.insert(key_package.end(), salt.begin(), salt.end());
        
        // Add IV
        key_package.push_back(static_cast<uint8_t>(iv.size()));
        key_package.insert(key_package.end(), iv.begin(), iv.end());
        
        // Encrypt key package with RSA
        RSACrypto rsa;
        return rsa.encryptFile(public_key_path, key_package, output_file);
        
    } catch (const std::exception& e) {
        std::cerr << "Key package creation error: " << e.what() << std::endl;
        return false;
    }
}

// Callback function for libcurl to read file data
size_t read_callback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fread(ptr, size, nmemb, stream);
}

// Callback function for libcurl to write response
size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

bool Encryptor::uploadToServer(const EncryptionConfig& config, const std::string& sha256_hash) {
    CURL* curl;
    CURLcode res;
    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastptr = NULL;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }
    
    try {
        std::string upload_url = config.server_url + "/api/upload";
        
        // Add encrypted file
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "encrypted_file",
                    CURLFORM_FILE, config.encrypted_file.c_str(),
                    CURLFORM_FILENAME, "encrypted_file.bin",
                    CURLFORM_END);
        
        // Add encrypted key package
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "encrypted_key_package",
                    CURLFORM_FILE, config.key_package_file.c_str(),
                    CURLFORM_FILENAME, "encrypted_key.bin",
                    CURLFORM_END);
        
        // Add form fields
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "receiver_id",
                    CURLFORM_COPYCONTENTS, config.receiver_id.c_str(),
                    CURLFORM_END);
                    
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "sender_info",
                    CURLFORM_COPYCONTENTS, config.sender_info.c_str(),
                    CURLFORM_END);
                    
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "original_filename",
                    CURLFORM_COPYCONTENTS, config.input_file.c_str(),
                    CURLFORM_END);
                    
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "release_time",
                    CURLFORM_COPYCONTENTS, config.release_time.c_str(),
                    CURLFORM_END);
                    
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "sha256_hash",
                    CURLFORM_COPYCONTENTS, sha256_hash.c_str(),
                    CURLFORM_END);
        
        curl_formadd(&formpost, &lastptr,
                    CURLFORM_COPYNAME, "file_size",
                    CURLFORM_COPYCONTENTS, std::to_string(getFileSize(config.encrypted_file)).c_str(),
                    CURLFORM_END);
        
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, upload_url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "TimeCapsule-Sender/1.0");
        
        // Response handling
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        // Perform upload
        res = curl_easy_perform(curl);
        
        // Check result
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            curl_formfree(formpost);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
        
        // Check HTTP status code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code != 200) {
            std::cerr << "Server returned error: " << http_code << std::endl;
            std::cerr << "Response: " << response << std::endl;
            curl_formfree(formpost);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
        
        std::cout << "Upload successful! Server response: " << response << std::endl;
        
        // Cleanup
        curl_formfree(formpost);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Upload error: " << e.what() << std::endl;
        curl_formfree(formpost);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }
}

std::string Encryptor::computeSHA256(const std::string& file_path) {
    try {
        HashUtils hasher;
        return hasher.computeFileSHA256(file_path);
    } catch (const std::exception& e) {
        std::cerr << "SHA256 computation error: " << e.what() << std::endl;
        return "";
    }
}

size_t Encryptor::getFileSize(const std::string& file_path) {
    return SenderUtils::getFileSize(file_path);
}

void Encryptor::cleanupTempFiles(const EncryptionConfig& config) {
    try {
        if (SenderUtils::fileExists(config.compressed_file)) {
            std::remove(config.compressed_file.c_str());
        }
    } catch (...) {
        // Ignore cleanup errors
    }
}

bool Encryptor::validateConfig(const EncryptionConfig& config) {
    if (!SenderUtils::fileExists(config.input_file)) {
        std::cerr << "Input file does not exist: " << config.input_file << std::endl;
        return false;
    }
    
    if (!SenderUtils::fileExists(config.receiver_public_key_path)) {
        std::cerr << "Receiver public key does not exist: " << config.receiver_public_key_path << std::endl;
        return false;
    }
    
    if (config.receiver_id.empty()) {
        std::cerr << "Receiver ID cannot be empty" << std::endl;
        return false;
    }
    
    if (config.release_time.empty() || !SenderUtils::isFutureTimestamp(config.release_time)) {
        std::cerr << "Release time must be a valid future timestamp" << std::endl;
        return false;
    }
    
    if (config.server_url.empty()) {
        std::cerr << "Server URL cannot be empty" << std::endl;
        return false;
    }
    
    return true;
}