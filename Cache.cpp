#include "Cache.h"
#include <iostream>

// Constructor: set up each cache set and initialize counters
Cache::Cache(int coreId,
             int numSets,
             int associativity,
             int blockSize,
             int setBits,
             int blockBits,
             MainMemory& mainMemory)
    : coreId(coreId)
    , mainMemory(mainMemory)
    , setBits(setBits)
    , blockBits(blockBits)
    , numSets(numSets)
    , blockSize(blockSize)
    , associativity(associativity)
    , accessCount(0)
    , hitCount(0)
    , missCount(0)
    , readCount(0)
    , writeCount(0)
    , coherenceCount(0)
    , pendingMiss(false)
    , missResolveTime(0)
    , currentCycle(0)
    , dataSourceCache(-1)
{
    // Allocate and initialize each cache set
    sets.reserve(numSets);
    for (int i = 0; i < numSets; ++i) {
        sets.emplace_back(associativity, blockSize);
    }
}

// Read operation: returns true on hit (1 cycle), false on miss (blocks processor)
bool Cache::read(const Address& addr) {
    accessCount++;
    readCount++;

    // Stall if a previous miss is unresolved
    if (pendingMiss) {
        return false;
    }

    // Decode address: set index, tag
    uint32_t setIndex = addr.getIndex();
    uint32_t tag      = addr.getTag();

    // Bounds check
    if (setIndex >= sets.size()) {
        std::cerr << "Cache::read(): setIndex out of range: " << setIndex << std::endl;
        return false;
    }

    CacheSet& cacheSet = sets[setIndex];
    CacheLine* line    = cacheSet.findLine(tag);

    if (line) {
        // CACHE HIT
        hitCount++;
        cacheSet.updateLRU(line);
        // No MESI state change required on read hit
        return true;
    }

    // CACHE MISS: begin block fill
    missCount++;
    pendingMiss     = true;
    dataSourceCache = -1;

    // Select victim line (invalid preferred, else LRU)
    CacheLine* victim = cacheSet.findVictim();

    // If victim is dirty (Modified), write back to memory first
    if (victim->isValid() && victim->isDirty()) {
        uint32_t victimTag       = victim->getTag();
        uint32_t victimBlockAddr = (victimTag << (setBits + blockBits))
                                    | (setIndex << blockBits);

        // Notify others of flush
        Address flushAddr(victimBlockAddr, setBits, blockBits);
        bool dummyProvided = false;
        int  dummySource   = -1;
        issueCoherenceRequest(BusTransaction::FLUSH, flushAddr, dummyProvided, dummySource);

        // Write back to memory (100-cycle penalty)
        mainMemory.writeBlock(victimBlockAddr, victim->getData());
        missResolveTime = currentCycle + 100;
    } else {
        missResolveTime = currentCycle;
    }

    // Issue BusRd to probe other caches
    bool providedByPeer = false;
    int  peerId         = -1;
    issueCoherenceRequest(BusTransaction::BUS_RD, addr, providedByPeer, peerId);
    if (providedByPeer) dataSourceCache = peerId;

    // Decide MESI state: SHARED if any peer had it, else EXCLUSIVE
    MESIState newState = (dataSourceCache >= 0)
                        ? MESIState::SHARED
                        : MESIState::EXCLUSIVE;

    // Fetch block (simulate data transfer timing separately)
    std::vector<uint8_t> data = fetchBlockFromMemoryOrCache(addr, newState);

    // Timing: 2 cycles/word if from cache, else 100 cycles memory
    if (dataSourceCache >= 0) {
        int numWords = blockSize / 4;
        missResolveTime += 2 * numWords;
    } else {
        missResolveTime += 100;
    }

    // Install block into cache line
    victim->loadData(data, tag, newState);

    return false;  // processor must stall until miss resolves
}

