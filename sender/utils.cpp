#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <curl/curl.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace SenderUtils {

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

size_t getFileSize(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return 0;
    }
    return file.tellg();
}

std::string toHexString(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::vector<uint8_t> fromHexString(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string base64Encode(const std::vector<uint8_t>& data) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string encoded;
    int val = 0, valb = -6;
    
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    
    return encoded;
}

std::vector<uint8_t> base64Decode(const std::string& encoded) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::vector<uint8_t> decoded;
    std::vector<int> T(256, -1);
    
    for (int i = 0; i < 64; i++) {
        T[base64_chars[i]] = i;
    }
    
    int val = 0, valb = -8;
    
    for (uint8_t c : encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(uint8_t((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return decoded;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

bool isValidTimestamp(const std::string& timestamp) {
    // Basic ISO 8601 format validation
    if (timestamp.length() != 20) return false;
    if (timestamp[4] != '-' || timestamp[7] != '-' || timestamp[10] != 'T' || 
        timestamp[13] != ':' || timestamp[16] != ':' || timestamp[19] != 'Z') {
        return false;
    }
    
    try {
        int year = std::stoi(timestamp.substr(0, 4));
        int month = std::stoi(timestamp.substr(5, 2));
        int day = std::stoi(timestamp.substr(8, 2));
        int hour = std::stoi(timestamp.substr(11, 2));
        int minute = std::stoi(timestamp.substr(14, 2));
        int second = std::stoi(timestamp.substr(17, 2));
        
        // Basic validation
        if (year < 2000 || year > 2100) return false;
        if (month < 1 || month > 12) return false;
        if (day < 1 || day > 31) return false;
        if (hour < 0 || hour > 23) return false;
        if (minute < 0 || minute > 59) return false;
        if (second < 0 || second > 59) return false;
        
        return true;
    } catch (...) {
        return false;
    }
}

bool isFutureTimestamp(const std::string& timestamp) {
    if (!isValidTimestamp(timestamp)) return false;
    
    try {
        // Parse the timestamp
        std::tm tm = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        
        auto target_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto current_time = std::chrono::system_clock::now();
        
        return target_time > current_time;
    } catch (...) {
        return false;
    }
}

std::string formatTimestamp(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::vector<uint8_t> generateRandomBytes(size_t length) {
    std::vector<uint8_t> bytes(length);
    std::random_device rd;
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    
    for (size_t i = 0; i < length; i++) {
        bytes[i] = dist(rd);
    }
    
    return bytes;
}

std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    
    for (int i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            ss << "-";
        }
        if (i == 12) {
            ss << 4; // Version 4
        } else if (i == 16) {
            ss << dis2(gen); // Variant
        } else {
            ss << dis(gen);
        }
    }
    
    return ss.str();
}

std::string urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (curl) {
        char* output = curl_easy_escape(curl, value.c_str(), value.length());
        if (output) {
            std::string result(output);
            curl_free(output);
            curl_easy_cleanup(curl);
            return result;
        }
        curl_easy_cleanup(curl);
    }
    return value;
}

bool isValidEmail(const std::string& email) {
    // Basic email validation
    size_t at_pos = email.find('@');
    size_t dot_pos = email.find('.', at_pos);
    
    return (at_pos != std::string::npos && 
            dot_pos != std::string::npos && 
            dot_pos > at_pos + 1 && 
            email.length() > dot_pos + 1);
}

bool isValidFilename(const std::string& filename) {
    // Basic filename validation
    if (filename.empty() || filename.length() > 255) return false;
    
    // Check for invalid characters
    std::string invalid_chars = "<>:\"/\\|?*";
    for (char c : filename) {
        if (invalid_chars.find(c) != std::string::npos) {
            return false;
        }
    }
    
    // Check for reserved names (Windows)
    std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL", 
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_name = filename;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    
    for (const auto& reserved : reserved_names) {
        if (upper_name == reserved) return false;
    }
    
    return true;
}

std::string getSystemInfo() {
    std::stringstream ss;
    
#ifdef _WIN32
    ss << "Windows";
#elif __linux__
    ss << "Linux";
#elif __APPLE__
    ss << "macOS";
#else
    ss << "Unknown";
#endif
    
    ss << " | TimeCapsule Sender 1.0";
    return ss.str();
}

} // namespace SenderUtils