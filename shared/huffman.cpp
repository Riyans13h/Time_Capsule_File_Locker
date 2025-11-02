#include "huffman.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <bitset>

HuffmanCompressor::HuffmanCompressor() 
    : root_(nullptr), original_size_(0), compressed_size_(0) {
}

HuffmanCompressor::~HuffmanCompressor() {
    // Smart pointers handle cleanup
}

bool HuffmanCompressor::compressFile(const std::string& input_file, const std::string& output_file) {
    try {
        // Read input file
        std::ifstream in_file(input_file, std::ios::binary);
        if (!in_file) {
            std::cerr << "Cannot open input file: " << input_file << std::endl;
            return false;
        }
        
        std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(in_file)),
                                      std::istreambuf_iterator<char>());
        in_file.close();
        
        if (input_data.empty()) {
            std::cerr << "Input file is empty: " << input_file << std::endl;
            return false;
        }
        
        original_size_ = input_data.size();
        
        // Compress data
        std::vector<uint8_t> compressed_data;
        if (!compressData(input_data, compressed_data)) {
            std::cerr << "Compression failed" << std::endl;
            return false;
        }
        
        compressed_size_ = compressed_data.size();
        
        // Write compressed file
        std::ofstream out_file(output_file, std::ios::binary);
        if (!out_file) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return false;
        }
        
        out_file.write(reinterpret_cast<const char*>(compressed_data.data()), 
                      compressed_data.size());
        out_file.close();
        
        if (out_file.fail()) {
            std::cerr << "Failed to write compressed file" << std::endl;
            return false;
        }
        
        std::cout << "Compression successful: " << input_file << " -> " << output_file << std::endl;
        std::cout << "Original size: " << original_size_ << " bytes" << std::endl;
        std::cout << "Compressed size: " << compressed_size_ << " bytes" << std::endl;
        std::cout << "Compression ratio: " << getCompressionRatio() << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return false;
    }
}

bool HuffmanCompressor::decompressFile(const std::string& input_file, const std::string& output_file) {
    try {
        // Read compressed file
        std::ifstream in_file(input_file, std::ios::binary);
        if (!in_file) {
            std::cerr << "Cannot open input file: " << input_file << std::endl;
            return false;
        }
        
        std::vector<uint8_t> compressed_data((std::istreambuf_iterator<char>(in_file)),
                                           std::istreambuf_iterator<char>());
        in_file.close();
        
        if (compressed_data.empty()) {
            std::cerr << "Compressed file is empty: " << input_file << std::endl;
            return false;
        }
        
        // Decompress data
        std::vector<uint8_t> decompressed_data;
        if (!decompressData(compressed_data, decompressed_data)) {
            std::cerr << "Decompression failed" << std::endl;
            return false;
        }
        
        // Write decompressed file
        std::ofstream out_file(output_file, std::ios::binary);
        if (!out_file) {
            std::cerr << "Cannot create output file: " << output_file << std::endl;
            return false;
        }
        
        out_file.write(reinterpret_cast<const char*>(decompressed_data.data()), 
                      decompressed_data.size());
        out_file.close();
        
        if (out_file.fail()) {
            std::cerr << "Failed to write decompressed file" << std::endl;
            return false;
        }
        
        std::cout << "Decompression successful: " << input_file << " -> " << output_file << std::endl;
        std::cout << "Decompressed size: " << decompressed_data.size() << " bytes" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return false;
    }
}

bool HuffmanCompressor::compressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    if (input.empty()) {
        return false;
    }
    
    // Build frequency table and Huffman tree
    buildFrequencyTable(input);
    buildHuffmanTree();
    generateCodes(root_, "");
    
    // Serialize Huffman tree and encode data
    std::vector<uint8_t> tree_data = serializeTree();
    std::vector<uint8_t> encoded_data = encodeData(input);
    
    // Prepare output: [tree_size(4 bytes)][tree_data][encoded_data_size(4 bytes)][encoded_data]
    output.clear();
    
    // Write tree size
    uint32_t tree_size = tree_data.size();
    output.insert(output.end(), 
                 reinterpret_cast<uint8_t*>(&tree_size),
                 reinterpret_cast<uint8_t*>(&tree_size) + sizeof(tree_size));
    
    // Write tree data
    output.insert(output.end(), tree_data.begin(), tree_data.end());
    
    // Write encoded data size (in bits)
    uint32_t data_bits = encoded_data.size() * 8;
    output.insert(output.end(),
                 reinterpret_cast<uint8_t*>(&data_bits),
                 reinterpret_cast<uint8_t*>(&data_bits) + sizeof(data_bits));
    
    // Write encoded data
    output.insert(output.end(), encoded_data.begin(), encoded_data.end());
    
    return true;
}

