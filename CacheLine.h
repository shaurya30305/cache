#ifndef CACHE_LINE_H
#define CACHE_LINE_H

#include <cstdint>
#include <vector>
#include "Address.h"

// Forward declaration
class Cache;

class CacheLine {
private:
    bool valid;           // Valid bit
    bool dirty;           // Dirty bit (for write-back)
    uint32_t tag;         // Tag bits from address
    std::vector<uint8_t> data; // Actual data stored in the cache line
    unsigned int lruCounter;   // Counter used for LRU replacement (higher value = more recently used)

public:
    // Constructor - initialize an empty cache line with specified block size
    CacheLine(int blockSize);
    
    // Reset the cache line (invalidate)
    void reset();
    
    // Check if the line is valid
    bool isValid() const;
    
    // Check if the line is dirty
    bool isDirty() const;
    
    // Get the tag
    uint32_t getTag() const;
    
    // Match tag - returns true if this line contains the specified tag
    bool matchTag(uint32_t tagToMatch) const;
    
    // Get LRU counter value
    unsigned int getLRUCounter() const;
    
    // Set LRU counter value
    void setLRUCounter(unsigned int counter);
    
    // Update LRU counter (mark as most recently used)
    void updateLRU(unsigned int newValue);
    
    // Load data into the cache line
    void loadData(const std::vector<uint8_t>& newData, uint32_t newTag);
    
    // Read a word (4 bytes) from the cache line
    uint32_t readWord(uint32_t offset) const;
    
    // Write a word (4 bytes) to the cache line
    void writeWord(uint32_t offset, uint32_t value);
    
    // Mark line as dirty (after a write)
    void setDirty();
    
    // Clear dirty bit (after write-back)
    void clearDirty();
    
    // Get entire data block
    const std::vector<uint8_t>& getData() const;
    
    // Get block size
    size_t getBlockSize() const;
};

#endif // CACHE_LINE_H