#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

using namespace CryptoPP;

struct FileMetadata {
    std::string filename;
    std::string hash;
    std::time_t createTime;
    std::time_t unlockTime;
    std::string encryptionType;
};
std::string calculateSHA256(const std::string& filePath) {
    try {
        // First verify file exists and is readable
        std::ifstream testFile(filePath.c_str());
        if (!testFile.good()) {
            throw std::runtime_error("Cannot access file: " + filePath);
        }
        testFile.close();

        SHA256 hash;
        std::string digest;
        
        FileSource(
            filePath.c_str(),
            true,
            new HashFilter(
                hash,
                new HexEncoder(
                    new StringSink(digest),
                    false // lowercase
                )
            )
        );
        
        return digest;
    }
    catch(const CryptoPP::Exception& e) {
        std::cerr << "CryptoPP Error: " << e.what() << std::endl;
        throw std::runtime_error("Hash computation failed for file: " + filePath);
    }
    catch(const std::exception& e) {
        throw std::runtime_error(std::string("File access error: ") + e.what());
    }
}
void writeMetadata(const FileMetadata& meta, const std::string& outputPath) {
    std::ofstream out(outputPath, std::ios::binary);
    out << "FILENAME=" << meta.filename << "\n"
        << "SHA256=" << meta.hash << "\n"
        << "CREATED=" << meta.createTime << "\n"
        << "UNLOCK=" << meta.unlockTime << "\n"
        << "ENCRYPTION=" << meta.encryptionType << "\n";
}

FileMetadata generateMetadata(const std::string& inputFile, const std::string& unlockDate) {
    FileMetadata meta;
    
    // Parse unlock date (YYYY-MM-DD)
    struct tm tm = {};
    strptime(unlockDate.c_str(), "%Y-%m-%d", &tm);
    
    meta.filename = inputFile.substr(inputFile.find_last_of("/\\") + 1);
    meta.hash = calculateSHA256(inputFile);
    meta.createTime = std::time(nullptr);
    meta.unlockTime = mktime(&tm);
    meta.encryptionType = "AES-256-CBC";
    
    return meta;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <unlock_date> <output_meta>\n";
        return 1;
    }

    try {
        FileMetadata meta = generateMetadata(argv[1], argv[2]);
        writeMetadata(meta, argv[3]);
        std::cout << "Metadata generated: " << argv[3] << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}