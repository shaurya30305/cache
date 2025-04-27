#include "MainMemory.h"
#include <iostream>

// Constructor
MainMemory::MainMemory(unsigned int blockSize) 
    : blockSize(blockSize), readCount(0), writeCount(0) {
}

// Read a block from memory
std::vector<uint8_t> MainMemory::readBlock(uint32_t blockAddress) {
    // Increment statistics
    readCount++;
    
    // Check if this block exists in our sparse memory
    auto it = memory.find(blockAddress);
    
    if (it != memory.end()) {
        // Return existing block
        return it->second;
    } else {
        // If block doesn't exist, create a new block filled with zeros
        std::vector<uint8_t> newBlock(blockSize, 0);
        
        // Store it in memory (optional - could just return zeros without storing)
        memory[blockAddress] = newBlock;
        
        return newBlock;
    }
}

// Write a block to memory
void MainMemory::writeBlock(uint32_t blockAddress, const std::vector<uint8_t>& data) {
    // Increment statistics
    writeCount++;
    
    // Validate data size
    if (data.size() != blockSize) {
        std::cerr << "Error: Attempt to write incorrect block size to memory." << std::endl;
        std::cerr << "Expected: " << blockSize << " bytes, Got: " << data.size() << " bytes" << std::endl;
        return;
    }
    
    // Store block in memory
    memory[blockAddress] = data;
}

// Get read count
unsigned int MainMemory::getReadCount() const {
    return readCount;
}

// Get write count
unsigned int MainMemory::getWriteCount() const {
    return writeCount;
}

// Reset statistics
void MainMemory::resetStats() {
    readCount = 0;
    writeCount = 0;
}