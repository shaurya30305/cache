#include "Address.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <bitset>

// Constructor for string-based hex address (from trace file)
Address::Address(const std::string& hexAddress, int setBits, int blockBits) 
    : s(setBits), b(blockBits) {
    
    // Remove "0x" prefix if present
    std::string cleanAddr = hexAddress;
    if (hexAddress.substr(0, 2) == "0x") {
        cleanAddr = hexAddress.substr(2);
    }
    
    // Convert hex string to uint32_t
    std::stringstream ss;
    ss << std::hex << cleanAddr;
    ss >> address;
    
    // Ensure address is 32-bit (pad with 0s if needed)
    address = address & 0xFFFFFFFF;
}

// Constructor for direct uint32_t address
Address::Address(uint32_t addr, int setBits, int blockBits) 
    : address(addr & 0xFFFFFFFF), s(setBits), b(blockBits) {
}

// Get the full 32-bit address
uint32_t Address::getAddress() const {
    return address;
}

// Extract the tag bits from the address
uint32_t Address::getTag() const {
    // Tag is the upper 32-s-b bits
    return address >> (s + b);
}

// Extract the set index bits from the address
uint32_t Address::getIndex() const {
    // Index is the middle s bits
    uint32_t mask = (1 << s) - 1;
    return (address >> b) & mask;
}

// Extract the block offset bits from the address
uint32_t Address::getOffset() const {
    // Offset is the lower b bits
    uint32_t mask = (1 << b) - 1;
    return address & mask;
}

// Get word offset within block (for 4-byte words)
uint32_t Address::getWordOffset() const {
    // Word offset is offset / 4 (assuming 4-byte words)
    return (address & ((1 << b) - 1)) >> 2;
}

// Get byte offset within word
uint32_t Address::getByteOffset() const {
    // Byte offset is the lowest 2 bits (for 4-byte words)
    return address & 0x3;
}

// Convert address to hex string
std::string Address::toHexString() const {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << address;
    return ss.str();
}

// Convert address to binary string
std::string Address::toBinaryString() const {
    return std::bitset<32>(address).to_string();
}

// Print address breakdown
std::string Address::toString() const {
    std::stringstream ss;
    ss << "Address: " << toHexString() << std::endl;
    ss << "  Binary: " << toBinaryString() << std::endl;
    ss << "  Tag   : " << std::bitset<32>(getTag()) << " (" << getTag() << ")" << std::endl;
    ss << "  Index : " << std::bitset<32>(getIndex()) << " (" << getIndex() << ")" << std::endl;
    ss << "  Offset: " << std::bitset<32>(getOffset()) << " (" << getOffset() << ")" << std::endl;
    return ss.str();
}

// Check if address aligns with word boundary (4-byte aligned)
bool Address::isWordAligned() const {
    return (address & 0x3) == 0;
}

// Get the address of the block containing this address
uint32_t Address::getBlockAddress() const {
    // Clear the offset bits to get block address
    return address & ~((1 << b) - 1);
}

// Get the address of the word containing this address
uint32_t Address::getWordAddress() const {
    // Clear the lowest 2 bits to get word address (4-byte aligned)
    return address & ~0x3;
}