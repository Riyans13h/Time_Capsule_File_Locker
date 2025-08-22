#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <bitset>
using namespace std;

struct HuffNode {
    unsigned char data;  // Changed to unsigned char for binary files
    int freq;
    HuffNode *left, *right;
    HuffNode(unsigned char data, int freq) : 
        data(data), freq(freq), left(nullptr), right(nullptr) {}
};

struct Compare {
    bool operator()(HuffNode* a, HuffNode* b) {
        return a->freq > b->freq;
    }
};

void generateCodes(HuffNode* root, string code, unordered_map<unsigned char, string>& huffCode) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffCode[root->data] = code;
    }
    generateCodes(root->left, code + "0", huffCode);
    generateCodes(root->right, code + "1", huffCode);
}

void writeEncodedData(const string& inputFile, const string& outputFile, 
                     const unordered_map<unsigned char, string>& huffCode) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);

    // 1. Write Huffman Code Table
    outFile << huffCode.size() << '\n';
    for (const auto& pair : huffCode) {
        outFile << static_cast<int>(pair.first) << '\0' << pair.second << '\0';
    }

    // 2. Write compressed data
    string encodedStr;
    unsigned char ch;
    while (inFile.read(reinterpret_cast<char*>(&ch), sizeof(ch))) {
        encodedStr += huffCode.at(ch);
    }

    int padding = 8 - (encodedStr.length() % 8);
    encodedStr += string(padding, '0');
    outFile << padding << '\n';

    for (size_t i = 0; i < encodedStr.length(); i += 8) {
        bitset<8> bits(encodedStr.substr(i, 8));
        outFile << static_cast<char>(bits.to_ulong());
    }

    inFile.close();
    outFile.close();
}

void compressFile(const string& inputFile, const string& outputFile) {
    ifstream inFile(inputFile, ios::binary);
    unordered_map<unsigned char, int> freq;
    unsigned char ch;
    while (inFile.read(reinterpret_cast<char*>(&ch), sizeof(ch))) {
        freq[ch]++;
    }
    inFile.close();

    priority_queue<HuffNode*, vector<HuffNode*>, Compare> pq;
    for (const auto& pair : freq) {
        pq.push(new HuffNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        HuffNode* left = pq.top(); pq.pop();
        HuffNode* right = pq.top(); pq.pop();
        HuffNode* newNode = new HuffNode('\0', left->freq + right->freq);
        newNode->left = left;
        newNode->right = right;
        pq.push(newNode);
    }

    unordered_map<unsigned char, string> huffCode;
    generateCodes(pq.top(), "", huffCode);
    writeEncodedData(inputFile, outputFile, huffCode);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file.pdf> <output_file.huff>\n";
        return 1;
    }
    compressFile(argv[1], argv[2]);
    cout << "PDF compressed successfully!\n";
    return 0;
}