#ifndef CACHE_H
#define CACHE_H

#include "CacheSet.h"
#include "Address.h"
#include "MainMemory.h"
#include <vector>
#include <functional>

// Bus transaction types for coherence protocol
enum class BusTransaction {
    BUS_RD,     // Bus Read (for getting shared copy)
    BUS_RDX,    // Bus Read Exclusive (for getting exclusive copy)
    BUS_UPGR,   // Bus Upgrade (for upgrading from shared to modified)
    FLUSH,      // Flush (writing back data)
    INVALIDATE  // Invalidate other copies
};

// Type for coherence callbacks
using CoherenceCallback = std::function<void(BusTransaction, const Address&, int, bool&, int&)>;

class Cache {
private:
    int coreId;                    // ID of the core this cache belongs to
    std::vector<CacheSet> sets;    // Array of cache sets
    MainMemory& mainMemory;        // Reference to main memory
    
    // Cache configuration
    int setBits;                   // Number of set index bits (s)
    int blockBits;                 // Number of block bits (b)
    int numSets;                   // Number of sets in the cache (2^s)
    int blockSize;                 // Size of each block in bytes (2^b)
    int associativity;             // Number of lines per set (E)
    
    // Statistics
    unsigned int accessCount;      // Total number of cache accesses
    unsigned int hitCount;         // Total number of cache hits
    unsigned int missCount;        // Total number of cache misses
    unsigned int readCount;        // Number of read operations
    unsigned int writeCount;       // Number of write operations
    unsigned int coherenceCount;   // Number of coherence operations
    
    // Miss handling
    bool pendingMiss;              // Whether there's a pending miss being handled
    unsigned int missResolveTime;  // When the miss will be resolved (in cycles)
    unsigned int currentCycle;     // Current simulation cycle
    int dataSourceCache;           // ID of cache that provided data (-1 if from memory)
    
    // Coherence callback
    CoherenceCallback coherenceCallback;
    

void issueCoherenceRequest(BusTransaction transType, const Address& addr, bool& dataProvided, int& sourceCache);

    std::vector<uint8_t> fetchBlockFromMemoryOrCache(const Address& addr, MESIState& state);
    
public:
    // Constructor
    Cache(int coreId, int numSets, int associativity, int blockSize, 
          int setBits, int blockBits, MainMemory& mainMemory);
    
    // Cache operations
    bool read(const Address& addr);
    bool write(const Address& addr);
    int getAssociativity() const { return associativity; }
    // Check if a pending miss has been resolved
    bool checkMissResolved();
    
    // Set the current cycle
    void setCycle(unsigned int cycle);
    
    // Set coherence callback
    void setCoherenceCallback(CoherenceCallback callback);
    
    // Coherence methods
    bool handleBusTransaction(BusTransaction transType, const Address& addr, int requestingCore, bool& providedData);
    bool invalidateLine(const Address& addr);
    bool downgradeToShared(const Address& addr);
    bool hasLineInCache(const Address& addr, MESIState& state);
    std::vector<uint8_t> getLineData(const Address& addr, bool& found);
    void setDataSourceCache(int sourceCacheId);
    
    // Statistics access
    unsigned int getAccessCount() const;
    unsigned int getHitCount() const;
    unsigned int getMissCount() const;
    unsigned int getReadCount() const;
    unsigned int getWriteCount() const;
    unsigned int getCoherenceCount() const;
    
    // Configuration access
    int getSetBits() const;
    int getBlockBits() const;
    int getCoreId() const;
    
    // Get cache sets (for testing/debugging)
    const std::vector<CacheSet>& getSets() const;
    
    // Check for pending miss
    bool hasPendingMiss() const;
};

#endif // CACHE_H