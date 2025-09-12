
#include <iostream>
#include <string>
#include <vector>

// Forward declarations
std::vector<unsigned char> compressData(const std::vector<unsigned char>& data);
std::vector<unsigned char> decompressData(const std::vector<unsigned char>& compressedData);
std::vector<unsigned char> encryptData(const std::vector<unsigned char>& data, const std::string& password);
std::vector<unsigned char> decryptData(const std::vector<unsigned char>& encryptedData, const std::string& password);
std::string generateHash(const std::vector<unsigned char>& data);
std::vector<unsigned char> serializeMetadata(const TimeCapsuleMetadata& metadata);
TimeCapsuleMetadata deserializeMetadata(const std::vector<unsigned char>& metadataBytes);
bool isUnlockTimeReached(const std::string& unlockTime);

// Create a time capsule file
bool createTimeCapsule(const std::string& inputFile, 
                      const std::string& outputFile,
                      const std::string& password,
                      const std::string& unlockTime) {
    try {
        // Read the input file
        std::vector<unsigned char> fileData = readFile(inputFile);
        std::cout << "Read " << fileData.size() << " bytes from " << inputFile << std::endl;
        
        // Generate file hash
        std::string fileHash = generateHash(fileData);
        std::cout << "Generated file hash: " << fileHash << std::endl;
        
        // Compress the data
        std::vector<unsigned char> compressedData = compressData(fileData);
        std::cout << "Compressed to " << compressedData.size() << " bytes (" 
                  << (100.0 * compressedData.size() / fileData.size()) << "%)" << std::endl;
        
        // Encrypt the compressed data
        std::vector<unsigned char> encryptedData = encryptData(compressedData, password);
        std::cout << "Encrypted to " << encryptedData.size() << " bytes" << std::endl;
        
        // Create metadata
        TimeCapsuleMetadata metadata;
        metadata.originalFilename = inputFile;
        metadata.unlockTime = unlockTime;
        metadata.originalSize = fileData.size();
        metadata.compressedSize = compressedData.size();
        metadata.encryptedSize = encryptedData.size();
        metadata.fileHash = fileHash;
        
        // Serialize metadata
        std::vector<unsigned char> metadataBytes = serializeMetadata(metadata);
        
        // Combine metadata and encrypted data
        std::vector<unsigned char> finalData;
        
        // Add metadata size (4 bytes)
        uint32_t metadataSize = metadataBytes.size();
        finalData.push_back((metadataSize >> 24) & 0xFF);
        finalData.push_back((metadataSize >> 16) & 0xFF);
        finalData.push_back((metadataSize >> 8) & 0xFF);
        finalData.push_back(metadataSize & 0xFF);
        
        // Add metadata
        finalData.insert(finalData.end(), metadataBytes.begin(), metadataBytes.end());
        
        // Add encrypted data
        finalData.insert(finalData.end(), encryptedData.begin(), encryptedData.end());
        
        // Write the final time capsule file
        writeFile(outputFile, finalData);
        std::cout << "Time capsule created: " << outputFile << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating time capsule: " << e.what() << std::endl;
        return false;
    }
}

// Extract from a time capsule file
bool extractFromTimeCapsule(const std::string& inputFile,
                           const std::string& outputFile,
                           const std::string& password) {
    try {
        // Read the time capsule file
        std::vector<unsigned char> capsuleData = readFile(inputFile);
        std::cout << "Read " << capsuleData.size() << " bytes from " << inputFile << std::endl;
        
        // Extract metadata size
        if (capsuleData.size() < 4) {
            throw std::runtime_error("Invalid time capsule file format");
        }
        
        uint32_t metadataSize = (capsuleData[0] << 24) | 
                               (capsuleData[1] << 16) | 
                               (capsuleData[2] << 8) | 
                               capsuleData[3];
        
        if (capsuleData.size() < 4 + metadataSize) {
            throw std::runtime_error("Invalid time capsule file format");
        }
        
        // Extract metadata
        std::vector<unsigned char> metadataBytes(
            capsuleData.begin() + 4,
            capsuleData.begin() + 4 + metadataSize
        );
        
        TimeCapsuleMetadata metadata = deserializeMetadata(metadataBytes);
        std::cout << "Metadata extracted: " << metadata.originalFilename 
                  << ", unlock time: " << metadata.unlockTime << std::endl;
        
        // Check if unlock time has been reached
        if (!isUnlockTimeReached(metadata.unlockTime)) {
            throw std::runtime_error("Unlock time has not been reached yet: " + metadata.unlockTime);
        }
        
        // Extract encrypted data
        std::vector<unsigned char> encryptedData(
            capsuleData.begin() + 4 + metadataSize,
            capsuleData.end()
        );
        
        // Decrypt the data
        std::vector<unsigned char> compressedData = decryptData(encryptedData, password);
        std::cout << "Decrypted to " << compressedData.size() << " bytes" << std::endl;
        
        // Decompress the data
        std::vector<unsigned char> fileData = decompressData(compressedData);
        std::cout << "Decompressed to " << fileData.size() << " bytes" << std::endl;
        
        // Verify file size
        if (fileData.size() != metadata.originalSize) {
            throw std::runtime_error("File size mismatch after decompression");
        }
        
        // Verify file hash
        std::string computedHash = generateHash(fileData);
        if (computedHash != metadata.fileHash) {
            throw std::runtime_error("File hash mismatch. File may have been tampered with.");
        }
        
        // Write the output file
        writeFile(outputFile, fileData);
        std::cout << "File extracted: " << outputFile << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error extracting from time capsule: " << e.what() << std::endl;
        return false;
    }
}

