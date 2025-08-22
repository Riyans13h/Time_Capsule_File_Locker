#include <iostream>
#include <fstream>
#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

using namespace CryptoPP;

void encryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password) {
    try {
        // Generate random IV
        AutoSeededRandomPool prng;
        byte iv[AES::BLOCKSIZE];
        prng.GenerateBlock(iv, sizeof(iv));

        // Derive key from password using SHA-256
        byte key[AES::MAX_KEYLENGTH];
        SHA256().CalculateDigest(key, (const byte*)password.data(), password.size());

        // Write IV to output file
        std::ofstream out(outputFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(iv), AES::BLOCKSIZE);

        // Encrypt and write the rest
        CBC_Mode<AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(key, AES::MAX_KEYLENGTH, iv);

        FileSource fs(
            inputFile.c_str(), 
            true,
            new StreamTransformationFilter(
                encryptor,
                new FileSink(out),
                BlockPaddingSchemeDef::PKCS_PADDING
            )
        );

        std::cout << "File encrypted successfully to: " << outputFile << std::endl;
    }
    catch(const CryptoPP::Exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <password>" << std::endl;
        return 1;
    }

    encryptFile(argv[1], argv[2], argv[3]);
    return 0;
}