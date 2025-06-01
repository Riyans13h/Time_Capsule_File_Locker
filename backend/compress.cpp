#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <bitset>
using namespace std;

// Huffman Tree Node
struct HuffNode {
    char data;
    int freq;
    HuffNode *left, *right;
    HuffNode(char data, int freq) : 
        data(data), freq(freq), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue
struct Compare {
    bool operator()(HuffNode* a, HuffNode* b) {
        return a->freq > b->freq;
    }
};

// Traverse Huffman Tree and generate codes
void generateCodes(HuffNode* root, string code, unordered_map<char, string>& huffCode) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffCode[root->data] = code;
    }
    generateCodes(root->left, code + "0", huffCode);
    generateCodes(root->right, code + "1", huffCode);
}

// Write binary data to file
void writeEncodedData(const string& inputFile, const string& outputFile, const unordered_map<char, string>& huffCode) {
    ifstream inFile(inputFile, ios::binary);
    ofstream outFile(outputFile, ios::binary);
    
    // 1. Write Huffman Code Table (for decoding)
    outFile << huffCode.size() << '\n';
    for (const auto& pair : huffCode) {
        outFile << pair.first << '\0' << pair.second << '\0';
    }

    // 2. Write compressed data
    string encodedStr;
    char ch;
    while (inFile.get(ch)) {
        encodedStr += huffCode.at(ch);
    }
    
    // Pad with '0's to make it 8-bit aligned
    int padding = 8 - (encodedStr.length() % 8);
    encodedStr += string(padding, '0');
    outFile << padding << '\n';

    // Write as bytes
    for (size_t i = 0; i < encodedStr.length(); i += 8) {
        bitset<8> bits(encodedStr.substr(i, 8));
        outFile << static_cast<char>(bits.to_ulong());
    }

    inFile.close();
    outFile.close();
}

// Main compression function
void compressFile(const string& inputFile, const string& outputFile) {
    // Step 1: Calculate character frequencies
    ifstream inFile(inputFile, ios::binary);
    unordered_map<char, int> freq;
    char ch;
    while (inFile.get(ch)) freq[ch]++;
    inFile.close();

    // Step 2: Build Huffman Tree
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

    // Step 3: Generate Huffman Codes
    unordered_map<char, string> huffCode;
    generateCodes(pq.top(), "", huffCode);

    // Step 4: Write compressed data
    writeEncodedData(inputFile, outputFile, huffCode);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file>\n";
        return 1;
    }
    compressFile(argv[1], argv[2]);
    cout << "File compressed successfully!\n";
    return 0;
}