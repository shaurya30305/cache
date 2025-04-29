#ifndef CACHE_H
#define CACHE_H

#include <vector>
#include <functional>
#include <cstdint>
#include "CacheSet.h"
#include "Address.h"
#include "MainMemory.h"

// Forward declaration of Cache for global list
class Cache;

// Bus transaction types for coherence protocol
enum class BusTransaction {
    BUS_RD,     // Bus Read (for shared copy)
    BUS_RDX,    // Bus Read Exclusive (for exclusive copy)
    BUS_UPGR,   // Bus Upgrade (from shared to modified)
    FLUSH,      // Flush (writing back dirty block)
    INVALIDATE  // Invalidate other copies
};

// Global list of all caches for cache-to-cache transfers
extern std::vector<Cache*> allCaches;

// Coherence callback signature
typedef std::function<
    void(BusTransaction, const Address&, int, bool&, int&)
> CoherenceCallback;

class Cache {
public:
    // Constructor
    Cache(int coreId,
          int numSets,
          int associativity,
          int blockSize,
          int setBits,
          int blockBits,
          MainMemory& mainMemory);

    // Cache operations
    bool read(const Address& addr);
    bool write(const Address& addr);

    // Statistics
    unsigned int getAccessCount() const;
    unsigned int getHitCount() const;
    unsigned int getMissCount() const;
    unsigned int getReadCount() const;
    unsigned int getWriteCount() const;
    unsigned int getCoherenceCount() const;

    // Configuration
    int getSetBits() const;
    int getBlockBits() const;
    int getCoreId() const;
    int getAssociativity() const;

    // Debug/testing
    const std::vector<CacheSet>& getSets() const;
    bool hasPendingMiss() const;

    // Miss resolution
    bool checkMissResolved();
    void setCycle(unsigned int cycle);

    // Coherence support
    void setCoherenceCallback(const CoherenceCallback& cb);
    bool handleBusTransaction(BusTransaction t,
                              const Address& addr,
                              int requestingCore,
                              bool& provided);

private:
    // Internal helpers
    void issueCoherenceRequest(BusTransaction t,
                               const Address& addr,
                               bool& dataProvided,
                               int& sourceCache);
    std::vector<uint8_t> fetchBlockFromMemoryOrCache(const Address& addr,
                                                     MESIState& state);

    // Members
    int coreId;
    std::vector<CacheSet> sets;
    MainMemory& mainMemory;
    int setBits;
    int blockBits;
    int numSets;
    int blockSize;
    int associativity;
    unsigned int accessCount;
    unsigned int hitCount;
    unsigned int missCount;
    unsigned int readCount;
    unsigned int writeCount;
    unsigned int coherenceCount;
    bool pendingMiss;
    unsigned int missResolveTime;
    unsigned int currentCycle;
    int dataSourceCache;
    CoherenceCallback coherenceCallback;
};

#endif // CACHE_H
