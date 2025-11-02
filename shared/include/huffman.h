#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <memory>

// Huffman Tree Node
struct HuffmanNode {
    uint8_t byte;
    unsigned frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;
    
    HuffmanNode(uint8_t b, unsigned freq) 
        : byte(b), frequency(freq), left(nullptr), right(nullptr) {}
    
    bool isLeaf() const {
        return left == nullptr && right == nullptr;
    }
};

// Comparison for priority queue
struct CompareNodes {
    bool operator()(const std::shared_ptr<HuffmanNode>& a, 
                   const std::shared_ptr<HuffmanNode>& b) {
        return a->frequency > b->frequency;
    }
};

class HuffmanCompressor {
public:
    HuffmanCompressor();
    ~HuffmanCompressor();
    
    // Main compression/decompression functions
    bool compressFile(const std::string& input_file, const std::string& output_file);
    bool decompressFile(const std::string& input_file, const std::string& output_file);
    
    // Memory-based operations
    bool compressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    bool decompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    
    // Utility functions
    double getCompressionRatio() const;
    size_t getOriginalSize() const;
    size_t getCompressedSize() const;
    
private:
    // Compression functions
    void buildFrequencyTable(const std::vector<uint8_t>& data);
    void buildHuffmanTree();
    void generateCodes(const std::shared_ptr<HuffmanNode>& node, const std::string& code);
    std::vector<uint8_t> serializeTree();
    std::vector<uint8_t> encodeData(const std::vector<uint8_t>& data);
    
    // Decompression functions
    std::shared_ptr<HuffmanNode> deserializeTree(const std::vector<uint8_t>& tree_data, size_t& pos);
    std::vector<uint8_t> decodeData(const std::vector<uint8_t>& encoded_data, 
                                   const std::shared_ptr<HuffmanNode>& root,
                                   size_t data_bits);
    
    // Bit manipulation
    void writeBit(uint8_t bit, std::vector<uint8_t>& buffer, size_t& bit_pos);
    uint8_t readBit(const std::vector<uint8_t>& buffer, size_t& bit_pos);
    void writeBits(const std::string& bits, std::vector<uint8_t>& buffer, size_t& bit_pos);
    
    // Member variables
    std::map<uint8_t, unsigned> frequency_table_;
    std::map<uint8_t, std::string> huffman_codes_;
    std::shared_ptr<HuffmanNode> root_;
    
    size_t original_size_;
    size_t compressed_size_;
};

#endif // HUFFMAN_H