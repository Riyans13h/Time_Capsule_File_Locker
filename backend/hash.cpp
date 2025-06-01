#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>

std::string calculateSHA256(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filePath);
    }

    SHA256_CTX shaContext;
    SHA256_Init(&shaContext);

    char buffer[65536]; // 64KB buffer
    while (file.read(buffer, sizeof(buffer))) {
        SHA256_Update(&shaContext, buffer, file.gcount());
    }
    SHA256_Update(&shaContext, buffer, file.gcount());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &shaContext);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>\n";
        return 1;
    }

    try {
        std::string hash = calculateSHA256(argv[1]);
        std::cout << "SHA-256: " << hash << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}