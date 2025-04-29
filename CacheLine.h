#ifndef CACHE_LINE_H
#define CACHE_LINE_H

#include <cstdint>
#include <vector>
#include "Address.h"

// Forward declaration
class Cache;

// MESI state enum
enum class MESIState {
    MODIFIED,  // Modified: Line is dirty and exclusive to this cache
    EXCLUSIVE, // Exclusive: Line is clean and exclusive to this cache
    SHARED,    // Shared: Line is clean and may exist in other caches
    INVALID    // Invalid: Line does not contain valid data
};

class CacheLine {
private:
    MESIState mesiState;   // MESI coherence state (replacing valid bit)
    bool dirty;            // Dirty bit (not needed for MESI but kept for clarity)
    uint32_t tag;          // Tag bits from address
    std::vector<uint8_t> data; // Actual data stored in the cache line
    unsigned int lruCounter;   // Counter used for LRU replacement

public:
    // Constructor - initialize an empty cache line with specified block size
    CacheLine(int blockSize);
    
    // Reset the cache line (invalidate)
    void reset();
    
    // Get MESI state
    MESIState getMESIState() const;
    
    // Set MESI state
    void setMESIState(MESIState state);
    
    // Helper methods for checking MESI states
    bool isModified() const;
    bool isExclusive() const;
    bool isShared() const;
    bool isInvalid() const;
    
    // Check if the line is valid (any state except INVALID)
    bool isValid() const;
    
    // Check if the line is dirty (either in MODIFIED state or dirty bit set)
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
    void loadData(const std::vector<uint8_t>& newData, uint32_t newTag, MESIState state = MESIState::EXCLUSIVE);
    
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
    
    // String representation of MESI state (for debugging)
    std::string getMESIStateString() const;
};

#endif // CACHE_LINE_H