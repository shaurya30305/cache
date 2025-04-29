#ifndef CACHE_SET_H
#define CACHE_SET_H

#include <vector>
#include "CacheLine.h"
#include "Address.h"

class CacheSet {
private:
    std::vector<CacheLine> lines;    // Cache lines in this set
    unsigned int lruCounter;         // Global counter for LRU tracking
    unsigned int associativity;      // Number of cache lines in this set (E)

public:
    // Constructor - initialize an empty set with specified associativity and block size
    CacheSet(unsigned int associativity, int blockSize);
    
    // Find a cache line matching the specified tag
    // Returns pointer to the line if found, nullptr if not found
    CacheLine* findLine(uint32_t tag);
    
    // Find a cache line matching the specified tag (const version)
    // Returns pointer to the line if found, nullptr if not found
    const CacheLine* findLine(uint32_t tag) const;
    
    // Find a victim line for replacement
    // Returns pointer to the LRU line
    CacheLine* findVictim();
    
    // Update LRU status of a line that was just accessed
    void updateLRU(CacheLine* line);
    
    // Check if set is full (no invalid lines)
    bool isFull() const;
    
    // Find an invalid line if one exists
    // Returns pointer to an invalid line, or nullptr if all lines are valid
    CacheLine* findInvalidLine();
    
    // Get vector of all cache lines in this set
    const std::vector<CacheLine>& getLines() const;
    
    // Get modifiable reference to lines (for coherence operations)
    std::vector<CacheLine>& getLinesModifiable();
    
    // Get number of lines in this set (associativity)
    unsigned int getAssociativity() const;
    
    // Get the current LRU counter value
    unsigned int getLRUCounter() const;
    
    // New methods for coherence
    
    // Invalidate any line with the specified tag
    bool invalidateLine(uint32_t tag);
    
    // Change state of a line with matching tag to Shared
    bool changeToShared(uint32_t tag);
    
    // Find any line in a specific MESI state
    CacheLine* findLineInState(uint32_t tag, MESIState state);
    
    // Check if any line has the tag in any valid state
    bool hasLineInAnyState(uint32_t tag);
};

#endif // CACHE_SET_H