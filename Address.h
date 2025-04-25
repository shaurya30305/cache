#ifndef ADDRESS_H
#define ADDRESS_H

#include <cstdint>
#include <string>

class Address {
private:
    uint32_t address;  // Full 32-bit address
    int s;             // Number of set index bits
    int b;             // Number of block bits
    
public:
    // Constructor taking a hex string address (from trace file)
    Address(const std::string& hexAddress, int setBits, int blockBits);
    
    // Constructor taking a uint32_t address
    Address(uint32_t addr, int setBits, int blockBits);
    
    // Get the full 32-bit address
    uint32_t getAddress() const;
    
    // Extract tag bits
    uint32_t getTag() const;
    
    // Extract set index
    uint32_t getIndex() const;
    
    // Extract block offset
    uint32_t getOffset() const;
    
    // Get word offset within block (for 4-byte words)
    uint32_t getWordOffset() const;
    
    // Get byte offset within word
    uint32_t getByteOffset() const;
    
    // Get the address of the block containing this address
    uint32_t getBlockAddress() const;
    
    // Get the address of the word containing this address
    uint32_t getWordAddress() const;
    
    // Check if address aligns with word boundary
    bool isWordAligned() const;
    
    // Convert to hex string representation
    std::string toHexString() const;
    
    // Convert to binary string representation
    std::string toBinaryString() const;
    
    // Get string representation with breakdown
    std::string toString() const;
};

#endif // ADDRESS_H