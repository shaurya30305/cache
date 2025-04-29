#include "Cache.h"
#include "Address.h"
#include "MainMemory.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>
#include <memory>  // Added for std::unique_ptr
#include <functional>

// Colors for console output
#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

// Function to print success/failure message
void printTestResult(const std::string& testName, bool success) {
    if (success) {
        std::cout << GREEN << "[PASS] " << RESET << testName << std::endl;
    } else {
        std::cout << RED << "[FAIL] " << RESET << testName << std::endl;
    }
}

// Function to print cache line state
void printCacheLineState(const CacheLine& line) {
    std::cout << "  Tag: 0x" << std::hex << std::setw(8) << std::setfill('0') 
              << line.getTag() << std::dec;
    
    switch (line.getMESIState()) {
        case MESIState::MODIFIED:  std::cout << " State: M "; break;
        case MESIState::EXCLUSIVE: std::cout << " State: E "; break;
        case MESIState::SHARED:    std::cout << " State: S "; break;
        case MESIState::INVALID:   std::cout << " State: I "; break;
    }
    
    std::cout << (line.isDirty() ? "Dirty" : "Clean") << std::endl;
}

// Function to print cache state
void printCacheState(const Cache& cache) {
    std::cout << BLUE << "Cache " << cache.getCoreId() << " State:" << RESET << std::endl;
    std::cout << "  Hits: " << cache.getHitCount() << ", Misses: " << cache.getMissCount() << std::endl;
    
    const auto& sets = cache.getSets();
    for (size_t i = 0; i < sets.size(); i++) {
        std::cout << "  Set " << i << ":" << std::endl;
        const auto& lines = sets[i].getLines();
        for (size_t j = 0; j < lines.size(); j++) {
            std::cout << "    Line " << j << ": ";
            if (lines[j].isValid()) {
                printCacheLineState(lines[j]);
            } else {
                std::cout << "Invalid" << std::endl;
            }
        }
    }
    std::cout << std::endl;
}

// Test class with all test cases
class CacheTest {
private:
    // Shared memory
    MainMemory memory;
    
    // Caches
    std::vector<std::unique_ptr<Cache>> caches;
    
    // Current simulation cycle
    unsigned int currentCycle;
    
    // Coherence callback for connecting caches
    void coherenceCallback(BusTransaction transType, const Address& addr, int requestingCore, 
                          bool& providedData, int& sourceCache) {
        // Notify all other caches
        for (size_t i = 0; i < caches.size(); i++) {
            if (static_cast<int>(i) != requestingCore) {
                bool providedData = false;
                if (caches[i]->handleBusTransaction(transType, addr, requestingCore, providedData)) {
                    if (providedData) {
                        sourceCache = i;
                    }
                }
            }
        }
    }

    // Initialize a cache with predefined data
    void initializeCache(Cache& cache, const std::vector<std::tuple<uint32_t, MESIState>>& entries) {
        for (const auto& entry : entries) {
            uint32_t addr = std::get<0>(entry);
            MESIState state = std::get<1>(entry);
            
            // Create address object
            Address address(addr, cache.getSetBits(), cache.getBlockBits());
            
            // Get set and tag
            uint32_t setIndex = address.getIndex();
            uint32_t tag = address.getTag();
            
            // Get block from memory
            uint32_t blockAddr = address.getBlockAddress();
            std::vector<uint8_t> data = memory.readBlock(blockAddr);
            
            // Find a line in the set
            auto& sets = const_cast<std::vector<CacheSet>&>(cache.getSets());
            CacheLine* line = sets[setIndex].findInvalidLine();
            if (!line) {
                line = sets[setIndex].findVictim();
            }
            
            // Load data with specified state
            line->loadData(data, tag, state);
            
            // If it's modified, also mark it dirty
            if (state == MESIState::MODIFIED) {
                line->setDirty();
            }
        }
    }
    
    // Run cache for specified cycles
    void runCycles(int cycles) {
        for (int i = 0; i < cycles; i++) {
            currentCycle++;
            for (auto& cache : caches) {
                cache->setCycle(currentCycle);
            }
        }
    }
    
