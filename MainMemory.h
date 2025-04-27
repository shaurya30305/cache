#ifndef MAIN_MEMORY_H
#define MAIN_MEMORY_H

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "Address.h"

class MainMemory {
private:
    // Using a sparse representation of memory (only store blocks that are accessed)
    std::unordered_map<uint32_t, std::vector<uint8_t>> memory;
    
    // Block size in bytes
    unsigned int blockSize;
    
    // Statistics
    unsigned int readCount;
    unsigned int writeCount;

public:
    // Constructor
    MainMemory(unsigned int blockSize);
    
    // Read a block from memory
    // blockAddress should be aligned to block boundaries
    std::vector<uint8_t> readBlock(uint32_t blockAddress);
    
    // Write a block to memory
    // blockAddress should be aligned to block boundaries
    void writeBlock(uint32_t blockAddress, const std::vector<uint8_t>& data);
    
    // Get read count
    unsigned int getReadCount() const;
    
    // Get write count
    unsigned int getWriteCount() const;
    
    // Reset statistics
    void resetStats();
};

#endif // MAIN_MEMORY_H