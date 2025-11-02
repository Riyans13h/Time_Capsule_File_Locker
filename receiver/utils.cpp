#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#endif

namespace ReceiverUtils {

// Callback function for libcurl to write data to file
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// Callback function for libcurl to write data to string
size_t write_string(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

bool fileExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode));
#endif
}

bool readFile(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    return !file.fail();
}

bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return !file.fail();
}

bool createDirectory(const std::string& path) {
#ifdef _WIN32
    return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

std::string getFileExtension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        return filename.substr(dot_pos + 1);
    }
    return "";
}

std::string getFileName(const std::string& path) {
    size_t slash_pos = path.find_last_of("/\\");
    if (slash_pos != std::string::npos) {
        return path.substr(slash_pos + 1);
    }
    return path;
}

bool isValidPrivateKey(const std::string& key_path) {
    FILE* file = fopen(key_path.c_str(), "r");
    if (!file) {
        return false;
    }
    
    RSA* rsa = PEM_read_RSAPrivateKey(file, NULL, NULL, NULL);
    fclose(file);
    
    if (rsa) {
        RSA_free(rsa);
        return true;
    }
    
    return false;
}

bool isValidPublicKey(const std::string& key_path) {
    FILE* file = fopen(key_path.c_str(), "r");
    if (!file) {
        return false;
    }
    
    RSA* rsa = PEM_read_RSA_PUBKEY(file, NULL, NULL, NULL);
    fclose(file);
    
    if (rsa) {
        RSA_free(rsa);
        return true;
    }
    
    return false;
}

std::string getKeyFingerprint(const std::string& key_path) {
    // Read key file
    std::ifstream file(key_path);
   