// Main function for command-line usage
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Time Capsule File Locker" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  Create: " << argv[0] << " create <input> <output> <password> <unlock-time>" << std::endl;
        std::cout << "  Extract: " << argv[0] << " extract <input> <output> <password>" << std::endl;
        return 1;
    }
    
    std::string command = argv[1];
    
    if (command == "create" && argc == 6) {
        std::string inputFile = argv[2];
        std::string outputFile = argv[3];
        std::string password = argv[4];
        std::string unlockTime = argv[5];
        
        return createTimeCapsule(inputFile, outputFile, password, unlockTime) ? 0 : 1;
    }
    else if (command == "extract" && argc == 5) {
        std::string inputFile = argv[2];
        std::string outputFile = argv[3];
        std::string password = argv[4];
        
        return extractFromTimeCapsule(inputFile, outputFile, password) ? 0 : 1;
    }
    else {
        std::cerr << "Invalid command or arguments" << std::endl;
        return 1;
    }
}

// WebAssembly wrapper functions
extern "C" {
    int wasm_create_time_capsule(const uint8_t* data, size_t data_size,
                                const char* password, const char* unlockTime,
                                const char* filename, uint8_t** result, size_t* result_size) {
        try {
            std::vector<unsigned char> fileData(data, data + data_size);
            
            // Generate file hash
            std::string fileHash = generateHash(fileData);
            
            // Compress the data
            std::vector<unsigned char> compressedData = compressData(fileData);
            
            // Encrypt the compressed data
            std::vector<unsigned char> encryptedData = encryptData(compressedData, password);
            
            // Create metadata
            TimeCapsuleMetadata metadata;
            metadata.originalFilename = filename;
            metadata.unlockTime = unlockTime;
            metadata.originalSize = fileData.size();
            metadata.compressedSize = compressedData.size();
            metadata.encryptedSize = encryptedData.size();
            metadata.fileHash = fileHash;
            
            // Serialize metadata
            std::vector<unsigned char> metadataBytes = serializeMetadata(metadata);
            
            // Combine metadata and encrypted data
            std::vector<unsigned char> finalData;
            
            // Add metadata size (4 bytes)
            uint32_t metadataSize = metadataBytes.size();
            finalData.push_back((metadataSize >> 24) & 0xFF);
            finalData.push_back((metadataSize >> 16) & 0xFF);
            finalData.push_back((metadataSize >> 8) & 0xFF);
            finalData.push_back(metadataSize & 0xFF);
            
            // Add metadata
            finalData.insert(finalData.end(), metadataBytes.begin(), metadataBytes.end());
            
            // Add encrypted data
            finalData.insert(finalData.end(), encryptedData.begin(), encryptedData.end());
            
            // Allocate memory for the result
            *result = new uint8_t[finalData.size()];
            std::copy(finalData.begin(), finalData.end(), *result);
            *result_size = finalData.size();
            
            return 1; // Success
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 0; // Error
        }
    }
    
    int wasm_extract_time_capsule(const uint8_t* data, size_t data_size,
                                 const char* password, uint8_t** result, 
                                 size_t* result_size, char** metadata_json) {
        try {
            std::vector<unsigned char> capsuleData(data, data + data_size);
            
            // Extract metadata size
            if (capsuleData.size() < 4) {
                throw std::runtime_error("Invalid time capsule file format");
            }
            
            uint32_t metadataSize = (capsuleData[0] << 24) | 
                                   (capsuleData[1] << 16) | 
                                   (capsuleData[2] << 8) | 
                                   capsuleData[3];
            
            if (capsuleData.size() < 4 + metadataSize) {
                throw std::runtime_error("Invalid time capsule file format");
            }
            
            // Extract metadata
            std::vector<unsigned char> metadataBytes(
                capsuleData.begin() + 4,
                capsuleData.begin() + 4 + metadataSize
            );
            
            TimeCapsuleMetadata metadata = deserializeMetadata(metadataBytes);
            
            // Check if unlock time has been reached
            if (!isUnlockTimeReached(metadata.unlockTime)) {
                throw std::runtime_error("Unlock time has not been reached yet: " + metadata.unlockTime);
            }
            
            // Extract encrypted data
            std::vector<unsigned char> encryptedData(
                capsuleData.begin() + 4 + metadataSize,
                capsuleData.end()
            );
            
            // Decrypt the data
            std::vector<unsigned char> compressedData = decryptData(encryptedData, password);
            
            // Decompress the data
            std::vector<unsigned char> fileData = decompressData(compressedData);
            
            // Verify file size
            if (fileData.size() != metadata.originalSize) {
                throw std::runtime_error("File size mismatch after decompression");
            }
            
            // Verify file hash
            std::string computedHash = generateHash(fileData);
            if (computedHash != metadata.fileHash) {
                throw std::runtime_error("File hash mismatch. File may have been tampered with.");
            }
            
            // Allocate memory for the result
            *result = new uint8_t[fileData.size()];
            std::copy(fileData.begin(), fileData.end(), *result);
            *result_size = fileData.size();
            
            // Create JSON metadata
            std::stringstream ss;
            ss << "{"
               << "\"unlockTime\":\"" << metadata.unlockTime << "\","
               << "\"originalFilename\":\"" << metadata.originalFilename << "\","
               << "\"fileHash\":\"" << metadata.fileHash << "\","
               << "\"originalSize\":" << metadata.originalSize << ","
               << "\"compressedSize\":" << metadata.compressedSize << ","
               << "\"encryptedSize\":" << metadata.encryptedSize
               << "}";
            
            std::string metadataStr = ss.str();
            *metadata_json = new char[metadataStr.size() + 1];
            std::copy(metadataStr.begin(), metadataStr.end(), *metadata_json);
            (*metadata_json)[metadataStr.size()] = '\0';
            
            return 1; // Success
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 0; // Error
        }
    }
}