#include "Cache.h"
#include <iostream>

// Constructor
Cache::Cache(int coreId, int numSets, int associativity, int blockSize, 
             int setBits, int blockBits, MainMemory& mainMemory)
    : coreId(coreId), mainMemory(mainMemory), 
      setBits(setBits), blockBits(blockBits),
      numSets(numSets), blockSize(blockSize), associativity(associativity),
      accessCount(0), hitCount(0), missCount(0), readCount(0), writeCount(0),
      pendingMiss(false), missResolveTime(0), currentCycle(0) {
    
    // Initialize cache sets
    sets.reserve(numSets);
    for (int i = 0; i < numSets; i++) {
        sets.emplace_back(associativity, blockSize);
    }
}

// Read operation
bool Cache::read(const Address& addr) {
    accessCount++;
    readCount++;
    
    // If there's a pending miss, we can't process this yet
    if (pendingMiss) {
        return false;
    }
    
    // Get set index and tag from address
    uint32_t setIndex = addr.getIndex();
    uint32_t tag = addr.getTag();
    
    // Bounds check
    if (setIndex >= sets.size()) {
        std::cerr << "Error: Set index out of bounds: " << setIndex << std::endl;
        return false;
    }
    
    // Look for the block in the cache
    CacheSet& set = sets[setIndex];
    CacheLine* line = set.findLine(tag);
    
    if (line != nullptr) {
        // Cache hit
        hitCount++;
        set.updateLRU(line);
        return true;
    } else {
        // Cache miss - start memory access
        missCount++;
        pendingMiss = true;
        
        // Memory access takes 100 cycles
        missResolveTime = currentCycle + 100;
        
        // Get victim line for replacement
        CacheLine* victim = set.findVictim();
        
        // Handle write-back if victim is dirty
        if (victim->isValid() && victim->isDirty()) {
            // Create address for the victim block
            uint32_t victimTag = victim->getTag();
            uint32_t victimBlockAddr = (victimTag << (setBits + blockBits)) | (setIndex << blockBits);
            
            // Write back to memory
            mainMemory.writeBlock(victimBlockAddr, victim->getData());
        }
        
        // Fetch block from memory (will be completed when miss resolves)
        uint32_t blockAddr = addr.getBlockAddress();
        std::vector<uint8_t> data = mainMemory.readBlock(blockAddr);
        
        // Load data into victim line
        victim->loadData(data, tag);
        
        return false; // Return false to indicate the processor should block
    }
}

// Write operation
bool Cache::write(const Address& addr) {
    accessCount++;
    writeCount++;
    
    // If there's a pending miss, we can't process this yet
    if (pendingMiss) {
        return false;
    }
    
    // Get set index and tag from address
    uint32_t setIndex = addr.getIndex();
    uint32_t tag = addr.getTag();
    uint32_t offset = addr.getOffset();
    
    // Bounds check
    if (setIndex >= sets.size()) {
        std::cerr << "Error: Set index out of bounds: " << setIndex << std::endl;
        return false;
    }
    
    // Look for the block in the cache
    CacheSet& set = sets[setIndex];
    CacheLine* line = set.findLine(tag);
    
    if (line != nullptr) {
        // Cache hit
        hitCount++;
        set.updateLRU(line);
        
        // Write data (assuming a full word write at word alignment)
        // In a real implementation, you would take the actual data to write
        // For this simulation, we'll just write a dummy value
        uint32_t wordOffset = offset & ~0x3; // Align to word boundary
        uint32_t dummyData = 0xDEADBEEF;     // Dummy data for simulation
        line->writeWord(wordOffset, dummyData);
        line->setDirty(); // Mark line as dirty after write
        
        return true;
    } else {
        // Cache miss with write-allocate policy
        missCount++;
        pendingMiss = true;
        
        // Memory access takes 100 cycles
        missResolveTime = currentCycle + 100;
        
        // Get victim line for replacement
        CacheLine* victim = set.findVictim();
        
        // Handle write-back if victim is dirty
        if (victim->isValid() && victim->isDirty()) {
            // Create address for the victim block
            uint32_t victimTag = victim->getTag();
            uint32_t victimBlockAddr = (victimTag << (setBits + blockBits)) | (setIndex << blockBits);
            
            // Write back to memory
            mainMemory.writeBlock(victimBlockAddr, victim->getData());
        }
        
        // Fetch block from memory (will be completed when miss resolves)
        uint32_t blockAddr = addr.getBlockAddress();
        std::vector<uint8_t> data = mainMemory.readBlock(blockAddr);
        
        // Load data into victim line
        victim->loadData(data, tag);
        
        // Apply the write (this will be done when miss resolves)
        uint32_t wordOffset = offset & ~0x3; // Align to word boundary
        uint32_t dummyData = 0xDEADBEEF;     // Dummy data for simulation
        victim->writeWord(wordOffset, dummyData);
        victim->setDirty(); // Mark line as dirty after write
        
        return false; // Return false to indicate the processor should block
    }
}

// Check if a pending miss has been resolved
bool Cache::checkMissResolved() {
    if (!pendingMiss) {
        return false;
    }
    
    // Check if enough cycles have passed
    if (currentCycle >= missResolveTime) {
        pendingMiss = false;
        return true;
    }
    
    return false;
}

// Set the current cycle
void Cache::setCycle(unsigned int cycle) {
    currentCycle = cycle;
}

// Get access count
unsigned int Cache::getAccessCount() const {
    return accessCount;
}

// Get hit count
unsigned int Cache::getHitCount() const {
    return hitCount;
}

// Get miss count
unsigned int Cache::getMissCount() const {
    return missCount;
}

// Get read count
unsigned int Cache::getReadCount() const {
    return readCount;
}

// Get write count
unsigned int Cache::getWriteCount() const {
    return writeCount;
}

// Get number of set bits
int Cache::getSetBits() const {
    return setBits;
}

// Get number of block bits
int Cache::getBlockBits() const {
    return blockBits;
}

// Get core ID
int Cache::getCoreId() const {
    return coreId;
}

// Get cache sets (for testing/debugging)
const std::vector<CacheSet>& Cache::getSets() const {
    return sets;
}