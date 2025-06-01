#include <iostream>
#include <fstream>
#include <unordered_map>
#include <bitset>
#include <string>
using namespace std;

struct HuffNode {
    char data;
    HuffNode *left, *right;
    HuffNode(char data) : data(data), left(nullptr), right(nullptr) {}
};

HuffNode* rebuildTree(const unordered_map<char, string>& huffCode) {
    HuffNode* root = new HuffNode('\0');
    for (const auto& pair : huffCode) {
        HuffNode* current = root;
        for (char bit : pair.second) {
            if (bit == '0') {
                if (!current->left) current->left = new HuffNode('\0');
                current = current->left;
            } else {
                if (!current->right) current->right = new HuffNode('\0');
                current = current->right;
            }
        }
        current->data = pair.first;
    }
    return root;
}

void decompressFile(const string& inputFile, const string& outputFile) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);

    // Step 1: Read Huffman Code Table
    unordered_map<char, string> huffCode;
    int codeTableSize;
    inFile >> codeTableSize;
    inFile.ignore(); // Skip newline

    for (int i = 0; i < codeTableSize; ++i) {
        char ch = inFile.get(); // Read character
        inFile.ignore(); // Skip null terminator
        string code;
        getline(inFile, code, '\0'); // Read code until null terminator
        huffCode[ch] = code;
    }

    // Step 2: Rebuild Huffman Tree
    HuffNode* root = rebuildTree(huffCode);

    // Step 3: Read compressed data
    int padding;
    inFile >> padding;
    inFile.ignore(); // Skip newline

    string bitStream;
    char byte;
    while (inFile.get(byte)) {
        bitset<8> bits(byte);
        bitStream += bits.to_string();
    }

    // Remove padding
    bitStream = bitStream.substr(0, bitStream.length() - padding);

    // Step 4: Decode using Huffman Tree
    HuffNode* current = root;
    for (char bit : bitStream) {
        current = (bit == '0') ? current->left : current->right;
        if (!current->left && !current->right) {
            outFile << current->data;
            current = root;
        }
    }

    inFile.close();
    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <compressed_file> <output_file>\n";
        return 1;
    }
    decompressFile(argv[1], argv[2]);
    cout << "File decompressed successfully!\n";
    return 0;
}