#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "CommandLine.h" // For SimulationConfig
#include "TraceReader.h"
#include "Cache.h"
#include "Processor.h"
#include "MainMemory.h"
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm> // For std::none_of

class Simulator {
private:
    // Statistics
    unsigned int totalInstructions;
    unsigned int totalCycles;
    unsigned int totalMemoryAccesses;
    unsigned int totalCacheHits;
    unsigned int totalCacheMisses;
    unsigned int invalidationCount = 0;
    unsigned int busTrafficBytes = 0;
    // Cache-to-cache transfers (for coherence)
    unsigned int cacheToCache;
    
    // Private methods
    void logStatistics();
    void handleCacheMissResolution();
    void initializeComponents(); // Added this declaration
    
protected: // Changed from private to protected for TestSimulator access
    // Configuration
    SimulationConfig config;
    
    // Main components
    TraceReader traceReader;
    MainMemory mainMemory;
    std::vector<std::unique_ptr<Cache>> caches;
    std::vector<std::unique_ptr<Processor>> processors;
    
    // Simulation state
    unsigned int currentCycle;
    std::ofstream logFile;
    
    // Protected methods for derived classes
    virtual bool processNextCycle();
    
    // Add this method for TestSimulator
    bool isSimulationComplete() {
        return traceReader.allTracesCompleted() && 
               std::none_of(processors.begin(), processors.end(), 
                          [](const auto& p) { return p->isBlocked(); });
    }
    
public:
    // Constructor
    Simulator(const SimulationConfig& config);
    
    // Destructor
    ~Simulator();
    
    // Run the simulation
    void run();
    
    // Initialize simulation
    bool initialize();
    // Expose the current cycle counter
    unsigned int getCurrentCycle() const { return currentCycle; }

    // Expose the cache and processor arrays
    const std::vector<std::unique_ptr<Cache>>& getCaches() const { return caches; }
    const std::vector<std::unique_ptr<Processor>>& getProcessors() const { return processors; }

    // Expose the memory stats
    const MainMemory& getMainMemory() const { return mainMemory; }
    // Get statistics
    unsigned int getTotalInstructions() const;
    unsigned int getTotalCycles() const;
    double getAverageMemoryAccessTime() const;
    unsigned int getInvalidationCount() const;
    unsigned int getBusTrafficBytes() const;
    // Print results to console
    void printResults() const;
};

#endif // SIMULATOR_H