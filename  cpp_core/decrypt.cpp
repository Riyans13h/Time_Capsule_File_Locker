#include <iostream>
#include <fstream>
#include <string>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

using namespace CryptoPP;

void decryptFile(const std::string& inputFile, const std::string& outputFile, const std::string& password) {
    try {
        // Read IV from input file
        byte iv[AES::BLOCKSIZE];
        std::ifstream in(inputFile, std::ios::binary);
        in.read(reinterpret_cast<char*>(iv), AES::BLOCKSIZE);

        // Derive key from password
        byte key[AES::MAX_KEYLENGTH];
        SHA256().CalculateDigest(key, (const byte*)password.data(), password.size());

        // Decrypt the rest
        CBC_Mode<AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key, AES::MAX_KEYLENGTH, iv);

        FileSource fs(
            in,
            true,
            new StreamTransformationFilter(
                decryptor,
                new FileSink(outputFile.c_str()),
                BlockPaddingSchemeDef::PKCS_PADDING
            )
        );

        std::cout << "File decrypted successfully to: " << outputFile << std::endl;
    }
    catch(const CryptoPP::Exception& e) {
        std::cerr << "Decryption error: " << e.what() << std::endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <password>" << std::endl;
        return 1;
    }

    decryptFile(argv[1], argv[2], argv[3]);
    return 0;
}