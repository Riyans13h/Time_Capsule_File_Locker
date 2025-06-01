#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdexcept>

const std::string COMPRESSOR = "./compress";
const std::string DECOMPRESSOR = "./decompress";
const std::string ENCRYPTOR = "./encrypt";
const std::string DECRYPTOR = "./decrypt";
const std::string METADATA_GEN = "./metadata";
const std::string TIME_CHECK = "./time_lock";

void verifyToolsExist() {
    const std::string tools[] = {COMPRESSOR, DECOMPRESSOR, ENCRYPTOR, 
                                DECRYPTOR, METADATA_GEN, TIME_CHECK};
    for (const auto& tool : tools) {
        if (access(tool.c_str(), X_OK) != 0) {
            throw std::runtime_error("Required tool missing or not executable: " + tool);
        }
    }
}

void lockFile(const std::string& input, const std::string& output, 
             const std::string& unlockDate, const std::string& password) {
    // Generate metadata
    std::string metaCmd = METADATA_GEN + " \"" + input + "\" \"" + unlockDate + "\" \"" + output + ".meta\"";
    if (system(metaCmd.c_str()) != 0) {
        throw std::runtime_error("Metadata generation failed");
    }
    
    // Compression -> Encryption pipeline
    std::string tempFile = output + ".temp";
    std::string compressCmd = COMPRESSOR + " \"" + input + "\" \"" + tempFile + "\"";
    std::string encryptCmd = ENCRYPTOR + " \"" + tempFile + "\" \"" + output + "\" \"" + password + "\"";
    
    if (system(compressCmd.c_str()) != 0 || system(encryptCmd.c_str()) != 0) {
        remove(tempFile.c_str());
        throw std::runtime_error("Lock process failed");
    }
    remove(tempFile.c_str());
}

void unlockFile(const std::string& input, const std::string& output, 
               const std::string& password) {
    // Verify time lock
    std::string timeCheckCmd = TIME_CHECK + " \"" + input + ".meta\"";
    if (system(timeCheckCmd.c_str()) != 0) {
        throw std::runtime_error("File is still time-locked or metadata missing");
    }
    
    // Decryption -> Decompression pipeline
    std::string tempFile = output + ".temp";
    std::string decryptCmd = DECRYPTOR + " \"" + input + "\" \"" + tempFile + "\" \"" + password + "\"";
    std::string decompressCmd = DECOMPRESSOR + " \"" + tempFile + "\" \"" + output + "\"";
    
    if (system(decryptCmd.c_str()) != 0 || system(decompressCmd.c_str()) != 0) {
        remove(tempFile.c_str());
        throw std::runtime_error("Unlock process failed");
    }
    remove(tempFile.c_str());
}

int main(int argc, char* argv[]) {
    try {
        verifyToolsExist();
        
        if (argc < 2) {
            throw std::runtime_error(
                "Time Capsule File Locker\n"
                "Usage:\n"
                "  ./main lock <input> <output> <unlock_date> <password>\n"
                "  ./main unlock <input> <output> <password>\n"
            );
        }

        std::string command = argv[1];
        
        if (command == "lock" && argc == 6) {
            lockFile(argv[2], argv[3], argv[4], argv[5]);
            std::cout << "Successfully locked until " << argv[4] << std::endl;
            return 0;
        } 
        else if (command == "unlock" && argc == 5) {
            unlockFile(argv[2], argv[3], argv[4]);
            std::cout << "Successfully unlocked to " << argv[3] << std::endl;
            return 0;
        }
        else {
            throw std::runtime_error("Invalid arguments");
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}