    // Check if a miss is resolved
    bool isMissResolved(Cache& cache) {
        return cache.checkMissResolved();
    }

public:
    // Constructor
    CacheTest() 
        : memory(64),  // 64-byte blocks
          currentCycle(0) {
        
        // Create caches
        for (int i = 0; i < 4; i++) {
            caches.push_back(std::make_unique<Cache>(
                i,      // Core ID
                4,      // 4 sets
                2,      // 2-way associative
                64,     // 64-byte blocks
                2,      // 2 set bits
                6,      // 6 block bits
                memory
            ));
            
            // Set coherence callback
            caches[i]->setCoherenceCallback(
                [this](BusTransaction transType, const Address& addr, int requestingCore, 
                       bool& providedData, int& sourceCache) {
                    this->coherenceCallback(transType, addr, requestingCore, providedData, sourceCache);
                }
            );
        }
    }
    
    // Test simple read hit
    bool testReadHit() {
        std::cout << YELLOW << "\nTest: Read Hit" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 0 with data at address 0x1000 in Exclusive state
        initializeCache(*caches[0], {
            {0x1000, MESIState::EXCLUSIVE}
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Create address object
        Address addr(0x1000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform read
        bool result = caches[0]->read(addr);
        
        // Check result and state
        std::cout << "After read:" << std::endl;
        printCacheState(*caches[0]);
        
        // Validate results
        bool success = result && 
                      caches[0]->getHitCount() == 1 && 
                      caches[0]->getMissCount() == 0;
        
        printTestResult("Read Hit", success);
        return success;
    }
    
    // Test read miss to memory
    bool testReadMissToMemory() {
        std::cout << YELLOW << "\nTest: Read Miss (Memory)" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Create address object
        Address addr(0x2000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Perform read - should miss
        bool result = caches[0]->read(addr);
        
        // Check result and state
        std::cout << "After read (before miss resolution):" << std::endl;
        printCacheState(*caches[0]);
        
        // Should not be resolved yet
        bool missResolvedEarly = isMissResolved(*caches[0]);
        
        // Run for 99 cycles
        runCycles(99);
        bool missResolvedTooEarly = isMissResolved(*caches[0]);
        
        // Run 1 more cycle for total of 100
        runCycles(1);
        bool missResolved = isMissResolved(*caches[0]);
        
        std::cout << "After 100 cycles:" << std::endl;
        printCacheState(*caches[0]);
        
        // Validate results
        bool success = !result && // Read should return false (miss)
                      !missResolvedEarly && // Miss should not resolve early
                      !missResolvedTooEarly && // Miss should not resolve at 99 cycles
                      missResolved && // Miss should resolve at 100 cycles
                      caches[0]->getHitCount() == 0 && 
                      caches[0]->getMissCount() == 1;
        
        printTestResult("Read Miss (Memory)", success);
        return success;
    }
    
    // Test read miss to cache in Modified state
    bool testReadMissToModifiedCache() {
        std::cout << YELLOW << "\nTest: Read Miss (to Modified Cache Line)" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 1 with data at address 0x3000 in Modified state
        initializeCache(*caches[1], {
            {0x3000, MESIState::MODIFIED}
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        printCacheState(*caches[1]);
        
        // Create address object
        Address addr(0x3000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform read from cache 0 - should miss and get from cache 1
        bool result = caches[0]->read(addr);
        
        // Check result and state
        std::cout << "After read (before miss resolution):" << std::endl;
        printCacheState(*caches[0]);
        printCacheState(*caches[1]);
        
        // Calculate expected cycles - 2 cycles per word (64-byte block / 4 = 16 words)
        int expectedCycles = 32; // 16 words * 2 cycles
        
        // Run for expected cycles - 1
        runCycles(expectedCycles - 1);
        bool missResolvedTooEarly = isMissResolved(*caches[0]);
        
        // Run 1 more cycle
        runCycles(1);
        bool missResolved = isMissResolved(*caches[0]);
        
        std::cout << "After " << expectedCycles << " cycles:" << std::endl;
        printCacheState(*caches[0]);
        printCacheState(*caches[1]);
        
        // Check if cache 1's line is now in Shared state
        bool cache1Shared = false;
        const auto& sets1 = caches[1]->getSets();
        for (const auto& set : sets1) {
            for (const auto& line : set.getLines()) {
                if (line.isValid() && line.getTag() == addr.getTag()) {
                    cache1Shared = (line.getMESIState() == MESIState::SHARED);
                    break;
                }
            }
        }
        
        // Check if cache 0's line is in Shared state
        bool cache0Shared = false;
        const auto& sets0 = caches[0]->getSets();
        for (const auto& set : sets0) {
            for (const auto& line : set.getLines()) {
                if (line.isValid() && line.getTag() == addr.getTag()) {
                    cache0Shared = (line.getMESIState() == MESIState::SHARED);
                    break;
                }
            }
        }
        
        // Validate results
        bool success = !result && // Read should return false (miss)
                      !missResolvedTooEarly && // Miss should not resolve early
                      missResolved && // Miss should resolve after expected cycles
                      cache0Shared && // Cache 0 should have the line in Shared state
                      cache1Shared && // Cache 1 should have downgraded to Shared
                      caches[0]->getHitCount() == 0 && 
                      caches[0]->getMissCount() == 1;
        
        printTestResult("Read Miss (to Modified Cache Line)", success);
        return success;
    }
    
    // Test write hit to Exclusive state
    bool testWriteHitToExclusive() {
        std::cout << YELLOW << "\nTest: Write Hit (to Exclusive Line)" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 0 with data at address 0x4000 in Exclusive state
        initializeCache(*caches[0], {
            {0x4000, MESIState::EXCLUSIVE}
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Create address object
        Address addr(0x4000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform write
        bool result = caches[0]->write(addr);
        
        // Check result and state
        std::cout << "After write:" << std::endl;
        printCacheState(*caches[0]);
        
        // Check if line is now in Modified state
        bool lineModified = false;
        const auto& sets = caches[0]->getSets();
        for (const auto& set : sets) {
            for (const auto& line : set.getLines()) {
                if (line.isValid() && line.getTag() == addr.getTag()) {
                    lineModified = (line.getMESIState() == MESIState::MODIFIED);
                    break;
                }
            }
        }
        
        // Validate results
        bool success = result && // Write should return true (hit)
                      lineModified && // Line should be Modified
                      caches[0]->getHitCount() == 1 && 
                      caches[0]->getMissCount() == 0;
        
        printTestResult("Write Hit (to Exclusive Line)", success);
        return success;
    }
    
    // Test write hit to Shared state
    bool testWriteHitToShared() {
        std::cout << YELLOW << "\nTest: Write Hit (to Shared Line)" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 0 and cache 1 with data at address 0x5000 in Shared state
        initializeCache(*caches[0], {
            {0x5000, MESIState::SHARED}
        });
        
        initializeCache(*caches[1], {
            {0x5000, MESIState::SHARED}
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        printCacheState(*caches[1]);
        
        // Create address object
        Address addr(0x5000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform write to cache 0
        bool result = caches[0]->write(addr);
        
        // Check result and state
        std::cout << "After write:" << std::endl;
        printCacheState(*caches[0]);
        printCacheState(*caches[1]);
        
        // Check if cache 0's line is now in Modified state
        bool cache0Modified = false;
        const auto& sets0 = caches[0]->getSets();
        for (const auto& set : sets0) {
            for (const auto& line : set.getLines()) {
                if (line.isValid() && line.getTag() == addr.getTag()) {
                    cache0Modified = (line.getMESIState() == MESIState::MODIFIED);
                    break;
                }
            }
        }
        
        // Check if cache 1's line is now Invalid
        bool cache1Invalid = true;
        const auto& sets1 = caches[1]->getSets();
        for (const auto& set : sets1) {
            for (const auto& line : set.getLines()) {
                if (line.isValid() && line.getTag() == addr.getTag()) {
                    cache1Invalid = false;
                    break;
                }
            }
        }
        
        // Validate results
        bool success = result && // Write should return true (hit)
                      cache0Modified && // Cache 0's line should be Modified
                      cache1Invalid && // Cache 1's line should be Invalid
                      caches[0]->getHitCount() == 1 && 
                      caches[0]->getMissCount() == 0;
        
        printTestResult("Write Hit (to Shared Line)", success);
        return success;
    }
    
    // Test write miss with dirty eviction
    bool testWriteMissWithDirtyEviction() {
        std::cout << YELLOW << "\nTest: Write Miss (with Dirty Eviction)" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Fill cache 0's set 2 with dirty lines to force eviction
        // Address 0x6000 maps to set 2 (bits 6-7 = 10)
        initializeCache(*caches[0], {
            {0x6000, MESIState::MODIFIED}, // Will be evicted
            {0x6040, MESIState::MODIFIED}  // Will stay
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Create address object for a new address that maps to the same set
        // Address 0x6080 also maps to set 2
        Address addr(0x6080, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform write - should miss and cause eviction
        bool result = caches[0]->write(addr);
        
        // Check result and state
        std::cout << "After write (before miss resolution):" << std::endl;
        printCacheState(*caches[0]);
        
        // Should take 100 cycles for eviction + 100 cycles for memory fetch
        runCycles(199);
        bool missResolvedTooEarly = isMissResolved(*caches[0]);
        
        // Run 1 more cycle for total of 200
        runCycles(1);
        bool missResolved = isMissResolved(*caches[0]);
        
        std::cout << "After 200 cycles:" << std::endl;
        printCacheState(*caches[0]);
        
        // Check if new line is in Modified state
        bool newLineModified = false;
        const auto& sets = caches[0]->getSets();
        // Set 2 should now have the new line (0x6080) and the kept line (0x6040)
        const auto& set2 = sets[2]; // Set 2
        for (const auto& line : set2.getLines()) {
            if (line.isValid() && line.getTag() == addr.getTag()) {
                newLineModified = (line.getMESIState() == MESIState::MODIFIED);
                break;
            }
        }
        
        // Validate results
        bool success = !result && // Write should return false (miss)
                      !missResolvedTooEarly && // Miss should not resolve early
                      missResolved && // Miss should resolve after 200 cycles
                      newLineModified && // New line should be Modified
                      caches[0]->getHitCount() == 0 && 
                      caches[0]->getMissCount() == 1;
        
        printTestResult("Write Miss (with Dirty Eviction)", success);
        return success;
    }
    
    // Test that dirty eviction happens on write miss but not on read miss to invalid
    bool testDirtyEvictionTiming() {
        std::cout << YELLOW << "\nTest: Dirty Eviction Timing" << RESET << std::endl;
        
        // Reset state
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 0 with a dirty line at 0x7000
        initializeCache(*caches[0], {
            {0x7000, MESIState::MODIFIED}
        });
        
        // Print initial state
        std::cout << "Initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Address in a different set
        Address addr1(0x8000, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform read - should not cause dirty eviction
        bool result1 = caches[0]->read(addr1);
        
        // Run 100 cycles - should resolve since no eviction needed
        runCycles(100);
        bool missResolved1 = isMissResolved(*caches[0]);
        
        std::cout << "After read and 100 cycles:" << std::endl;
        printCacheState(*caches[0]);
        
        // Reset for second test
        currentCycle = 0;
        for (auto& cache : caches) {
            cache->setCycle(currentCycle);
        }
        
        // Set up cache 0 with a dirty line and another line in the same set
        // 0x9000 and 0x9040 map to the same set
        initializeCache(*caches[0], {
            {0x9000, MESIState::MODIFIED}, // Will be evicted
            {0x9040, MESIState::EXCLUSIVE} // Will remain
        });
        
        // Print initial state for second test
        std::cout << "\nSecond test initial state:" << std::endl;
        printCacheState(*caches[0]);
        
        // Address mapping to the same set
        Address addr2(0x9080, caches[0]->getSetBits(), caches[0]->getBlockBits());
        
        // Perform write - should cause dirty eviction
        bool result2 = caches[0]->write(addr2);
        
        // Run 100 cycles - should not resolve yet due to eviction
        runCycles(100);
        bool missResolvedEarly = isMissResolved(*caches[0]);
        
        // Run another 100 cycles - now should resolve
        runCycles(100);
        bool missResolved2 = isMissResolved(*caches[0]);
        
        std::cout << "After write and 200 cycles:" << std::endl;
        printCacheState(*caches[0]);
        
        // Validate results
        bool success = !result1 && // Read should return false (miss)
                      missResolved1 && // Read miss should resolve after 100 cycles
                      !result2 && // Write should return false (miss)
                      !missResolvedEarly && // Write miss should not resolve after first 100 cycles
                      missResolved2; // Write miss should resolve after 200 cycles
        
        printTestResult("Dirty Eviction Timing", success);
        return success;
    }
    
    // Run all tests
    void runAllTests() {
        std::cout << BLUE << "========== Running Cache Tests ==========" << RESET << std::endl;
        
        // Basic operations
        testReadHit();
        testReadMissToMemory();
        testReadMissToModifiedCache();
        testWriteHitToExclusive();
        testWriteHitToShared();
        testWriteMissWithDirtyEviction();
        testDirtyEvictionTiming();
    }
};

int main() {
    CacheTest tester;
    tester.runAllTests();
    return 0;
}