bool HuffmanCompressor::decompressData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    if (input.size() < sizeof(uint32_t) * 2) {
        return false;
    }
    
    try {
        size_t pos = 0;
        
        // Read tree size
        uint32_t tree_size = *reinterpret_cast<const uint32_t*>(input.data() + pos);
        pos += sizeof(tree_size);
        
        if (pos + tree_size + sizeof(uint32_t) > input.size()) {
            return false;
        }
        
        // Read tree data
        std::vector<uint8_t> tree_data(input.begin() + pos, input.begin() + pos + tree_size);
        pos += tree_size;
        
        // Read encoded data size (in bits)
        uint32_t data_bits = *reinterpret_cast<const uint32_t*>(input.data() + pos);
        pos += sizeof(data_bits);
        
        // Read encoded data
        std::vector<uint8_t> encoded_data(input.begin() + pos, input.end());
        
        // Rebuild Huffman tree
        size_t tree_pos = 0;
        root_ = deserializeTree(tree_data, tree_pos);
        
        // Decode data
        output = decodeData(encoded_data, root_, data_bits);
        
        return !output.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return false;
    }
}

void HuffmanCompressor::buildFrequencyTable(const std::vector<uint8_t>& data) {
    frequency_table_.clear();
    for (uint8_t byte : data) {
        frequency_table_[byte]++;
    }
}

void HuffmanCompressor::buildHuffmanTree() {
    std::priority_queue<std::shared_ptr<HuffmanNode>, 
                       std::vector<std::shared_ptr<HuffmanNode>>, 
                       CompareNodes> pq;
    
    // Create leaf nodes for each byte
    for (const auto& pair : frequency_table_) {
        pq.push(std::make_shared<HuffmanNode>(pair.first, pair.second));
    }
    
    // Build tree by combining nodes
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        
        auto parent = std::make_shared<HuffmanNode>(0, left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        
        pq.push(parent);
    }
    
    root_ = pq.top();
}

void HuffmanCompressor::generateCodes(const std::shared_ptr<HuffmanNode>& node, const std::string& code) {
    if (!node) return;
    
    if (node->isLeaf()) {
        huffman_codes_[node->byte] = code;
    } else {
        generateCodes(node->left, code + "0");
        generateCodes(node->right, code + "1");
    }
}

std::vector<uint8_t> HuffmanCompressor::serializeTree() {
    std::vector<uint8_t> result;
    
    // Simple serialization: pre-order traversal
    // For leaf: '1' + byte, for internal node: '0'
    std::function<void(const std::shared_ptr<HuffmanNode>&)> serialize;
    serialize = [&](const std::shared_ptr<HuffmanNode>& node) {
        if (node->isLeaf()) {
            result.push_back(1); // Leaf marker
            result.push_back(node->byte);
        } else {
            result.push_back(0); // Internal node marker
            serialize(node->left);
            serialize(node->right);
        }
    };
    
    serialize(root_);
    return result;
}

std::shared_ptr<HuffmanNode> HuffmanCompressor::deserializeTree(const std::vector<uint8_t>& tree_data, size_t& pos) {
    if (pos >= tree_data.size()) {
        return nullptr;
    }
    
    uint8_t marker = tree_data[pos++];
    
    if (marker == 1) { // Leaf node
        if (pos >= tree_data.size()) {
            return nullptr;
        }
        uint8_t byte = tree_data[pos++];
        return std::make_shared<HuffmanNode>(byte, 0);
    } else if (marker == 0) { // Internal node
        auto node = std::make_shared<HuffmanNode>(0, 0);
        node->left = deserializeTree(tree_data, pos);
        node->right = deserializeTree(tree_data, pos);
        return node;
    }
    
    return nullptr;
}

std::vector<uint8_t> HuffmanCompressor::encodeData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> encoded;
    size_t bit_pos = 0;
    uint8_t current_byte = 0;
    
    for (uint8_t byte : data) {
        const std::string& code = huffman_codes_[byte];
        for (char bit : code) {
            if (bit == '1') {
                current_byte |= (1 << (7 - bit_pos));
            }
            bit_pos++;
            
            if (bit_pos == 8) {
                encoded.push_back(current_byte);
                current_byte = 0;
                bit_pos = 0;
            }
        }
    }
    
    // Add remaining bits
    if (bit_pos > 0) {
        encoded.push_back(current_byte);
    }
    
    return encoded;
}

std::vector<uint8_t> HuffmanCompressor::decodeData(const std::vector<uint8_t>& encoded_data, 
                                                  const std::shared_ptr<HuffmanNode>& root,
                                                  size_t data_bits) {
    std::vector<uint8_t> decoded;
    auto current_node = root;
    size_t bits_processed = 0;
    
    for (uint8_t byte : encoded_data) {
        for (int i = 7; i >= 0; i--) {
            if (bits_processed >= data_bits) {
                break;
            }
            
            uint8_t bit = (byte >> i) & 1;
            
            if (bit == 0) {
                current_node = current_node->left;
            } else {
                current_node = current_node->right;
            }
            
            if (current_node->isLeaf()) {
                decoded.push_back(current_node->byte);
                current_node = root;
            }
            
            bits_processed++;
        }
    }
    
    return decoded;
}

double HuffmanCompressor::getCompressionRatio() const {
    if (original_size_ == 0) return 0.0;
    return static_cast<double>(compressed_size_) / original_size_;
}

size_t HuffmanCompressor::getOriginalSize() const {
    return original_size_;
}

size_t HuffmanCompressor::getCompressedSize() const {
    return compressed_size_;
}