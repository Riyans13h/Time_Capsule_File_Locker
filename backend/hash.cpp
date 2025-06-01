#include <iostream>
#include <fstream>
#include <string>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

using namespace CryptoPP;

void calculateAndSaveSHA256(const std::string& inputFile, const std::string& outputFile) {
    try {
        SHA256 hash;
        
        // Create hash calculation pipeline
        FileSource fileSource(
            inputFile.c_str(),
            true,
            new HashFilter(
                hash,
                new HexEncoder(
                    new FileSink(outputFile.c_str()),
                    false // lowercase output
                )
            )
        );
        
        std::cout << "Hash saved to: " << outputFile << std::endl;
    }
    catch(const CryptoPP::Exception& e) {
        std::cerr << "CryptoPP Error: " << e.what() << std::endl;
        throw std::runtime_error("Hash computation failed");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_hash_file>\n";
        return 1;
    }

    try {
        calculateAndSaveSHA256(argv[1], argv[2]);
        return 0;
    } 
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}