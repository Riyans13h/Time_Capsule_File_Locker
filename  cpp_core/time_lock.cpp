#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>

std::map<std::string, std::string> parseMetadata(const std::string& metaFile) {
    std::ifstream in(metaFile);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open metadata file: " + metaFile);
    }

    std::map<std::string, std::string> metadata;
    std::string line;
    
    while (std::getline(in, line)) {
        size_t delim = line.find('=');
        if (delim != std::string::npos) {
            metadata[line.substr(0, delim)] = line.substr(delim + 1);
        }
    }
    
    if (metadata.empty()) {
        throw std::runtime_error("Empty or invalid metadata file");
    }
    return metadata;
}

bool isUnlockTimeReached(const std::string& metaFile) {
    auto meta = parseMetadata(metaFile);
    
    if (meta.find("UNLOCK") == meta.end()) {
        throw std::runtime_error("Missing UNLOCK field in metadata");
    }

    std::time_t now = std::time(nullptr);
    std::time_t unlockTime;
    
    try {
        unlockTime = std::stol(meta["UNLOCK"]);
    } catch (...) {
        throw std::runtime_error("Invalid UNLOCK timestamp format");
    }

    return now >= unlockTime;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <metadata_file>\n"
                  << "Returns 0 if unlocked, 1 if locked, 2 for errors\n";
        return 2;
    }

    try {
        if (isUnlockTimeReached(argv[1])) {
            std::cout << "File is unlocked\n";
            return 0;
        } else {
            // Calculate remaining time
            auto meta = parseMetadata(argv[1]);
            std::time_t remaining = std::stol(meta["UNLOCK"]) - std::time(nullptr);
            std::cout << "File locks for " << remaining/86400 << " more days\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
}