// Write operation: returns true on hit, false on miss (blocks processor)
bool Cache::write(const Address& addr) {
    accessCount++;
    writeCount++;

    if (pendingMiss) {
        return false;
    }

    uint32_t setIndex = addr.getIndex();
    uint32_t tag      = addr.getTag();
    uint32_t offset   = addr.getOffset();

    if (setIndex >= sets.size()) {
        std::cerr << "Cache::write(): setIndex out of range: " << setIndex << std::endl;
        return false;
    }

    CacheSet& cacheSet = sets[setIndex];
    CacheLine* line    = cacheSet.findLine(tag);

    if (line) {
        // WRITE HIT
        hitCount++;
        cacheSet.updateLRU(line);

        MESIState curState = line->getMESIState();
        if (curState == MESIState::SHARED) {
            // Invalidate other sharers
            bool dummyProvided = false;
            int  dummySource   = -1;
            issueCoherenceRequest(BusTransaction::BUS_UPGR, addr, dummyProvided, dummySource);
            // BUS_UPGR: 2-cycle bus transfer (no stall)
        }

        // Perform write (marks line Modified)
        uint32_t wordOffset = offset & ~0x3;
        uint32_t dummyData  = 0xDEADBEEF;
        line->writeWord(wordOffset, dummyData);
        return true;
    }

    // WRITE MISS (write-allocate)
    missCount++;
    pendingMiss     = true;
    dataSourceCache = -1;

    CacheLine* victim = cacheSet.findVictim();
    if (victim->isValid() && victim->isDirty()) {
        uint32_t victimTag       = victim->getTag();
        uint32_t victimBlockAddr = (victimTag << (setBits + blockBits))
                                    | (setIndex << blockBits);

        Address flushAddr(victimBlockAddr, setBits, blockBits);
        bool dummyProvided = false;
        int  dummySource   = -1;
        issueCoherenceRequest(BusTransaction::FLUSH, flushAddr, dummyProvided, dummySource);
        mainMemory.writeBlock(victimBlockAddr, victim->getData());
        missResolveTime = currentCycle + 100;
    } else {
        missResolveTime = currentCycle;
    }

    // Request exclusive ownership
    bool providedByPeer = false;
    int  peerId         = -1;
    issueCoherenceRequest(BusTransaction::BUS_RDX, addr, providedByPeer, peerId);
    if (providedByPeer) dataSourceCache = peerId;

    // We will modify, so load as MODIFIED
    MESIState newState = MESIState::MODIFIED;
    auto data = fetchBlockFromMemoryOrCache(addr, newState);

    if (dataSourceCache >= 0) {
        int numWords = blockSize / 4;
        missResolveTime += 2 * numWords;
    } else {
        missResolveTime += 100;
    }

    victim->loadData(data, tag, newState);
    uint32_t wordOffset = offset & ~0x3;
    uint32_t dummyData  = 0xDEADBEEF;
    victim->writeWord(wordOffset, dummyData);

    return false;
}

// Check if a pending miss has completed
bool Cache::checkMissResolved() {
    if (pendingMiss && currentCycle >= missResolveTime) {
        pendingMiss = false;
        return true;
    }
    return false;
}

// Advance the cycle
void Cache::setCycle(unsigned int cycle) {
    currentCycle = cycle;
}

// Provide simulator’s coherence callback
void Cache::setCoherenceCallback(CoherenceCallback callback) {
    coherenceCallback = callback;
}

// Issue a bus transaction to other caches
void Cache::issueCoherenceRequest(BusTransaction transType,
                                  const Address& addr,
                                  bool& dataProvided,
                                  int& sourceCache)
{
    if (coherenceCallback) {
        coherenceCallback(transType, addr, coreId, dataProvided, sourceCache);
        coherenceCount++;
    }
}

// Fetch a block’s bytes from cache or memory (always from memory here)
std::vector<uint8_t> Cache::fetchBlockFromMemoryOrCache(const Address& addr,
                                                         MESIState& /*state*/)
{
    uint32_t blockAddr = addr.getBlockAddress();
    return mainMemory.readBlock(blockAddr);
}

// Handle a snooped bus transaction from another cache
bool Cache::handleBusTransaction(BusTransaction transType,
                                 const Address& addr,
                                 int requestingCore,
                                 bool& providedData)
{
    uint32_t setIndex = addr.getIndex();
    uint32_t tag      = addr.getTag();

    if (setIndex >= sets.size()) return false;

    CacheSet& set   = sets[setIndex];
    CacheLine* line = set.findLine(tag);
    if (!line) return false;

    MESIState cur = line->getMESIState();
    switch (transType) {
      case BusTransaction::BUS_RD:
        if (cur == MESIState::MODIFIED) {
          // Write back, transition to SHARED
          mainMemory.writeBlock(addr.getBlockAddress(), line->getData());
          line->setMESIState(MESIState::SHARED);
          providedData = true;
          return true;
        } else if (cur == MESIState::EXCLUSIVE) {
          line->setMESIState(MESIState::SHARED);
          providedData = true;
          return true;
        } else if (cur == MESIState::SHARED) {
          providedData = true;
          return true;
        }
        break;

      case BusTransaction::BUS_RDX:
      case BusTransaction::INVALIDATE:
        if (cur != MESIState::INVALID) {
          if (cur == MESIState::MODIFIED) {
            // Write back before invalidating
            mainMemory.writeBlock(addr.getBlockAddress(), line->getData());
            providedData = true;
          }
          line->setMESIState(MESIState::INVALID);
          return true;
        }
        break;

      case BusTransaction::BUS_UPGR:
        if (cur == MESIState::SHARED) {
          // Drop shared copy
          line->setMESIState(MESIState::INVALID);
          return true;
        }
        break;

      case BusTransaction::FLUSH:
        // Another core wrote back; no local state change
        return true;
    }

    return false;
}

// Other getters (unchanged)...

unsigned int Cache::getAccessCount()   const { return accessCount; }
unsigned int Cache::getHitCount()      const { return hitCount; }
unsigned int Cache::getMissCount()     const { return missCount; }
unsigned int Cache::getReadCount()     const { return readCount; }
unsigned int Cache::getWriteCount()    const { return writeCount; }
unsigned int Cache::getCoherenceCount()const { return coherenceCount; }
int          Cache::getSetBits()       const { return setBits; }
int          Cache::getBlockBits()     const { return blockBits; }
int          Cache::getCoreId()        const { return coreId; }
const std::vector<CacheSet>& Cache::getSets() const { return sets; }
bool Cache::hasPendingMiss() const { return pendingMiss; }
